/*
 *
 *   Bacula Director -- Run Command
 *
 *     Kern Sibbald, December MMI
 */

/*
   Copyright (C) 2001, 2002 Kern Sibbald and John Walker

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
#include "ua.h"

/* Imported subroutines */
extern void run_job(JCR *jcr);

/* Imported variables */
extern struct s_jl joblevels[];

/*
 * For Backup and Verify Jobs
 *     run [job=]<job-name> level=<level-name>
 *
 * For Restore Jobs
 *     run <job-name> jobid=nn
 *
 */
int runcmd(UAContext *ua, char *cmd)
{
   JOB *job;
   JCR *jcr;
   char *job_name, *level_name, *jid, *store_name;
   char *where, *fileset_name, *client_name;
   int i, j, found;
   STORE *store;
   CLIENT *client;
   FILESET *fileset;
   static char *kw[] = {
      N_("job"),
      N_("jobid"),
      N_("client"),
      N_("fileset"),
      N_("level"),
      N_("storage"),
      N_("where"),
      NULL};

   if (!open_db(ua)) {
      return 1;
   }

   job_name = NULL;
   level_name = NULL;
   jid = NULL;
   store_name = NULL;
   where = NULL;
   client_name = NULL;
   fileset_name = NULL;

   Dmsg1(20, "run: %s\n", ua->UA_sock->msg);

   for (i=1; i<ua->argc; i++) {
      found = False;
      Dmsg2(200, "Doing arg %d = %s\n", i, ua->argk[i]);
      for (j=0; kw[j]; j++) {
	 if (strcasecmp(ua->argk[i], _(kw[j])) == 0) {
	    if (!ua->argv[i]) {
               bsendmsg(ua, _("Value missing for keyword %s\n"), ua->argk[i]);
	       return 1;
	    }
            Dmsg1(200, "Got keyword=%s\n", kw[j]);
	    switch (j) {
	       case 0: /* job */
		  if (job_name) {
                     bsendmsg(ua, _("Job name specified twice.\n"));
		     return 1;
		  }
		  job_name = ua->argv[i];
		  found = True;
		  break;
	       case 1: /* JobId */
		  if (jid) {
                     bsendmsg(ua, _("JobId specified twice.\n"));
		     return 1;
		  }
		  jid = ua->argv[i];
		  found = True;
		  break;
	       case 2: /* client */
		  if (client_name) {
                     bsendmsg(ua, _("Client specified twice.\n"));
		     return 1;
		  }
		  client_name = ua->argv[i];
		  found = True;
		  break;
	       case 3: /* fileset */
		  if (fileset_name) {
                     bsendmsg(ua, _("FileSet specified twice.\n"));
		     return 1;
		  }
		  fileset_name = ua->argv[i];
		  found = True;
		  break;
	       case 4: /* level */
		  if (level_name) {
                     bsendmsg(ua, _("Level specified twice.\n"));
		     return 1;
		  }
		  level_name = ua->argv[i];
		  found = True;
		  break;
	       case 5: /* storage */
		  if (store_name) {
                     bsendmsg(ua, _("Storage specified twice.\n"));
		     return 1;
		  }
		  store_name = ua->argv[i];
		  found = True;
		  break;
	       case 6: /* where */
		  if (where) {
                     bsendmsg(ua, _("Where specified twice.\n"));
		     return 1;
		  }
		  where = ua->argv[i];
		  break;
		  found = True;
	       default:
		  break;
	    }
	 }
      } /* end keyword loop */
      if (!found) {
         Dmsg1(200, "%s not found\n", ua->argk[i]);
	 /*
	  * Special case for Job Name, it can be the first
	  * keyword that has no value.
	  */
	 if (!job_name && !ua->argv[i]) {
	    job_name = ua->argk[i];   /* use keyword as job name */
            Dmsg1(200, "Set jobname=%s\n", job_name);
	 } else {
            bsendmsg(ua, _("Invalid keyword %s\n"), ua->argk[i]);
	    return 1;
	 }
      }
   } /* end argc loop */
	     
   Dmsg0(20, "Done scan.\n");
   if (job_name) {
      /* Find Job */
      job = (JOB *)GetResWithName(R_JOB, job_name);
      if (!job) {
         bsendmsg(ua, _("Job %s: not found\n"), job_name);
	 job = select_job_resource(ua);
      } else {
         Dmsg1(20, "Found job=%s\n", job_name);
      }
   } else {
      bsendmsg(ua, _("A job name must be specified.\n"));
      job = select_job_resource(ua);
   }
   if (!job) {
      return 1;
   }

   if (store_name) {
      store = (STORE *)GetResWithName(R_STORAGE, store_name);
      if (!store) {
         bsendmsg(ua, _("Storage %s not found.\n"), store_name);
	 store = select_storage_resource(ua);
      }
   } else {
      store = job->storage;	      /* use default */
   }
   if (!store) {
      return 1;
   }

   jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   set_jcr_defaults(jcr, job);
   jcr->store = store;		      /* set possible new Storage */

try_again:
   Dmsg1(20, "JobType=%c\n", jcr->JobType);
   switch (jcr->JobType) {
      case JT_BACKUP:
      case JT_VERIFY:
	 if (level_name) {
	    /* Look up level name and pull code */
	    lcase(level_name);
	    found = 0;
	    for (i=0; joblevels[i].level_name; i++) {
	       if (strcasecmp(level_name, _(joblevels[i].level_name)) == 0) {
		  jcr->level = joblevels[i].level;
		  found = 1;
		  break;
	       }
	    }
	    if (!found) { 
               bsendmsg(ua, _("Level %s not valid.\n"), level_name);
	       free_jcr(jcr);
	       return 1;
	    }
	 }
	 level_name = NULL;
         bsendmsg(ua, _("Run %s job\n\
JobName:  %s\n\
FileSet:  %s\n\
Level:    %s\n\
Client:   %s\n\
Storage:  %s\n"),
                 jcr->JobType==JT_BACKUP?_("Backup"):_("Verify"),
		 job->hdr.name,
		 jcr->fileset->hdr.name,
		 level_to_str(jcr->level),
		 jcr->client->hdr.name,
		 jcr->store->hdr.name);
	 break;
      case JT_RESTORE:
	 if (jcr->RestoreJobId == 0) {
	    if (jid) {
	       jcr->RestoreJobId = atoi(jid);
	    } else {
               if (!get_cmd(ua, _("Please enter a JobId for restore: "))) {
		  free_jcr(jcr);
		  return 1;
	       }  
	       jcr->RestoreJobId = atoi(ua->cmd);
	    }
	 }
         jcr->level = 'F';            /* ***FIXME*** */
         Dmsg1(20, "JobId to restore=%d\n", jcr->RestoreJobId);
         bsendmsg(ua, _("Run Restore job\n\
JobName:    %s\n\
Where:      %s\n\
RestoreId:  %d\n\
Level:      %s\n\
FileSet:    %s\n\
Client:     %s\n\
Storage:    %s\n"),
		 job->hdr.name,
		 jcr->RestoreWhere?jcr->RestoreWhere:job->RestoreWhere,
		 jcr->RestoreJobId,
		 level_to_str(jcr->level),
		 jcr->fileset->hdr.name,
		 jcr->client->hdr.name,
		 jcr->store->hdr.name);
	 break;
      default:
         bsendmsg(ua, _("Unknown Job Type=%d\n"), jcr->JobType);
	 free_jcr(jcr);
	 return 1;
   }
   if (!get_cmd(ua, _("OK to run? (yes/mod/no): "))) {
      free_jcr(jcr);
      return 1;
   }
   if (strcasecmp(ua->cmd, _("mod")) == 0) {
      start_prompt(ua, _("Parameters to modify:\n"));
      add_prompt(ua, _("Job"));
      add_prompt(ua, _("Level"));
      add_prompt(ua, _("FileSet"));
      add_prompt(ua, _("Client"));
      add_prompt(ua, _("Storage"));
      if (jcr->JobType == JT_RESTORE) {
         add_prompt(ua, _("Where"));
         add_prompt(ua, _("JobId"));
      }
      switch (do_prompt(ua, _("Select parameter to modify"), NULL)) {
      case 0:
	 /* Job */
	 job = select_job_resource(ua);
	 if (job) {
	    jcr->job = job;
	    set_jcr_defaults(jcr, job);
	    goto try_again;
	 }
	 break;
      case 1:
	 /* Level */
	 if (jcr->JobType == JT_BACKUP) {
            start_prompt(ua, _("Levels:\n"));
            add_prompt(ua, _("Full"));
            add_prompt(ua, _("Incremental"));
            add_prompt(ua, _("Differential"));
            add_prompt(ua, _("Level"));
            add_prompt(ua, _("Since"));
            switch (do_prompt(ua, _("Select level"), NULL)) {
	    case 0:
	       jcr->level = L_FULL;
	       break;
	    case 1:
	       jcr->level = L_INCREMENTAL;
	       break;
	    case 2:
	       jcr->level = L_DIFFERENTIAL;
	       break;
	    case 3:
	       jcr->level = L_LEVEL;
	       break;
	    case 4:
	       jcr->level = L_SINCE;
	       break;
	    default:
	       break;
	    }
	    goto try_again;
	 } else if (jcr->JobType == JT_VERIFY) {
            start_prompt(ua, _("Levels:\n"));
            add_prompt(ua, _("Initialize Catalog"));
            add_prompt(ua, _("Verify from Catalog"));
            add_prompt(ua, _("Verify Volume"));
            add_prompt(ua, _("Verify Volume Data"));
            switch (do_prompt(ua, _("Select level"), NULL)) {
	    case 0:
	       jcr->level = L_VERIFY_INIT;
	       break;
	    case 1:
	       jcr->level = L_VERIFY_CATALOG;
	       break;
	    case 2:
	       jcr->level = L_VERIFY_VOLUME;
	       break;
	    case 3:
	       jcr->level = L_VERIFY_DATA;
	       break;
	    default:
	       break;
	    }
	    goto try_again;
	 }
	 goto try_again;
      case 2:
	 /* FileSet */
	 fileset = select_fs_resource(ua);
	 if (fileset) {
	    jcr->fileset = fileset;
	    goto try_again;
	 }	
	 break;
      case 3:
	 client = select_client_resource(ua);
	 if (client) {
	    jcr->client = client;
	    goto try_again;
	 }
	 break;
      case 4:
	 store = select_storage_resource(ua);
	 if (store) {
	    jcr->store = store;
	    goto try_again;
	 }
	 break;
      case 5:
	 /* Where */
         if (!get_cmd(ua, _("Please enter path prefix (where) for restore: "))) {
	    break;
	 }
         if (ua->cmd[0] != '/') {
            bsendmsg(ua, _("Prefix must begin with a /\n"));
	 } else {
	    if (jcr->RestoreWhere) {
	       free(jcr->RestoreWhere);
	    }
	    jcr->RestoreWhere = bstrdup(ua->cmd);
	 }  
	 goto try_again;
      case 6:
	 /* JobId */
	 jid = NULL;		      /* force reprompt */
	 jcr->RestoreJobId = 0;
	 goto try_again;
      default: 
	 goto try_again;
      }
      bsendmsg(ua, _("Job not run.\n"));
      free_jcr(jcr);
      return 1;
   }
   if (strcasecmp(ua->cmd, _("yes")) != 0) {
      bsendmsg(ua, _("Job not run.\n"));
      free_jcr(jcr);
      return 1;
   }

   Dmsg1(200, "Calling run_job job=%x\n", jcr->job);
   run_job(jcr);
   return 1;
}
