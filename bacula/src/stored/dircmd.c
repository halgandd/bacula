/*
 *  This file handles accepting Director Commands
 *
 *    Most Director commands are handled here, with the 
 *    exception of the Job command command and subsequent 
 *    subcommands that are handled
 *    in job.c.  
 *
 *    File daemon commands are handled in fdcmd.c
 *
 *     Kern Sibbald, May MMI
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
#include "stored.h"

/* Exported variables */

/* Imported variables */
extern BSOCK *filed_chan;
extern int r_first, r_last;
extern struct s_res resources[];
extern char my_name[];
extern time_t daemon_start_time;
extern struct s_last_job last_job;

/* Static variables */
static char derrmsg[]       = "3900 Invalid command\n";
static char OKsetdebug[]   = "3000 OK setdebug=%d\n";


/* Imported functions */
extern void terminate_child();
extern int job_cmd(JCR *jcr);

/* Forward referenced functions */
static int label_cmd(JCR *jcr);
static int setdebug_cmd(JCR *jcr);
static int cancel_cmd(JCR *cjcr);
static int mount_cmd(JCR *jcr);
static int unmount_cmd(JCR *jcr);
static int status_cmd(JCR *sjcr);
static void label_volume_if_ok(JCR *jcr, DEVICE *dev, char *vname, char *poolname);

struct s_cmds {
   char *cmd;
   int (*func)(JCR *jcr);
};

/*  
 * The following are the recognized commands from the Director. 
 */
static struct s_cmds cmds[] = {
   {"JobId=",    job_cmd},            /* start Job */
   {"setdebug=", setdebug_cmd},       /* set debug level */
   {"cancel",    cancel_cmd},
   {"label",     label_cmd},          /* label a tape */
   {"mount",     mount_cmd},
   {"unmount",   unmount_cmd},
   {"status",    status_cmd},
   {NULL,	 NULL}		      /* list terminator */
};


/* 
 * Connection request. We accept connections either from the 
 *  Director or a Client.
 * 
 * Note, we are running as a seperate thread of the Storage daemon.
 *  and it is because a Director has made a connection with
 *  us on the "Message" channel.    
 *
 * Basic tasks done here:  
 *  - Create a JCR record
 *  - Authenticate the Director
 *  - We wait for a command
 *  - We execute the command
 *  - We continue or exit depending on the return status
 */
void connection_request(void *arg)
{
   BSOCK *bs = (BSOCK *)arg;
   JCR *jcr;
   int i, found, quit;
   int bnet_stat;
   char name[MAX_NAME_LENGTH];

   if (bnet_recv(bs) <= 0) {
      Emsg0(M_ERROR, 0, "Connection request failed.\n");
      return;
   }

   /* 
    * See if this is a File daemon connection
    */
   if (sscanf(bs->msg, "Hello Start Job %127s calling\n", name) == 1) {
      handle_filed_connection(bs, name);
      return;
   }
   
   jcr = new_jcr(sizeof(JCR), stored_free_jcr);     /* create Job Control Record */
   jcr->dir_bsock = bs; 	      /* save Director bsock */

   Dmsg0(1000, "stored in start_job\n");

   /*
    * Authenticate the Director
    */
   if (!authenticate_director(jcr)) {
      Emsg0(M_FATAL, 0, _("Unable to authenticate Director\n"));
      free_jcr(jcr);
      return;
   }
   Dmsg0(90, "Message channel init completed.\n");

   for (quit=0; !quit;) {

      /* Read command */
      if ((bnet_stat = bnet_recv(bs)) <= 0) {
	 break; 		      /* connection terminated */
      }
      Dmsg1(9, "<dird: %s\n", bs->msg);
      found = FALSE;
      for (i=0; cmds[i].cmd; i++) {
	 if (strncmp(cmds[i].cmd, bs->msg, strlen(cmds[i].cmd)) == 0) {
	    if (!cmds[i].func(jcr)) {	 /* do command */
	       quit = TRUE;		 /* error, get out */
               Dmsg1(90, "Command %s requsts quit\n", cmds[i].cmd);
	    }
	    found = TRUE;	     /* indicate command found */
	    break;
	 }
      }
      if (!found) {		      /* command not found */
	 bnet_fsend(bs, derrmsg);
	 quit = TRUE;
	 break;
      }
   }
   if (bnet_stat != BNET_TERMINATE) {
      bnet_sig(bs, BNET_TERMINATE);
   }
   free_jcr(jcr);
   return;
}

/*
 * Set debug level as requested by the Director
 *
 */
static int setdebug_cmd(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;
   int level;

   Dmsg1(10, "setdebug_cmd: %s", dir->msg);
   if (sscanf(dir->msg, "setdebug=%d", &level) != 1 || level < 0) {
      bnet_fsend(dir, "3991 Bad setdebug command: %s\n", dir->msg);
      return 0;
   }
   debug_level = level;
   return bnet_fsend(dir, OKsetdebug, level);
}


/*
 * Cancel a Job
 */
static int cancel_cmd(JCR *cjcr)
{
   BSOCK *dir = cjcr->dir_bsock;
   int oldStatus;
   char Job[MAX_NAME_LENGTH];
   JCR *jcr;

   if (sscanf(dir->msg, "cancel Job=%127s", Job) == 1) {
      if (!(jcr=get_jcr_by_full_name(Job))) {
         bnet_fsend(dir, _("3992 Job %s not found.\n"), Job);
      } else {
	 P(jcr->mutex);
	 oldStatus = jcr->JobStatus;
	 jcr->JobStatus = JS_Cancelled;
	 if (!jcr->authenticated && jcr->JobStatus == JS_WaitFD) {
	    pthread_cond_signal(&jcr->job_start_wait); /* wake waiting thread */
	 }
	 V(jcr->mutex);
	 if (jcr->file_bsock) {
	    bnet_sig(jcr->file_bsock, BNET_TERMINATE);
	 }
         bnet_fsend(dir, _("3000 Job %s Status=%c marked to be cancelled.\n"), 
	    jcr->Job, oldStatus);
	 free_jcr(jcr);
      }
   } else {
      bnet_fsend(dir, _("3993 Error scanning cancel command.\n"));
   }
   bnet_sig(dir, BNET_EOF);
   return 1;
}

/*
 * Label a tape
 *
 */
static int label_cmd(JCR *jcr) 
{
   char *dname, *volname, *poolname, *mtype;
   BSOCK *dir = jcr->dir_bsock;
   DEVRES *device;
   DEVICE *dev;
   int found = 0;

   dname = (char *) get_memory(dir->msglen+1);
   volname = (char *) get_memory(dir->msglen+1);
   poolname = (char *) get_memory(dir->msglen+1);
   mtype = (char *) get_memory(dir->msglen+1);
   if (sscanf(dir->msg, "label %s VolumeName=%s PoolName=%s MediaType=%s", 
       dname, volname, poolname, mtype) == 4) {
      unbash_spaces(dname);
      unbash_spaces(volname);
      unbash_spaces(poolname);
      unbash_spaces(mtype);
      device = NULL;
      LockRes();
      while ((device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device))) {
	 /* Find resource, and make sure we were able to open it */
	 if (strcmp(device->hdr.name, dname) == 0 && device->dev) {
            Dmsg1(20, "Found device %s\n", device->hdr.name);
	    found = 1;
	    break;
	 }
      }
      UnlockRes();
      if (found) {
	 /******FIXME**** compare MediaTypes */
	 jcr->device = device;
	 dev = device->dev;
	 P(dev->mutex);
	 if (!(dev->state & ST_OPENED)) {
	    if (open_dev(dev, volname, READ_WRITE) < 0) {
               bnet_fsend(dir, _("3994 Connot open device: %s\n"), strerror_dev(dev));
	    } else {
	       label_volume_if_ok(jcr, dev, volname, poolname);
	       force_close_dev(dev);
	    }
	 } else if (dev->dev_blocked && 
		    dev->dev_blocked != BST_DOING_ACQUIRE) {  /* device blocked? */
	    label_volume_if_ok(jcr, dev, volname, poolname);
	 } else if (dev->state & ST_READ || dev->num_writers) {
	    if (dev->state & ST_READ) {
                bnet_fsend(dir, _("3901 Device %s is busy with 1 reader.\n"),
		   dev_name(dev));
	    } else {
                bnet_fsend(dir, _("3902 Device %s is busy with %d writer(s).\n"),
		   dev_name(dev), dev->num_writers);
	    }
	 } else {		      /* device not being used */
	    label_volume_if_ok(jcr, dev, volname, poolname);
	 }
	 V(dev->mutex);
      } else {
         bnet_fsend(dir, _("3999 Device %s not found\n"), dname);
      }
   } else {
      /* NB dir->msg gets clobbered in bnet_fsend, so save command */
      strcpy(dname, dir->msg);
      bnet_fsend(dir, _("3903 Error scanning label command: %s\n"), dname);
   }
   free_memory(dname);
   free_memory(volname);
   free_memory(poolname);
   free_memory(mtype);
   bnet_sig(dir, BNET_EOF);
   return 1;
}

/* 
 * Read the tape label and determine if we can safely
 * label the tape (not a Bacula volume), then label it.
 *
 *  Enter with the mutex set
 */
static void label_volume_if_ok(JCR *jcr, DEVICE *dev, char *vname, char *poolname)
{
   BSOCK *dir = jcr->dir_bsock;
   DEV_BLOCK *block;
   int blocked;
   pthread_t no_wait_id;
   
   blocked = dev->dev_blocked;	      /* save any prev blocked state */
   no_wait_id = dev->no_wait_id;
   dev->dev_blocked = BST_WRITING_LABEL;
   dev->no_wait_id = pthread_self();  /* let us use the tape */
   V(dev->mutex);		      /* release lock */
   
   strcpy(jcr->VolumeName, vname);
   block = new_block(dev);
   switch (read_dev_volume_label(jcr, dev, block)) {		    
      case VOL_NAME_ERROR:
      case VOL_VERSION_ERROR:
      case VOL_LABEL_ERROR:
      case VOL_OK:
         bnet_fsend(dir, _("3901 Cannot label Volume because it is \
already labeled: %s\n"), dev->VolHdr.VolName);
	 break;
      case VOL_IO_ERROR:
      case VOL_NO_LABEL:
	 write_volume_label_to_dev(jcr, jcr->device, vname, poolname);
	 strcpy(jcr->VolumeName, vname);
         bnet_fsend(dir, _("3000 OK label. Volume=%s Device=%s\n"), 
	    vname, dev->dev_name);
	 break;
      default:
         bnet_fsend(dir, _("3902 Cannot label Volume. \
Unknown status %d from read_volume_label()\n"), jcr->label_status);
	 break;
   }
   free_block(block);
   P(dev->mutex);
   dev->dev_blocked = blocked;	      /* reset blocked state */
   dev->no_wait_id = no_wait_id;      /* reset blocking thread id */
}


/* 
 * Read the tape label
 *
 *  Enter with the mutex set
 */
static int read_label(JCR *jcr, DEVICE *dev)
{
   BSOCK *dir = jcr->dir_bsock;
   DEV_BLOCK *block;
   int blocked;
   pthread_t no_wait_id;
   int stat;
   
   blocked = dev->dev_blocked;	      /* save any prev blocked state */
   no_wait_id = dev->no_wait_id;
   dev->dev_blocked = BST_DOING_ACQUIRE;
   dev->no_wait_id = pthread_self();  /* let us use the tape */
   V(dev->mutex);		      /* release lock */
   
   jcr->VolumeName[0] = 0;
   block = new_block(dev);
   dev->state &= ~ST_LABEL;	      /* force read of label */
   switch (read_dev_volume_label(jcr, dev, block)) {		    
      case VOL_OK:
         bnet_fsend(dir, _("3001 Mounted Volume: %s\n"), dev->VolHdr.VolName);
	 stat = 1;
	 break;
      default:
         bnet_fsend(dir, _("3902 Cannot mount Volume on Storage Device \"%s\" because:\n%s\n"),
	    dev->dev_name, jcr->errmsg);
	 stat = 0;
	 break;
   }
   free_block(block);
   P(dev->mutex);
   dev->dev_blocked = blocked;	      /* reset blocked state */
   dev->no_wait_id = no_wait_id;      /* reset blocking thread id */
   return stat;
}

/*
 * Mount command from Director
 */
static int mount_cmd(JCR *jcr)
{
   char *dev_name;
   BSOCK *dir = jcr->dir_bsock;
   DEVRES *device;
   DEVICE *dev;
   int found = 0;

   dev_name = (char *) get_memory(dir->msglen);
   if (sscanf(dir->msg, "mount %s", dev_name) == 1) {
      unbash_spaces(dev_name);
      device = NULL;
      LockRes();
      while ((device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device))) {
	 /* Find resource, and make sure we were able to open it */
	 if (strcmp(device->hdr.name, dev_name) == 0 && device->dev) {
            Dmsg1(20, "Found device %s\n", device->hdr.name);
	    found = 1;
	    break;
	 }
      }
      UnlockRes();
      if (found) {
	 jcr->device = device;
	 dev = device->dev;
	 P(dev->mutex);
	 switch (dev->dev_blocked) {	     /* device blocked? */
	    DEV_BLOCK *block;
	    case BST_WAITING_FOR_SYSOP:
	       /* Someone is waiting, wake him */
               Dmsg0(90, "Waiting for mount attempt to wake thread\n");
	       pthread_cond_signal(&dev->wait_next_vol);
               bnet_fsend(dir, "3001 OK mount. Device=%s\n", dev->dev_name);
	       break;

	    case BST_UNMOUNTED_WAITING_FOR_SYSOP:
	    case BST_UNMOUNTED:
	       /* We freed the device, so reopen it and wake any waiting threads */
	       if (open_dev(dev, NULL, READ_WRITE) < 0) {
                  bnet_fsend(dir, _("3901 open device failed: ERR=%s\n"), 
		     strerror_dev(dev));
		  break;
	       }
	       block = new_block(dev);
	       read_dev_volume_label(jcr, dev, block);
	       free_block(block);
	       if (dev->dev_blocked == BST_UNMOUNTED) {
                  Dmsg0(90, "Unmounted unblocking device\n");
		  read_label(jcr, dev);
		  unblock_device(dev);
	       } else {
                  Dmsg0(90, "Unmounted waiting for mount attempt to wake thread\n");
		  dev->dev_blocked = BST_WAITING_FOR_SYSOP;
		  pthread_cond_signal(&dev->wait_next_vol);
	       }
	       if (dev->state & ST_LABEL) {
                  bnet_fsend(dir, _("3001 Device %s is mounted with Volume %s\n"), 
		     dev->dev_name, dev->VolHdr.VolName);
	       } else {
                  bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"), 
			     dev->dev_name);
	       }
	       break;

	    case BST_DOING_ACQUIRE:
               bnet_fsend(dir, _("3001 Device %s is mounted; doing acquire.\n"), 
			  dev->dev_name);
	       break;

	    case BST_WRITING_LABEL:
               bnet_fsend(dir, _("3903 Device %s is being labeled.\n"), dev->dev_name);
	       break;

	    case BST_NOT_BLOCKED:
	       if (dev->state & ST_OPENED) {
		  if (dev->state & ST_LABEL) {
                     bnet_fsend(dir, _("3001 Device %s is mounted with Volume %s\n"),
			dev->dev_name, dev->VolHdr.VolName);
		  } else {
                     bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"), 
				dev->dev_name);
		  }
	       } else {
		  if (!dev_is_tape(dev)) {
                     bnet_fsend(dir, _("3906 cannot mount non-tape.\n"));
		     break;
		  }
		  if (open_dev(dev, NULL, READ_WRITE) < 0) {
                     bnet_fsend(dir, _("3901 open device failed: ERR=%s\n"), 
			strerror_dev(dev));
		     break;
		  }
		  read_label(jcr, dev);
		  if (dev->state & ST_LABEL) {
                     bnet_fsend(dir, _("3001 Device %s is mounted with Volume %s\n"), 
			dev->dev_name, dev->VolHdr.VolName);
		  } else {
                     bnet_fsend(dir, _("3905 Device %s open but no Bacula volume is mounted.\n"), 
				dev->dev_name);
		  }
	       }
	       break;

	    default:
               bnet_fsend(dir, _("3905 Bizarre wait state %d\n"), dev->dev_blocked);
	       break;
	 }
	 V(dev->mutex);
      } else {
         bnet_fsend(dir, _("3999 Device %s not found\n"), dev_name);
      }
   } else {
      strcpy(dev_name, dir->msg);
      bnet_fsend(dir, _("3906 Error scanning mount command: %s\n"), dev_name);
   }
   free_memory(dev_name);
   bnet_sig(dir, BNET_EOF);
   return 1;
}

/*
 * unmount command from Director
 */
static int unmount_cmd(JCR *jcr)
{
   char *dname;
   BSOCK *dir = jcr->dir_bsock;
   DEVRES *device;
   DEVICE *dev;
   int found = 0;

   dname = (char *) get_memory(dir->msglen+1);
   if (sscanf(dir->msg, "unmount %s", dname) == 1) {
      unbash_spaces(dname);
      device = NULL;
      LockRes();
      while ((device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device))) {
	 /* Find resource, and make sure we were able to open it */
	 if (strcmp(device->hdr.name, dname) == 0 && device->dev) {
            Dmsg1(20, "Found device %s\n", device->hdr.name);
	    found = 1;
	    break;
	 }
      }
      UnlockRes();
      if (found) {
	 jcr->device = device;
	 dev = device->dev;
	 P(dev->mutex);
	 if (!(dev->state & ST_OPENED)) {
            Dmsg0(90, "Device already unmounted\n");
            bnet_fsend(dir, _("3901 Device %s is already unmounted.\n"), dev_name(dev));

	 } else if (dev->dev_blocked == BST_WAITING_FOR_SYSOP) {
            Dmsg2(90, "%d waiter dev_block=%d. doing unmount\n", dev->num_waiting,
	       dev->dev_blocked);
	    force_close_dev(dev);
	    dev->dev_blocked = BST_UNMOUNTED_WAITING_FOR_SYSOP;
            bnet_fsend(dir, _("3001 Device %s unmounted.\n"), dev_name(dev));

	 } else if (dev->dev_blocked == BST_DOING_ACQUIRE) {
            bnet_fsend(dir, _("3902 Device %s is busy in acquire.\n"),
	       dev_name(dev));

	 } else if (dev->dev_blocked == BST_WRITING_LABEL) {
            bnet_fsend(dir, _("3903 Device %s is being labeled.\n"),
	       dev_name(dev));

	 } else if (dev->state & ST_READ || dev->num_writers) {
	    if (dev->state & ST_READ) {
                Dmsg0(90, "Device in read mode\n");
                bnet_fsend(dir, _("3904 Device %s is busy with 1 reader.\n"),
		   dev_name(dev));
	    } else {
                Dmsg1(90, "Device busy with %d writers\n", dev->num_writers);
                bnet_fsend(dir, _("3905 Device %s is busy with %d writer(s).\n"),
		   dev_name(dev), dev->num_writers);
	    }

	 } else {		      /* device not being used */
            Dmsg0(90, "Device not in use, unmounting\n");
	    block_device(dev, BST_UNMOUNTED);
	    force_close_dev(dev);
            bnet_fsend(dir, _("3002 Device %s unmounted.\n"), dev_name(dev));
	 }
	 V(dev->mutex);
      } else {
         bnet_fsend(dir, _("3999 Device %s not found\n"), dname);
      }
   } else {
      /* NB dir->msg gets clobbered in bnet_fsend, so save command */
      strcpy(dname, dir->msg);
      bnet_fsend(dir, _("3907 Error scanning unmount command: %s\n"), dname);
   }
   free_memory(dname);
   bnet_sig(dir, BNET_EOF);
   return 1;
}

/*
 * Status command from Director
 */
static int status_cmd(JCR *jcr)
{
   DEVRES *device;
   DEVICE *dev;
   int found, bps, sec, bpb;
   BSOCK *user = jcr->dir_bsock;
   char dt[MAX_TIME_LENGTH];
   char b1[30], b2[30], b3[30];

   bnet_fsend(user, "\n%s Version: " VERSION " (" DATE ")\n", my_name);
   bstrftime(dt, sizeof(dt), daemon_start_time);
   bnet_fsend(user, _("Daemon started %s, %d Job%s run.\n"), dt, last_job.NumJobs,
        last_job.NumJobs == 1 ? "" : "s");
   if (last_job.NumJobs > 0) {
      char *termstat, jstat[2];

      bstrftime(dt, sizeof(dt), last_job.end_time);
      bnet_fsend(user, _("Last Job %s finished at %s\n"), last_job.Job, dt);
      switch (last_job.JobStatus) {
	 case JS_Terminated:
            termstat = _("OK");
	    break;
	case JS_ErrorTerminated:
            termstat = _("Error");
	    break;
	default:
	    jstat[0] = last_job.JobStatus;
	    jstat[1] = 0;
	    termstat = jstat;
	    break;
      }
	   
      bnet_fsend(user, _("  Files=%s Bytes=%s Termination Status=%s\n"), 
	   edit_uint64_with_commas(last_job.JobFiles, b1),
	   edit_uint64_with_commas(last_job.JobBytes, b2),
	   termstat);
   }

   LockRes();
   for (device=NULL;  (device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device)); ) {
      dev = device->dev;
      if (dev) {
	 if (dev->state & ST_OPENED) {
	    if (dev->state & ST_LABEL) {
               bnet_fsend(user, _("Device %s is mounted with Volume %s\n"), 
		  dev_name(dev), dev->VolHdr.VolName);
	    } else {
               bnet_fsend(user, _("Device %s open but no Bacula volume is mounted.\n"), dev_name(dev));
	    }
	    switch (dev->dev_blocked) {
	       case BST_UNMOUNTED:
                  bnet_fsend(user, _("    Deviced is blocked. User unmounted.\n"));
		  break;
	       case BST_UNMOUNTED_WAITING_FOR_SYSOP:
                  bnet_fsend(user, _("    Deviced is blocked. User unmounted during wait for media/mount.\n"));
		  break;
	       case BST_WAITING_FOR_SYSOP:
		  if (jcr->JobStatus == JS_WaitMount) {
                     bnet_fsend(user, _("    Device is blocked waiting for mount.\n"));
		  } else {
                     bnet_fsend(user, _("    Device is blocked waiting for appendable media.\n"));
		  }
		  break;
	       case BST_DOING_ACQUIRE:
                  bnet_fsend(user, _("    Device is being initialized.\n"));
		  break;
	       case BST_WRITING_LABEL:
                  bnet_fsend(user, _("    Device is blocked labeling a Volume.\n"));
		  break;
	       default:
		  break;
	    }
	    bpb = dev->VolCatInfo.VolCatBlocks;
	    if (bpb <= 0) {
	       bpb = 1;
	    }
	    bpb = dev->VolCatInfo.VolCatBytes / bpb;
            bnet_fsend(user, _("    Total Bytes=%s Blocks=%s Bytes/block=%s\n"),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatBytes, b1),
	       edit_uint64_with_commas(dev->VolCatInfo.VolCatBlocks, b2), 
	       edit_uint64_with_commas(bpb, b3));
            bnet_fsend(user, _("    Positioned at File=%s Block=%s\n"), 
	       edit_uint64_with_commas(dev->file, b1),
	       edit_uint64_with_commas(dev->block_num, b2));

	 } else {
            bnet_fsend(user, _("Device %s is not open.\n"), dev_name(dev));
	 }
      }
   }
   UnlockRes();

   found = 0;
   lock_jcr_chain();
   /* NOTE, we reuse a calling argument jcr. Be warned! */ 
   for (jcr=NULL; (jcr=get_next_jcr(jcr)); ) {
      if (jcr->JobStatus == JS_WaitFD) {
         bnet_fsend(user, _("Job %s is waiting for the Client connection.\n"),
	    jcr->Job);
      }
      if (jcr->device) {
         bnet_fsend(user, _("Job %s is using device %s\n"), 
		   jcr->Job, jcr->device->device_name);
	 sec = time(NULL) - jcr->run_time;
	 if (sec <= 0) {
	    sec = 1;
	 }
	 bps = jcr->JobBytes / sec;
         bnet_fsend(user, _("    Files=%s Bytes=%s Bytes/sec=%s\n"), 
	    edit_uint64_with_commas(jcr->JobFiles, b1),
	    edit_uint64_with_commas(jcr->JobBytes, b2),
	    edit_uint64_with_commas(bps, b3));
	 found = 1;
#ifdef DEBUG
	 if (jcr->file_bsock) {
            bnet_fsend(user, "    FDReadSeqNo=%" lld " fd=%d\n", 
	       jcr->file_bsock->read_seqno, jcr->file_bsock->fd);
	 } else {
            bnet_fsend(user, "    FDSocket closed\n");
	 }
#endif
      }
      free_locked_jcr(jcr);
   }
   unlock_jcr_chain();
   if (!found) {
      bnet_fsend(user, _("No jobs running.\n"));
   }

#ifdef full_status
   bnet_fsend(user, "\n\n");
   dump_resource(R_DEVICE, resources[R_DEVICE-r_first].res_head, sendit, user);
#endif
   bnet_fsend(user, "====\n");

   bnet_sig(user, BNET_EOF);
   return 1;
}
