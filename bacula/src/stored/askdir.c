/*
 *  Subroutines to handle Catalog reqests sent to the Director
 *   Reqests/commands from the Director are handled in dircmd.c
 *
 *   Kern Sibbald, December 2000
 *
 *   Version $Id$
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

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Requests sent to the Director */
static char Find_media[]    = "CatReq Job=%s FindMedia=%d\n";
static char Get_Vol_Info[] = "CatReq Job=%s GetVolInfo VolName=%s write=%d\n";

static char Update_media[] = "CatReq Job=%s UpdateMedia VolName=%s\
 VolJobs=%u VolFiles=%u VolBlocks=%u VolBytes=%s VolMounts=%u\
 VolErrors=%u VolWrites=%u MaxVolBytes=%s EndTime=%d VolStatus=%s\
 Slot=%d relabel=%d\n";

static char Create_job_media[] = "CatReq Job=%s CreateJobMedia \
 FirstIndex=%u LastIndex=%u StartFile=%u EndFile=%u \
 StartBlock=%u EndBlock=%u\n";


static char FileAttributes[] = "UpdCat Job=%s FileAttributes ";

static char Job_status[]   = "3012 Job %s jobstatus %d\n";


/* Responses received from the Director */
static char OK_media[] = "1000 OK VolName=%127s VolJobs=%u VolFiles=%u\
 VolBlocks=%u VolBytes=%" lld " VolMounts=%u VolErrors=%u VolWrites=%u\
 MaxVolBytes=%" lld " VolCapacityBytes=%" lld " VolStatus=%20s\
 Slot=%d MaxVolJobs=%u MaxVolFiles=%u\n";

static char OK_update[] = "1000 OK UpdateMedia\n";


/*
 * Send current JobStatus to Director
 */
int dir_send_job_status(JCR *jcr)
{
   return bnet_fsend(jcr->dir_bsock, Job_status, jcr->Job, jcr->JobStatus);
}

/*
 * Common routine for:
 *   dir_get_volume_info()
 * and
 *   dir_find_next_appendable_volume()
 */
static int do_request_volume_info(JCR *jcr)
{
    BSOCK *dir = jcr->dir_bsock;
    VOLUME_CAT_INFO *vol = &jcr->VolCatInfo;

    jcr->VolumeName[0] = 0;	      /* No volume */
    if (bnet_recv(dir) <= 0) {
       Dmsg0(200, "getvolname error bnet_recv\n");
       Mmsg(&jcr->errmsg, _("Network error on bnet_recv in req_vol_info.\n"));
       return 0;
    }
    if (sscanf(dir->msg, OK_media, vol->VolCatName, 
	       &vol->VolCatJobs, &vol->VolCatFiles,
	       &vol->VolCatBlocks, &vol->VolCatBytes,
	       &vol->VolCatMounts, &vol->VolCatErrors,
	       &vol->VolCatWrites, &vol->VolCatMaxBytes,
	       &vol->VolCatCapacityBytes, vol->VolCatStatus,
	       &vol->Slot, &vol->VolCatMaxJobs, &vol->VolCatMaxFiles) != 14) {

       Dmsg1(200, "Bad response from Dir: %s\n", dir->msg);
       Mmsg(&jcr->errmsg, _("Error scanning Dir response: %s\n"), dir->msg);
       return 0;
    }
    unbash_spaces(vol->VolCatName);
    strcpy(jcr->VolumeName, vol->VolCatName); /* set desired VolumeName */
    
    Dmsg2(200, "do_reqest_vol_info got slot=%d Volume=%s\n", 
	  vol->Slot, vol->VolCatName);
    return 1;
}


/*
 * Get Volume info for a specific volume from the Director's Database
 *
 * Returns: 1 on success   (not Director guarantees that Pool and MediaType
 *			    are correct and VolStatus==Append or
 *			    VolStatus==Recycle)
 *	    0 on failure
 *
 *	    Volume information returned in jcr
 */
int dir_get_volume_info(JCR *jcr, int writing)
{
    BSOCK *dir = jcr->dir_bsock;

    strcpy(jcr->VolCatInfo.VolCatName, jcr->VolumeName);
    Dmsg1(200, "dir_get_volume_info=%s\n", jcr->VolCatInfo.VolCatName);
    bash_spaces(jcr->VolCatInfo.VolCatName);
    bnet_fsend(dir, Get_Vol_Info, jcr->Job, jcr->VolCatInfo.VolCatName, writing);
    return do_request_volume_info(jcr);
}



/*
 * Get info on the next appendable volume in the Director's database
 * Returns: 1 on success
 *	    0 on failure
 *
 *	    Volume information returned in jcr
 *
 */
int dir_find_next_appendable_volume(JCR *jcr)
{
    BSOCK *dir = jcr->dir_bsock;

    Dmsg0(200, "dir_find_next_appendable_volume\n");
    bnet_fsend(dir, Find_media, jcr->Job, 1);
    return do_request_volume_info(jcr);
}

    
/*
 * After writing a Volume, send the updated statistics
 * back to the director.
 */
int dir_update_volume_info(JCR *jcr, VOLUME_CAT_INFO *vol, int relabel)
{
   BSOCK *dir = jcr->dir_bsock;
   time_t EndTime = time(NULL);
   char ed1[50], ed2[50];

   if (vol->VolCatName[0] == 0) {
      Jmsg0(jcr, M_ERROR, 0, _("NULL Volume name. This shouldn't happen!!!\n"));
      return 0;
   }
   bash_spaces(vol->VolCatName);
   bnet_fsend(dir, Update_media, jcr->Job, 
      vol->VolCatName, vol->VolCatJobs, vol->VolCatFiles,
      vol->VolCatBlocks, edit_uint64(vol->VolCatBytes, ed1),
      vol->VolCatMounts, vol->VolCatErrors,
      vol->VolCatWrites, edit_uint64(vol->VolCatMaxBytes, ed2), 
      EndTime, vol->VolCatStatus, vol->Slot, relabel);
   Dmsg1(120, "update_volume_data(): %s", dir->msg);
   unbash_spaces(vol->VolCatName);
   if (bnet_recv(dir) <= 0) {
      Dmsg0(190, "updateVolCatInfo error bnet_recv\n");
      Jmsg(jcr, M_ERROR, 0, _("Error updating Volume Info: %s\n"), 
	   bnet_strerror(dir));
      return 0;
   }
   Dmsg1(120, "Updatevol: %s", dir->msg);
   if (strcmp(dir->msg, OK_update) != 0) {
      Dmsg1(130, "Bad response from Dir: %s\n", dir->msg);
      Jmsg(jcr, M_ERROR, 0, _("Error updating Volume Info: %s\n"), dir->msg);
      return 0;
   }
   return 1;
}

/*
 * After writing a Volume, create the JobMedia record.
 */
int dir_create_jobmedia_record(JCR *jcr)
{
   BSOCK *dir = jcr->dir_bsock;

   bnet_fsend(dir, Create_job_media, jcr->Job, 
      jcr->VolFirstFile, jcr->JobFiles,
      jcr->StartFile, jcr->EndFile,
      jcr->StartBlock, jcr->EndBlock);
   Dmsg1(100, "create_jobmedia(): %s", dir->msg);
   if (bnet_recv(dir) <= 0) {
      Dmsg0(190, "create_jobmedia error bnet_recv\n");
      Jmsg(jcr, M_ERROR, 0, _("Error creating JobMedia record: %s\n"), 
	   bnet_strerror(dir));
      return 0;
   }
   Dmsg1(120, "Create_jobmedia: %s", dir->msg);
   if (strcmp(dir->msg, OK_update) != 0) {
      Dmsg1(130, "Bad response from Dir: %s\n", dir->msg);
      Jmsg(jcr, M_ERROR, 0, _("Error creating JobMedia record: %s\n"), dir->msg);
      return 0;
   }
   return 1;
}


/* 
 * Update File Attribute data
 */
int dir_update_file_attributes(JCR *jcr, DEV_RECORD *rec)
{
   BSOCK *dir = jcr->dir_bsock;
   ser_declare;

   dir->msglen = sprintf(dir->msg, FileAttributes, jcr->Job);
   dir->msg = check_pool_memory_size(dir->msg, dir->msglen + 
		sizeof(DEV_RECORD) + rec->data_len);
   ser_begin(dir->msg + dir->msglen, 0);
   ser_uint32(rec->VolSessionId);
   ser_uint32(rec->VolSessionTime);
   ser_int32(rec->FileIndex);
   ser_int32(rec->Stream);
   ser_uint32(rec->data_len);
   ser_bytes(rec->data, rec->data_len);
   dir->msglen = ser_length(dir->msg);
   return bnet_send(dir);
}


/*
 *   
 *   Entered with device blocked.
 *   Leaves with device blocked.
 *
 *   Returns: 1 on success (operator issues a mount command)
 *	      0 on failure
 *		Note, must create dev->errmsg on error return.
 *
 *    On success, jcr->VolumeName and jcr->VolCatInfo contain
 *	information on suggested volume, but this may not be the
 *	same as what is actually mounted.
 *
 *    When we return with success, the correct tape may or may not
 *	actually be mounted. The calling routine must read it and
 *	verify the label.
 */
int dir_ask_sysop_to_mount_next_volume(JCR *jcr, DEVICE *dev)
{
   struct timeval tv;
   struct timezone tz;
   struct timespec timeout;
   int stat = 0, jstat;
   /* ******FIXME******* put these on config variable */
   int min_wait = 60 * 60;
   int max_wait = 24 * 60 * 60;
   int max_num_wait = 9;	      /* 5 waits =~ 1 day, then 1 day at a time */

   int wait_sec;
   int num_wait = 0;
   int dev_blocked;

   Dmsg0(130, "enter dir_ask_sysop_to_mount_next_volume\n");
   ASSERT(dev->dev_blocked);
   wait_sec = min_wait;
   for ( ;; ) {
      if (job_cancelled(jcr)) {
         Mmsg(&dev->errmsg, _("Job %s cancelled while waiting for mount on Storage Device \"%s\".\n"), 
	      jcr->Job, jcr->dev_name);
         Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
	 return 0;
      }
      if (dir_find_next_appendable_volume(jcr)) {    /* get suggested volume */
	 jstat = JS_WaitMount;
	 /*
	  * If we have a valid volume name and we are not
	  * removable media, return now, otherwise wait
	  * for the operator to mount the media.
	  */
	 if (jcr->VolumeName[0] && !dev_cap(dev, CAP_REM) && dev_cap(dev, CAP_LABEL)) {
            Dmsg0(190, "Return 1 from mount without wait.\n");
	    return 1;
	 }
	 Jmsg(jcr, M_MOUNT, 0, _(
"Please mount Volume \"%s\" on Storage Device \"%s\" for Job %s\n"
"Use \"mount\" command to release Job.\n"),
	      jcr->VolumeName, jcr->dev_name, jcr->Job);
         Dmsg3(190, "Mount %s on %s for Job %s\n",
		jcr->VolumeName, jcr->dev_name, jcr->Job);
      } else {
	 jstat = JS_WaitMedia;
	 Jmsg(jcr, M_MOUNT, 0, _(
"Job %s waiting. Cannot find any appendable volumes.\n\
Please use the \"label\"  command to create a new Volume for:\n\
    Storage:      %s\n\
    Media type:   %s\n\
    Pool:         %s\n"),
	      jcr->Job, 
	      jcr->dev_name, 
	      jcr->media_type,
	      jcr->pool_name);
      }
      /*
       * Wait then send message again
       */
      gettimeofday(&tv, &tz);
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec + wait_sec;

      P(dev->mutex);
      dev_blocked = dev->dev_blocked;
      dev->dev_blocked = BST_WAITING_FOR_SYSOP; /* indicate waiting for mount */
      jcr->JobStatus = jstat;
      dir_send_job_status(jcr);

      for ( ;!job_cancelled(jcr); ) {
         Dmsg1(190, "I'm going to sleep on device %s\n", dev->dev_name);
	 stat = pthread_cond_timedwait(&dev->wait_next_vol, &dev->mutex, &timeout);
	 if (dev->dev_blocked == BST_WAITING_FOR_SYSOP) {
	    break;
	 }
	 /*	    
	  * Someone other than us blocked the device (probably the
	  *  user via the Console program.   
	  * So, we continue waiting.
	  */
	 gettimeofday(&tv, &tz);
	 timeout.tv_nsec = 0;
	 timeout.tv_sec = tv.tv_sec + 10; /* wait 10 seconds */
      }
      dev->dev_blocked = dev_blocked;
      V(dev->mutex);

      if (stat == ETIMEDOUT) {
	 wait_sec *= 2; 	      /* double wait time */
	 if (wait_sec > max_wait) {   /* but not longer than maxtime */
	    wait_sec = max_wait;
	 }
	 num_wait++;
	 if (num_wait >= max_num_wait) {
            Mmsg(&dev->errmsg, _("Gave up waiting to mount Storage Device \"%s\" for Job %s\n"), 
		 jcr->dev_name, jcr->Job);
            Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
            Dmsg1(190, "Gave up waiting on device %s\n", dev->dev_name);
	    return 0;		      /* exceeded maximum waits */
	 }
	 continue;
      }
      if (stat == EINVAL) {
         Mmsg2(&dev->errmsg, _("pthread error in mount_next_volume stat=%d ERR=%s\n"),
	       stat, strerror(stat));
         Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
	 return 0;
      }
      if (stat != 0) {
         Jmsg(jcr, M_WARNING, 0, _("pthread error in mount_next_volume stat=%d ERR=%s\n"), stat,
	    strerror(stat));
      }
      Dmsg1(190, "Someone woke me for device %s\n", dev->dev_name);

      /* Restart wait counters */
      wait_sec = min_wait;
      num_wait = 0;
      /* If no VolumeName, and cannot get one, try again */
      if (jcr->VolumeName[0] == 0 && 
	  !dir_find_next_appendable_volume(jcr)) {
	 Jmsg(jcr, M_MOUNT, 0, _(
"Someone woke me up, but I cannot find any appendable\n\
volumes for Job=%s.\n"), jcr->Job);
	 continue;
      }       
      break;
   }
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);
   Dmsg0(130, "leave dir_ask_sysop_to_mount_next_volume\n");
   return 1;
}

/*
 *   
 *   Entered with device blocked and jcr->VolumeName is desired
 *	volume.
 *   Leaves with device blocked.
 *
 *   Returns: 1 on success (operator issues a mount command)
 *	      0 on failure
 *		Note, must create dev->errmsg on error return.
 *
 */
int dir_ask_sysop_to_mount_volume(JCR *jcr, DEVICE *dev)
{
   int stat = 0;
   /* ******FIXME******* put these on config variable */
   int min_wait = 60 * 60;
   int max_wait = 24 * 60 * 60;
   int max_num_wait = 9;	      /* 5 waits =~ 1 day, then 1 day at a time */
   int wait_sec;
   int num_wait = 0;
   int dev_blocked;
   char *msg;
   struct timeval tv;
   struct timezone tz;
   struct timespec timeout;

   Dmsg0(130, "enter dir_ask_sysop_to_mount_next_volume\n");
   if (!jcr->VolumeName[0]) {
      Mmsg0(&dev->errmsg, _("Cannot request another volume: no volume name given.\n"));
      return 0;
   }
   ASSERT(dev->dev_blocked);
   wait_sec = min_wait;
   for ( ;; ) {
      if (job_cancelled(jcr)) {
         Mmsg(&dev->errmsg, _("Job %s cancelled while waiting for mount on Storage Device \"%s\".\n"), 
	      jcr->Job, jcr->dev_name);
	 return 0;
      }
      msg = _("Please mount");
      Jmsg(jcr, M_MOUNT, 0, _("%s Volume \"%s\" on Storage Device \"%s\" for Job %s\n"),
	   msg, jcr->VolumeName, jcr->dev_name, jcr->Job);
      Dmsg3(190, "Mount %s on %s for Job %s\n",
	    jcr->VolumeName, jcr->dev_name, jcr->Job);

      /*
       * Wait then send message again
       */
      gettimeofday(&tv, &tz);
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec + wait_sec;

      P(dev->mutex);
      dev_blocked = dev->dev_blocked;
      dev->dev_blocked = BST_WAITING_FOR_SYSOP; /* indicate waiting for mount */
      jcr->JobStatus = JS_WaitMount;
      dir_send_job_status(jcr);

      for ( ;!job_cancelled(jcr); ) {
         Dmsg1(190, "I'm going to sleep on device %s\n", dev->dev_name);
	 stat = pthread_cond_timedwait(&dev->wait_next_vol, &dev->mutex, &timeout);
	 if (dev->dev_blocked == BST_WAITING_FOR_SYSOP) {
	    break;
	 }
	 /*	    
	  * Someone other than us blocked the device (probably the
	  *  user via the Console program.   
	  * So, we continue waiting.
	  */
	 gettimeofday(&tv, &tz);
	 timeout.tv_nsec = 0;
	 timeout.tv_sec = tv.tv_sec + 10; /* wait 10 seconds */
      }
      dev->dev_blocked = dev_blocked;
      V(dev->mutex);

      if (stat == ETIMEDOUT) {
	 wait_sec *= 2; 	      /* double wait time */
	 if (wait_sec > max_wait) {   /* but not longer than maxtime */
	    wait_sec = max_wait;
	 }
	 num_wait++;
	 if (num_wait >= max_num_wait) {
            Mmsg(&dev->errmsg, _("Gave up waiting to mount Storage Device \"%s\" for Job %s\n"), 
		 jcr->dev_name, jcr->Job);
            Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
            Dmsg1(190, "Gave up waiting on device %s\n", dev->dev_name);
	    return 0;		      /* exceeded maximum waits */
	 }
	 continue;
      }
      if (stat == EINVAL) {
         Mmsg2(&dev->errmsg, _("pthread error in mount_volume stat=%d ERR=%s\n"),
	       stat, strerror(stat));
         Jmsg(jcr, M_FATAL, 0, "%s", dev->errmsg);
	 return 0;
      }
      if (stat != 0) {
         Jmsg(jcr, M_ERROR, 0, _("pthread error in mount_next_volume stat=%d ERR=%s\n"), stat,
	    strerror(stat));
      }
      Dmsg1(190, "Someone woke me for device %s\n", dev->dev_name);

      /* Restart wait counters */
      wait_sec = min_wait;
      num_wait = 0;
      break;
   }
   set_jcr_job_status(jcr, JS_Running);
   dir_send_job_status(jcr);
   Dmsg0(130, "leave dir_ask_sysop_to_mount_next_volume\n");
   return 1;
}
