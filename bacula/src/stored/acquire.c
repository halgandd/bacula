/*
 *  Routines to acquire and release a device for read/write
 *
 *   Kern Sibbald, August MMII
 *			      
 *   Version $Id$
 */
/*
   Copyright (C) 2002-2003 Kern Sibbald and John Walker

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

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/********************************************************************* 
 * Acquire device for reading.	We permit (for the moment)
 *  only one reader.  We read the Volume label from the block and
 *  leave the block pointers just after the label.
 *
 *  Returns: 0 if failed for any reason
 *	     1 if successful
 */
int acquire_device_for_read(JCR *jcr, DEVICE *dev, DEV_BLOCK *block)
{
   int stat = 0;
   int tape_previously_mounted;
   VOL_LIST *vol;
   int autochanger = 0;

   lock_device(dev);
   block_device(dev, BST_DOING_ACQUIRE);
   unlock_device(dev);

   tape_previously_mounted = (dev->state & ST_READ) || (dev->state & ST_APPEND);

   if (dev->state & ST_READ || dev->num_writers > 0) {
      Jmsg1(jcr, M_FATAL, 0, _("Device %s is busy. Job canceled.\n"), dev_name(dev));
      goto get_out;
   }

   /* Find next Volume, if any */
   vol = jcr->VolList;
   if (!vol) {
      Jmsg(jcr, M_FATAL, 0, _("No volumes specified. Job canceled.\n"));
      goto get_out;
   }
   jcr->CurVolume++;
   for (int i=1; i<jcr->CurVolume; i++) {
      vol = vol->next;
   }
   pm_strcpy(&jcr->VolumeName, vol->VolumeName);

   for (int i=0; i<5; i++) {
      if (job_canceled(jcr)) {
         Mmsg0(&dev->errmsg, _("Job canceled.\n"));
	 goto get_out;		      /* error return */
      }
      /*
       * This code ensures that the device is ready for
       * reading. If it is a file, it opens it.
       * If it is a tape, it checks the volume name 
       */
      for ( ; !(dev->state & ST_OPENED); ) {
          Dmsg1(120, "bstored: open vol=%s\n", jcr->VolumeName);
	  if (open_dev(dev, jcr->VolumeName, READ_ONLY) < 0) {
             Jmsg(jcr, M_FATAL, 0, _("Open device %s volume %s failed, ERR=%s\n"), 
		 dev_name(dev), jcr->VolumeName, strerror_dev(dev));
	     goto get_out;
	  }
          Dmsg1(129, "open_dev %s OK\n", dev_name(dev));
      }
      dev->state &= ~ST_LABEL;		 /* force reread of label */
      Dmsg0(200, "calling read-vol-label\n");
      switch (read_dev_volume_label(jcr, dev, block)) {
      case VOL_OK:
	 stat = 1;
	 break; 		   /* got it */
      case VOL_IO_ERROR:
	 /*
	  * Send error message generated by read_dev_volume_label()
	  *  only we really had a tape mounted. This supresses superfluous
	  *  error messages when nothing is mounted.
	  */
	 if (tape_previously_mounted) {
            Jmsg(jcr, M_WARNING, 0, "%s", jcr->errmsg);                         
	 }
	 goto default_path;
      default:
         Jmsg(jcr, M_WARNING, 0, "%s", jcr->errmsg);
default_path:
	 tape_previously_mounted = 1;
         Dmsg0(200, "dir_get_volume_info\n");
	 if (!dir_get_volume_info(jcr, 0)) { 
            Jmsg1(jcr, M_WARNING, 0, "%s", jcr->errmsg);
	 }
	 /* Call autochanger only once unless ask_sysop called */
	 if (!autochanger) {
            Dmsg2(200, "calling autoload Vol=%s Slot=%d\n",
	       jcr->VolumeName, jcr->VolCatInfo.Slot);			       
	    if ((autochanger=autoload_device(jcr, dev, 0, NULL))) {
	       continue;
	    }
	 }
	 /* Mount a specific volume and no other */
         Dmsg0(200, "calling dir_ask_sysop\n");
	 if (!dir_ask_sysop_to_mount_volume(jcr, dev)) {
	    goto get_out;	      /* error return */
	 }
	 autochanger = 0;	      /* permit using autochanger again */
	 continue;		      /* try reading again */
      } /* end switch */
      break;
   } /* end for loop */
   if (stat == 0) {
      Jmsg1(jcr, M_FATAL, 0, _("Too many errors trying to mount device \"%s\".\n"),
	    dev_name(dev));
      goto get_out;
   }

   dev->state |= ST_READ;
   attach_jcr_to_device(dev, jcr);    /* attach jcr to device */
   Jmsg(jcr, M_INFO, 0, _("Ready to read from volume \"%s\" on device %s.\n"),
      jcr->VolumeName, dev_name(dev));
   if ((dev->state & ST_TAPE) && vol->start_file > 0) {
      Dmsg1(200, "====== Got start_file = %d\n", vol->start_file);
      Jmsg(jcr, M_INFO, 0, _("Forward spacing to file %d.\n"), vol->start_file);
      fsf_dev(dev, vol->start_file);
   }

get_out:
   P(dev->mutex); 
   unblock_device(dev);
   V(dev->mutex);
   return stat;
}

/*
 * Acquire device for writing. We permit multiple writers.
 *  If this is the first one, we read the label.
 *
 *  Returns: NULL if failed for any reason
 *	     dev if successful (may change if new dev opened)
 *  This routine must be single threaded because we may create
 *   multiple devices (for files), thus we have our own mutex 
 *   on top of the device mutex.
 */
DEVICE * acquire_device_for_append(JCR *jcr, DEVICE *dev, DEV_BLOCK *block)
{
   int release = 0;
   int do_mount = 0;
   DEVICE *rtn_dev = NULL;

   lock_device(dev);
   block_device(dev, BST_DOING_ACQUIRE);
   unlock_device(dev);
   P(mutex);			     /* lock all devices */
   Dmsg1(190, "acquire_append device is %s\n", dev_is_tape(dev)?"tape":"disk");
	     

   if (dev->state & ST_APPEND) {
      /* 
       * Device already in append mode	 
       *
       * Check if we have the right Volume mounted   
       *   OK if current volume info OK
       *   OK if next volume matches current volume
       *   otherwise mount desired volume obtained from
       *    dir_find_next_appendable_volume
       */
      pm_strcpy(&jcr->VolumeName, dev->VolHdr.VolName);
      if (!dir_get_volume_info(jcr, 1) &&
	  !(dir_find_next_appendable_volume(jcr) &&
	    strcmp(dev->VolHdr.VolName, jcr->VolumeName) == 0)) { /* wrong tape mounted */
	 if (dev->num_writers != 0) {
	    DEVICE *d = ((DEVRES *)dev->device)->dev;
	    uint32_t open_vols = 0;
	    for ( ; d; d=d->next) {
	       open_vols++;
	    }
	    if (dev->state & ST_FILE && dev->max_open_vols > open_vols) {
	       d = init_dev(NULL, (DEVRES *)dev->device); /* init new device */
	       d->prev = dev;			/* chain in new device */
	       d->next = dev->next;
	       dev->next = d;
	       /* Release old device */
	       P(dev->mutex); 
	       unblock_device(dev);
	       V(dev->mutex);
	       /* Make new device current device and lock it */
	       dev = d;
	       lock_device(dev);
	       block_device(dev, BST_DOING_ACQUIRE);
	       unlock_device(dev);
	    } else {
               Jmsg(jcr, M_FATAL, 0, _("Device %s is busy writing on another Volume.\n"), dev_name(dev));
	       goto get_out;
	    }
	 }
	 /* Wrong tape mounted, release it, then fall through to get correct one */
	 release = 1;
	 do_mount = 1;
      }
   } else { 
      /* Not already in append mode, so mount the device */
      if (dev->state & ST_READ) {
         Jmsg(jcr, M_FATAL, 0, _("Device %s is busy reading.\n"), dev_name(dev));
	 goto get_out;
      } 
      ASSERT(dev->num_writers == 0);
      do_mount = 1;
   }

   if (do_mount) {
      if (!mount_next_write_volume(jcr, dev, block, release)) {
         Jmsg(jcr, M_FATAL, 0, _("Could not ready device %s for append.\n"),
	    dev_name(dev));
	 goto get_out;
      }
   }

   dev->num_writers++;
   if (dev->num_writers > 1) {
      Dmsg2(100, "Hey!!!! There are %d writers on device %s\n", dev->num_writers,
	 dev_name(dev));
   }
   if (jcr->NumVolumes == 0) {
      jcr->NumVolumes = 1;
   }
   attach_jcr_to_device(dev, jcr);    /* attach jcr to device */
   rtn_dev = dev;		      /* return device */

get_out:
   P(dev->mutex); 
   unblock_device(dev);
   V(dev->mutex);
   V(mutex);			      /* unlock other threads */
   return rtn_dev;
}

/*
 * This job is done, so release the device. From a Unix standpoint,
 *  the device remains open.
 *
 */
int release_device(JCR *jcr, DEVICE *dev)
{
   lock_device(dev);
   Dmsg1(100, "release_device device is %s\n", dev_is_tape(dev)?"tape":"disk");
   if (dev->state & ST_READ) {
      dev->state &= ~ST_READ;	      /* clear read bit */
      if (!dev_is_tape(dev) || !dev_cap(dev, CAP_ALWAYSOPEN)) {
	 offline_or_rewind_dev(dev);
	 close_dev(dev);
      }
      /******FIXME**** send read volume usage statistics to director */

   } else if (dev->num_writers > 0) {
      dev->num_writers--;
      if (dev->state & ST_TAPE) {
	 jcr->EndBlock = dev->EndBlock;
	 jcr->EndFile  = dev->EndFile;
         Dmsg2(200, "Release device: EndFile=%u EndBlock=%u\n", jcr->EndFile, jcr->EndBlock);
      } else {
	 jcr->EndBlock = (uint32_t)dev->file_addr;
	 jcr->EndFile = (uint32_t)(dev->file_addr >> 32);
      }
      Dmsg1(100, "There are %d writers in release_device\n", dev->num_writers);
      if (dev->num_writers == 0) {
	 /* If we have fully acquired the tape */
	 if (dev->state & ST_LABEL) {
            Dmsg0(100, "dir_create_jobmedia_record. Release\n");
	    dir_create_jobmedia_record(jcr);
	    if (dev_can_write(dev)) {
	       weof_dev(dev, 1);
	    }
	    dev->VolCatInfo.VolCatFiles = dev->file;   /* set number of files */
	    dev->VolCatInfo.VolCatJobs++;	       /* increment number of jobs */
	    /* Note! do volume update before close, which zaps VolCatInfo */
            Dmsg0(200, "dir_update_vol_info. Release0\n");
	    dir_update_volume_info(jcr, &dev->VolCatInfo, 0); /* send Volume info to Director */
	 }

	 if (!dev_is_tape(dev) || !dev_cap(dev, CAP_ALWAYSOPEN)) {
	    offline_or_rewind_dev(dev);
	    close_dev(dev);
	 }
      } else if (dev->state & ST_LABEL) {
         Dmsg0(100, "dir_create_jobmedia_record. Release\n");
	 dir_create_jobmedia_record(jcr);
         Dmsg0(200, "dir_update_vol_info. Release1\n");
	 dev->VolCatInfo.VolCatFiles = dev->file;   /* set number of files */
	 dev->VolCatInfo.VolCatJobs++;		    /* increment number of jobs */
	 dir_update_volume_info(jcr, &dev->VolCatInfo, 0); /* send Volume info to Director */
      }
   } else {
      Jmsg2(jcr, M_ERROR, 0, _("BAD ERROR: release_device %s, Volume %s not in use.\n"), 
	    dev_name(dev), NPRT(jcr->VolumeName));
   }
   detach_jcr_from_device(dev, jcr);
   if (dev->prev && !(dev->state & ST_READ) && dev->num_writers == 0) {
      P(mutex);
      unlock_device(dev);
      dev->prev->next = dev->next;    /* dechain */
      term_dev(dev);
      V(mutex);
   } else {
      unlock_device(dev);
   }
   return 1;
}
