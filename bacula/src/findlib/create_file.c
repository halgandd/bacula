/*
 *  Create a file, and reset the modes
 *
 *    Kern Sibbald, November MM
 *
 *   Version $Id$
 *
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
#include "find.h"

#ifndef S_IRWXUGO
#define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#ifndef IS_CTG
#define IS_CTG(x) 0
#define O_CTG 0
#endif

static int separate_path_and_file(void *jcr, char *fname, char *ofile);
static int path_already_seen(char *path, int pnl);


/*
 * Create the file, or the directory
 *
 *  fname is the original filename  
 *  ofile is the output filename (may be in a different directory)
 *
 * Returns:  CF_SKIP	 if file should be skipped
 *	     CF_ERROR	 on error
 *	     CF_EXTRACT  file created and data to restore  
 *	     CF_CREATED  file created no data to restore
 *
 *   Note, we create the file here, except for special files,
 *     we do not set the attributes because we want to first 
 *     write the file, then when the writing is done, set the
 *     attributes.
 *   So, we return with the file descriptor open for normal 
 *     files.
 *
 */
int create_file(void *jcr, char *fname, char *ofile, char *lname,
		int type, int stream, struct stat *statp, 
		char *attribsEx, BFILE *ofd, int replace)
{
   int new_mode, parent_mode, mode;
   uid_t uid;
   gid_t gid;
   int pnl;

   binit(ofd);
   new_mode = statp->st_mode;
   Dmsg2(300, "newmode=%x file=%s\n", new_mode, ofile);
   parent_mode = S_IWUSR | S_IXUSR | new_mode;
   gid = statp->st_gid;
   uid = statp->st_uid;

   Dmsg2(400, "Replace=%c %d\n", (char)replace, replace);
   /* If not always replacing, do a stat and decide */
   if (replace != REPLACE_ALWAYS) {
      struct stat mstatp;
      if (lstat(ofile, &mstatp) == 0) {
	 switch (replace) {
	 case REPLACE_IFNEWER:
	    if (statp->st_mtime <= mstatp.st_mtime) {
               Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Not newer: %s\n"), ofile);
	       return CF_SKIP;
	    }
	    break;
	 case REPLACE_IFOLDER:
	    if (statp->st_mtime >= mstatp.st_mtime) {
               Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Not older: %s\n"), ofile);
	       return CF_SKIP;
	    }
	    break;
	 case REPLACE_NEVER:
            Jmsg(jcr, M_SKIPPED, 0, _("File skipped. Already exists: %s\n"), ofile);
	    return CF_SKIP;
	 }
      }
   }
   switch (type) {
   case FT_LNKSAVED:		      /* Hard linked, file already saved */
   case FT_LNK:
   case FT_RAW:
   case FT_FIFO:
   case FT_SPEC:
   case FT_REGE:		      /* empty file */
   case FT_REG: 		      /* regular file */
      /* 
       * Here we do some preliminary work for all the above
       *   types to create the path to the file if it does
       *   not already exist.  Below, we will split to
       *   do the file type specific work
       */
      pnl = separate_path_and_file(jcr, fname, ofile);
      if (pnl < 0) {
	 return CF_ERROR;
      }

      /*
       * If path length is <= 0 we are making a file in the root
       *  directory. Assume that the directory already exists.
       */
      if (pnl > 0) {
	 char savechr;
	 savechr = ofile[pnl];
	 ofile[pnl] = 0;		 /* terminate path */

	 if (!path_already_seen(ofile, pnl)) {
            Dmsg1(50, "Make path %s\n", ofile);
	    /*
	     * If we need to make the directory, ensure that it is with
	     * execute bit set (i.e. parent_mode), and preserve what already
	     * exists. Normally, this should do nothing.
	     */
	    if (make_path(jcr, ofile, parent_mode, parent_mode, uid, gid, 1, NULL) != 0) {
               Dmsg1(0, "Could not make path. %s\n", ofile);
	       return CF_ERROR;
	    }
	 }
	 ofile[pnl] = savechr;		 /* restore full name */
      }

      /* Now we do the specific work for each file type */
      switch(type) {
      case FT_REGE:
      case FT_REG:
         Dmsg1(100, "Create file %s\n", ofile);
	 mode =  O_WRONLY | O_CREAT | O_TRUNC | O_BINARY; /*  O_NOFOLLOW; */
	 if (IS_CTG(statp->st_mode)) {
	    mode |= O_CTG;		 /* set contiguous bit if needed */
	 }
         Dmsg1(50, "Create file: %s\n", ofile);
	 if ((bopen(ofd, ofile, mode, S_IRUSR | S_IWUSR)) < 0) {
            Jmsg2(jcr, M_ERROR, 0, _("Could not create %s: ERR=%s\n"), 
		  ofile, berror(ofd));
	    return CF_ERROR;
	 }
	 return CF_EXTRACT;

      case FT_RAW:		      /* Bacula raw device e.g. /dev/sda1 */
      case FT_FIFO:		      /* Bacula fifo to save data */
      case FT_SPEC:			 
	 if (S_ISFIFO(statp->st_mode)) {
            Dmsg1(200, "Restore fifo: %s\n", ofile);
	    if (mkfifo(ofile, statp->st_mode) != 0 && errno != EEXIST) {
               Jmsg2(jcr, M_ERROR, 0, _("Cannot make fifo %s: ERR=%s\n"), 
		     ofile, strerror(errno));
	       return CF_ERROR;
	    }
	 } else {	   
            Dmsg1(200, "Restore node: %s\n", ofile);
	    if (mknod(ofile, statp->st_mode, statp->st_rdev) != 0 && errno != EEXIST) {
               Jmsg2(jcr, M_ERROR, 0, _("Cannot make node %s: ERR=%s\n"), 
		     ofile, strerror(errno));
	       return CF_ERROR;
	    }
	 }	 
	 if (type == FT_RAW || type == FT_FIFO) {
	    btimer_id tid;
            Dmsg1(200, "FT_RAW|FT_FIFO %s\n", ofile);
	    mode =  O_WRONLY | O_BINARY;
	    /* Timeout open() in 60 seconds */
	    if (type == FT_FIFO) {
	       tid = start_thread_timer(pthread_self(), 60);
	    } else {
	       tid = NULL;
	    }
	    if ((bopen(ofd, ofile, mode, 0)) < 0) {
               Jmsg2(jcr, M_ERROR, 0, _("Could not open %s: ERR=%s\n"), 
		     ofile, berror(ofd));
	       stop_thread_timer(tid);
	       return CF_ERROR;
	    }
	    stop_thread_timer(tid);
	    return CF_EXTRACT;
	 }
         Dmsg1(200, "FT_SPEC %s\n", ofile);
	 return CF_CREATED;

      case FT_LNK:
         Dmsg2(130, "FT_LNK should restore: %s -> %s\n", ofile, lname);
	 if (symlink(lname, ofile) != 0 && errno != EEXIST) {
            Jmsg3(jcr, M_ERROR, 0, _("Could not symlink %s -> %s: ERR=%s\n"),
		  ofile, lname, strerror(errno));
	    return CF_ERROR;
	 }
	 return CF_CREATED;

      case FT_LNKSAVED: 		 /* Hard linked, file already saved */
      Dmsg2(130, "Hard link %s => %s\n", ofile, lname);
      if (link(lname, ofile) != 0) {
         Jmsg3(jcr, M_ERROR, 0, _("Could not hard link %s -> %s: ERR=%s\n"),
	       ofile, lname, strerror(errno));
	 return CF_ERROR;
      }
      return CF_CREATED;

      } /* End inner switch */

   case FT_DIR:
      Dmsg2(300, "Make dir mode=%o dir=%s\n", new_mode, ofile);
      if (make_path(jcr, ofile, new_mode, parent_mode, uid, gid, 0, NULL) != 0) {
	 return CF_ERROR;
      }
#ifdef HAVE_CYGWIN
      if ((bopen(ofd, ofile, O_WRONLY|O_BINARY, 0)) < 0) {
         Jmsg2(jcr, M_ERROR, 0, _("Could not open %s: ERR=%s\n"), 
	       ofile, berror(ofd));
	 return CF_ERROR;
      }
      return CF_EXTRACT;
#else
      return CF_CREATED;
#endif

   /* The following should not occur */
   case FT_NOACCESS:
   case FT_NOFOLLOW:
   case FT_NOSTAT:
   case FT_DIRNOCHG:
   case FT_NOCHG:
   case FT_ISARCH:
   case FT_NORECURSE:
   case FT_NOFSCHG:
   case FT_NOOPEN:
      Jmsg2(jcr, M_ERROR, 0, _("Original file %s not saved: type=%d\n"), fname, type);
   default:
      Jmsg2(jcr, M_ERROR, 0, _("Unknown file type %d; not restored: %s\n"), type, fname);
   }
   return CF_ERROR;
}

/*
 *  Returns: > 0 index into path where last path char is.
 *	     0	no path
 *	     -1 filename is zero length
 */ 
static int separate_path_and_file(void *jcr, char *fname, char *ofile)
{
   char *f, *p;
   int fnl, pnl;

   /* Separate pathname and filename */
   for (p=f=ofile; *p; p++) {
      if (*p == '/') {
	 f = p; 		   /* possible filename */
      }
   }
   if (*f == '/') {
      f++;
   }

   fnl = p - f;
   if (fnl == 0) {
      /* The filename length must not be zero here because we
       *  are dealing with a file (i.e. FT_REGE or FT_REG).
       */
      Jmsg1(jcr, M_ERROR, 0, _("Zero length filename: %s\n"), fname);
      return -1;
   }
   pnl = f - ofile - 1;    
   return pnl;
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* 
 * Primitive caching of path to prevent recreating a pathname 
 *   each time as long as we remain in the same directory.
 */
static int path_already_seen(char *path, int pnl)
{
   static int cached_pnl = 0;
   static char cached_path[1000];

   P(mutex);
   if (cached_pnl == pnl && strcmp(path, cached_path) == 0) {
      V(mutex);
      return 1;
   }
   if (pnl < (int)(sizeof(cached_path)-1)) {
      strcpy(cached_path, path);
      cached_pnl = pnl;
   }
   V(mutex);
   return 0;
}
