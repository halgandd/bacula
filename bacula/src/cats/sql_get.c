/*
 * Bacula Catalog Database Get record interface routines
 *  Note, these routines generally get a record by id or
 *	  by name.  If more logic is involved, the routine
 *	  should be in find.c 
 *
 *    Kern Sibbald, March 2000
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

/* *****FIXME**** fix fixed length of select_cmd[] and insert_cmd[] */

/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C 		      /* indicate that this is sql.c */

#include "bacula.h"
#include "cats.h"

#if    HAVE_MYSQL || HAVE_SQLITE

/* -----------------------------------------------------------------------
 *
 *   Generic Routines (or almost generic)
 *
 * -----------------------------------------------------------------------
 */

/* Forward referenced functions */
static int db_get_file_record(B_DB *mdb, FILE_DBR *fdbr);
static int db_get_filename_record(B_DB *mdb, char *fname);
static int db_get_path_record(B_DB *mdb, char *path);


/* Imported subroutines */
extern void print_result(B_DB *mdb);
extern int QueryDB(char *file, int line, B_DB *db, char *select_cmd);


/*
 * Given a full filename (with path), look up the File record
 * (with attributes) in the database.
 *
 *  Returns: 0 on failure
 *	     1 on success with the File record in FILE_DBR
 */
int db_get_file_attributes_record(B_DB *mdb, char *fname, FILE_DBR *fdbr)
{
   int fnl, pnl;
   char *l, *p;
   uint64_t id;
   char file[MAXSTRING];
   char spath[MAXSTRING];
   char buf[MAXSTRING];
   Dmsg0(20, "get_file_from_catalog\n");

   /* Find path without the filename */
   for (p=l=fname; *p; p++) {
      if (*p == '/') {
	 l = p;
      }
   }
   if (*l == '/') {
      l++;
   }

   fnl = p - l;
   strcpy(file, l);

   pnl = l - fname;    
   strncpy(spath, fname, pnl);
   spath[l-fname] = 0;

   if (pnl == 0) {
      return 0;
   }

   strip_trailing_junk(spath);
   Dmsg1(50, "spath=%s\n", spath);

   strip_trailing_junk(file);
   Dmsg1(50, "file=%s\n", file);

   db_escape_string(buf, file, fnl);
   fdbr->FilenameId = db_get_filename_record(mdb, buf);
   Dmsg1(50, "db_get_filename_record FilenameId=%d\n", fdbr->FilenameId);

   db_escape_string(buf, spath, pnl);
   fdbr->PathId = db_get_path_record(mdb, buf);
   Dmsg1(50, "db_get_path_record PathId=%d\n", fdbr->PathId);

   id = db_get_file_record(mdb, fdbr);

   return id;
}

 

/* Get a File record   
 * Returns: 0 on failure
 *	    1 on success
 */
static
int db_get_file_record(B_DB *mdb, FILE_DBR *fdbr)
{
   SQL_ROW row;

   P(mdb->mutex);
   Mmsg(&mdb->cmd, 
"SELECT FileId, LStat, MD5 from File where File.JobId=%d and File.PathId=%d and \
File.FilenameId=%d", fdbr->JobId, fdbr->PathId, fdbr->FilenameId);

   if (QUERY_DB(mdb, mdb->cmd)) {

      mdb->num_rows = sql_num_rows(mdb);

      /* 
       * Note, we can find more than one File record with the same
       *  filename if the file is linked.   ????????
       */
      if (mdb->num_rows > 1) {
         Emsg1(M_WARNING, 0, _("get_file_record want 1 got rows=%d\n"), mdb->num_rows);
         Emsg1(M_WARNING, 0, "%s\n", mdb->cmd);
      }
      if (mdb->num_rows >= 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Emsg1(M_ERROR, 0, "Error fetching row: %s\n", sql_strerror(mdb));
	 } else {
	    fdbr->FileId = (FileId_t)strtod(row[0], NULL);
	    strncpy(fdbr->LStat, row[1], sizeof(fdbr->LStat));
	    fdbr->LStat[sizeof(fdbr->LStat)] = 0;
	    strncpy(fdbr->MD5, row[2], sizeof(fdbr->MD5));
	    fdbr->MD5[sizeof(fdbr->MD5)] = 0;
	    sql_free_result(mdb);
	    V(mdb->mutex);
	    return 1;
	 }
      }

      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return 0;			      /* failed */

}

/* Get Filename record	 
 * Returns: 0 on failure
 *	    FilenameId on success
 */
static int db_get_filename_record(B_DB *mdb, char *fname) 
{
   SQL_ROW row;
   int FilenameId;

   if (*fname == 0) {
      Mmsg0(&mdb->errmsg, _("Null name given to db_get_filename_record\n"));
      Emsg0(M_ABORT, 0, mdb->errmsg);
   }

   P(mdb->mutex);
   Mmsg(&mdb->cmd, "SELECT FilenameId FROM Filename WHERE Name=\"%s\"", fname);
   if (QUERY_DB(mdb, mdb->cmd)) {

      mdb->num_rows = sql_num_rows(mdb);

      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Filename!: %d\n"), (int)(mdb->num_rows));
	 Emsg0(M_FATAL, 0, mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	    Emsg0(M_FATAL, 0, mdb->errmsg);
	 }
	 FilenameId = atoi(row[0]);
	 if (FilenameId <= 0) {
            Mmsg2(&mdb->errmsg, _("Create db Filename record %s found bad record: %d\n"),
	       mdb->cmd, FilenameId); 
	    Emsg0(M_ERROR, 0, mdb->errmsg);
	 }
	 sql_free_result(mdb);
	 V(mdb->mutex);
	 return FilenameId;

      }
      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return 0;			      /* failed */
}

/* Get path record   
 * Returns: 0 on failure
 *	    PathId on success
 */
static int db_get_path_record(B_DB *mdb, char *path)
{
   SQL_ROW row;
   int PathId;
   /*******FIXME***** move into mdb record and allocate */
   static int cached_id = 0;
   static char cached_path[MAXSTRING];

   if (*path == 0) {
      Emsg0(M_ABORT, 0, _("Null path given to db_get_path_record\n"));
   }
   if (cached_id != 0 && strcmp(cached_path, path) == 0) {
      return cached_id;
   }	      

   P(mdb->mutex);
   Mmsg(&mdb->cmd, "SELECT PathId FROM Path WHERE Path=\"%s\"", path);

   if (QUERY_DB(mdb, mdb->cmd)) {
      char ed1[30];
      mdb->num_rows = sql_num_rows(mdb);

      if (mdb->num_rows > 1) {
         Mmsg1(&mdb->errmsg, _("More than one Path!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
	 Emsg0(M_FATAL, 0, mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	    Emsg0(M_FATAL, 0, mdb->errmsg);
	 }
	 PathId = atoi(row[0]);
	 if (PathId != cached_id) {
	    cached_id = PathId;
	    strcpy(cached_path, path);
	 }
	 sql_free_result(mdb);
	 V(mdb->mutex);
	 return PathId;
      }

      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return 0;			      /* failed */
}


/* 
 * Get Job record for given JobId or Job name
 * Returns: 0 on failure
 *	    1 on success
 */
int db_get_job_record(B_DB *mdb, JOB_DBR *jr)
{
   SQL_ROW row;

   P(mdb->mutex);
   if (jr->JobId == 0) {
      Mmsg(&mdb->cmd, "SELECT VolSessionId, VolSessionTime, \
PoolId, StartTime, EndTime, JobFiles, JobBytes, JobTDate, Job \
FROM Job WHERE Job=\"%s\"", jr->Job);
    } else {
      Mmsg(&mdb->cmd, "SELECT VolSessionId, VolSessionTime, \
PoolId, StartTime, EndTime, JobFiles, JobBytes, JobTDate, Job \
FROM Job WHERE JobId=%d", jr->JobId);
    }

   if (!QUERY_DB(mdb, mdb->cmd)) {
      V(mdb->mutex);
      return 0; 		      /* failed */
   }
   if ((row = sql_fetch_row(mdb)) == NULL) {
      Mmsg1(&mdb->errmsg, _("No Job found for JobId %d\n"), jr->JobId);
      sql_free_result(mdb);
      V(mdb->mutex);
      return 0; 		      /* failed */
   }

   jr->VolSessionId = atol(row[0]);
   jr->VolSessionTime = atol(row[1]);
   jr->PoolId = atoi(row[2]);
   strcpy(jr->cStartTime, row[3]);
   strcpy(jr->cEndTime, row[4]);
   jr->JobFiles = atol(row[5]);
   jr->JobBytes = (uint64_t)strtod(row[6], NULL);
   jr->JobTDate = (btime_t)strtod(row[7], NULL);
   strcpy(jr->Job, row[8]);
   sql_free_result(mdb);

   V(mdb->mutex);
   return 1;
}

/*
 * Find VolumeNames for a give JobId
 *  Returns: 0 on error or no Volumes found
 *	     number of volumes on success
 *		Volumes are concatenated in VolumeNames
 *		separated by a vertical bar (|).
 */
int db_get_job_volume_names(B_DB *mdb, uint32_t JobId, char *VolumeNames)
{
   SQL_ROW row;
   int stat = 0;
   int i;

   P(mdb->mutex);
   Mmsg(&mdb->cmd, 
"SELECT VolumeName FROM JobMedia,Media WHERE JobMedia.JobId=%d \
AND JobMedia.MediaId=Media.MediaId", JobId);

   Dmsg1(130, "VolNam=%s\n", mdb->cmd);
   VolumeNames[0] = 0;
   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      Dmsg1(130, "Num rows=%d\n", mdb->num_rows);
      if (mdb->num_rows <= 0) {
         Mmsg1(&mdb->errmsg, _("No volumes found for JobId=%d"), JobId);
	 stat = 0;
      } else {
	 stat = mdb->num_rows;
	 for (i=0; i < stat; i++) {
	    if ((row = sql_fetch_row(mdb)) == NULL) {
               Mmsg2(&mdb->errmsg, _("Error fetching row %d: ERR=%s\n"), i, sql_strerror(mdb));
	       stat = 0;
	       break;
	    } else {
	       if (VolumeNames[0] != 0) {
                  strcat(VolumeNames, "|");
	       }
	       strcat(VolumeNames, row[0]);
	    }
	 }
      }
      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return stat;
}


/* 
 * Get the number of pool records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_pool_records(B_DB *mdb)
{
   int stat = 0;

   P(mdb->mutex);
   Mmsg(&mdb->cmd, "SELECT count(*) from Pool");
   stat = get_sql_record_max(mdb);
   V(mdb->mutex);
   return stat;
}

/*
 * This function returns a list of all the Pool record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_pool_ids(B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   P(mdb->mutex);
   *ids = NULL;
   Mmsg(&mdb->cmd, "SELECT PoolId FROM Pool");
   if (QUERY_DB(mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = (uint32_t)atoi(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(&mdb->errmsg, _("Pool id select failed: ERR=%s\n"), sql_strerror(mdb));
      stat = 0;
   }
   V(mdb->mutex);
   return stat;
}


/* Get Pool Record   
 * If the PoolId is non-zero, we get its record,
 *  otherwise, we search on the PoolName
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_pool_record(B_DB *mdb, POOL_DBR *pdbr)
{
   SQL_ROW row;
   int stat = 0;

   P(mdb->mutex);
   if (pdbr->PoolId != 0) {		  /* find by id */
      Mmsg(&mdb->cmd, 
"SELECT PoolId, Name, NumVols, MaxVols, UseOnce, UseCatalog, AcceptAnyVolume, \
AutoPrune, Recycle, VolRetention, \
PoolType, LabelFormat FROM Pool WHERE Pool.PoolId=%d", pdbr->PoolId);
   } else {			      /* find by name */
      Mmsg(&mdb->cmd, 
"SELECT PoolId, Name, NumVols, MaxVols, UseOnce, UseCatalog, AcceptAnyVolume, \
AutoPrune, Recycle, VolRetention, \
PoolType, LabelFormat FROM Pool WHERE Pool.Name=\"%s\"", pdbr->Name);
   }  

   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Pool!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
	 Emsg0(M_ERROR, 0, mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	    Emsg0(M_ERROR, 0, mdb->errmsg);
	 } else {
	    pdbr->PoolId = atoi(row[0]);
	    strcpy(pdbr->Name, row[1]);
	    pdbr->NumVols = atoi(row[2]);
	    pdbr->MaxVols = atoi(row[3]);
	    pdbr->UseOnce = atoi(row[4]);
	    pdbr->UseCatalog = atoi(row[5]);
	    pdbr->AcceptAnyVolume = atoi(row[6]);
	    pdbr->AutoPrune = atoi(row[7]);
	    pdbr->Recycle = atoi(row[8]);
	    pdbr->VolRetention = (btime_t)strtod(row[9], NULL);
	    strcpy(pdbr->PoolType, row[10]);
	    if (row[11]) {
	       strcpy(pdbr->LabelFormat, row[11]);
	    } else {
	       pdbr->LabelFormat[0] = 0;
	    }
	    stat = pdbr->PoolId;
	 }
      }
      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return stat;
}


/* 
 * Get the number of Media records
 *
 * Returns: -1 on failure
 *	    number on success
 */
int db_get_num_media_records(B_DB *mdb)
{
   int stat = 0;

   P(mdb->mutex);
   Mmsg(&mdb->cmd, "SELECT count(*) from Media");
   stat = get_sql_record_max(mdb);
   V(mdb->mutex);
   return stat;
}


/*
 * This function returns a list of all the Media record ids.
 *  The caller must free ids if non-NULL.
 *
 *  Returns 0: on failure
 *	    1: on success
 */
int db_get_media_ids(B_DB *mdb, int *num_ids, uint32_t *ids[])
{
   SQL_ROW row;
   int stat = 0;
   int i = 0;
   uint32_t *id;

   P(mdb->mutex);
   *ids = NULL;
   Mmsg(&mdb->cmd, "SELECT MediaId FROM Media");
   if (QUERY_DB(mdb, mdb->cmd)) {
      *num_ids = sql_num_rows(mdb);
      if (*num_ids > 0) {
	 id = (uint32_t *)malloc(*num_ids * sizeof(uint32_t));
	 while ((row = sql_fetch_row(mdb)) != NULL) {
	    id[i++] = (uint32_t)atoi(row[0]);
	 }
	 *ids = id;
      }
      sql_free_result(mdb);
      stat = 1;
   } else {
      Mmsg(&mdb->errmsg, _("Media id select failed: ERR=%s\n"), sql_strerror(mdb));
      stat = 0;
   }
   V(mdb->mutex);
   return stat;
}


/* Get Media Record   
 *
 * Returns: 0 on failure
 *	    id on success 
 */
int db_get_media_record(B_DB *mdb, MEDIA_DBR *mr)
{
   SQL_ROW row;
   int stat = 0;

   P(mdb->mutex);
   if (mr->MediaId == 0 && mr->VolumeName[0] == 0) {
      Mmsg(&mdb->cmd, "SELECT count(*) from Media");
      mr->MediaId = get_sql_record_max(mdb);
      V(mdb->mutex);
      return 1;
   }
   if (mr->MediaId != 0) {		 /* find by id */
      Mmsg(&mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,\
VolBytes,VolMounts,VolErrors,VolWrites,VolMaxBytes,VolCapacityBytes,\
MediaType,VolStatus,PoolId,VoRetention,Recycle \
FROM Media WHERE MediaId=%d", mr->MediaId);
   } else {			      /* find by name */
      Mmsg(&mdb->cmd, "SELECT MediaId,VolumeName,VolJobs,VolFiles,VolBlocks,\
VolBytes,VolMounts,VolErrors,VolWrites,VolMaxBytes,VolCapacityBytes,\
MediaType,VolStatus,PoolId,VolRetention,Recycle \
FROM Media WHERE VolumeName=\"%s\"", mr->VolumeName);
   }  

   if (QUERY_DB(mdb, mdb->cmd)) {
      mdb->num_rows = sql_num_rows(mdb);
      if (mdb->num_rows > 1) {
	 char ed1[30];
         Mmsg1(&mdb->errmsg, _("More than one Volume!: %s\n"), 
	    edit_uint64(mdb->num_rows, ed1));
	 Emsg0(M_ERROR, 0, mdb->errmsg);
      } else if (mdb->num_rows == 1) {
	 if ((row = sql_fetch_row(mdb)) == NULL) {
            Mmsg1(&mdb->errmsg, _("error fetching row: %s\n"), sql_strerror(mdb));
	    Emsg0(M_ERROR, 0, mdb->errmsg);
	 } else {
	    /* return values */
	    mr->MediaId = atoi(row[0]);
	    strcpy(mr->VolumeName, row[1]);
	    mr->VolJobs = atoi(row[2]);
	    mr->VolFiles = atoi(row[3]);
	    mr->VolBlocks = atoi(row[4]);
	    mr->VolBytes = (uint64_t)strtod(row[5], NULL);
	    mr->VolMounts = atoi(row[6]);
	    mr->VolErrors = atoi(row[7]);
	    mr->VolWrites = atoi(row[8]);
	    mr->VolMaxBytes = (uint64_t)strtod(row[9], NULL);
	    mr->VolCapacityBytes = (uint64_t)strtod(row[10], NULL);
	    strcpy(mr->MediaType, row[11]);
	    strcpy(mr->VolStatus, row[12]);
	    mr->PoolId = atoi(row[13]);
	    mr->VolRetention = (btime_t)strtod(row[14], NULL);
	    mr->Recycle = atoi(row[15]);
	    stat = mr->MediaId;
	 }
      } else {
         Mmsg0(&mdb->errmsg, _("Media record not found.\n"));
      }
      sql_free_result(mdb);
   }
   V(mdb->mutex);
   return stat;
}


#endif /* HAVE_MYSQL || HAVE_SQLITE */
