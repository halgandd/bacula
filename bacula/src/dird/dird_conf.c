/*
 *   Main configuration file parser for Bacula Directors,
 *    some parts may be split into separate files such as
 *    the schedule configuration (sch_config.c).
 *
 *   Note, the configuration file parser consists of three parts
 *
 *   1. The generic lexical scanner in lib/lex.c and lib/lex.h
 *
 *   2. The generic config  scanner in lib/parse_config.c and 
 *	lib/parse_config.h.
 *	These files contain the parser code, some utility
 *	routines, and the common store routines (name, int,
 *	string).
 *
 *   3. The daemon specific file, which contains the Resource
 *	definitions as well as any specific store routines
 *	for the resource records.
 *
 *     Kern Sibbald, January MM
 *
 *     $Id:
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

/* Define the first and last resource ID record
 * types. Note, these should be unique for each
 * daemon though not a requirement.
 */
int r_first = R_FIRST;
int r_last  = R_LAST;
pthread_mutex_t res_mutex =  PTHREAD_MUTEX_INITIALIZER;

/* Imported subroutines */
extern void store_run(LEX *lc, struct res_items *item, int index, int pass);


/* Forward referenced subroutines */

static void store_inc(LEX *lc, struct res_items *item, int index, int pass);
static void store_backup(LEX *lc, struct res_items *item, int index, int pass);
static void store_restore(LEX *lc, struct res_items *item, int index, int pass);


/* We build the current resource here as we are
 * scanning the resource configuration definition,
 * then move it to allocated memory when the resource
 * scan is complete.
 */
URES res_all;
int  res_all_size = sizeof(res_all);


/* Definition of records permitted within each
 * resource with the routine to process the record 
 * information.  NOTE! quoted names must be in lower case.
 */ 
/* 
 *    Director Resource
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items dir_items[] = {
   {"name",        store_name,     ITEM(res_dir.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(res_dir.hdr.desc), 0, 0, 0},
   {"messages",    store_res,      ITEM(res_dir.messages), R_MSGS, 0, 0},
   {"dirport",     store_pint,     ITEM(res_dir.DIRport),  0, ITEM_REQUIRED, 0},
   {"queryfile",   store_dir,      ITEM(res_dir.query_file), 0, 0, 0},
   {"workingdirectory", store_dir, ITEM(res_dir.working_directory), 0, ITEM_REQUIRED, 0},
   {"piddirectory", store_dir,     ITEM(res_dir.pid_directory), 0, ITEM_REQUIRED, 0},
   {"subsysdirectory", store_dir,  ITEM(res_dir.subsys_directory), 0, ITEM_REQUIRED, 0},
   {"maximumconcurrentjobs", store_pint, ITEM(res_dir.MaxConcurrentJobs), 0, ITEM_DEFAULT, 1},
   {"password",    store_password, ITEM(res_dir.password), 0, ITEM_REQUIRED, 0},
   {"fdconnecttimeout", store_time,ITEM(res_dir.FDConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {"sdconnecttimeout", store_time,ITEM(res_dir.SDConnectTimeout), 0, ITEM_DEFAULT, 60 * 30},
   {NULL, NULL, NULL, 0, 0, 0}
};

/* 
 *    Client or File daemon resource
 *
 *   name	   handler     value		     code flags    default_value
 */

static struct res_items cli_items[] = {
   {"name",     store_name,       ITEM(res_client.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,     ITEM(res_client.hdr.desc), 0, 0, 0},
   {"address",  store_str,        ITEM(res_client.address),  0, ITEM_REQUIRED, 0},
   {"fdport",   store_pint,       ITEM(res_client.FDport),   0, ITEM_REQUIRED, 0},
   {"password", store_password,   ITEM(res_client.password), 0, ITEM_REQUIRED, 0},
   {"catalog",  store_res,        ITEM(res_client.catalog),  R_CATALOG, 0, 0},
   {"fileretention", store_time,  ITEM(res_client.FileRetention), 0, ITEM_DEFAULT, 60*60*24*60},
   {"jobretention",  store_time,  ITEM(res_client.JobRetention),  0, ITEM_DEFAULT, 60*60*24*180},
   {"autoprune", store_yesno,     ITEM(res_client.AutoPrune), 1, ITEM_DEFAULT, 1},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Storage daemon resource
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items store_items[] = {
   {"name",      store_name,     ITEM(res_store.hdr.name),   0, ITEM_REQUIRED, 0},
   {"description", store_str,    ITEM(res_store.hdr.desc),   0, 0, 0},
   {"sdport",    store_pint,     ITEM(res_store.SDport),     0, ITEM_REQUIRED, 0},
   {"sddport",   store_pint,     ITEM(res_store.SDDport),    0, 0, 0}, /* deprecated */
   {"address",   store_str,      ITEM(res_store.address),    0, ITEM_REQUIRED, 0},
   {"password",  store_password, ITEM(res_store.password),   0, ITEM_REQUIRED, 0},
   {"device",    store_strname,  ITEM(res_store.dev_name),   0, ITEM_REQUIRED, 0},
   {"mediatype", store_strname,  ITEM(res_store.media_type), 0, ITEM_REQUIRED, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* 
 *    Catalog Resource Directives
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items cat_items[] = {
   {"name",     store_name,     ITEM(res_cat.hdr.name),    0, ITEM_REQUIRED, 0},
   {"description", store_str,   ITEM(res_cat.hdr.desc),    0, 0, 0},
   {"address",  store_str,      ITEM(res_cat.address),     0, 0, 0},
   {"dbport",   store_pint,     ITEM(res_cat.DBport),      0, 0, 0},
   /* keep this password as store_str for the moment */
   {"password", store_str,      ITEM(res_cat.db_password), 0, 0, 0},
   {"user",     store_str,      ITEM(res_cat.db_user),     0, 0, 0},
   {"dbname",   store_str,      ITEM(res_cat.db_name),     0, ITEM_REQUIRED, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* 
 *    Job Resource Directives
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items job_items[] = {
   {"name",     store_name,   ITEM(res_job.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str, ITEM(res_job.hdr.desc), 0, 0, 0},
   {"backup",   store_backup, ITEM(res_job),          JT_BACKUP, 0, 0},
   {"verify",   store_backup, ITEM(res_job),          JT_VERIFY, 0, 0},
   {"restore",  store_restore, ITEM(res_job),         JT_RESTORE, 0, 0},
   {"schedule", store_res,    ITEM(res_job.schedule), R_SCHEDULE, 0, 0},
   {"messages", store_res,    ITEM(res_job.messages), R_MSGS, 0, 0},
   {"storage",  store_res,    ITEM(res_job.storage),  R_STORAGE, 0, 0},
   {"pool",     store_res,    ITEM(res_job.pool),     R_POOL, 0, 0},
   {"maxruntime", store_time, ITEM(res_job.MaxRunTime), 0, 0, 0},
   {"maxstartdelay", store_time, ITEM(res_job.MaxStartDelay), 0, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* FileSet resource
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items fs_items[] = {
   {"name",        store_name, ITEM(res_fs.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,  ITEM(res_fs.hdr.desc), 0, 0, 0},
   {"include",     store_inc,  NULL,                  0, 0, 0},
   {"exclude",     store_inc,  NULL,                  1, 0, 0},
   {NULL,	   NULL,       NULL,		      0, 0, 0} 
};

/* Schedule -- see run_conf.c */
/* Schedule
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items sch_items[] = {
   {"name",     store_name,  ITEM(res_sch.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str, ITEM(res_sch.hdr.desc), 0, 0, 0},
   {"run",      store_run,   ITEM(res_sch.run),      0, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Group resource -- not implemented
 *
 *   name	   handler     value		     code flags    default_value
 */
static struct res_items group_items[] = {
   {"name",        store_name, ITEM(res_group.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,  ITEM(res_group.hdr.desc), 0, 0, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Pool resource
 *
 *   name	      handler	  value 		       code flags default_value
 */
static struct res_items pool_items[] = {
   {"name",            store_name,    ITEM(res_pool.hdr.name),        0, ITEM_REQUIRED, 0},
   {"description",     store_str,     ITEM(res_pool.hdr.desc),        0, 0,     0},
   {"pooltype",        store_strname, ITEM(res_pool.pool_type),       0, ITEM_REQUIRED, 0},
   {"labelformat",     store_strname, ITEM(res_pool.label_format),    0, 0,     0},
   {"usecatalog",      store_yesno, ITEM(res_pool.use_catalog),     1, ITEM_DEFAULT,  1},
   {"usevolumeonce",   store_yesno, ITEM(res_pool.use_volume_once), 1, 0,       0},
   {"maximumvolumes",  store_pint,  ITEM(res_pool.max_volumes),     0, 0,             0},
   {"acceptanyvolume", store_yesno, ITEM(res_pool.accept_any_volume), 1, 0,     0},
   {"catalogfiles",    store_yesno, ITEM(res_pool.catalog_files),   1, ITEM_DEFAULT,  1},
   {"volumeretention", store_time,  ITEM(res_pool.VolRetention), 0, ITEM_DEFAULT, 60*60*24*365},
   {"autoprune",       store_yesno, ITEM(res_pool.AutoPrune), 1, ITEM_DEFAULT, 1},
   {"recycle",         store_yesno, ITEM(res_pool.Recycle),     1, ITEM_DEFAULT, 1},
   {NULL, NULL, NULL, 0, 0, 0} 
};

/* Message resource */
extern struct res_items msgs_items[];

/* 
 * This is the master resource definition.  
 * It must have one item for each of the resources.
 *
 *  name	     items	  rcode        res_head
 */
struct s_res resources[] = {
   {"director",      dir_items,   R_DIRECTOR,  NULL},
   {"client",        cli_items,   R_CLIENT,    NULL},
   {"job",           job_items,   R_JOB,       NULL},
   {"storage",       store_items, R_STORAGE,   NULL},
   {"catalog",       cat_items,   R_CATALOG,   NULL},
   {"schedule",      sch_items,   R_SCHEDULE,  NULL},
   {"fileset",       fs_items,    R_FILESET,   NULL},
   {"group",         group_items, R_GROUP,     NULL},
   {"pool",          pool_items,  R_POOL,      NULL},
   {"messages",      msgs_items,  R_MSGS,      NULL},
   {NULL,	     NULL,	  0,	       NULL}
};


/* Keywords (RHS) permitted in Job Level records   
 *
 *   level_name      level		level_class
 */
struct s_jl joblevels[] = {
   {"Full",          L_FULL,            JT_BACKUP},
   {"Incremental",   L_INCREMENTAL,     JT_BACKUP},
   {"Differential",  L_DIFFERENTIAL,    JT_BACKUP},
   {"Level",         L_LEVEL,           JT_BACKUP},
   {"Since",         L_SINCE,           JT_BACKUP},
   {"Catalog",       L_VERIFY_CATALOG,  JT_VERIFY},
   {"Initcatalog",   L_VERIFY_INIT,     JT_VERIFY},
   {"Volume",        L_VERIFY_VOLUME,   JT_VERIFY},
   {"Data",          L_VERIFY_DATA,     JT_VERIFY},
   {NULL,	     0}
};

/* Keywords (RHS) permitted in Backup and Verify records */
static struct s_kw BakVerFields[] = {
   {"client",        'C'},
   {"fileset",       'F'},
   {"level",         'L'}, 
   {NULL,	     0}
};

/* Keywords (RHS) permitted in Restore records */
static struct s_kw RestoreFields[] = {
   {"client",        'C'},
   {"fileset",       'F'},
   {"jobid",         'J'},            /* JobId to restore */
   {"where",         'W'},            /* root of restore */
   {"replace",       'R'},            /* replacement options */
   {NULL,	       0}
};

/* Options permitted in Restore replace= */
static struct s_kw ReplaceOptions[] = {
   {"always",         'A'},           /* always */
   {"ifnewer",        'W'},
   {"never",          'N'},
   {NULL,		0}
};



/* Define FileSet KeyWord values */

#define FS_KW_NONE	   0
#define FS_KW_COMPRESSION  1
#define FS_KW_SIGNATURE    2
#define FS_KW_ENCRYPTION   3
#define FS_KW_VERIFY	   4

/* FileSet keywords */
static struct s_kw FS_option_kw[] = {
   {"compression", FS_KW_COMPRESSION},
   {"signature",   FS_KW_SIGNATURE},
   {"encryption",  FS_KW_ENCRYPTION},
   {"verify",      FS_KW_VERIFY},
   {NULL,	   0}
};

/* Options for FileSet keywords */

struct s_fs_opt {
   char *name;
   int keyword;
   char option;
};

/* Options permitted for each keyword and resulting value */
static struct s_fs_opt FS_options[] = {
   {"md5",      FS_KW_SIGNATURE,    'M'},
   {"gzip",     FS_KW_COMPRESSION,  'Z'},
   {"blowfish", FS_KW_ENCRYPTION,   'B'},   /* ***FIXME*** not implemented */
   {"3des",     FS_KW_ENCRYPTION,   '3'},   /* ***FIXME*** not implemented */
   {NULL,	0,		     0}
};

char *level_to_str(int level)
{
   int i;
   static char level_no[30];
   char *str = level_no;

   sprintf(level_no, "%d", level);    /* default if not found */
   for (i=0; joblevels[i].level_name; i++) {
      if (level == joblevels[i].level) {
	 str = joblevels[i].level_name;
	 break;
      }
   }
   return str;
}



/* Dump contents of resource */
void dump_resource(int type, RES *reshdr, void sendit(void *sock, char *fmt, ...), void *sock)
{
   int i;
   URES *res = (URES *)reshdr;
   int recurse = 1;

   if (res == NULL) {
      sendit(sock, "No %s resource defined\n", res_to_str(type));
      return;
   }
   if (type < 0) {		      /* no recursion */
      type = - type;
      recurse = 0;
   }
   switch (type) {
      case R_DIRECTOR:
	 char ed1[30], ed2[30];
         sendit(sock, "Director: name=%s maxjobs=%d FDtimeout=%s SDtimeout=%s\n", 
	    reshdr->name, res->res_dir.MaxConcurrentJobs, 
	    edit_uint64(res->res_dir.FDConnectTimeout, ed1),
	    edit_uint64(res->res_dir.SDConnectTimeout, ed2));
	 if (res->res_dir.query_file) {
            sendit(sock, "   query_file=%s\n", res->res_dir.query_file);
	 }
	 if (res->res_dir.messages) {
            sendit(sock, "  --> ");
	    dump_resource(-R_MSGS, (RES *)res->res_dir.messages, sendit, sock);
	 }
	 break;
      case R_CLIENT:
         sendit(sock, "Client: name=%s address=%s FDport=%d\n",
	    res->res_client.hdr.name, res->res_client.address, res->res_client.FDport);
         sendit(sock, "JobRetention=%" lld " FileRetention=%" lld " AutoPrune=%d\n",
	    res->res_client.JobRetention, res->res_client.FileRetention,
	    res->res_client.AutoPrune);
	 if (res->res_client.catalog) {
            sendit(sock, "  --> ");
	    dump_resource(-R_CATALOG, (RES *)res->res_client.catalog, sendit, sock);
	 }
	 break;
      case R_STORAGE:
         sendit(sock, "Storage: name=%s address=%s SDport=%d\n\
         DeviceName=%s MediaType=%s\n",
	    res->res_store.hdr.name, res->res_store.address, res->res_store.SDport,
	    res->res_store.dev_name, res->res_store.media_type);
	 break;
      case R_CATALOG:
         sendit(sock, "Catalog: name=%s address=%s DBport=%d db_name=%s\n\
         db_user=%s\n",
	    res->res_cat.hdr.name, res->res_cat.address, res->res_cat.DBport,
	    res->res_cat.db_name, res->res_cat.db_user);
	 break;
      case R_JOB:
         sendit(sock, "Job: name=%s JobType=%d level=%s\n", res->res_job.hdr.name, 
	    res->res_job.JobType, level_to_str(res->res_job.level));
	 if (res->res_job.client) {
            sendit(sock, "  --> ");
	    dump_resource(-R_CLIENT, (RES *)res->res_job.client, sendit, sock);
	 }
	 if (res->res_job.fs) {
            sendit(sock, "  --> ");
	    dump_resource(-R_FILESET, (RES *)res->res_job.fs, sendit, sock);
	 }
	 if (res->res_job.schedule) {
            sendit(sock, "  --> ");
	    dump_resource(-R_SCHEDULE, (RES *)res->res_job.schedule, sendit, sock);
	 }
	 if (res->res_job.RestoreWhere) {
            sendit(sock, "  --> Where=%s\n", res->res_job.RestoreWhere);
	 }
	 if (res->res_job.storage) {
            sendit(sock, "  --> ");
	    dump_resource(-R_STORAGE, (RES *)res->res_job.storage, sendit, sock);
	 }
	 if (res->res_job.pool) {
            sendit(sock, "  --> ");
	    dump_resource(-R_POOL, (RES *)res->res_job.pool, sendit, sock);
	 } else {
            sendit(sock, "!!! No Pool resource\n");
	 }
	 if (res->res_job.messages) {
            sendit(sock, "  --> ");
	    dump_resource(-R_MSGS, (RES *)res->res_job.messages, sendit, sock);
	 }
	 break;
      case R_FILESET:
         sendit(sock, "FileSet: name=%s\n", res->res_fs.hdr.name);
	 for (i=0; i<res->res_fs.num_includes; i++)
            sendit(sock, "      Inc: %s\n", res->res_fs.include_array[i]);
	 for (i=0; i<res->res_fs.num_excludes; i++)
            sendit(sock, "      Exc: %s\n", res->res_fs.exclude_array[i]);
	 break;
      case R_SCHEDULE:
	 if (res->res_sch.run)
            sendit(sock, "Schedule: name=%s Level=%s\n", res->res_sch.hdr.name,
	       level_to_str(res->res_sch.run->level));
	 else
            sendit(sock, "Schedule: name=%s\n", res->res_sch.hdr.name);
	 break;
      case R_GROUP:
         sendit(sock, "Group: name=%s\n", res->res_group.hdr.name);
	 break;
      case R_POOL:
         sendit(sock, "Pool: name=%s PoolType=%s\n", res->res_pool.hdr.name,
		 res->res_pool.pool_type);
         sendit(sock, "      use_cat=%d use_once=%d acpt_any=%d cat_files=%d\n",
		 res->res_pool.use_catalog, res->res_pool.use_volume_once,
		 res->res_pool.accept_any_volume, res->res_pool.catalog_files);
         sendit(sock, "      max_vols=%d auto_prune=%d VolRetention=%" lld "\n",
		 res->res_pool.max_volumes, res->res_pool.AutoPrune,
		 res->res_pool.VolRetention);
         sendit(sock, "      recycle=%d\n",  res->res_pool.Recycle);
	 

         sendit(sock, "      LabelFormat=%s\n", res->res_pool.label_format?
                 res->res_pool.label_format:"NONE");
	 break;
      case R_MSGS:
         sendit(sock, "Messages: name=%s\n", res->res_msgs.hdr.name);
	 if (res->res_msgs.mail_cmd) 
            sendit(sock, "      mailcmd=%s\n", res->res_msgs.mail_cmd);
	 if (res->res_msgs.operator_cmd) 
            sendit(sock, "      opcmd=%s\n", res->res_msgs.operator_cmd);
	 break;
      default:
         sendit(sock, "Unknown resource type %d\n", type);
	 break;
   }
   if (recurse && res->res_dir.hdr.next) {
      dump_resource(type, res->res_dir.hdr.next, sendit, sock);
   }
}

/* 
 * Free memory of resource.  
 * NB, we don't need to worry about freeing any references
 * to other resources as they will be freed when that 
 * resource chain is traversed.  Mainly we worry about freeing
 * allocated strings (names).
 */
void free_resource(int type)
{
   int num;
   URES *res;
   RES *nres;
   int rindex = type - r_first;

   res = (URES *)resources[rindex].res_head;

   if (res == NULL)
      return;

   /* common stuff -- free the resource name and description */
   nres = (RES *)res->res_dir.hdr.next;
   if (res->res_dir.hdr.name) {
      free(res->res_dir.hdr.name);
   }
   if (res->res_dir.hdr.desc) {
      free(res->res_dir.hdr.desc);
   }

   switch (type) {
      case R_DIRECTOR:
	 if (res->res_dir.working_directory)
	    free(res->res_dir.working_directory);
	 if (res->res_dir.pid_directory)
	    free(res->res_dir.pid_directory);
	 if (res->res_dir.subsys_directory)
	    free(res->res_dir.subsys_directory);
	 if (res->res_dir.password)
	    free(res->res_dir.password);
	 if (res->res_dir.query_file)
	    free(res->res_dir.query_file);
	 break;
      case R_CLIENT:
	 if (res->res_client.address)
	    free(res->res_client.address);
	 if (res->res_client.password)
	    free(res->res_client.password);
	 break;
      case R_STORAGE:
	 if (res->res_store.address)
	    free(res->res_store.address);
	 if (res->res_store.password)
	    free(res->res_store.password);
	 if (res->res_store.media_type)
	    free(res->res_store.media_type);
	 if (res->res_store.dev_name)
	    free(res->res_store.dev_name);
	 break;
      case R_CATALOG:
	 if (res->res_cat.address)
	    free(res->res_cat.address);
	 if (res->res_cat.db_user)
	    free(res->res_cat.db_user);
	 if (res->res_cat.db_name)
	    free(res->res_cat.db_name);
	 if (res->res_cat.db_password)
	    free(res->res_cat.db_password);
	 break;
      case R_FILESET:
	 if ((num=res->res_fs.num_includes)) {
	    while (--num >= 0)	  
	       free(res->res_fs.include_array[num]);
	    free(res->res_fs.include_array);
	 }
	 if ((num=res->res_fs.num_excludes)) {
	    while (--num >= 0)	  
	       free(res->res_fs.exclude_array[num]);
	    free(res->res_fs.exclude_array);
	 }
	 break;
      case R_POOL:
	 if (res->res_pool.pool_type) {
	    free(res->res_pool.pool_type);
	 }
	 if (res->res_pool.label_format) {
	    free(res->res_pool.label_format);
	 }
	 break;
      case R_SCHEDULE:
	 if (res->res_sch.run) {
	    RUN *nrun, *next;
	    nrun = res->res_sch.run;
	    while (nrun) {
	       next = nrun->next;
	       free(nrun);
	       nrun = next;
	    }
	 }
	 break;
      case R_JOB:
	 if (res->res_job.RestoreWhere) {
	    free(res->res_job.RestoreWhere);
	 }
	 break;
      case R_MSGS:
	 if (res->res_msgs.mail_cmd)
	    free(res->res_msgs.mail_cmd);
	 if (res->res_msgs.operator_cmd)
	    free(res->res_msgs.operator_cmd);
	 break;
      case R_GROUP:
	 break;
      default:
         printf("Unknown resource type %d\n", type);
   }
   /* Common stuff again -- free the resource, recurse to next one */
   free(res);
   resources[rindex].res_head = nres;
   if (nres)
      free_resource(type);
}

/* Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * pointers (currently only in the Job resource).
 */
void save_resource(int type, struct res_items *items, int pass)
{
   URES *res;
   int rindex = type - r_first;
   int i, size;
   int error = 0;
   
   /* 
    * Ensure that all required items are present
    */
   for (i=0; items[i].name; i++) {
      if (items[i].flags & ITEM_REQUIRED) {
	    if (!bit_is_set(i, res_all.res_dir.hdr.item_present)) {  
               Emsg2(M_ABORT, 0, "%s item is required in %s resource, but not found.\n",
		 items[i].name, resources[rindex]);
	     }
      }
      /* If this triggers, take a look at lib/parse_conf.h */
      if (i >= MAX_RES_ITEMS) {
         Emsg1(M_ABORT, 0, "Too many items in %s resource\n", resources[rindex]);
      }
   }

   /* During pass 2, we looked up pointers to all the resources
    * referrenced in the current resource, , now we
    * must copy their address from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      switch (type) {
	 /* Resources not containing a resource */
	 case R_CATALOG:
	 case R_STORAGE:
	 case R_FILESET:
	 case R_SCHEDULE:
	 case R_GROUP:
	 case R_POOL:
	 case R_MSGS:
	    break;

	 /* Resources containing another resource */
	 case R_DIRECTOR:
	    if ((res = (URES *)GetResWithName(R_DIRECTOR, res_all.res_dir.hdr.name)) == NULL) {
               Emsg1(M_ABORT, 0, "Cannot find Director resource %s\n", res_all.res_dir.hdr.name);
	    }
	    res->res_dir.messages = res_all.res_dir.messages;
	    break;
	 case R_JOB:
	    if ((res = (URES *)GetResWithName(R_JOB, res_all.res_dir.hdr.name)) == NULL) {
               Emsg1(M_ABORT, 0, "Cannot find Job resource %s\n", res_all.res_dir.hdr.name);
	    }
	    res->res_job.messages = res_all.res_job.messages;
	    res->res_job.schedule = res_all.res_job.schedule;
	    res->res_job.client   = res_all.res_job.client;
	    res->res_job.fs	  = res_all.res_job.fs;
	    res->res_job.storage  = res_all.res_job.storage;
	    res->res_job.pool	  = res_all.res_job.pool;
	    break;
	 case R_CLIENT:
	    if ((res = (URES *)GetResWithName(R_CLIENT, res_all.res_client.hdr.name)) == NULL) {
               Emsg1(M_ABORT, 0, "Cannot find Client resource %s\n", res_all.res_client.hdr.name);
	    }
	    res->res_client.catalog = res_all.res_client.catalog;
	    break;
	 default:
            Emsg1(M_ERROR, 0, "Unknown resource type %d\n", type);
	    error = 1;
	    break;
      }
      /* Note, the resource name was already saved during pass 1,
       * so here, we can just release it.
       */
      if (res_all.res_dir.hdr.name) {
	 free(res_all.res_dir.hdr.name);
	 res_all.res_dir.hdr.name = NULL;
      }
      return;
   }

   switch (type) {
      case R_DIRECTOR:
	 size = sizeof(DIRRES);
	 break;
      case R_CLIENT:
	 size =sizeof(CLIENT);
	 break;
      case R_STORAGE:
	 size = sizeof(STORE); 
	 break;
      case R_CATALOG:
	 size = sizeof(CAT);
	 break;
      case R_JOB:
	 size = sizeof(JOB);
	 break;
      case R_FILESET:
	 size = sizeof(FILESET);
	 break;
      case R_SCHEDULE:
	 size = sizeof(SCHED);
	 break;
      case R_GROUP:
	 size = sizeof(GROUP);
	 break;
      case R_POOL:
	 size = sizeof(POOL);
	 break;
      case R_MSGS:
	 size = sizeof(MSGS);
	 break;
      default:
         printf("Unknown resource type %d\n", type);
	 error = 1;
	 break;
   }
   /* Common */
   if (!error) {
      res = (URES *) malloc(size);
      memcpy(res, &res_all, size);
      res->res_dir.hdr.next = resources[rindex].res_head;
      resources[rindex].res_head = (RES *)res;
      Dmsg2(90, "dir_conf: inserting %s res: %s\n", res_to_str(type),
	 res->res_dir.hdr.name);
   }

}

/* 
 * Store backup/verify info for Job record 
 *
 * Note, this code is used for both BACKUP and VERIFY jobs
 *
 *    Backup = Client=<client-name> FileSet=<FileSet-name> Level=<level>
 */
static void store_backup(LEX *lc, struct res_items *item, int index, int pass)
{
   int token, i;
   RES *res;
   int options = lc->options;

   lc->options |= LOPT_NO_IDENT;      /* make spaces significant */

   
   ((JOB *)(item->value))->JobType = item->code;
   while ((token = lex_get_token(lc)) != T_EOL) {
      int found;

      if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
         scan_err1(lc, "Expected a backup/verify keyword, got: %s", lc->str);
      } else {
	 lcase(lc->str);
         Dmsg1(190, "Got keyword: %s\n", lc->str);
	 found = FALSE;
	 for (i=0; BakVerFields[i].name; i++) {
	    if (strcasecmp(lc->str, BakVerFields[i].name) == 0) {
	       found = TRUE;
	       if (lex_get_token(lc) != T_EQUALS) {
                  scan_err1(lc, "Expected an equals, got: %s", lc->str);
	       }
	       token = lex_get_token(lc);
	       if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
                  scan_err1(lc, "Expected a keyword name, got: %s", lc->str);
	       }
               Dmsg1(190, "Got value: %s\n", lc->str);
	       switch (BakVerFields[i].token) {
                  case 'C':
		     /* Find Client Resource */
		     if (pass == 2) {
			res = GetResWithName(R_CLIENT, lc->str);
			if (res == NULL) {
                           scan_err1(lc, "Could not find specified Client Resource: %s",
				      lc->str);
			}
			res_all.res_job.client = (CLIENT *)res;
		     }
		     break;
                  case 'F':
		     /* Find FileSet Resource */
		     if (pass == 2) {
			res = GetResWithName(R_FILESET, lc->str);
			if (res == NULL) {
                           scan_err1(lc, "Could not find specified FileSet Resource: %s\n",
				       lc->str);
			}
			res_all.res_job.fs = (FILESET *)res;
		     }
		     break;
                  case 'L':
		     /* Get level */
		     lcase(lc->str);
		     for (i=0; joblevels[i].level_name; i++) {
			if (joblevels[i].job_class == item->code && 
			     strcasecmp(lc->str, joblevels[i].level_name) == 0) {
			   ((JOB *)(item->value))->level = joblevels[i].level;
			   i = 0;
			   break;
			}
		     }
		     if (i != 0) {
                        scan_err1(lc, "Expected a Job Level keyword, got: %s", lc->str);
		     }
		     break;
	       } /* end switch */
	       break;
	    } /* end if strcmp() */
	 } /* end for */
	 if (!found) {
            scan_err1(lc, "%s not a valid Backup/verify keyword", lc->str);
	 }
      }
   } /* end while */
   lc->options = options;	      /* reset original options */
   set_bit(index, res_all.hdr.item_present);
}

/* 
 * Store restore info for Job record 
 *
 *    Restore = JobId=<job-id> Where=<root-directory> Replace=<options>
 *
 */
static void store_restore(LEX *lc, struct res_items *item, int index, int pass)
{
   int token, i;
   RES *res;
   int options = lc->options;

   lc->options |= LOPT_NO_IDENT;      /* make spaces significant */

   Dmsg0(190, "Enter store_restore()\n");
   
   ((JOB *)(item->value))->JobType = item->code;
   while ((token = lex_get_token(lc)) != T_EOL) {
      int found; 

      if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
         scan_err1(lc, "Expected a Restore keyword, got: %s", lc->str);
      } else {
	 lcase(lc->str);
	 found = FALSE;
	 for (i=0; RestoreFields[i].name; i++) {
            Dmsg1(190, "Restore kw=%s\n", lc->str);
	    if (strcmp(lc->str, RestoreFields[i].name) == 0) {
	       found = TRUE;
	       if (lex_get_token(lc) != T_EQUALS) {
                  scan_err1(lc, "Expected an equals, got: %s", lc->str);
	       }
	       token = lex_get_token(lc);
               Dmsg1(190, "Restore value=%s\n", lc->str);
	       switch (RestoreFields[i].token) {
                  case 'C':
		     /* Find Client Resource */
		     if (pass == 2) {
			res = GetResWithName(R_CLIENT, lc->str);
			if (res == NULL) {
                           scan_err1(lc, "Could not find specified Client Resource: %s",
				      lc->str);
			}
			res_all.res_job.client = (CLIENT *)res;
		     }
		     break;
                  case 'F':
		     /* Find FileSet Resource */
		     if (pass == 2) {
			res = GetResWithName(R_FILESET, lc->str);
			if (res == NULL) {
                           scan_err1(lc, "Could not find specified FileSet Resource: %s\n",
				       lc->str);
			}
			res_all.res_job.fs = (FILESET *)res;
		     }
		     break;
                  case 'J':
		     /* JobId */
		     if (token != T_NUMBER) {
                        scan_err1(lc, "expected an integer number, got: %s", lc->str);
		     }
		     errno = 0;
		     res_all.res_job.RestoreJobId = strtol(lc->str, NULL, 0);
                     Dmsg1(190, "RestorJobId=%d\n", res_all.res_job.RestoreJobId);
		     if (errno != 0) {
                        scan_err1(lc, "expected an integer number, got: %s", lc->str);
		     }
		     break;
                  case 'W':
		     /* Where */
		     if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
                        scan_err1(lc, "Expected a Restore root directory, got: %s", lc->str);
		     }
		     if (pass == 1) {
			res_all.res_job.RestoreWhere = bstrdup(lc->str);
		     }
		     break;
                  case 'R':
		     /* Replacement options */
		     if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
                        scan_err1(lc, "Expected a keyword name, got: %s", lc->str);
		     }
		     lcase(lc->str);
		     /* Fix to scan Replacement options */
		     for (i=0; ReplaceOptions[i].name; i++) {
			if (strcmp(lc->str, ReplaceOptions[i].name) == 0) {
			    ((JOB *)(item->value))->RestoreOptions = ReplaceOptions[i].token;
			   i = 0;
			   break;
			}
		     }
		     if (i != 0) {
                        scan_err1(lc, "Expected a Restore replacement option, got: %s", lc->str);
		     }
		     break;
	       } /* end switch */
	       break;
	    } /* end if strcmp() */
	 } /* end for */
	 if (!found) {
            scan_err1(lc, "%s not a valid Restore keyword", lc->str);
	 }
      }
   } /* end while */
   lc->options = options;	      /* reset original options */
   set_bit(index, res_all.hdr.item_present);
}



/* 
 * Scan for FileSet options
 */
static char *scan_fs_options(LEX *lc, int keyword)
{
   int token, i;
   static char opts[100];
   char option[2];

   option[0] = 0;		      /* default option = none */
   opts[0] = option[1] = 0;	      /* terminate options */
   for (;;) {
      token = lex_get_token(lc);	     /* expect at least one option */	    
      if (token != T_IDENTIFIER && token != T_STRING && token != T_QUOTED_STRING) {
         scan_err1(lc, "expected a FileSet option, got: %s", lc->str);
      }
      lcase(lc->str);
      if (keyword == FS_KW_VERIFY) { /* special case */
	 /* ***FIXME**** ensure these are in permitted set */
         strcpy(option, "V");         /* indicate Verify */
	 strcat(option, lc->str);
         strcat(option, ":");         /* terminate it */
      } else {
	 for (i=0; FS_options[i].name; i++) {
	    if (strcmp(lc->str, FS_options[i].name) == 0 && FS_options[i].keyword == keyword) {
	       option[0] = FS_options[i].option;
	       i = 0;
	       break;
	    }
	 }
	 if (i != 0) {
            scan_err1(lc, "Expected a FileSet option keyword, got: %s", lc->str);
	 }
      }
      strcat(opts, option);

      /* check if more options are specified */
      if (lc->ch != ',') {
	 break; 		      /* no, get out */
      }
      token = lex_get_token(lc);      /* yes, eat comma */
   }

   return opts;
}


/* Store FileSet Include/Exclude info */
static void store_inc(LEX *lc, struct res_items *item, int index, int pass)
{
   int token, i;
   int options = lc->options;
   int keyword;
   char *fname;
   char inc_opts[100];
   int inc_opts_len;

   lc->options |= LOPT_NO_IDENT;      /* make spaces significant */

   /* Get include options */
   strcpy(inc_opts, "0");             /* set no options */
   while ((token=lex_get_token(lc)) != T_BOB) {
      if (token != T_STRING) {
         scan_err1(lc, "expected a FileSet option keyword, got: %s", lc->str);
      } else {
	 keyword = FS_KW_NONE;
	 lcase(lc->str);
	 for (i=0; FS_option_kw[i].name; i++) {
	    if (strcmp(lc->str, FS_option_kw[i].name) == 0) {
	       keyword = FS_option_kw[i].token;
	       break;
	    }
	 }
	 if (keyword == FS_KW_NONE) {
            scan_err1(lc, "Expected a FileSet keyword, got: %s", lc->str);
	 }
      }
      /* Option keyword should be following by = <option> */
      if ((token=lex_get_token(lc)) != T_EQUALS) {
         scan_err1(lc, "expected an = following keyword, got: %s", lc->str);
      }
      strcat(inc_opts, scan_fs_options(lc, keyword));
      if (token == T_BOB) {
	 break;
      }
   }
   strcat(inc_opts, " ");             /* add field separator */
   inc_opts_len = strlen(inc_opts);


   if (pass == 1) {
      if (!res_all.res_fs.have_MD5) {
	 MD5Init(&res_all.res_fs.md5c);
	 res_all.res_fs.have_MD5 = TRUE;
      }
      /* Pickup include/exclude names. Note, they are stored as
       * XYZ fname
       * where XYZ are the include/exclude options for the FileSet
       *     a "0 " (zero) indicates no options,
       * and fname is the file/directory name given
       */
      while ((token = lex_get_token(lc)) != T_EOB) {
	 switch (token) {
	    case T_COMMA:
	    case T_EOL:
	       continue;

	    case T_IDENTIFIER:
	    case T_STRING:
	    case T_QUOTED_STRING:
	       fname = (char *) malloc(lc->str_len + inc_opts_len + 1);
	       strcpy(fname, inc_opts);
	       strcat(fname, lc->str);
	       if (res_all.res_fs.have_MD5) {
		  MD5Update(&res_all.res_fs.md5c, (unsigned char *) fname, inc_opts_len + lc->str_len);
	       }
	       if (item->code == 0) { /* include */
		  if (res_all.res_fs.num_includes == res_all.res_fs.include_size) {
		     res_all.res_fs.include_size += 10;
		     if (res_all.res_fs.include_array == NULL) {
			res_all.res_fs.include_array = (char **) malloc(sizeof(char *) * res_all.res_fs.include_size);
		     } else {
			res_all.res_fs.include_array = (char **) realloc(res_all.res_fs.include_array,
			   sizeof(char *) * res_all.res_fs.include_size);
		     }
		  }
		  res_all.res_fs.include_array[res_all.res_fs.num_includes++] =    
		     fname;
	       } else { 	       /* exclude */
		  if (res_all.res_fs.num_excludes == res_all.res_fs.exclude_size) {
		     res_all.res_fs.exclude_size += 10;
		     if (res_all.res_fs.exclude_array == NULL) {
			res_all.res_fs.exclude_array = (char **) malloc(sizeof(char *) * res_all.res_fs.exclude_size);
		     } else {
			res_all.res_fs.exclude_array = (char **) realloc(res_all.res_fs.exclude_array,
			   sizeof(char *) * res_all.res_fs.exclude_size);
		     }
		  }
		  res_all.res_fs.exclude_array[res_all.res_fs.num_excludes++] =    
		     fname;
	       }
	       break;
	    default:
               scan_err1(lc, "Expected a filename, got: %s", lc->str);
	 }				   
      }
   } else { /* pass 2 */
      while (lex_get_token(lc) != T_EOB) 
	 {}
   }
   scan_to_eol(lc);
   lc->options = options;
   set_bit(index, res_all.hdr.item_present);
}
