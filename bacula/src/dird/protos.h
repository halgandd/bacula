/*
 * Director external function prototypes
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

/* authenticate.c */
extern int authenticate_storage_daemon(JCR *jcr);
extern int authenticate_file_daemon(JCR *jcr);
extern int authenticate_user_agent(BSOCK *ua);

/* autoprune.c */
extern int do_autoprune(JCR *jcr);
extern int prune_volumes(JCR *jcr);
void create_ua_context(JCR *jcr, UAContext *ua);
void free_ua_context(UAContext *ua);

/* autorecycle.c */
extern int recycle_oldest_purged_volume(JCR *jcr, MEDIA_DBR *mr);
extern int find_recycled_volume(JCR *jcr, MEDIA_DBR *mr);


/* catreq.c */
extern void catalog_request(JCR *jcr, BSOCK *bs, char *buf);
extern void catalog_update(JCR *jcr, BSOCK *bs, char *buf);

/* dird_conf.c */
extern char *level_to_str(int level);

/* fd_cmds.c */
extern int connect_to_file_daemon(JCR *jcr, int retry_interval,
				  int max_retry_time, int verbose);
extern int send_include_list(JCR *jcr);
extern int send_exclude_list(JCR *jcr);
extern int get_attributes_and_put_in_catalog(JCR *jcr);
extern int get_attributes_and_compare_to_catalog(JCR *jcr, JobId_t JobId);
extern int put_file_into_catalog(JCR *jcr, long file_index, char *fname, 
			  char *link, char *attr, int stream);

/* job.c */
extern void set_jcr_defaults(JCR *jcr, JOB *job);
extern void create_unique_job_name(JCR *jcr, char *base_name);
extern void update_job_end_record(JCR *jcr);
extern int get_or_create_client_record(JCR *jcr);

/* mountreq.c */
extern void mount_request(JCR *jcr, BSOCK *bs, char *buf);

/* msgchan.c */
extern int connect_to_storage_daemon(JCR *jcr, int retry_interval,    
			      int max_retry_time, int verbose);
extern int start_storage_daemon_job(JCR *jcr);
extern int start_storage_daemon_message_thread(JCR *jcr);
extern int32_t bget_msg(BSOCK *bs, int type);
extern int response(BSOCK *fd, char *resp, char *cmd, int prtmsg);
extern void wait_for_storage_daemon_termination(JCR *jcr);

/* newvol.c */
int newVolume(JCR *jcr, MEDIA_DBR *mr);

/* ua_cmds.c */
int do_a_command(UAContext *ua, char *cmd);
int do_a_dot_command(UAContext *ua, char *cmd);
int qmessagescmd(UAContext *ua, char *cmd);
int open_db(UAContext *ua);
void close_db(UAContext *ua);
int create_pool(JCR *jcr, B_DB *db, POOL *pool, int create);
void set_pool_dbr_defaults_in_media_dbr(MEDIA_DBR *mr, POOL_DBR *pr);

/* ua_input.c */
int get_cmd(UAContext *ua, char *prompt);
int get_pint(UAContext *ua, char *prompt);
int get_yesno(UAContext *ua, char *prompt);
void parse_ua_args(UAContext *ua);

/* ua_output.c */
void prtit(void *ctx, char *msg);

/* ua_server.c */
void bsendmsg(void *sock, char *fmt, ...);

/* ua_select.c */
STORE	*select_storage_resource(UAContext *ua);
JOB	*select_job_resource(UAContext *ua);
JOB	*select_restore_job_resource(UAContext *ua);
CLIENT	*select_client_resource(UAContext *ua);
FILESET *select_fileset_resource(UAContext *ua);
int	select_pool_and_media_dbr(UAContext *ua, POOL_DBR *pr, MEDIA_DBR *mr);
int	select_media_dbr(UAContext *ua, MEDIA_DBR *mr);
int	select_pool_dbr(UAContext *ua, POOL_DBR *pr);
int	select_client_dbr(UAContext *ua, CLIENT_DBR *cr);

void	start_prompt(UAContext *ua, char *msg);
void	add_prompt(UAContext *ua, char *prompt);
int	do_prompt(UAContext *ua, char *msg, char *prompt, int max_prompt);
CAT    *get_catalog_resource(UAContext *ua);	       
STORE  *get_storage_resource(UAContext *ua, int use_default);
int	get_media_type(UAContext *ua, char *MediaType, int max_media);
int	get_pool_dbr(UAContext *ua, POOL_DBR *pr);
int	get_client_dbr(UAContext *ua, CLIENT_DBR *cr);
POOL   *get_pool_resource(UAContext *ua);
POOL   *select_pool_resource(UAContext *ua);
CLIENT *get_client_resource(UAContext *ua);
int	get_job_dbr(UAContext *ua, JOB_DBR *jr);

int find_arg_keyword(UAContext *ua, char **list);
int find_arg(UAContext *ua, char *keyword);
int find_arg_with_value(UAContext *ua, char *keyword);
int do_keyword_prompt(UAContext *ua, char *msg, char **list);
int confirm_retention(UAContext *ua, utime_t *ret, char *msg);

/* ua_prune.c */
int prune_files(UAContext *ua, CLIENT *client);
int prune_jobs(UAContext *ua, CLIENT *client, int JobType);
int prune_volume(UAContext *ua, POOL_DBR *pr, MEDIA_DBR *mr);

/* ua_purge.c */
int purge_jobs_from_volume(UAContext *ua, MEDIA_DBR *mr);
