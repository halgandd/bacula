/*
 *
 *   Bacula Director -- User Agent Server
 *
 *     Kern Sibbald, September MM
 *
 *    Version $Id$
 */

/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "dird.h"

/* Imported subroutines */

/* Imported variables */
extern int r_first;
extern int r_last;
extern struct s_res resources[];
extern int console_msg_pending;
extern char my_name[];

/* Static variables */

/* Exported variables */
int quit_cmd_thread = 0;

/* Imported functions */

/* Forward referenced functions */

static void *connect_thread(void *arg);
static void *handle_UA_client_request(void *arg);


/* Global variables */
static int started = FALSE;
static workq_t ua_workq;

struct s_addr_port {
   char *addr;
   int port;
};

/* Called here by Director daemon to start UA (user agent)
 * command thread. This routine creates the thread and then
 * returns.
 */
void start_UA_server(char *UA_addr, int UA_port)
{
   pthread_t thid;
   int status;
   static struct s_addr_port arg;

   arg.port = UA_port;
   arg.addr = UA_addr;
   if ((status=pthread_create(&thid, NULL, connect_thread, (void *)&arg)) != 0) {
      Emsg1(M_ABORT, 0, _("Cannot create UA thread: %s\n"), strerror(status));
   }
   started = TRUE;
   return;
}

static void *connect_thread(void *arg)
{
   struct s_addr_port *UA = (struct s_addr_port *)arg;

   pthread_detach(pthread_self());

   /*  ****FIXME**** put # 10 on config parameter */
   bnet_thread_server(UA->addr, UA->port, 10, &ua_workq, handle_UA_client_request);
   return NULL;
}

/*
 * Create a Job Control Record for a control "job",
 *   filling in all the appropriate fields.
 */
JCR *create_control_jcr(char *base_name, int job_type)
{
   JCR *jcr;
   jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   jcr->sd_auth_key = bstrdup("dummy"); /* dummy Storage daemon key */
   create_unique_job_name(jcr, base_name);
   jcr->sched_time = jcr->start_time;
   jcr->JobType = job_type;
   jcr->JobLevel = L_FULL;
   jcr->JobStatus = JS_Running;
   /* None of these are really defined for control JCRs, so we
    * simply take the first of each one. This ensures that there
    * will be no null pointer references.
    */
   LockRes();
   jcr->job = (JOB *)GetNextRes(R_JOB, NULL);
   jcr->messages = (MSGS *)GetNextRes(R_MSGS, NULL);
   jcr->client = (CLIENT *)GetNextRes(R_CLIENT, NULL);
   jcr->pool = (POOL *)GetNextRes(R_POOL, NULL);
   jcr->catalog = (CAT *)GetNextRes(R_CATALOG, NULL);
   jcr->store = (STORE *)GetNextRes(R_STORAGE, NULL);
   jcr->fileset = (FILESET *)GetNextRes(R_FILESET, NULL);
   UnlockRes();
   return jcr;
}

/*
 * Handle Director User Agent commands	 
 *
 */
static void *handle_UA_client_request(void *arg)
{
   int stat;
   UAContext *ua;
   JCR *jcr;
   BSOCK *UA_sock = (BSOCK *)arg;

   pthread_detach(pthread_self());

   jcr = create_control_jcr("*Console*", JT_CONSOLE);

   ua = new_ua_context(jcr);
   ua->UA_sock = UA_sock;

   bnet_recv(ua->UA_sock);	    /* Get first message */
   if (!authenticate_user_agent(ua)) {
      goto getout;
   }

   while (!ua->quit) {
      stat = bnet_recv(ua->UA_sock);
      if (stat >= 0) {
	 pm_strcpy(&ua->cmd, ua->UA_sock->msg);
	 parse_ua_args(ua);
         if (ua->argc > 0 && ua->argk[0][0] == '.') {
	    do_a_dot_command(ua, ua->cmd);
	 } else {
	    do_a_command(ua, ua->cmd);
	 }
	 if (!ua->quit) {
	    if (ua->auto_display_messages) {
               strcpy(ua->cmd, "messages");
	       qmessagescmd(ua, ua->cmd);
	       ua->user_notified_msg_pending = FALSE;
	    } else if (!ua->user_notified_msg_pending && console_msg_pending) {
               bsendmsg(ua, _("You have messages.\n"));
	       ua->user_notified_msg_pending = TRUE;
	    }
	    bnet_sig(ua->UA_sock, BNET_EOD); /* send end of command */
	 }
      } else if (is_bnet_stop(ua->UA_sock)) {
	 ua->quit = TRUE;
	 break;
      } else { /* signal */
	 bnet_sig(ua->UA_sock, BNET_POLL);
      }
   }

getout:

   close_db(ua);
   free_ua_context(ua);
   free_jcr(jcr);

   return NULL;
}

/*
 * Create a UAContext for a Job that is running so that
 *   it can the User Agent routines and    
 *   to ensure that the Job gets the proper output.
 *   This is a sort of mini-kludge, and should be
 *   unified at some point.
 */
UAContext *new_ua_context(JCR *jcr)
{
   UAContext *ua;

   ua = (UAContext *)malloc(sizeof(UAContext));
   memset(ua, 0, sizeof(UAContext));
   ua->jcr = jcr;
   ua->db = jcr->db;
   ua->cmd = get_pool_memory(PM_FNAME);
   ua->args = get_pool_memory(PM_FNAME);
   ua->verbose = 1;
   ua->automount = TRUE;
   return ua;
}

void free_ua_context(UAContext *ua)
{
   if (ua->cmd) {
      free_pool_memory(ua->cmd);
   }
   if (ua->args) {
      free_pool_memory(ua->args);
   }
   if (ua->prompt) {
      free(ua->prompt);
   }

   if (ua->UA_sock) {
      bnet_close(ua->UA_sock);
   }
   free(ua);
}


/*
 * Called from main Bacula thread 
 */
void term_ua_server()
{
   if (!started) {
      return;
   }
   quit_cmd_thread = TRUE;
}
