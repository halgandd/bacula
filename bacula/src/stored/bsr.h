/*
 * BootStrap record definition -- for restoring files.
 *
 *    Kern Sibbald, June 2002
 *
 *   Version $Id$
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


#ifndef __BSR_H
#define __BSR_H 1

#include "findlib/find.h"

/*
 * List of Volume names to be read by Storage daemon.
 *  Formed by Storage daemon from BSR  
 */
struct s_vol_list {
   struct s_vol_list *next;
   char VolumeName[MAX_NAME_LENGTH];
};
typedef struct s_vol_list VOL_LIST;


/*
 * !!!!!!!!!!!!!!!!!! NOTE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!						     !!!
 * !!!	 All records must have a pointer to	     !!!
 * !!!	 the next item as the first item defined.    !!!
 * !!!						     !!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

typedef struct s_bsr_client {
   struct s_bsr_client *next;
   char ClientName[MAX_NAME_LENGTH];
} BSR_CLIENT;

typedef struct s_bsr_sessid {
   struct s_bsr_sessid *next;
   uint32_t sessid;
   uint32_t sessid2;
   int found;
} BSR_SESSID;

typedef struct s_bsr_sesstime {
   struct s_bsr_sesstime *next;
   uint32_t sesstime;
   int found;
} BSR_SESSTIME;

typedef struct s_bsr_volfile {
   struct s_bsr_volfile *next;
   uint32_t sfile;		      /* start file */
   uint32_t efile;		      /* end file */
   int found;
} BSR_VOLFILE;

typedef struct s_bsr_findex {
   struct s_bsr_findex *next;
   int32_t findex;		      /* start file index */
   int32_t findex2;		      /* end file index */
   int found;
} BSR_FINDEX;

typedef struct s_bsr_jobid {
   struct s_bsr_jobid *next;
   uint32_t JobId;
   uint32_t JobId2;
   int found;
} BSR_JOBID;

typedef struct s_bsr_jobtype {
   struct s_bsr_jobtype *next;
   uint32_t JobType;
} BSR_JOBTYPE;

typedef struct s_bsr_joblevel {
   struct s_bsr_joblevel *next;
   uint32_t JobLevel;
} BSR_JOBLEVEL;

typedef struct s_bsr_job {
   struct s_bsr_job *next;
   char Job[MAX_NAME_LENGTH];
   int found;
} BSR_JOB;


typedef struct s_bsr {
   struct s_bsr *next;		      /* pointer to next one */
   int		 done;		      /* set when everything found */
   char 	*VolumeName;
   BSR_VOLFILE	*volfile;
   BSR_SESSTIME *sesstime;
   BSR_SESSID	*sessid;
   BSR_JOBID	*JobId;
   BSR_JOB	*job;
   BSR_CLIENT	*client;
   BSR_FINDEX	*FileIndex;
   BSR_JOBTYPE	*JobType;
   BSR_JOBLEVEL *JobLevel;
   FF_PKT *ff;			      /* include/exclude */
} BSR;


#endif
