/*
 *
 *   Bacula Director -- Tape labeling commands
 *
 *     Kern Sibbald, April MMIII
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

#include "bacula.h"
#include "dird.h"

/* Forward referenced functions */
static int do_label(UAContext *ua, char *cmd, int relabel);
static void label_from_barcodes(UAContext *ua);
static int is_legal_volume_name(UAContext *ua, char *name);
static int send_label_request(UAContext *ua, MEDIA_DBR *mr, MEDIA_DBR *omr, 
	       POOL_DBR *pr, int relabel);


/*
 * Label a tape 
 *  
 *   label storage=xxx volume=vvv
 */
int labelcmd(UAContext *ua, char *cmd)
{
   return do_label(ua, cmd, 0);       /* standard label */
}

int relabelcmd(UAContext *ua, char *cmd)
{
   return do_label(ua, cmd, 1);      /* relabel tape */
}


/*
 * Common routine for both label and relabel
 */
static int do_label(UAContext *ua, char *cmd, int relabel)
{
   STORE *store;
   BSOCK *sd;
   sd = ua->jcr->store_bsock;
   char dev_name[MAX_NAME_LENGTH];
   MEDIA_DBR mr, omr;
   POOL_DBR pr;
   int ok = FALSE;
   int mounted = FALSE;
   int i;
   static char *name_keyword[] = {
      "name",
      NULL};
   static char *vol_keyword[] = {
      "volume",
      NULL};
   static char *barcode_keyword[] = {
      "barcode",
      "barcodes",
      NULL};


   memset(&pr, 0, sizeof(pr));
   if (!open_db(ua)) {
      return 1;
   }
   store = get_storage_resource(ua, cmd);
   if (!store) {
      return 1;
   }
   ua->jcr->store = store;

   if (!relabel && find_arg_keyword(ua, barcode_keyword) >= 0) {
      label_from_barcodes(ua);
      return 1;
   }

   /* If relabel get name of Volume to relabel */
   if (relabel) {
      /* Check for volume=OldVolume */
      i = find_arg_keyword(ua, vol_keyword); 
      if (i >= 0 && ua->argv[i]) {
	 memset(&omr, 0, sizeof(omr));
	 bstrncpy(omr.VolumeName, ua->argv[i], sizeof(omr.VolumeName));
	 if (db_get_media_record(ua->jcr, ua->db, &omr)) {
	    goto checkVol;
	 } 
         bsendmsg(ua, "%s", db_strerror(ua->db));
      }
      /* No keyword or Vol not found, ask user to select */
      if (!select_pool_and_media_dbr(ua, &pr, &omr)) {
	 return 1;
      }

      /* Require Volume to be Purged */
checkVol:
      if (strcmp(omr.VolStatus, "Purged") != 0) {
         bsendmsg(ua, _("Volume \"%s\" has VolStatus %s. It must be purged before relabeling.\n"),
	    omr.VolumeName, omr.VolStatus);
	 return 1;
      }
   }

   /* Check for name=NewVolume */
   i = find_arg_keyword(ua, name_keyword);
   if (i >=0 && ua->argv[i]) {
      strcpy(ua->cmd, ua->argv[i]);
      goto checkName;
   }

   /* Get a new Volume name */
   for ( ;; ) {
      if (!get_cmd(ua, _("Enter new Volume name: "))) {
	 return 1;
      }
checkName:
      if (!is_legal_volume_name(ua, ua->cmd)) {
	 continue;
      }

      memset(&mr, 0, sizeof(mr));
      strcpy(mr.VolumeName, ua->cmd);
      if (db_get_media_record(ua->jcr, ua->db, &mr)) {
          bsendmsg(ua, _("Media record for new Volume \"%s\" already exists.\n"), 
	     mr.VolumeName);
	  continue;
      }
      break;			      /* Got it */
   }

   /* If autochanger, request slot */
   if (store->autochanger) {
      for ( ;; ) {
         if (!get_cmd(ua, _("Enter slot (0 for none): "))) {
	    return 1;
	 }
	 mr.Slot = atoi(ua->cmd);
	 if (mr.Slot >= 0) {	      /* OK */
	    break;
	 }
         bsendmsg(ua, _("Slot numbers must be positive.\n"));
      }
   }
   strcpy(mr.MediaType, store->media_type);

   /* Must select Pool if not already done */
   if (pr.PoolId == 0) {
      memset(&pr, 0, sizeof(pr));
      if (!select_pool_dbr(ua, &pr)) {
	 return 1;
      }
   }

   bsendmsg(ua, _("Connecting to Storage daemon %s at %s:%d ...\n"), 
      store->hdr.name, store->address, store->SDport);
   if (!connect_to_storage_daemon(ua->jcr, 10, SDConnectTimeout, 1)) {
      bsendmsg(ua, _("Failed to connect to Storage daemon.\n"));
      return 1;   
   }
   sd = ua->jcr->store_bsock;

   ok = send_label_request(ua, &mr, &omr, &pr, relabel);

   if (ok) {
      if (relabel) {
	 if (!db_delete_media_record(ua->jcr, ua->db, &omr)) {
            bsendmsg(ua, _("Delete of Volume \"%s\" failed. ERR=%s"),
	       omr.VolumeName, db_strerror(ua->db));
	 } else {
            bsendmsg(ua, _("Old volume \"%s\" deleted from catalog.\n"), 
	       omr.VolumeName);
	 }
      }
      if (ua->automount) {
	 strcpy(dev_name, store->dev_name);
         bsendmsg(ua, _("Requesting mount %s ...\n"), dev_name);
	 bash_spaces(dev_name);
         bnet_fsend(sd, "mount %s", dev_name);
	 unbash_spaces(dev_name);
	 while (bnet_recv(sd) >= 0) {
            bsendmsg(ua, "%s", sd->msg);
	    /* Here we can get
	     *	3001 OK mount. Device=xxx      or
	     *	3001 Mounted Volume vvvv
	     */
            mounted = strncmp(sd->msg, "3001 ", 5) == 0;
	 }
      }
   }
   if (!mounted) {
      bsendmsg(ua, _("Do not forget to mount the drive!!!\n"));
   }
   bnet_sig(sd, BNET_TERMINATE);
   bnet_close(sd);
   ua->jcr->store_bsock = NULL;

   return 1;
}

/*
 * Request SD to send us the slot:barcodes, then wiffle
 *  through them all labeling them.
 */
static void label_from_barcodes(UAContext *ua)
{
   STORE *store = ua->jcr->store;
   BSOCK *sd = ua->jcr->store_bsock;
   POOL_DBR pr;
   char dev_name[MAX_NAME_LENGTH];
//   MEDIA_DBR mr, omr;

   bsendmsg(ua, _("Connecting to Storage daemon %s at %s:%d ...\n"), 
      store->hdr.name, store->address, store->SDport);
   if (!connect_to_storage_daemon(ua->jcr, 10, SDConnectTimeout, 1)) {
      bsendmsg(ua, _("Failed to connect to Storage daemon.\n"));
      return;
   }

   strcpy(dev_name, store->dev_name);
   bash_spaces(dev_name);
   bnet_fsend(sd, _("autochanger list %s \n"), dev_name);
   while (bget_msg(sd, 0) >= 0) {
      char *p;
      int slot;
      strip_trailing_junk(sd->msg);
      if (strncmp(sd->msg, "3902 Issuing", 12) == 0 ||
          strncmp(sd->msg, "3903 Issuing", 12) == 0) {
         bsendmsg(ua, "%s\n", sd->msg);
	 continue;
      }
      p = strchr(sd->msg, ':');
      if (p && strlen(p) > 1) {
	 *p++ = 0;
	 if (!is_an_integer(sd->msg)) {
	    continue;
	 }
      } else {
	 continue;
      }
      slot = atoi(sd->msg);
      bsendmsg(ua, "Got slot=%d label: %s\n", slot, p);
   }

#ifdef xxxx
   memset(&mr, 0, sizeof(mr));
   strcpy(mr.VolumeName, ua->cmd);
   if (db_get_media_record(ua->jcr, ua->db, &mr)) {
       bsendmsg(ua, _("Media record for new Volume \"%s\" already exists.\n"), 
	  mr.VolumeName);
       continue;
   }
#endif

   bnet_sig(sd, BNET_TERMINATE);
   bnet_close(sd);
   ua->jcr->store_bsock = NULL;

   return;

   memset(&pr, 0, sizeof(pr));
   if (!select_pool_dbr(ua, &pr)) {
      return;
   }

   return;
}

static int is_legal_volume_name(UAContext *ua, char *name)
{
   int len;
   /* Restrict the characters permitted in the Volume name */
   if (strpbrk(name, "`~!@#$%^&*()[]{}|\\;'\"<>?,/")) {
      bsendmsg(ua, _("Illegal character | in a volume name.\n"));
      return 0;
   }
   len = strlen(name);
   if (len >= MAX_NAME_LENGTH) {
      bsendmsg(ua, _("Volume name too long.\n"));
      return 0;
   }
   if (len == 0) {
      bsendmsg(ua, _("Volume name must be at least one character long.\n"));
      return 0;
   }
   return 1;
}

static int send_label_request(UAContext *ua, MEDIA_DBR *mr, MEDIA_DBR *omr, 
			      POOL_DBR *pr, int relabel)
{
   BSOCK *sd;
   char dev_name[MAX_NAME_LENGTH];
   int ok = FALSE;

   sd = ua->jcr->store_bsock;
   strcpy(dev_name, ua->jcr->store->dev_name);
   bash_spaces(dev_name);
   bash_spaces(mr->VolumeName);
   bash_spaces(mr->MediaType);
   bash_spaces(pr->Name);
   if (relabel) {
      bash_spaces(omr->VolumeName);
      bnet_fsend(sd, _("relabel %s OldName=%s NewName=%s PoolName=%s MediaType=%s Slot=%d"), 
	 dev_name, omr->VolumeName, mr->VolumeName, pr->Name, mr->MediaType, mr->Slot);
      bsendmsg(ua, _("Sending relabel command ...\n"));
   } else {
      bnet_fsend(sd, _("label %s VolumeName=%s PoolName=%s MediaType=%s Slot=%d"), 
	 dev_name, mr->VolumeName, pr->Name, mr->MediaType, mr->Slot);
      bsendmsg(ua, _("Sending label command ...\n"));
   }
   while (bget_msg(sd, 0) >= 0) {
      bsendmsg(ua, "%s", sd->msg);
      if (strncmp(sd->msg, "3000 OK label.", 14) == 0) {
	 ok = TRUE;
      } else {
         bsendmsg(ua, _("Label command failed.\n"));
      }
   }
   ua->jcr->store_bsock = NULL;
   unbash_spaces(dev_name);
   unbash_spaces(mr->VolumeName);
   unbash_spaces(mr->MediaType);
   unbash_spaces(pr->Name);
   mr->LabelDate = time(NULL);
   if (ok) {
      set_pool_dbr_defaults_in_media_dbr(mr, pr);
      if (db_create_media_record(ua->jcr, ua->db, mr)) {
         bsendmsg(ua, _("Media record for Volume \"%s\" successfully created.\n"),
	    mr->VolumeName);
      } else {
         bsendmsg(ua, "%s", db_strerror(ua->db));
	 ok = FALSE;
      }
   }
   return ok;
}
