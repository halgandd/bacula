/*
 * Prototypes for finlib directory of Bacula
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

/* from attribs.c */
void	  encode_stat		 (char *buf, struct stat *statp, uint32_t LinkFI);
void	  decode_stat		 (char *buf, struct stat *statp, uint32_t *LinkFI);
int	  encode_attribsEx	 (void *jcr, char *attribsEx, FF_PKT *ff_pkt);
int set_attributes(void *jcr, char *fname, char *ofile, char *lname,
		   int type, int stream, struct stat *statp, 
		   char *attribsEx, BFILE *ofd);

/* from create_file.c */
int create_file(void *jcr, char *fname, char *ofile, char *lname,
		int type, int stream, struct stat *statp, 
		char *attribsEx, BFILE *ofd, int replace, int win_io);

/* From find.c */
FF_PKT *init_find_files();
void set_find_options(FF_PKT *ff, int incremental, time_t mtime);
int find_files(JCR *jcr, FF_PKT *ff, int sub(FF_PKT *ff_pkt, void *hpkt), void *pkt);
int term_find_files(FF_PKT *ff);

/* From match.c */
void init_include_exclude_files(FF_PKT *ff);
void term_include_exclude_files(FF_PKT *ff);
void add_fname_to_include_list(FF_PKT *ff, int prefixed, char *fname);
void add_fname_to_exclude_list(FF_PKT *ff, char *fname);
int file_is_excluded(FF_PKT *ff, char *file);
int file_is_included(FF_PKT *ff, char *file);
struct s_included_file *get_next_included_file(FF_PKT *ff, 
			   struct s_included_file *inc);

/* From find_one.c */
int find_one_file(JCR *jcr, FF_PKT *ff, int handle_file(FF_PKT *ff_pkt, void *hpkt), 
	       void *pkt, char *p, dev_t parent_device, int top_level);
int term_find_one(FF_PKT *ff);


/* From get_priv.c */
void get_backup_privileges(void *jcr, int ignore_errors);


/* from makepath.c */
int make_path(void *jcr, const char *argpath, int mode,
	   int parent_mode, uid_t owner, gid_t group,
	   int preserve_existing, char *verbose_fmt_string);

/* from file_io.c */
ssize_t  bread(BFILE *bfd, void *buf, size_t count);
int	 bopen(BFILE *bfd, const char *fname, int flags, mode_t mode);
int	 bclose(BFILE *bfd);
ssize_t  bread(BFILE *bfd, void *buf, size_t count);
ssize_t  bwrite(BFILE *bfd, void *buf, size_t count);
off_t	 blseek(BFILE *bfd, off_t offset, int whence);
int	 is_bopen(BFILE *bfd);
void	 binit(BFILE *bfd, int use_win_api);
char	*berror(BFILE *bfd);
