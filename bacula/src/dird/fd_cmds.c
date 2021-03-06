/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula Director -- fd_cmds.c -- send commands to File daemon
 *
 *     Kern Sibbald, October MM
 *
 *    This routine is run as a separate thread.  There may be more
 *    work to be done to make it totally reentrant!!!!
 *
 *  Utility functions for sending info to File Daemon.
 *   These functions are used by both backup and verify.
 *
 *   Version $Id$
 */

#include "bacula.h"
#include "dird.h"
#include "findlib/find.h"

const int dbglvl = 400;

/* Commands sent to File daemon */
static char filesetcmd[]  = "fileset%s\n"; /* set full fileset */
static char jobcmd[]      = "JobId=%s Job=%s SDid=%u SDtime=%u Authorization=%s\n";
/* Note, mtime_only is not used here -- implemented as file option */
static char levelcmd[]    = "level = %s%s%s mtime_only=%d\n";
static char runscript[]   = "Run OnSuccess=%u OnFailure=%u AbortOnError=%u When=%u Command=%s\n";
static char runbeforenow[]= "RunBeforeNow\n";

/* Responses received from File daemon */
static char OKinc[]          = "2000 OK include\n";
static char OKjob[]          = "2000 OK Job";
static char OKlevel[]        = "2000 OK level\n";
static char OKRunScript[]    = "2000 OK RunScript\n";
static char OKRunBeforeNow[] = "2000 OK RunBeforeNow\n";

/* Forward referenced functions */
static bool send_list_item(JCR *jcr, const char *code, char *item, BSOCK *fd);

/* External functions */
extern DIRRES *director;
extern int FDConnectTimeout;

#define INC_LIST 0
#define EXC_LIST 1

/*
 * Open connection with File daemon.
 * Try connecting every retry_interval (default 10 sec), and
 *   give up after max_retry_time (default 30 mins).
 */

int connect_to_file_daemon(JCR *jcr, int retry_interval, int max_retry_time,
                           int verbose)
{
   BSOCK   *fd = new_bsock();
   char ed1[30];
   utime_t heart_beat;

   if (jcr->client->heartbeat_interval) {
      heart_beat = jcr->client->heartbeat_interval;
   } else {           
      heart_beat = director->heartbeat_interval;
   }

   if (!jcr->file_bsock) {
      char name[MAX_NAME_LENGTH + 100];
      bstrncpy(name, _("Client: "), sizeof(name));
      bstrncat(name, jcr->client->name(), sizeof(name));

      fd->set_source_address(director->DIRsrc_addr);
      if (!fd->connect(jcr,retry_interval,max_retry_time, heart_beat, name, jcr->client->address,
           NULL, jcr->client->FDport, verbose)) {
        fd->destroy();
        fd = NULL;
      }

      if (fd == NULL) {
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         return 0;
      }
      Dmsg0(10, "Opened connection with File daemon\n");
   } else {
      fd = jcr->file_bsock;           /* use existing connection */
   }
   fd->res = (RES *)jcr->client;      /* save resource in BSOCK */
   jcr->file_bsock = fd;
   set_jcr_job_status(jcr, JS_Running);

   if (!authenticate_file_daemon(jcr)) {
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }

   /*
    * Now send JobId and authorization key
    */
   fd->fsend(jobcmd, edit_int64(jcr->JobId, ed1), jcr->Job, jcr->VolSessionId,
      jcr->VolSessionTime, jcr->sd_auth_key);
   if (strcmp(jcr->sd_auth_key, "dummy") != 0) {
      memset(jcr->sd_auth_key, 0, strlen(jcr->sd_auth_key));
   }
   Dmsg1(100, ">filed: %s", fd->msg);
   if (bget_dirmsg(fd) > 0) {
       Dmsg1(110, "<filed: %s", fd->msg);
       if (strncmp(fd->msg, OKjob, strlen(OKjob)) != 0) {
          Jmsg(jcr, M_FATAL, 0, _("File daemon \"%s\" rejected Job command: %s\n"),
             jcr->client->hdr.name, fd->msg);
          set_jcr_job_status(jcr, JS_ErrorTerminated);
          return 0;
       } else if (jcr->db) {
          CLIENT_DBR cr;
          memset(&cr, 0, sizeof(cr));
          bstrncpy(cr.Name, jcr->client->hdr.name, sizeof(cr.Name));
          cr.AutoPrune = jcr->client->AutoPrune;
          cr.FileRetention = jcr->client->FileRetention;
          cr.JobRetention = jcr->client->JobRetention;
          bstrncpy(cr.Uname, fd->msg+strlen(OKjob)+1, sizeof(cr.Uname));
          if (!db_update_client_record(jcr, jcr->db, &cr)) {
             Jmsg(jcr, M_WARNING, 0, _("Error updating Client record. ERR=%s\n"),
                db_strerror(jcr->db));
          }
       }
   } else {
      Jmsg(jcr, M_FATAL, 0, _("FD gave bad response to JobId command: %s\n"),
         bnet_strerror(fd));
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return 0;
   }
   return 1;
}

/*
 * This subroutine edits the last job start time into a
 *   "since=date/time" buffer that is returned in the
 *   variable since.  This is used for display purposes in
 *   the job report.  The time in jcr->stime is later
 *   passed to tell the File daemon what to do.
 */
void get_level_since_time(JCR *jcr, char *since, int since_len)
{
   int JobLevel;
   bool have_full;
   bool do_full = false;
   bool do_diff = false;
   utime_t now;
   utime_t last_full_time;
   utime_t last_diff_time;

   since[0] = 0;
   /* If job cloned and a since time already given, use it */
   if (jcr->cloned && jcr->stime && jcr->stime[0]) {
      bstrncpy(since, _(", since="), since_len);
      bstrncat(since, jcr->stime, since_len);
      return;
   }
   /* Make sure stime buffer is allocated */
   if (!jcr->stime) {
      jcr->stime = get_pool_memory(PM_MESSAGE);
   } 
   jcr->stime[0] = 0;
   /*
    * Lookup the last FULL backup job to get the time/date for a
    * differential or incremental save.
    */
   switch (jcr->get_JobLevel()) {
   case L_DIFFERENTIAL:
   case L_INCREMENTAL:
      POOLMEM *stime = get_pool_memory(PM_MESSAGE);
      /* Look up start time of last Full job */
      now = (utime_t)time(NULL);
      jcr->jr.JobId = 0;     /* flag to return since time */
      have_full = db_find_job_start_time(jcr, jcr->db, &jcr->jr, &jcr->stime);
      /* If there was a successful job, make sure it is recent enough */
      if (jcr->get_JobLevel() == L_INCREMENTAL && have_full && jcr->job->MaxDiffInterval > 0) {
         /* Lookup last diff job */
         if (db_find_last_job_start_time(jcr, jcr->db, &jcr->jr, &stime, L_DIFFERENTIAL)) {
            last_diff_time = str_to_utime(stime);
            do_diff = ((now - last_diff_time) >= jcr->job->MaxDiffInterval);
         }
      }
      if (have_full && jcr->job->MaxFullInterval > 0 &&
         db_find_last_job_start_time(jcr, jcr->db, &jcr->jr, &stime, L_FULL)) {
         last_full_time = str_to_utime(stime);
         do_full = ((now - last_full_time) >= jcr->job->MaxFullInterval);
      }
      free_pool_memory(stime);

      if (!have_full || do_full) {
         /* No recent Full job found, so upgrade this one to Full */
         Jmsg(jcr, M_INFO, 0, "%s", db_strerror(jcr->db));
         Jmsg(jcr, M_INFO, 0, _("No prior or suitable Full backup found in catalog. Doing FULL backup.\n"));
         bsnprintf(since, since_len, _(" (upgraded from %s)"),
            level_to_str(jcr->get_JobLevel()));
         jcr->set_JobLevel(jcr->jr.JobLevel = L_FULL);
       } else if (do_diff) {
         /* No recent diff job found, so upgrade this one to Full */
         Jmsg(jcr, M_INFO, 0, _("No prior or suitable Differential backup found in catalog. Doing Differential backup.\n"));
         bsnprintf(since, since_len, _(" (upgraded from %s)"),
            level_to_str(jcr->get_JobLevel()));
         jcr->set_JobLevel(jcr->jr.JobLevel = L_DIFFERENTIAL);
      } else {
         if (jcr->job->rerun_failed_levels) {
            if (db_find_failed_job_since(jcr, jcr->db, &jcr->jr, jcr->stime, JobLevel)) {
               Jmsg(jcr, M_INFO, 0, _("Prior failed job found in catalog. Upgrading to %s.\n"),
                  level_to_str(JobLevel));
               bsnprintf(since, since_len, _(" (upgraded from %s)"),
                  level_to_str(jcr->get_JobLevel()));
               jcr->set_JobLevel(jcr->jr.JobLevel = JobLevel);
               jcr->jr.JobId = jcr->JobId;
               break;
            }
         }
         bstrncpy(since, _(", since="), since_len);
         bstrncat(since, jcr->stime, since_len);
      }
      jcr->jr.JobId = jcr->JobId;
      break;
   }
   Dmsg2(100, "Level=%c last start time=%s\n", jcr->get_JobLevel(), jcr->stime);
}

static void send_since_time(JCR *jcr)
{
   BSOCK   *fd = jcr->file_bsock;
   utime_t stime;
   char ed1[50];

   stime = str_to_utime(jcr->stime);
   fd->fsend(levelcmd, "", NT_("since_utime "), edit_uint64(stime, ed1), 0);
   while (bget_dirmsg(fd) >= 0) {  /* allow him to poll us to sync clocks */
      Jmsg(jcr, M_INFO, 0, "%s\n", fd->msg);
   }
}

/*
 * Send level command to FD.
 * Used for backup jobs and estimate command.
 */
bool send_level_command(JCR *jcr)
{
   BSOCK   *fd = jcr->file_bsock;
   const char *accurate = jcr->job->accurate?"accurate_":"";
   const char *not_accurate = "";
   /*
    * Send Level command to File daemon
    */
   switch (jcr->get_JobLevel()) {
   case L_BASE:
      fd->fsend(levelcmd, not_accurate, "base", " ", 0);
      break;
   /* L_NONE is the console, sending something off to the FD */
   case L_NONE:
   case L_FULL:
      fd->fsend(levelcmd, not_accurate, "full", " ", 0);
      break;
   case L_DIFFERENTIAL:
      fd->fsend(levelcmd, accurate, "differential", " ", 0);
      send_since_time(jcr);
      break;
   case L_INCREMENTAL:
      fd->fsend(levelcmd, accurate, "incremental", " ", 0);
      send_since_time(jcr);
      break;
   case L_SINCE:
   default:
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented backup level %d %c\n"),
         jcr->get_JobLevel(), jcr->get_JobLevel());
      return 0;
   }
   Dmsg1(120, ">filed: %s", fd->msg);
   if (!response(jcr, fd, OKlevel, "Level", DISPLAY_ERROR)) {
      return 0;
   }
   return 1;
}

/*
 * Send either an Included or an Excluded list to FD
 */
static bool send_fileset(JCR *jcr)
{
   FILESET *fileset = jcr->fileset;
   BSOCK   *fd = jcr->file_bsock;
   int num;
   bool include = true;

   for ( ;; ) {
      if (include) {
         num = fileset->num_includes;
      } else {
         num = fileset->num_excludes;
      }
      for (int i=0; i<num; i++) {
         char *item;
         INCEXE *ie;
         int j, k;

         if (include) {
            ie = fileset->include_items[i];
            fd->fsend("I\n");
         } else {
            ie = fileset->exclude_items[i];
            fd->fsend("E\n");
         }
         for (j=0; j<ie->num_opts; j++) {
            FOPTS *fo = ie->opts_list[j];
            fd->fsend("O %s\n", fo->opts);

            bool enhanced_wild = false;
            for (k=0; fo->opts[k]!='\0'; k++) {
               if (fo->opts[k]=='W') {
                  enhanced_wild = true;
                  break;
               }
            }

            for (k=0; k<fo->regex.size(); k++) {
               fd->fsend("R %s\n", fo->regex.get(k));
            }
            for (k=0; k<fo->regexdir.size(); k++) {
               fd->fsend("RD %s\n", fo->regexdir.get(k));
            }
            for (k=0; k<fo->regexfile.size(); k++) {
               fd->fsend("RF %s\n", fo->regexfile.get(k));
            }
            for (k=0; k<fo->wild.size(); k++) {
               fd->fsend("W %s\n", fo->wild.get(k));
            }
            for (k=0; k<fo->wilddir.size(); k++) {
               fd->fsend("WD %s\n", fo->wilddir.get(k));
            }
            for (k=0; k<fo->wildfile.size(); k++) {
               fd->fsend("WF %s\n", fo->wildfile.get(k));
            }
            for (k=0; k<fo->wildbase.size(); k++) {
               fd->fsend("W%c %s\n", enhanced_wild ? 'B' : 'F', fo->wildbase.get(k));
            }
            for (k=0; k<fo->base.size(); k++) {
               fd->fsend("B %s\n", fo->base.get(k));
            }
            for (k=0; k<fo->fstype.size(); k++) {
               fd->fsend("X %s\n", fo->fstype.get(k));
            }
            for (k=0; k<fo->drivetype.size(); k++) {
               fd->fsend("XD %s\n", fo->drivetype.get(k));
            }
            if (fo->plugin) {
               fd->fsend("G %s\n", fo->plugin);
            }
            if (fo->ignoredir) {
               bnet_fsend(fd, "Z %s\n", fo->ignoredir);
            }
            if (fo->reader) {
               fd->fsend("D %s\n", fo->reader);
            }
            if (fo->writer) {
               fd->fsend("T %s\n", fo->writer);
            }
            fd->fsend("N\n");
         }

         for (j=0; j<ie->name_list.size(); j++) {
            item = (char *)ie->name_list.get(j);
            if (!send_list_item(jcr, "F ", item, fd)) {
               goto bail_out;
            }
         }
         fd->fsend("N\n");
         for (j=0; j<ie->plugin_list.size(); j++) {
            item = (char *)ie->plugin_list.get(j);
            if (!send_list_item(jcr, "P ", item, fd)) {
               goto bail_out;
            }
         }
         fd->fsend("N\n");
      }
      if (!include) {                 /* If we just did excludes */
         break;                       /*   all done */
      }
      include = false;                /* Now do excludes */
   }

   fd->signal(BNET_EOD);              /* end of data */
   if (!response(jcr, fd, OKinc, "Include", DISPLAY_ERROR)) {
      goto bail_out;
   }
   return true;

bail_out:
   set_jcr_job_status(jcr, JS_ErrorTerminated);
   return false;

}

static bool send_list_item(JCR *jcr, const char *code, char *item, BSOCK *fd)
{
   BPIPE *bpipe;
   FILE *ffd;
   char buf[2000];
   int optlen, stat;
   char *p = item;

   switch (*p) {
   case '|':
      p++;                      /* skip over the | */
      fd->msg = edit_job_codes(jcr, fd->msg, p, "");
      bpipe = open_bpipe(fd->msg, 0, "r");
      if (!bpipe) {
         berrno be;
         Jmsg(jcr, M_FATAL, 0, _("Cannot run program: %s. ERR=%s\n"),
            p, be.bstrerror());
         return false;
      }
      bstrncpy(buf, code, sizeof(buf));
      Dmsg1(500, "code=%s\n", buf);
      optlen = strlen(buf);
      while (fgets(buf+optlen, sizeof(buf)-optlen, bpipe->rfd)) {
         fd->msglen = Mmsg(fd->msg, "%s", buf);
         Dmsg2(500, "Inc/exc len=%d: %s", fd->msglen, fd->msg);
         if (!bnet_send(fd)) {
            Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
            return false;
         }
      }
      if ((stat=close_bpipe(bpipe)) != 0) {
         berrno be;
         Jmsg(jcr, M_FATAL, 0, _("Error running program: %s. ERR=%s\n"),
            p, be.bstrerror(stat));
         return false;
      }
      break;
   case '<':
      p++;                      /* skip over < */
      if ((ffd = fopen(p, "rb")) == NULL) {
         berrno be;
         Jmsg(jcr, M_FATAL, 0, _("Cannot open included file: %s. ERR=%s\n"),
            p, be.bstrerror());
         return false;
      }
      bstrncpy(buf, code, sizeof(buf));
      Dmsg1(500, "code=%s\n", buf);
      optlen = strlen(buf);
      while (fgets(buf+optlen, sizeof(buf)-optlen, ffd)) {
         fd->msglen = Mmsg(fd->msg, "%s", buf);
         if (!bnet_send(fd)) {
            Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
            return false;
         }
      }
      fclose(ffd);
      break;
   case '\\':
      p++;                      /* skip over \ */
      /* Note, fall through wanted */
   default:
      pm_strcpy(fd->msg, code);
      fd->msglen = pm_strcat(fd->msg, p);
      Dmsg1(500, "Inc/Exc name=%s\n", fd->msg);
      if (!fd->send()) {
         Jmsg(jcr, M_FATAL, 0, _(">filed: write error on socket\n"));
         return false;
      }
      break;
   }
   return true;
}            


/*
 * Send include list to File daemon
 */
bool send_include_list(JCR *jcr)
{
   BSOCK *fd = jcr->file_bsock;
   if (jcr->fileset->new_include) {
      fd->fsend(filesetcmd, jcr->fileset->enable_vss ? " vss=1" : "");
      return send_fileset(jcr);
   }
   return true;
}


/*
 * Send exclude list to File daemon
 *   Under the new scheme, the Exclude list
 *   is part of the FileSet sent with the
 *   "include_list" above.
 */
bool send_exclude_list(JCR *jcr)
{
   return true;
}


/*
 * Send bootstrap file if any to the socket given (FD or SD).
 *  This is used for restore, verify VolumeToCatalog, and
 *  for migration.
 */
bool send_bootstrap_file(JCR *jcr, BSOCK *sock)
{
   FILE *bs;
   char buf[1000];
   const char *bootstrap = "bootstrap\n";

   Dmsg1(400, "send_bootstrap_file: %s\n", jcr->RestoreBootstrap);
   if (!jcr->RestoreBootstrap) {
      return true;
   }
   bs = fopen(jcr->RestoreBootstrap, "rb");
   if (!bs) {
      berrno be;
      Jmsg(jcr, M_FATAL, 0, _("Could not open bootstrap file %s: ERR=%s\n"),
         jcr->RestoreBootstrap, be.bstrerror());
      set_jcr_job_status(jcr, JS_ErrorTerminated);
      return false;
   }
   sock->fsend(bootstrap);
   while (fgets(buf, sizeof(buf), bs)) {
      sock->fsend("%s", buf);
   }
   sock->signal(BNET_EOD);
   fclose(bs);
   if (jcr->unlink_bsr) {
      unlink(jcr->RestoreBootstrap);
      jcr->unlink_bsr = false;
   }                         
   return true;
}

/* TODO: drop this with runscript.old_proto in bacula 1.42 */
static char runbefore[]   = "RunBeforeJob %s\n";
static char runafter[]    = "RunAfterJob %s\n";
static char OKRunBefore[] = "2000 OK RunBefore\n";
static char OKRunAfter[]  = "2000 OK RunAfter\n";

int send_runscript_with_old_proto(JCR *jcr, int when, POOLMEM *msg)
{
   int ret;
   Dmsg1(120, "bdird: sending old runcommand to fd '%s'\n",msg);
   if (when & SCRIPT_Before) {
      bnet_fsend(jcr->file_bsock, runbefore, msg);
      ret = response(jcr, jcr->file_bsock, OKRunBefore, "ClientRunBeforeJob", DISPLAY_ERROR);
   } else {
      bnet_fsend(jcr->file_bsock, runafter, msg);
      ret = response(jcr, jcr->file_bsock, OKRunAfter, "ClientRunAfterJob", DISPLAY_ERROR);
   }
   return ret;
} /* END OF TODO */

/*
 * Send RunScripts to File daemon
 * 1) We send all runscript to FD, they can be executed Before, After, or twice
 * 2) Then, we send a "RunBeforeNow" command to the FD to tell him to do the
 *    first run_script() call. (ie ClientRunBeforeJob)
 */
int send_runscripts_commands(JCR *jcr)
{
   POOLMEM *msg = get_pool_memory(PM_FNAME);
   BSOCK *fd = jcr->file_bsock;
   RUNSCRIPT *cmd;
   bool launch_before_cmd = false;
   POOLMEM *ehost = get_pool_memory(PM_FNAME);
   int result;

   Dmsg0(120, "bdird: sending runscripts to fd\n");
   
   foreach_alist(cmd, jcr->job->RunScripts) {
      if (cmd->can_run_at_level(jcr->get_JobLevel()) && cmd->target) {
         ehost = edit_job_codes(jcr, ehost, cmd->target, "");
         Dmsg2(200, "bdird: runscript %s -> %s\n", cmd->target, ehost);

         if (strcmp(ehost, jcr->client->name()) == 0) {
            pm_strcpy(msg, cmd->command);
            bash_spaces(msg);

            Dmsg1(120, "bdird: sending runscripts to fd '%s'\n", cmd->command);
            
            /* TODO: remove this with bacula 1.42 */
            if (cmd->old_proto) {
               result = send_runscript_with_old_proto(jcr, cmd->when, msg);

            } else {
               fd->fsend(runscript, cmd->on_success, 
                                    cmd->on_failure,
                                    cmd->fail_on_error,
                                    cmd->when,
                                    msg);

               result = response(jcr, fd, OKRunScript, "RunScript", DISPLAY_ERROR);
               launch_before_cmd = true;
            }
            
            if (!result) {
               goto bail_out;
            }
         }
         /* TODO : we have to play with other client */
         /*
           else {
           send command to an other client
           }
         */
      }        
   } 

   /* Tell the FD to execute the ClientRunBeforeJob */
   if (launch_before_cmd) {
      fd->fsend(runbeforenow);
      if (!response(jcr, fd, OKRunBeforeNow, "RunBeforeNow", DISPLAY_ERROR)) {
        goto bail_out;
      }
   }
   free_pool_memory(msg);
   free_pool_memory(ehost);
   return 1;

bail_out:
   Jmsg(jcr, M_FATAL, 0, _("Client \"%s\" RunScript failed.\n"), ehost);
   free_pool_memory(msg);
   free_pool_memory(ehost);
   return 0;
}



/*
 * Read the attributes from the File daemon for
 * a Verify job and store them in the catalog.
 */
int get_attributes_and_put_in_catalog(JCR *jcr)
{
   BSOCK   *fd;
   int n = 0;
   ATTR_DBR *ar = NULL;
   char digest[MAXSTRING];

   fd = jcr->file_bsock;
   jcr->jr.FirstIndex = 1;
   jcr->FileIndex = 0;
   /* Start transaction allocates jcr->attr and jcr->ar if needed */
   db_start_transaction(jcr, jcr->db);     /* start transaction if not already open */
   ar = jcr->ar;

   Dmsg0(120, "bdird: waiting to receive file attributes\n");
   /* Pickup file attributes and digest */
   while (!fd->errors && (n = bget_dirmsg(fd)) > 0) {
      uint32_t file_index;
      int stream, len;
      char *p, *fn;
      char Digest[MAXSTRING];      /* either Verify opts or MD5/SHA1 digest */

      if ((len = sscanf(fd->msg, "%ld %d %s", &file_index, &stream, Digest)) != 3) {
         Jmsg(jcr, M_FATAL, 0, _("<filed: bad attributes, expected 3 fields got %d\n"
"msglen=%d msg=%s\n"), len, fd->msglen, fd->msg);
         set_jcr_job_status(jcr, JS_ErrorTerminated);
         return 0;
      }
      p = fd->msg;
      /* The following three fields were sscanf'ed above so skip them */
      skip_nonspaces(&p);             /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Stream */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Opts_Digest */
      p++;                            /* skip space */
      Dmsg1(dbglvl, "Stream=%d\n", stream);
      if (stream == STREAM_UNIX_ATTRIBUTES || stream == STREAM_UNIX_ATTRIBUTES_EX) {
         if (jcr->cached_attribute) {
            Dmsg3(dbglvl, "Cached attr. Stream=%d fname=%s\n", ar->Stream, ar->fname,
               ar->attr);
            if (!db_create_file_attributes_record(jcr, jcr->db, ar)) {
               Jmsg1(jcr, M_FATAL, 0, _("Attribute create error. %s"), db_strerror(jcr->db));
            }
         }
         /* Any cached attr is flushed so we can reuse jcr->attr and jcr->ar */
         fn = jcr->fname = check_pool_memory_size(jcr->fname, fd->msglen);
         while (*p != 0) {
            *fn++ = *p++;                /* copy filename */
         }
         *fn = *p++;                     /* term filename and point p to attribs */
         pm_strcpy(jcr->attr, p);        /* save attributes */
         jcr->JobFiles++;
         jcr->FileIndex = file_index;
         ar->attr = jcr->attr;
         ar->fname = jcr->fname;
         ar->FileIndex = file_index;
         ar->Stream = stream;
         ar->link = NULL;
         ar->JobId = jcr->JobId;
         ar->ClientId = jcr->ClientId;
         ar->PathId = 0;
         ar->FilenameId = 0;
         ar->Digest = NULL;
         ar->DigestType = CRYPTO_DIGEST_NONE;
         jcr->cached_attribute = true;

         Dmsg2(dbglvl, "dird<filed: stream=%d %s\n", stream, jcr->fname);
         Dmsg1(dbglvl, "dird<filed: attr=%s\n", ar->attr);
         jcr->FileId = ar->FileId;
      /*
       * First, get STREAM_UNIX_ATTRIBUTES and fill ATTR_DBR structure
       * Next, we CAN have a CRYPTO_DIGEST, so we fill ATTR_DBR with it (or not)
       * When we get a new STREAM_UNIX_ATTRIBUTES, we known that we can add file to the catalog
       * At the end, we have to add the last file
       */
      } else if (crypto_digest_stream_type(stream) != CRYPTO_DIGEST_NONE) {
         if (jcr->FileIndex != (uint32_t)file_index) {
            Jmsg3(jcr, M_ERROR, 0, _("%s index %d not same as attributes %d\n"),
               stream_to_ascii(stream), file_index, jcr->FileIndex);
            continue;
         }
         ar->Digest = digest;
         ar->DigestType = crypto_digest_stream_type(stream);
         db_escape_string(jcr, jcr->db, digest, Digest, strlen(Digest));
         Dmsg4(dbglvl, "stream=%d DigestLen=%d Digest=%s type=%d\n", stream,
               strlen(digest), digest, ar->DigestType);
      }
      jcr->jr.JobFiles = jcr->JobFiles = file_index;
      jcr->jr.LastIndex = file_index;
   }
   if (is_bnet_error(fd)) {
      Jmsg1(jcr, M_FATAL, 0, _("<filed: Network error getting attributes. ERR=%s\n"),
            fd->bstrerror());
      return 0;
   }
   if (jcr->cached_attribute) {
      Dmsg3(dbglvl, "Cached attr with digest. Stream=%d fname=%s attr=%s\n", ar->Stream,            
         ar->fname, ar->attr);
      if (!db_create_file_attributes_record(jcr, jcr->db, ar)) {
         Jmsg1(jcr, M_FATAL, 0, _("Attribute create error. %s"), db_strerror(jcr->db));
      }
      jcr->cached_attribute = false; 
   }
   set_jcr_job_status(jcr, JS_Terminated);
   return 1;
}
