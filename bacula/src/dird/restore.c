/*
 *
 *   Bacula Director -- restore.c -- responsible for restoring files
 *
 *     Kern Sibbald, November MM
 *
 *    This routine is run as a separate thread.  There may be more
 *    work to be done to make it totally reentrant!!!!
 * 
 * Current implementation is Catalog verification only (i.e. no
 *  verification versus tape).
 *
 *  Basic tasks done here:
 *     Open DB
 *     Open Message Channel with Storage daemon to tell him a job will be starting.
 *     Open connection with File daemon and pass him commands
 *	 to do the restore.
 *     Update the DB according to what files where restored????
 *
 */

/*
   Copyright (C) 2000, 2001, 2002 Kern Sibbald and John Walker

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

/* Commands sent to File daemon */
static char restorecmd[]   = "restore where=%s\n";
static char storaddr[]     = "storage address=%s port=%d\n";
static char sessioncmd[]   = "session %s %ld %ld %ld %ld %ld %ld\n";  

/* Responses received from File daemon */
static char OKrestore[]   = "2000 OK restore\n";
static char OKstore[]     = "2000 OK storage\n";
static char OKsession[]   = "2000 OK session\n";

/* Forward referenced functions */
static void restore_cleanup(JCR *jcr, int status);

/* External functions */

/* 
 * Do a restore of the specified files
 *    
 *  Returns:  0 on failure
 *	      1 on success
 */
int do_restore(JCR *jcr) 
{
   char dt[MAX_TIME_LENGTH];
   BSOCK   *fd;
   JOB_DBR rjr; 		      /* restore job record */
   CLIENT_DBR cr;

   /*
    * Get or Create a client record
    */
   memset(&cr, 0, sizeof(cr));
   strcpy(cr.Name, jcr->client->hdr.name);
   cr.AutoPrune = jcr->client->AutoPrune;
   cr.FileRetention = jcr->client->FileRetention;
   cr.JobRetention = jcr->client->JobRetention;
   if (jcr->client_name) {
      free(jcr->client_name);
   }
   jcr->client_name = bstrdup(jcr->client->hdr.name);
   if (!db_create_client_record(jcr->db, &cr)) {
      Jmsg(jcr, M_ERROR, 0, _("Could not create Client record. %s"), 
	 db_strerror(jcr->db));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   jcr->jr.ClientId = cr.ClientId;

   memset(&rjr, 0, sizeof(rjr));
   jcr->jr.Level = 'F';            /* Full restore */
   jcr->jr.StartTime = jcr->start_time;
   if (!db_update_job_start_record(jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(jcr->db));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   Dmsg0(20, "Updated job start record\n");
   jcr->fname = (char *) get_pool_memory(PM_FNAME);

   Dmsg1(20, "RestoreJobId=%d\n", jcr->job->RestoreJobId);

   /*
    * Find Job Record for Files to be restored
    */
   if (jcr->RestoreJobId != 0) {
      rjr.JobId = jcr->RestoreJobId;	 /* specified by UA */
   } else {
      rjr.JobId = jcr->job->RestoreJobId; /* specified by Job Resource */
   }
   if (!db_get_job_record(jcr->db, &rjr)) {
      Jmsg2(jcr, M_FATAL, 0, _("Cannot get job record id=%d %s"), rjr.JobId,
	 db_strerror(jcr->db));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   Dmsg3(20, "Got JobId=%d VolSessId=%ld, VolSesTime=%ld\n", 
	     rjr.JobId, rjr.VolSessionId, rjr.VolSessionTime);
   Dmsg4(20, "StartFile=%ld, EndFile=%ld StartBlock=%ld EndBlock=%ld\n", 
	     rjr.StartFile, rjr.EndFile, rjr.StartBlock, rjr.EndBlock);

   /*
    * Now find the Volumes we will need for the Restore
    */
   jcr->VolumeName[0] = 0;
   if (!db_get_job_volume_names(jcr->db, rjr.JobId, jcr->VolumeName) ||
	jcr->VolumeName[0] == 0) {
      Jmsg(jcr, M_FATAL, 0, _("Cannot find Volume Name for restore Job %d. %s"), 
	 rjr.JobId, db_strerror(jcr->db));
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   Dmsg1(20, "Got job Volume Names: %s\n", jcr->VolumeName);
      

   /* Print Job Start message */
   bstrftime(dt, sizeof(dt), jcr->start_time);
   Jmsg(jcr, M_INFO, 0, _("%s Start Restore Job %s Name=%s, Client=%s, FileSet=%s\n"), 
      dt, jcr->Job, jcr->job->hdr.name, jcr->client->hdr.name, 
      jcr->fileset->hdr.name);

   /*
    * Open a message channel connection with the Storage
    * daemon. This is to let him know that our client
    * will be contacting him for a backup  session.
    *
    */
   Dmsg0(10, "Open connection with storage daemon\n");
   jcr->JobStatus = JS_Blocked;
   /*
    * Start conversation with Storage daemon  
    */
   if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   /*
    * Now start a job with the Storage daemon
    */
   if (!start_storage_daemon_job(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   /*
    * Now start a Storage daemon message thread
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   Dmsg0(50, "Storage daemon connection OK\n");

   /* 
    * Start conversation with File daemon  
    */
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }

   fd = jcr->file_bsock;
   jcr->JobStatus = JS_Running;

   if (!send_include_list(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }

   if (!send_exclude_list(jcr)) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }

   /* 
    * send Storage daemon address to the File daemon,
    *	then wait for File daemon to make connection
    *	with Storage daemon.
    */
   jcr->JobStatus = JS_Blocked;
   if (jcr->store->SDDport == 0) {
      jcr->store->SDDport = jcr->store->SDport;
   }
   bnet_fsend(fd, storaddr, jcr->store->address, jcr->store->SDDport);
   Dmsg1(6, "dird>filed: %s\n", fd->msg);
   if (!response(fd, OKstore, "Storage")) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }
   jcr->JobStatus = JS_Running;

   /*
    * Pass the VolSessionId, VolSessionTime, Start and
    * end File and Blocks on the session command.
    */
   bnet_fsend(fd, sessioncmd, 
	     jcr->VolumeName,
	     rjr.VolSessionId, rjr.VolSessionTime, 
	     rjr.StartFile, rjr.EndFile, rjr.StartBlock, 
	     rjr.EndBlock);
   if (!response(fd, OKsession, "Session")) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }

   /* Send restore command */
   if (jcr->RestoreWhere) {
      bnet_fsend(fd, restorecmd, jcr->RestoreWhere);
   } else {
      bnet_fsend(fd, restorecmd,			       
         jcr->job->RestoreWhere==NULL ? "" : jcr->job->RestoreWhere);
   }
   if (!response(fd, OKrestore, "Restore")) {
      restore_cleanup(jcr, JS_ErrorTerminated);
      return 0;
   }

   /* Wait for Job Termination */
   /*** ****FIXME**** get job termination data */
   Dmsg0(20, "wait for job termination\n");
   while (bget_msg(fd, 0) >  0) {
      Dmsg1(0, "dird<filed: %s\n", fd->msg);
   }

   restore_cleanup(jcr, JS_Terminated);

   return 1;
}

/*
 * Release resources allocated during restore.
 *
 */
static void restore_cleanup(JCR *jcr, int status) 
{
   char dt[MAX_TIME_LENGTH];

   Dmsg0(20, "In restore_cleanup\n");
   if (jcr->jr.EndTime == 0) {
      jcr->jr.EndTime = time(NULL);
   }
   jcr->jr.JobStatus = jcr->JobStatus = status;
   if (!db_update_job_end_record(jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_WARNING, 0, _("Error updating job record. %s"), 
	 db_strerror(jcr->db));
   }

   bstrftime(dt, sizeof(dt), jcr->jr.EndTime);
   Jmsg(jcr, M_INFO, 0, _("%s End Restore Job %s.\n"),
      dt, jcr->Job);

   Dmsg0(20, "Leaving restore_cleanup\n");
}
