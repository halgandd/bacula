/*
 * Prototypes for lib directory of Bacula
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

/* base64.c */
void      base64_init            __PROTO((void));
int       to_base64              __PROTO((intmax_t value, char *where));
int       from_base64            __PROTO((intmax_t *value, char *where));
void      encode_stat            __PROTO((char *buf, struct stat *statp));
void      decode_stat            __PROTO((char *buf, struct stat *statp));
int       bin_to_base64          __PROTO((char *buf, char *bin, int len));

/* bmisc.c */
void     *b_malloc               (char *file, int line, size_t size);
#ifndef DEBUG
void     *bmalloc                (size_t size);
#endif
void     *brealloc               (void *buf, size_t size);
void     *bcalloc                (size_t size1, size_t size2);
int       bsnprintf              (char *str, size_t size, const  char  *format, ...);
int       bvsnprintf             (char *str, size_t size, const char  *format, va_list ap);
int       pool_sprintf           (char *pool_buf, char *fmt, ...);
int       create_pid_file        (char *dir, char *progname, int port, char *errmsg);
int       delete_pid_file        (char *dir, char *progname, int port);

/* bnet.c */
int32_t    bnet_recv             __PROTO((BSOCK *bsock));
int        bnet_send             __PROTO((BSOCK *bsock));
int        bnet_fsend              (BSOCK *bs, char *fmt, ...);
int        bnet_set_buffer_size    (BSOCK *bs, uint32_t size, int rw);
int        bnet_sig                (BSOCK *bs, int sig);
BSOCK *    bnet_connect            (void *jcr, int retry_interval,
               int max_retry_time, char *name, char *host, char *service, 
               int port, int verbose);
int        bnet_wait_data         (BSOCK *bsock, int sec);
void       bnet_close            __PROTO((BSOCK *bsock));
BSOCK *    init_bsock            __PROTO((int sockfd, char *who, char *ip, int port));
BSOCK *    dup_bsock             __PROTO((BSOCK *bsock));
void       term_bsock            __PROTO((BSOCK *bsock));
char *     bnet_strerror         __PROTO((BSOCK *bsock));
char *     bnet_sig_to_ascii     __PROTO((BSOCK *bsock));
int        bnet_wait_data        __PROTO((BSOCK *bsock, int sec));


/* cram-md5.c */
int cram_md5_get_auth(BSOCK *bs, char *password);
int cram_md5_auth(BSOCK *bs, char *password);
void hmac_md5(uint8_t* text, int text_len, uint8_t*  key,
              int key_len, uint8_t *hmac);

/* create_file.c */
int create_file(void *jcr, char *fname, char *ofile, char *lname,
                       int type, struct stat *statp, int *ofd);
int set_statp(void *jcr, char *fname, char *ofile, char *lname, int type, 
                       struct stat *statp);


/* crc32.c */
uint32_t bcrc32(uint8_t *buf, int len);

/* daemon.c */
void     daemon_start            __PROTO(());

/* lex.c */
LEX *     lex_close_file         __PROTO((LEX *lf));
LEX *     lex_open_file          __PROTO((LEX *lf, char *fname));
int       lex_get_char           __PROTO((LEX *lf));
void      lex_unget_char         __PROTO((LEX *lf));
char *    lex_tok_to_str         __PROTO((int token));
int       lex_get_token          __PROTO((LEX *lf));

/* makepath.c */
int make_path(
           void *jcr,
           const char *argpath,
           int mode,
           int parent_mode,
           uid_t owner,
           gid_t group,
           int preserve_existing,
           char *verbose_fmt_string);


/* message.c */
void       my_name_is            __PROTO((int argc, char *argv[], char *name));
void       init_msg              __PROTO((void *jcr, MSGS *msg));
void       term_msg              __PROTO((void));
void       close_msg             __PROTO((void *jcr));
void       add_msg_dest          __PROTO((MSGS *msg, int dest, int type, char *where, char *dest_code));
void       rem_msg_dest          __PROTO((MSGS *msg, int dest, int type, char *where));
void       Jmsg                  (void *jcr, int type, int level, char *fmt, ...);
void       dispatch_message      __PROTO((void *jcr, int type, int level, char *buf));
void       init_console_msg      __PROTO((char *wd));


/* bnet_server.c */
void       bnet_thread_server(int port, int max_clients, workq_t *client_wq, 
                   void handle_client_request(void *bsock));
void             bnet_server             __PROTO((int port, void handle_client_request(BSOCK *bsock)));
int              net_connect             __PROTO((int port));
BSOCK *          bnet_bind               __PROTO((int port));
BSOCK *          bnet_accept             __PROTO((BSOCK *bsock, char *who));

/* signal.c */
void             init_signals             __PROTO((void terminate(int sig)));
void             init_stack_dump          (void);

/* util.c */
void             lcase                   __PROTO((char *str));
void             bash_spaces             __PROTO((char *str));
void             unbash_spaces           __PROTO((char *str));
void             strip_trailing_junk     __PROTO((char *str));
void             strip_trailing_slashes  __PROTO((char *dir));
int              skip_spaces             __PROTO((char **msg));
int              skip_nonspaces          __PROTO((char **msg));
int              fstrsch                 __PROTO((char *a, char *b));
char *           encode_time             __PROTO((time_t time, char *buf));
char *           encode_mode             __PROTO((mode_t mode, char *buf));
char *           edit_uint64_with_commas   __PROTO((uint64_t val, char *buf));
char *           add_commas              __PROTO((char *val, char *buf));
char *           edit_uint64             (uint64_t val, char *buf);
int              do_shell_expansion      (char *name);
int              is_a_number             (const char *num);
int              string_to_btime(char *str, btime_t *value);
char             *edit_btime(btime_t val, char *buf);


/*
 *void           print_ls_output         __PROTO((char *fname, char *lname, int type, struct stat *statp));
 */

/* watchdog.c */
int start_watchdog(void);
int stop_watchdog(void);
