/*
 *   util.c  miscellaneous utility subroutines for Bacula
 * 
 *    Kern Sibbald, MM
 *
 *   Version $Id$
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
#include "jcr.h"
#include "findlib/find.h"

/*
 * Various Bacula Utility subroutines
 *
 */

/* Return true of buffer has all zero bytes */
int is_buf_zero(char *buf, int len)
{
   uint64_t *ip = (uint64_t *)buf;
   char *p;
   int i, len64, done, rem;

   /* Optimize by checking uint64_t for zero */
   len64 = len >> sizeof(uint64_t);
   for (i=0; i < len64; i++) {
      if (ip[i] != 0) {
	 return 0;
      }
   }
   done = len64 << sizeof(uint64_t);  /* bytes already checked */
   p = buf + done;
   rem = len - done;
   for (i = 0; i < rem; i++) {
      if (p[i] != 0) {
	 return 0;
      }
   }
   return 1;
}


/* Convert a string in place to lower case */
void lcase(char *str)
{
   while (*str) {
      if (B_ISUPPER(*str))
	 *str = tolower((int)(*str));
       str++;
   }
}

/* Convert spaces to non-space character. 
 * This makes scanf of fields containing spaces easier.
 */
void
bash_spaces(char *str)
{
   while (*str) {
      if (*str == ' ')
	 *str = 0x1;
      str++;
   }
}

/* Convert non-space characters (0x1) back into spaces */
void
unbash_spaces(char *str)
{
   while (*str) {
     if (*str == 0x1)
        *str = ' ';
     str++;
   }
}


char *encode_time(time_t time, char *buf)
{
   struct tm tm;
   int n = 0;

   if (localtime_r(&time, &tm)) {
      n = sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		   tm.tm_hour, tm.tm_min, tm.tm_sec);
   }
   return buf+n;
}

/*
 * Concatenate a string (str) onto a pool memory buffer pm
 */
void pm_strcat(POOLMEM **pm, char *str)
{
   int pmlen = strlen(*pm);
   int len = strlen(str) + 1;

   *pm = check_pool_memory_size(*pm, pmlen + len);
   memcpy(*pm+pmlen, str, len);
}


/*
 * Copy a string (str) into a pool memory buffer pm
 */
void pm_strcpy(POOLMEM **pm, char *str)
{
   int len = strlen(str) + 1;

   *pm = check_pool_memory_size(*pm, len);
   memcpy(*pm, str, len);
}


/*
 * Convert a JobStatus code into a human readable form
 */
void jobstatus_to_ascii(int JobStatus, char *msg, int maxlen)
{
   char *termstat, jstat[2];

   switch (JobStatus) {
      case JS_Terminated:
         termstat = _("OK");
	 break;
     case JS_FatalError:
     case JS_ErrorTerminated:
         termstat = _("Error");
	 break;
     case JS_Error:
         termstat = _("Non-fatal error");
	 break;
     case JS_Canceled:
         termstat = _("Canceled");
	 break;
     case JS_Differences:
         termstat = _("Verify differences");
	 break;
     default:
	 jstat[0] = last_job.JobStatus;
	 jstat[1] = 0;
	 termstat = jstat;
	 break;
   }
   bstrncpy(msg, termstat, maxlen);
}

/*
 * Convert Job Termination Status into a string
 */
char *job_status_to_str(int stat) 
{
   char *str;

   switch (stat) {
   case JS_Terminated:
      str = _("OK");
      break;
   case JS_ErrorTerminated:
   case JS_Error:
      str = _("Error");
      break;
   case JS_FatalError:
      str = _("Fatal Error");
      break;
   case JS_Canceled:
      str = _("Canceled");
      break;
   case JS_Differences:
      str = _("Differences");
      break;
   default:
      str = _("Unknown term code");
      break;
   }
   return str;
}


/*
 * Convert Job Type into a string
 */
char *job_type_to_str(int type) 
{
   char *str;

   switch (type) {
   case JT_BACKUP:
      str = _("Backup");
      break;
   case JT_VERIFY:
      str = _("Verify");
      break;
   case JT_RESTORE:
      str = _("Restore");
      break;
   case JT_ADMIN:
      str = _("Admin");
      break;
   default:
      str = _("Unknown Type");
      break;
   }
   return str;
}

/*
 * Convert Job Level into a string
 */
char *job_level_to_str(int level) 
{
   char *str;

   switch (level) {
   case L_BASE:
      str = _("Base");
   case L_FULL:
      str = _("Full");
      break;
   case L_INCREMENTAL:
      str = _("Incremental");
      break;
   case L_DIFFERENTIAL:
      str = _("Differential");
      break;
   case L_SINCE:
      str = _("Since");
      break;
   case L_VERIFY_CATALOG:
      str = _("Verify Catalog");
      break;
   case L_VERIFY_INIT:
      str = _("Verify Init Catalog");
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      str = _("Verify Volume to Catalog");
      break;
   case L_VERIFY_DATA:
      str = _("Verify Data");
      break;
   default:
      str = _("Unknown Job Level");
      break;
   }
   return str;
}


/***********************************************************************
 * Encode the mode bits into a 10 character string like LS does
 ***********************************************************************/

char *encode_mode(mode_t mode, char *buf)
{
  char *cp = buf;  

  *cp++ = S_ISDIR(mode) ? 'd' : S_ISBLK(mode)  ? 'b' : S_ISCHR(mode)  ? 'c' :
          S_ISLNK(mode) ? 'l' : S_ISFIFO(mode) ? 'f' : S_ISSOCK(mode) ? 's' : '-';
  *cp++ = mode & S_IRUSR ? 'r' : '-';
  *cp++ = mode & S_IWUSR ? 'w' : '-';
  *cp++ = (mode & S_ISUID
               ? (mode & S_IXUSR ? 's' : 'S')
               : (mode & S_IXUSR ? 'x' : '-'));
  *cp++ = mode & S_IRGRP ? 'r' : '-';
  *cp++ = mode & S_IWGRP ? 'w' : '-';
  *cp++ = (mode & S_ISGID
               ? (mode & S_IXGRP ? 's' : 'S')
               : (mode & S_IXGRP ? 'x' : '-'));
  *cp++ = mode & S_IROTH ? 'r' : '-';
  *cp++ = mode & S_IWOTH ? 'w' : '-';
  *cp++ = (mode & S_ISVTX
               ? (mode & S_IXOTH ? 't' : 'T')
               : (mode & S_IXOTH ? 'x' : '-'));
  *cp = '\0';
  return cp;
}


int do_shell_expansion(char *name, int name_len)
{
/*  ****FIXME***** this should work for Win32 too */
#define UNIX
#ifdef UNIX
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

   int pid, wpid, stat;
   int waitstatus;
   char *shellcmd;
   int i;
   char echout[PATH_MAX + 256];
   int pfd[2];
   static char meta[] = "~\\$[]*?`'<>\"";
   int found = FALSE;
   int len;

   /* Check if any meta characters are present */
   len = strlen(meta);
   for (i = 0; i < len; i++) {
      if (strchr(name, meta[i])) {
	 found = TRUE;
	 break;
      }
   }
   stat = 0;
   if (found) {
#ifdef nt
       /* If the filename appears to be a DOS filename,
          convert all backward slashes \ to Unix path
          separators / and insert a \ infront of spaces. */
       len = strlen(name);
       if (len >= 3 && name[1] == ':' && name[2] == '\\') {
	  for (i=2; i<len; i++)
             if (name[i] == '\\')
                name[i] = '/';
       }
#else
       /* Pass string off to the shell for interpretation */
       if (pipe(pfd) == -1)
	  return 0;
       switch(pid = fork()) {
       case -1:
	  break;

       case 0:				  /* child */
	  /* look for shell */
          if ((shellcmd = getenv("SHELL")) == NULL) {
             shellcmd = "/bin/sh";
	  }
	  close(pfd[0]);		  /* close stdin */
	  dup2(pfd[1], 1);		  /* attach to stdout */
	  dup2(pfd[1], 2);		  /* and stderr */
          strcpy(echout, "echo ");        /* form echo command */
	  bstrncat(echout, name, sizeof(echout));
          execl(shellcmd, shellcmd, "-c", echout, NULL); /* give to shell */
          exit(127);                      /* shouldn't get here */

       default: 			  /* parent */
	  /* read output from child */
	  echout[0] = 0;
	  do {
	     i = read(pfd[0], echout, sizeof echout);
	  } while (i == -1 && errno == EINTR); 

	  if (i > 0) {
	     echout[--i] = 0;		     /* set end of string */
	     /* look for first line. */
	     while (--i >= 0) {
                if (echout[i] == '\n') {
		   echout[i] = 0;	     /* keep only first one */
		}
	     }
	  }
	  /* wait for child to exit */
	  while ((wpid = wait(&waitstatus)) != pid && wpid != -1)
	     { ; }
	  strip_trailing_junk(echout);
	  if (strlen(echout) > 0) {
	     bstrncpy(name, echout, name_len);
	  }
	  stat = 1;
	  break;
       }
       close(pfd[0]);			  /* close pipe */
       close(pfd[1]);
#endif /* nt */
   }
   return stat;

#endif /* UNIX */

#if  MSC | MSDOS | __WATCOMC__

   char prefix[100], *env, *getenv();

   /* Home directory reference? */
   if (*name == '~' && (env=getenv("HOME"))) {
      strcpy(prefix, env);	      /* copy HOME directory name */
      name++;			      /* skip over ~ in name */
      strcat(prefix, name);
      name--;			      /* get back to beginning */
      strcpy(name, prefix);	      /* move back into name */
   }
   return 1;
#endif

}


/*  MAKESESSIONKEY  --	Generate session key with optional start
			key.  If mode is TRUE, the key will be
			translated to a string, otherwise it is
			returned as 16 binary bytes.

    from SpeakFreely by John Walker */

void make_session_key(char *key, char *seed, int mode)
{
     int j, k;
     struct MD5Context md5c;
     unsigned char md5key[16], md5key1[16];
     char s[1024];

     s[0] = 0;
     if (seed != NULL) {
	strcat(s, seed);
     }

     /* The following creates a seed for the session key generator
	based on a collection of volatile and environment-specific
	information unlikely to be vulnerable (as a whole) to an
        exhaustive search attack.  If one of these items isn't
	available on your machine, replace it with something
	equivalent or, if you like, just delete it. */

     sprintf(s + strlen(s), "%lu", (unsigned long) getpid());
     sprintf(s + strlen(s), "%lu", (unsigned long) getppid());
     getcwd(s + strlen(s), 256);
     sprintf(s + strlen(s), "%lu", (unsigned long) clock());
     sprintf(s + strlen(s), "%lu", (unsigned long) time(NULL));
#ifdef Solaris
     sysinfo(SI_HW_SERIAL,s + strlen(s), 12);
#endif
#ifdef HAVE_GETHOSTID
     sprintf(s + strlen(s), "%lu", (unsigned long) gethostid());
#endif
#ifdef HAVE_GETDOMAINNAME
     getdomainname(s + strlen(s), 256);
#endif
     gethostname(s + strlen(s), 256);
     sprintf(s + strlen(s), "%u", (unsigned)getuid());
     sprintf(s + strlen(s), "%u", (unsigned)getgid());
     MD5Init(&md5c);
     MD5Update(&md5c, (unsigned char *)s, strlen(s));
     MD5Final(md5key, &md5c);
     sprintf(s + strlen(s), "%lu", (unsigned long) ((time(NULL) + 65121) ^ 0x375F));
     MD5Init(&md5c);
     MD5Update(&md5c, (unsigned char *)s, strlen(s));
     MD5Final(md5key1, &md5c);
#define nextrand    (md5key[j] ^ md5key1[j])
     if (mode) {
	for (j = k = 0; j < 16; j++) {
	    unsigned char rb = nextrand;

#define Rad16(x) ((x) + 'A')
	    key[k++] = Rad16((rb >> 4) & 0xF);
	    key[k++] = Rad16(rb & 0xF);
#undef Rad16
	    if (j & 1) {
                 key[k++] = '-';
	    }
	}
	key[--k] = 0;
     } else {
	for (j = 0; j < 16; j++) {
	    key[j] = nextrand;
	}
     }
}
#undef nextrand



/*
 * Edit job codes into main command line
 *  %% = %
 *  %j = Job name
 *  %t = Job type (Backup, ...)
 *  %e = Job Exit code
 *  %i = JobId
 *  %l = job level
 *  %c = Client's name
 *  %r = Recipients
 *  %d = Director's name
 *
 *  omsg = edited output message
 *  imsg = input string containing edit codes (%x)
 *  to = recepients list 
 *
 */
POOLMEM *edit_job_codes(JCR *jcr, char *omsg, char *imsg, char *to)   
{
   char *p, *str;
   char add[20];

   *omsg = 0;
   Dmsg1(200, "edit_job_codes: %s\n", imsg);
   for (p=imsg; *p; p++) {
      if (*p == '%') {
	 switch (*++p) {
         case '%':
            str = "%";
	    break;
         case 'c':
	    str = jcr->client_name;
	    if (!str) {
               str = "";
	    }
	    break;
         case 'd':
            str = my_name;            /* Director's name */
	    break;
         case 'e':
	    str = job_status_to_str(jcr->JobStatus); 
	    break;
         case 'i':
            sprintf(add, "%d", jcr->JobId);
	    str = add;
	    break;
         case 'j':                    /* Job name */
	    str = jcr->Job;
	    break;
         case 'l':
	    str = job_level_to_str(jcr->JobLevel);
	    break;
         case 'r':
	    str = to;
	    break;
         case 't':
	    str = job_type_to_str(jcr->JobType);
	    break;
	 default:
            add[0] = '%';
	    add[1] = *p;
	    add[2] = 0;
	    str = add;
	    break;
	 }
      } else {
	 add[0] = *p;
	 add[1] = 0;
	 str = add;
      }
      Dmsg1(1200, "add_str %s\n", str);
      pm_strcat(&omsg, str);
      Dmsg1(1200, "omsg=%s\n", omsg);
   }
   return omsg;
}

void set_working_directory(char *wd)
{
   struct stat stat_buf; 

   if (wd == NULL) {
      Emsg0(M_ERROR_TERM, 0, _("Working directory not defined. Cannot continue.\n"));
   }
   if (stat(wd, &stat_buf) != 0) {
      Emsg1(M_ERROR_TERM, 0, _("Working Directory: \"%s\" not found. Cannot continue.\n"),
	 wd);
   }
   if (!S_ISDIR(stat_buf.st_mode)) {
      Emsg1(M_ERROR_TERM, 0, _("Working Directory: \"%s\" is not a directory. Cannot continue.\n"),
	 wd);
   }
   working_directory = wd;	      /* set global */
}
