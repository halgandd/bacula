/*
 * Bacula message handling routines
 *
 *   Kern Sibbald, April 2000
 *
 *   Version $Id$
 *
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as ammended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */



#include "bacula.h"
#include "jcr.h"

#if !defined(HAVE_CONSOLE)
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
#include <windows.h>
#endif
#endif

#define FULL_LOCATION 1               /* set for file:line in Debug messages */

/*
 *  This is where we define "Globals" because all the
 *    daemons include this file.
 */
const char *working_directory = NULL;       /* working directory path stored here */
int verbose = 0;                      /* increase User messages */
int debug_level = 0;                  /* debug level */
time_t daemon_start_time = 0;         /* Daemon start time */
const char *version = VERSION " (" BDATE ")";
char my_name[30];                     /* daemon name is stored here */
char *exepath = (char *)NULL;
char *exename = (char *)NULL;
int console_msg_pending = 0;
char con_fname[500];                  /* Console filename */
FILE *con_fd = NULL;                  /* Console file descriptor */
brwlock_t con_lock;                   /* Console lock structure */

#ifdef HAVE_POSTGRESQL
char catalog_db[] = "PostgreSQL";
#else
#ifdef HAVE_MYSQL
char catalog_db[] = "MySQL";
#else
#ifdef HAVE_SQLITE
char catalog_db[] = "SQLite";
#else
char catalog_db[] = "Internal";
#endif
#endif
#endif

const char *host_os = HOST_OS;
const char *distname = DISTNAME;
const char *distver = DISTVER;
static FILE *trace_fd = NULL;
#ifdef HAVE_WIN32
static bool trace = true;
#else
static bool trace = false;
#endif

/* Forward referenced functions */

/* Imported functions */


/* Static storage */

/* Used to allow only one thread close the daemon messages at a time */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static MSGS *daemon_msgs;              /* global messages */


/*
 * Set daemon name. Also, find canonical execution
 *  path.  Note, exepath has spare room for tacking on
 *  the exename so that we can reconstruct the full name.
 *
 * Note, this routine can get called multiple times
 *  The second time is to put the name as found in the
 *  Resource record. On the second call, generally,
 *  argv is NULL to avoid doing the path code twice.
 */
#define BTRACE_EXTRA 20
void my_name_is(int argc, char *argv[], const char *name)
{
   char *l, *p, *q;
   char cpath[1024];
   int len;

   bstrncpy(my_name, name, sizeof(my_name));
   if (argc>0 && argv && argv[0]) {
      /* strip trailing filename and save exepath */
      for (l=p=argv[0]; *p; p++) {
         if (*p == '/') {
            l = p;                       /* set pos of last slash */
         }
      }
      if (*l == '/') {
         l++;
      } else {
         l = argv[0];
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
         /* On Windows allow c: junk */
         if (l[1] == ':') {
            l += 2;
         }
#endif
      }
      len = strlen(l) + 1;
      if (exename) {
         free(exename);
      }
      exename = (char *)malloc(len);
      strcpy(exename, l);

      if (exepath) {
         free(exepath);
      }
      exepath = (char *)malloc(strlen(argv[0]) + 1 + len);
      for (p=argv[0],q=exepath; p < l; ) {
         *q++ = *p++;
      }
      *q = 0;
      if (strchr(exepath, '.') || exepath[0] != '/') {
         if (getcwd(cpath, sizeof(cpath))) {
            free(exepath);
            exepath = (char *)malloc(strlen(cpath) + 1 + len);
            strcpy(exepath, cpath);
         }
      }
      Dmsg2(500, "exepath=%s\nexename=%s\n", exepath, exename);
   }
}

/*
 * Initialize message handler for a daemon or a Job
 *   We make a copy of the MSGS resource passed, so it belows
 *   to the job or daemon and thus can be modified.
 *
 *   NULL for jcr -> initialize global messages for daemon
 *   non-NULL     -> initialize jcr using Message resource
 */
void
init_msg(JCR *jcr, MSGS *msg)
{
   DEST *d, *dnew, *temp_chain = NULL;
   int i;

   if (jcr == NULL && msg == NULL) {
      init_last_jobs_list();
   }

#ifndef HAVE_WIN32
   /*
    * Make sure we have fd's 0, 1, 2 open
    *  If we don't do this one of our sockets may open
    *  there and if we then use stdout, it could
    *  send total garbage to our socket.
    *
    */
   int fd;
   fd = open("/dev/null", O_RDONLY, 0644);
   if (fd > 2) {
      close(fd);
   } else {
      for(i=1; fd + i <= 2; i++) {
         dup2(fd, fd+i);
      }
   }

#endif
   /*
    * If msg is NULL, initialize global chain for STDOUT and syslog
    */
   if (msg == NULL) {
      daemon_msgs = (MSGS *)malloc(sizeof(MSGS));
      memset(daemon_msgs, 0, sizeof(MSGS));
#ifndef WIN32
      for (i=1; i<=M_MAX; i++) {
         add_msg_dest(daemon_msgs, MD_STDOUT, i, NULL, NULL);
      }
#endif
      Dmsg1(050, "Create daemon global message resource 0x%x\n", daemon_msgs);
      return;
   }

   /*
    * Walk down the message resource chain duplicating it
    * for the current Job.
    */
   for (d=msg->dest_chain; d; d=d->next) {
      dnew = (DEST *)malloc(sizeof(DEST));
      memcpy(dnew, d, sizeof(DEST));
      dnew->next = temp_chain;
      dnew->fd = NULL;
      dnew->mail_filename = NULL;
      if (d->mail_cmd) {
         dnew->mail_cmd = bstrdup(d->mail_cmd);
      }
      if (d->where) {
         dnew->where = bstrdup(d->where);
      }
      temp_chain = dnew;
   }

   if (jcr) {
      jcr->jcr_msgs = (MSGS *)malloc(sizeof(MSGS));
      memset(jcr->jcr_msgs, 0, sizeof(MSGS));
      jcr->jcr_msgs->dest_chain = temp_chain;
      memcpy(jcr->jcr_msgs->send_msg, msg->send_msg, sizeof(msg->send_msg));
   } else {
      /* If we have default values, release them now */
      if (daemon_msgs) {
         free_msgs_res(daemon_msgs);
      }
      daemon_msgs = (MSGS *)malloc(sizeof(MSGS));
      memset(daemon_msgs, 0, sizeof(MSGS));
      daemon_msgs->dest_chain = temp_chain;
      memcpy(daemon_msgs->send_msg, msg->send_msg, sizeof(msg->send_msg));
   }
   Dmsg2(250, "Copy message resource 0x%x to 0x%x\n", msg, temp_chain);

}

/* Initialize so that the console (User Agent) can
 * receive messages -- stored in a file.
 */
void init_console_msg(const char *wd)
{
   int fd;

   bsnprintf(con_fname, sizeof(con_fname), "%s/%s.conmsg", wd, my_name);
   fd = open(con_fname, O_CREAT|O_RDWR|O_BINARY, 0600);
   if (fd == -1) {
      berrno be;
      Emsg2(M_ERROR_TERM, 0, _("Could not open console message file %s: ERR=%s\n"),
          con_fname, be.strerror());
   }
   if (lseek(fd, 0, SEEK_END) > 0) {
      console_msg_pending = 1;
   }
   close(fd);
   con_fd = fopen(con_fname, "a+");
   if (!con_fd) {
      berrno be;
      Emsg2(M_ERROR, 0, _("Could not open console message file %s: ERR=%s\n"),
          con_fname, be.strerror());
   }
   if (rwl_init(&con_lock) != 0) {
      berrno be;
      Emsg1(M_ERROR_TERM, 0, _("Could not get con mutex: ERR=%s\n"),
         be.strerror());
   }
}

/*
 * Called only during parsing of the config file.
 *
 * Add a message destination. I.e. associate a message type with
 *  a destination (code).
 * Note, where in the case of dest_code FILE is a filename,
 *  but in the case of MAIL is a space separated list of
 *  email addresses, ...
 */
void add_msg_dest(MSGS *msg, int dest_code, int msg_type, char *where, char *mail_cmd)
{
   DEST *d;
   /*
    * First search the existing chain and see if we
    * can simply add this msg_type to an existing entry.
    */
   for (d=msg->dest_chain; d; d=d->next) {
      if (dest_code == d->dest_code && ((where == NULL && d->where == NULL) ||
                     (strcmp(where, d->where) == 0))) {
         Dmsg4(850, "Add to existing d=%x msgtype=%d destcode=%d where=%s\n",
             d, msg_type, dest_code, NPRT(where));
         set_bit(msg_type, d->msg_types);
         set_bit(msg_type, msg->send_msg);  /* set msg_type bit in our local */
         return;
      }
   }
   /* Not found, create a new entry */
   d = (DEST *)malloc(sizeof(DEST));
   memset(d, 0, sizeof(DEST));
   d->next = msg->dest_chain;
   d->dest_code = dest_code;
   set_bit(msg_type, d->msg_types);      /* set type bit in structure */
   set_bit(msg_type, msg->send_msg);     /* set type bit in our local */
   if (where) {
      d->where = bstrdup(where);
   }
   if (mail_cmd) {
      d->mail_cmd = bstrdup(mail_cmd);
   }
   Dmsg5(850, "add new d=%x msgtype=%d destcode=%d where=%s mailcmd=%s\n",
          d, msg_type, dest_code, NPRT(where), NPRT(d->mail_cmd));
   msg->dest_chain = d;
}

/*
 * Called only during parsing of the config file.
 *
 * Remove a message destination
 */
void rem_msg_dest(MSGS *msg, int dest_code, int msg_type, char *where)
{
   DEST *d;

   for (d=msg->dest_chain; d; d=d->next) {
      Dmsg2(850, "Remove_msg_dest d=%x where=%s\n", d, NPRT(d->where));
      if (bit_is_set(msg_type, d->msg_types) && (dest_code == d->dest_code) &&
          ((where == NULL && d->where == NULL) ||
                     (strcmp(where, d->where) == 0))) {
         Dmsg3(850, "Found for remove d=%x msgtype=%d destcode=%d\n",
               d, msg_type, dest_code);
         clear_bit(msg_type, d->msg_types);
         Dmsg0(850, "Return rem_msg_dest\n");
         return;
      }
   }
}


/*
 * Create a unique filename for the mail command
 */
static void make_unique_mail_filename(JCR *jcr, POOLMEM *&name, DEST *d)
{
   if (jcr) {
      Mmsg(name, "%s/%s.mail.%s.%d", working_directory, my_name,
                 jcr->Job, (int)(long)d);
   } else {
      Mmsg(name, "%s/%s.mail.%s.%d", working_directory, my_name,
                 my_name, (int)(long)d);
   }
   Dmsg1(850, "mailname=%s\n", name);
}

/*
 * Open a mail pipe
 */
static BPIPE *open_mail_pipe(JCR *jcr, POOLMEM *&cmd, DEST *d)
{
   BPIPE *bpipe;

   if (d->mail_cmd) {
      cmd = edit_job_codes(jcr, cmd, d->mail_cmd, d->where);
   } else {
      Mmsg(cmd, "/usr/lib/sendmail -F Bacula %s", d->where);
   }
   fflush(stdout);

   if (!(bpipe = open_bpipe(cmd, 120, "rw"))) {
      berrno be;
      Jmsg(jcr, M_ERROR, 0, "open mail pipe %s failed: ERR=%s\n",
         cmd, be.strerror());
   }

   /* If we had to use sendmail, add subject */
   if (!d->mail_cmd) {
       fprintf(bpipe->wfd, "Subject: Bacula Message\r\n\r\n");
   }

   return bpipe;
}

/*
 * Close the messages for this Messages resource, which means to close
 *  any open files, and dispatch any pending email messages.
 */
void close_msg(JCR *jcr)
{
   MSGS *msgs;
   DEST *d;
   BPIPE *bpipe;
   POOLMEM *cmd, *line;
   int len, stat;

   Dmsg1(850, "Close_msg jcr=0x%x\n", jcr);

   if (jcr == NULL) {                /* NULL -> global chain */
      msgs = daemon_msgs;
      P(mutex);                       /* only one thread walking the chain */
   } else {
      msgs = jcr->jcr_msgs;
      jcr->jcr_msgs = NULL;
   }
   if (msgs == NULL) {
      return;
   }
   Dmsg1(850, "===Begin close msg resource at 0x%x\n", msgs);
   cmd = get_pool_memory(PM_MESSAGE);
   for (d=msgs->dest_chain; d; ) {
      if (d->fd) {
         switch (d->dest_code) {
         case MD_FILE:
         case MD_APPEND:
            if (d->fd) {
               fclose(d->fd);            /* close open file descriptor */
            }
            break;
         case MD_MAIL:
         case MD_MAIL_ON_ERROR:
            Dmsg0(850, "Got MD_MAIL or MD_MAIL_ON_ERROR\n");
            if (!d->fd) {
               break;
            }
            if (d->dest_code == MD_MAIL_ON_ERROR && jcr &&
                jcr->JobStatus == JS_Terminated) {
               goto rem_temp_file;
            }

            if (!(bpipe=open_mail_pipe(jcr, cmd, d))) {
               Pmsg0(000, "open mail pipe failed.\n");
               goto rem_temp_file;
            }
            Dmsg0(850, "Opened mail pipe\n");
            len = d->max_len+10;
            line = get_memory(len);
            rewind(d->fd);
            while (fgets(line, len, d->fd)) {
               fputs(line, bpipe->wfd);
            }
            if (!close_wpipe(bpipe)) {       /* close write pipe sending mail */
               berrno be;
               Pmsg1(000, "close error: ERR=%s\n", be.strerror());
            }

            /*
             * Since we are closing all messages, before "recursing"
             * make sure we are not closing the daemon messages, otherwise
             * kaboom.
             */
            if (msgs != daemon_msgs) {
               /* read what mail prog returned -- should be nothing */
               while (fgets(line, len, bpipe->rfd)) {
                  Jmsg1(jcr, M_INFO, 0, _("Mail prog: %s"), line);
               }
            }

            stat = close_bpipe(bpipe);
            if (stat != 0 && msgs != daemon_msgs) {
               berrno be;
               be.set_errno(stat);
               Dmsg1(850, "Calling emsg. CMD=%s\n", cmd);
               Jmsg2(jcr, M_ERROR, 0, _("Mail program terminated in error.\n"
                                        "CMD=%s\n"
                                        "ERR=%s\n"), cmd, be.strerror());
            }
            free_memory(line);
rem_temp_file:
            /* Remove temp file */
            fclose(d->fd);
            unlink(d->mail_filename);
            free_pool_memory(d->mail_filename);
            d->mail_filename = NULL;
            Dmsg0(850, "end mail or mail on error\n");
            break;
         default:
            break;
         }
         d->fd = NULL;
      }
      d = d->next;                    /* point to next buffer */
   }
   free_pool_memory(cmd);
   Dmsg0(850, "Done walking message chain.\n");
   if (jcr) {
      free_msgs_res(msgs);
      msgs = NULL;
   } else {
      V(mutex);
   }
   Dmsg0(850, "===End close msg resource\n");
}

/*
 * Free memory associated with Messages resource
 */
void free_msgs_res(MSGS *msgs)
{
   DEST *d, *old;

   /* Walk down the message chain releasing allocated buffers */
   for (d=msgs->dest_chain; d; ) {
      if (d->where) {
         free(d->where);
      }
      if (d->mail_cmd) {
         free(d->mail_cmd);
      }
      old = d;                        /* save pointer to release */
      d = d->next;                    /* point to next buffer */
      free(old);                      /* free the destination item */
   }
   msgs->dest_chain = NULL;
   free(msgs);                        /* free the head */
}


/*
 * Terminate the message handler for good.
 * Release the global destination chain.
 *
 * Also, clean up a few other items (cons, exepath). Note,
 *   these really should be done elsewhere.
 */
void term_msg()
{
   Dmsg0(850, "Enter term_msg\n");
   close_msg(NULL);                   /* close global chain */
   free_msgs_res(daemon_msgs);        /* free the resources */
   daemon_msgs = NULL;
   if (con_fd) {
      fflush(con_fd);
      fclose(con_fd);
      con_fd = NULL;
   }
   if (exepath) {
      free(exepath);
      exepath = NULL;
   }
   if (exename) {
      free(exename);
      exename = NULL;
   }
   if (trace_fd) {
      fclose(trace_fd);
      trace_fd = NULL;
   }
   term_last_jobs_list();
}



/*
 * Handle sending the message to the appropriate place
 */
void dispatch_message(JCR *jcr, int type, time_t mtime, char *msg)
{
    DEST *d;
    char dt[MAX_TIME_LENGTH];
    POOLMEM *mcmd;
    int len, dtlen;
    MSGS *msgs;
    BPIPE *bpipe;

    Dmsg2(850, "Enter dispatch_msg type=%d msg=%s", type, msg);

    /*
     * Most messages are prefixed by a date and time. If mtime is
     *  zero, then we use the current time.  If mtime is 1 (special
     *  kludge), we do not prefix the date and time. Otherwise,
     *  we assume mtime is a time_t and use it.
     */
    if (mtime == 0) {
       mtime = time(NULL);
    }
    if (mtime == 1) {
       *dt = 0;
       dtlen = 0;
    } else {
       bstrftime_ny(dt, sizeof(dt), mtime);
       dtlen = strlen(dt);
       dt[dtlen++] = ' ';
       dt[dtlen] = 0;
    }

    if (type == M_ABORT || type == M_ERROR_TERM) {
#ifndef HAVE_WIN32
       fputs(dt, stdout);
       fputs(msg, stdout);         /* print this here to INSURE that it is printed */
       fflush(stdout);
#endif
    }

    /* Now figure out where to send the message */
    msgs = NULL;
    if (jcr) {
       msgs = jcr->jcr_msgs;
    }
    if (msgs == NULL) {
       msgs = daemon_msgs;
    }
    for (d=msgs->dest_chain; d; d=d->next) {
       if (bit_is_set(type, d->msg_types)) {
          switch (d->dest_code) {
             case MD_CONSOLE:
                Dmsg1(850, "CONSOLE for following msg: %s", msg);
                if (!con_fd) {
                   con_fd = fopen(con_fname, "a+");
                   Dmsg0(850, "Console file not open.\n");
                }
                if (con_fd) {
                   Pw(con_lock);      /* get write lock on console message file */
                   errno = 0;
                   if (dtlen) {
                      fwrite(dt, dtlen, 1, con_fd);
                   }
                   len = strlen(msg);
                   if (len > 0) {
                      fwrite(msg, len, 1, con_fd);
                      if (msg[len-1] != '\n') {
                         fwrite("\n", 2, 1, con_fd);
                      }
                   } else {
                      fwrite("\n", 2, 1, con_fd);
                   }
                   fflush(con_fd);
                   console_msg_pending = TRUE;
                   Vw(con_lock);
                }
                break;
             case MD_SYSLOG:
                Dmsg1(850, "SYSLOG for collowing msg: %s\n", msg);
                /*
                 * We really should do an openlog() here.
                 */
                syslog(LOG_DAEMON|LOG_ERR, "%s", msg);
                break;
             case MD_OPERATOR:
                Dmsg1(850, "OPERATOR for following msg: %s\n", msg);
                mcmd = get_pool_memory(PM_MESSAGE);
                if ((bpipe=open_mail_pipe(jcr, mcmd, d))) {
                   int stat;
                   fputs(dt, bpipe->wfd);
                   fputs(msg, bpipe->wfd);
                   /* Messages to the operator go one at a time */
                   stat = close_bpipe(bpipe);
                   if (stat != 0) {
                      berrno be;
                      be.set_errno(stat);
                      Qmsg2(jcr, M_ERROR, 0, _("Operator mail program terminated in error.\n"
                            "CMD=%s\n"
                            "ERR=%s\n"), mcmd, be.strerror());
                   }
                }
                free_pool_memory(mcmd);
                break;
             case MD_MAIL:
             case MD_MAIL_ON_ERROR:
                Dmsg1(850, "MAIL for following msg: %s", msg);
                if (!d->fd) {
                   POOLMEM *name = get_pool_memory(PM_MESSAGE);
                   make_unique_mail_filename(jcr, name, d);
                   d->fd = fopen(name, "w+");
                   if (!d->fd) {
                      berrno be;
                      d->fd = stdout;
                      Qmsg2(jcr, M_ERROR, 0, "fopen %s failed: ERR=%s\n", name,
                            be.strerror());
                      d->fd = NULL;
                      free_pool_memory(name);
                      break;
                   }
                   d->mail_filename = name;
                }
                fputs(dt, d->fd);
                len = strlen(msg) + dtlen;;
                if (len > d->max_len) {
                   d->max_len = len;      /* keep max line length */
                }
                fputs(msg, d->fd);
                break;
             case MD_FILE:
                Dmsg1(850, "FILE for following msg: %s", msg);
                if (!d->fd) {
                   d->fd = fopen(d->where, "w+");
                   if (!d->fd) {
                      berrno be;
                      d->fd = stdout;
                      Qmsg2(jcr, M_ERROR, 0, "fopen %s failed: ERR=%s\n", d->where,
                            be.strerror());
                      d->fd = NULL;
                      break;
                   }
                }
                fputs(dt, d->fd);
                fputs(msg, d->fd);
                break;
             case MD_APPEND:
                Dmsg1(850, "APPEND for following msg: %s", msg);
                if (!d->fd) {
                   d->fd = fopen(d->where, "a");
                   if (!d->fd) {
                      berrno be;
                      d->fd = stdout;
                      Qmsg2(jcr, M_ERROR, 0, "fopen %s failed: ERR=%s\n", d->where,
                            be.strerror());
                      d->fd = NULL;
                      break;
                   }
                }
                fputs(dt, d->fd);
                fputs(msg, d->fd);
                break;
             case MD_DIRECTOR:
                Dmsg1(850, "DIRECTOR for following msg: %s", msg);
                if (jcr && jcr->dir_bsock && !jcr->dir_bsock->errors) {
                   bnet_fsend(jcr->dir_bsock, "Jmsg Job=%s type=%d level=%d %s",
                      jcr->Job, type, mtime, msg);
                }
                break;
             case MD_STDOUT:
                Dmsg1(850, "STDOUT for following msg: %s", msg);
                if (type != M_ABORT && type != M_ERROR_TERM) { /* already printed */
                   fputs(dt, stdout);
                   fputs(msg, stdout);
                }
                break;
             case MD_STDERR:
                Dmsg1(850, "STDERR for following msg: %s", msg);
                fputs(dt, stderr);
                fputs(msg, stderr);
                break;
             default:
                break;
          }
       }
    }
}


/*********************************************************************
 *
 *  This subroutine prints a debug message if the level number
 *  is less than or equal the debug_level. File and line numbers
 *  are included for more detail if desired, but not currently
 *  printed.
 *
 *  If the level is negative, the details of file and line number
 *  are not printed.
 */
void
d_msg(const char *file, int line, int level, const char *fmt,...)
{
    char      buf[5000];
    int       len;
    va_list   arg_ptr;
    bool      details = true;

    if (level < 0) {
       details = false;
       level = -level;
    }

    if (level <= debug_level) {
#ifdef FULL_LOCATION
       if (details) {
          /* visual studio passes the whole path to the file as well
           * which makes for very long lines
           */
          const char *f = strrchr(file, '\\');
          if (f) file = f + 1;
          len = bsnprintf(buf, sizeof(buf), "%s: %s:%d ", my_name, file, line);
       } else {
          len = 0;
       }
#else
       len = 0;
#endif
       va_start(arg_ptr, fmt);
       bvsnprintf(buf+len, sizeof(buf)-len, (char *)fmt, arg_ptr);
       va_end(arg_ptr);

       /*
        * Used the "trace on" command in the console to turn on
        *  output to the trace file.  "trace off" will close the file.
        */
       if (trace) {
          if (!trace_fd) {
             char fn[200];
             bsnprintf(fn, sizeof(fn), "%s/bacula.trace", working_directory ? working_directory : ".");
             trace_fd = fopen(fn, "a+");
          }
          if (trace_fd) {
             fputs(buf, trace_fd);
             fflush(trace_fd);
          } else {
             /* Some problem, turn off tracing */
             trace = false;
          }
       } else {   /* not tracing */
          fputs(buf, stdout);
       }
    }
}

/*
 * Set trace flag on/off. If argument is negative, there is no change
 */
void set_trace(int trace_flag)
{
   if (trace_flag < 0) {
      return;
   } else if (trace_flag > 0) {
      trace = true;
   } else {
      trace = false;
   }
   if (!trace && trace_fd) {
      FILE *ltrace_fd = trace_fd;
      trace_fd = NULL;
      bmicrosleep(0, 100000);         /* yield to prevent seg faults */
      fclose(ltrace_fd);
   }
}

bool get_trace(void)
{
   return trace;
}

/*********************************************************************
 *
 *  This subroutine prints a message regardless of the debug level
 *
 *  If the level is negative, the details of file and line number
 *  are not printed.
 */
void
p_msg(const char *file, int line, int level, const char *fmt,...)
{
    char      buf[5000];
    int       len;
    va_list   arg_ptr;

#ifdef FULL_LOCATION
    if (level >= 0) {
       len = bsnprintf(buf, sizeof(buf), "%s: %s:%d ", my_name, file, line);
    } else {
       len = 0;
    }
#else
    len = 0;
#endif
    va_start(arg_ptr, fmt);
    bvsnprintf(buf+len, sizeof(buf)-len, (char *)fmt, arg_ptr);
    va_end(arg_ptr);
    fputs(buf, stdout);
}


/*********************************************************************
 *
 *  subroutine writes a debug message to the trace file if the level number
 *  is less than or equal the debug_level. File and line numbers
 *  are included for more detail if desired, but not currently
 *  printed.
 *
 *  If the level is negative, the details of file and line number
 *  are not printed.
 */
void
t_msg(const char *file, int line, int level, const char *fmt,...)
{
    char      buf[5000];
    int       len;
    va_list   arg_ptr;
    int       details = TRUE;

    if (level < 0) {
       details = FALSE;
       level = -level;
    }

    if (level <= debug_level) {
       if (!trace_fd) {
          bsnprintf(buf, sizeof(buf), "%s/bacula.trace", working_directory);
          trace_fd = fopen(buf, "a+");
       }

#ifdef FULL_LOCATION
       if (details) {
          len = bsnprintf(buf, sizeof(buf), "%s: %s:%d ", my_name, file, line);
       } else {
          len = 0;
       }
#else
       len = 0;
#endif
       va_start(arg_ptr, fmt);
       bvsnprintf(buf+len, sizeof(buf)-len, (char *)fmt, arg_ptr);
       va_end(arg_ptr);
       if (trace_fd != NULL) {
           fputs(buf, trace_fd);
           fflush(trace_fd);
       }
   }
}



/* *********************************************************
 *
 * print an error message
 *
 */
void
e_msg(const char *file, int line, int type, int level, const char *fmt,...)
{
    char     buf[5000];
    va_list   arg_ptr;
    int len;

    /*
     * Check if we have a message destination defined.
     * We always report M_ABORT and M_ERROR_TERM
     */
    if (!daemon_msgs || ((type != M_ABORT && type != M_ERROR_TERM) &&
                         !bit_is_set(type, daemon_msgs->send_msg))) {
       return;                        /* no destination */
    }
    switch (type) {
    case M_ABORT:
       len = bsnprintf(buf, sizeof(buf), "%s: ABORTING due to ERROR in %s:%d\n",
               my_name, file, line);
       break;
    case M_ERROR_TERM:
       len = bsnprintf(buf, sizeof(buf), "%s: ERROR TERMINATION at %s:%d\n",
               my_name, file, line);
       break;
    case M_FATAL:
       if (level == -1)            /* skip details */
          len = bsnprintf(buf, sizeof(buf), "%s: Fatal Error because: ", my_name);
       else
          len = bsnprintf(buf, sizeof(buf), "%s: Fatal Error at %s:%d because:\n", my_name, file, line);
       break;
    case M_ERROR:
       if (level == -1)            /* skip details */
          len = bsnprintf(buf, sizeof(buf), "%s: ERROR: ", my_name);
       else
          len = bsnprintf(buf, sizeof(buf), "%s: ERROR in %s:%d ", my_name, file, line);
       break;
    case M_WARNING:
       len = bsnprintf(buf, sizeof(buf), "%s: Warning: ", my_name);
       break;
    case M_SECURITY:
       len = bsnprintf(buf, sizeof(buf), "%s: Security violation: ", my_name);
       break;
    default:
       len = bsnprintf(buf, sizeof(buf), "%s: ", my_name);
       break;
    }

    va_start(arg_ptr, fmt);
    bvsnprintf(buf+len, sizeof(buf)-len, (char *)fmt, arg_ptr);
    va_end(arg_ptr);

    dispatch_message(NULL, type, 0, buf);

    if (type == M_ABORT) {
       char *p = 0;
       p[0] = 0;                      /* generate segmentation violation */
    }
    if (type == M_ERROR_TERM) {
       exit(1);
    }
}

/* *********************************************************
 *
 * Generate a Job message
 *
 */
void
Jmsg(JCR *jcr, int type, time_t mtime, const char *fmt,...)
{
    char     rbuf[5000];
    va_list   arg_ptr;
    int len;
    MSGS *msgs;
    const char *job;


    Dmsg1(850, "Enter Jmsg type=%d\n", type);

    /* Special case for the console, which has a dir_bsock and JobId==0,
     *  in that case, we send the message directly back to the
     *  dir_bsock.
     */
    if (jcr && jcr->JobId == 0 && jcr->dir_bsock) {
       BSOCK *dir = jcr->dir_bsock;
       va_start(arg_ptr, fmt);
       dir->msglen = bvsnprintf(dir->msg, sizeof_pool_memory(dir->msg),
                                fmt, arg_ptr);
       va_end(arg_ptr);
       bnet_send(jcr->dir_bsock);
       return;
    }

    msgs = NULL;
    job = NULL;
    if (jcr) {
       msgs = jcr->jcr_msgs;
       job = jcr->Job;
    }
    if (!msgs) {
       msgs = daemon_msgs;            /* if no jcr, we use daemon handler */
    }
    if (!job) {
       job = "";                      /* Set null job name if none */
    }

    /*
     * Check if we have a message destination defined.
     * We always report M_ABORT and M_ERROR_TERM
     */
    if (msgs && (type != M_ABORT && type != M_ERROR_TERM) &&
         !bit_is_set(type, msgs->send_msg)) {
       return;                        /* no destination */
    }
    switch (type) {
    case M_ABORT:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s ABORTING due to ERROR\n", my_name);
       break;
    case M_ERROR_TERM:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s ERROR TERMINATION\n", my_name);
       break;
    case M_FATAL:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s: %s Fatal error: ", my_name, job);
       if (jcr) {
          set_jcr_job_status(jcr, JS_FatalError);
       }
       break;
    case M_ERROR:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s: %s Error: ", my_name, job);
       if (jcr) {
          jcr->Errors++;
       }
       break;
    case M_WARNING:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s: %s Warning: ", my_name, job);
       break;
    case M_SECURITY:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s: %s Security violation: ", my_name, job);
       break;
    default:
       len = bsnprintf(rbuf, sizeof(rbuf), "%s: ", my_name);
       break;
    }

    va_start(arg_ptr, fmt);
    bvsnprintf(rbuf+len,  sizeof(rbuf)-len, fmt, arg_ptr);
    va_end(arg_ptr);

    dispatch_message(jcr, type, mtime, rbuf);

    if (type == M_ABORT){
       char *p = 0;
       p[0] = 0;                      /* generate segmentation violation */
    }
    if (type == M_ERROR_TERM) {
       exit(1);
    }
}

/*
 * If we come here, prefix the message with the file:line-number,
 *  then pass it on to the normal Jmsg routine.
 */
void j_msg(const char *file, int line, JCR *jcr, int type, time_t mtime, const char *fmt,...)
{
   va_list   arg_ptr;
   int i, len, maxlen;
   POOLMEM *pool_buf;

   pool_buf = get_pool_memory(PM_EMSG);
   i = Mmsg(pool_buf, "%s:%d ", file, line);

   for (;;) {
      maxlen = sizeof_pool_memory(pool_buf) - i - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf+i, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf = realloc_pool_memory(pool_buf, maxlen + i + maxlen/2);
         continue;
      }
      break;
   }

   Jmsg(jcr, type, mtime, "%s", pool_buf);
   free_memory(pool_buf);
}


/*
 * Edit a message into a Pool memory buffer, with file:lineno
 */
int m_msg(const char *file, int line, POOLMEM **pool_buf, const char *fmt, ...)
{
   va_list   arg_ptr;
   int i, len, maxlen;

   i = sprintf(*pool_buf, "%s:%d ", file, line);

   for (;;) {
      maxlen = sizeof_pool_memory(*pool_buf) - i - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(*pool_buf+i, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         *pool_buf = realloc_pool_memory(*pool_buf, maxlen + i + maxlen/2);
         continue;
      }
      break;
   }
   return len;
}

int m_msg(const char *file, int line, POOLMEM *&pool_buf, const char *fmt, ...)
{
   va_list   arg_ptr;
   int i, len, maxlen;

   i = sprintf(pool_buf, "%s:%d ", file, line);

   for (;;) {
      maxlen = sizeof_pool_memory(pool_buf) - i - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf+i, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf = realloc_pool_memory(pool_buf, maxlen + i + maxlen/2);
         continue;
      }
      break;
   }
   return len;
}


/*
 * Edit a message into a Pool Memory buffer NO file:lineno
 *  Returns: string length of what was edited.
 */
int Mmsg(POOLMEM **pool_buf, const char *fmt, ...)
{
   va_list   arg_ptr;
   int len, maxlen;

   for (;;) {
      maxlen = sizeof_pool_memory(*pool_buf) - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(*pool_buf, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         *pool_buf = realloc_pool_memory(*pool_buf, maxlen + maxlen/2);
         continue;
      }
      break;
   }
   return len;
}

int Mmsg(POOLMEM *&pool_buf, const char *fmt, ...)
{
   va_list   arg_ptr;
   int len, maxlen;

   for (;;) {
      maxlen = sizeof_pool_memory(pool_buf) - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf = realloc_pool_memory(pool_buf, maxlen + maxlen/2);
         continue;
      }
      break;
   }
   return len;
}

int Mmsg(POOL_MEM &pool_buf, const char *fmt, ...)
{
   va_list   arg_ptr;
   int len, maxlen;

   for (;;) {
      maxlen = pool_buf.max_size() - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf.c_str(), maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf.realloc_pm(maxlen + maxlen/2);
         continue;
      }
      break;
   }
   return len;
}


static pthread_mutex_t msg_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * We queue messages rather than print them directly. This
 *  is generally used in low level routines (msg handler, bnet)
 *  to prevent recursion (i.e. if you are in the middle of
 *  sending a message, it is a bit messy to recursively call
 *  yourself when the bnet packet is not reentrant).
 */
void Qmsg(JCR *jcr, int type, time_t mtime, const char *fmt,...)
{
   va_list   arg_ptr;
   int len, maxlen;
   POOLMEM *pool_buf;
   MQUEUE_ITEM *item;

   pool_buf = get_pool_memory(PM_EMSG);

   for (;;) {
      maxlen = sizeof_pool_memory(pool_buf) - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf = realloc_pool_memory(pool_buf, maxlen + maxlen/2);
         continue;
      }
      break;
   }
   item = (MQUEUE_ITEM *)malloc(sizeof(MQUEUE_ITEM) + strlen(pool_buf) + 1);
   item->type = type;
   item->mtime = time(NULL);
   strcpy(item->msg, pool_buf);
   /* If no jcr or dequeuing send to daemon to avoid recursion */
   if (!jcr || jcr->dequeuing) {
      /* jcr==NULL => daemon message, safe to send now */
      Jmsg(NULL, item->type, item->mtime, "%s", item->msg);
      free(item);
   } else {
      /* Queue message for later sending */
      P(msg_queue_mutex);
      jcr->msg_queue->append(item);
      V(msg_queue_mutex);
//    Dmsg1(000, "queue item=%lu\n", (long unsigned)item);
   }
   free_memory(pool_buf);
}

/*
 * Dequeue messages
 */
void dequeue_messages(JCR *jcr)
{
   MQUEUE_ITEM *item;
   P(msg_queue_mutex);
   jcr->dequeuing = true;
   foreach_dlist(item, jcr->msg_queue) {
//    Dmsg1(000, "dequeue item=%lu\n", (long unsigned)item);
      Jmsg(jcr, item->type, item->mtime, "%s", item->msg);
   }
   jcr->msg_queue->destroy();
   jcr->dequeuing = false;
   V(msg_queue_mutex);
}


/*
 * If we come here, prefix the message with the file:line-number,
 *  then pass it on to the normal Qmsg routine.
 */
void q_msg(const char *file, int line, JCR *jcr, int type, time_t mtime, const char *fmt,...)
{
   va_list   arg_ptr;
   int i, len, maxlen;
   POOLMEM *pool_buf;

   pool_buf = get_pool_memory(PM_EMSG);
   i = Mmsg(pool_buf, "%s:%d ", file, line);

   for (;;) {
      maxlen = sizeof_pool_memory(pool_buf) - i - 1;
      va_start(arg_ptr, fmt);
      len = bvsnprintf(pool_buf+i, maxlen, fmt, arg_ptr);
      va_end(arg_ptr);
      if (len < 0 || len >= (maxlen-5)) {
         pool_buf = realloc_pool_memory(pool_buf, maxlen + i + maxlen/2);
         continue;
      }
      break;
   }

   Qmsg(jcr, type, mtime, "%s", pool_buf);
   free_memory(pool_buf);
}
