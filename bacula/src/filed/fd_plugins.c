/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation, which is 
   listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * Main program to test loading and running Bacula plugins.
 *   Destined to become Bacula pluginloader, ...
 *
 * Kern Sibbald, October 2007
 */
#include "bacula.h"
#include "filed.h"

const int dbglvl = 50;
const char *plugin_type = "-fd.so";

extern int save_file(JCR *jcr, FF_PKT *ff_pkt, bool top_level);

/* Function pointers to be set here */
extern DLL_IMP_EXP int     (*plugin_bopen)(JCR *jcr, const char *fname, int flags, mode_t mode);
extern DLL_IMP_EXP int     (*plugin_bclose)(JCR *jcr);
extern DLL_IMP_EXP ssize_t (*plugin_bread)(JCR *jcr, void *buf, size_t count);
extern DLL_IMP_EXP ssize_t (*plugin_bwrite)(JCR *jcr, void *buf, size_t count);
extern DLL_IMP_EXP boffset_t (*plugin_blseek)(JCR *jcr, boffset_t offset, int whence);


/* Forward referenced functions */
static bRC baculaGetValue(bpContext *ctx, bVariable var, void *value);
static bRC baculaSetValue(bpContext *ctx, bVariable var, void *value);
static bRC baculaRegisterEvents(bpContext *ctx, ...);
static bRC baculaJobMsg(bpContext *ctx, const char *file, int line,
  int type, time_t mtime, const char *fmt, ...);
static bRC baculaDebugMsg(bpContext *ctx, const char *file, int line,
  int level, const char *fmt, ...);

static int     my_plugin_bopen(JCR *jcr, const char *fname, int flags, mode_t mode);
static int     my_plugin_bclose(JCR *jcr);
static ssize_t my_plugin_bread(JCR *jcr, void *buf, size_t count);
static ssize_t my_plugin_bwrite(JCR *jcr, void *buf, size_t count);
static boffset_t my_plugin_blseek(JCR *jcr, boffset_t offset, int whence);


/* Bacula info */
static bInfo binfo = {
   sizeof(bFuncs),
   FD_PLUGIN_INTERFACE_VERSION 
};

/* Bacula entry points */
static bFuncs bfuncs = {
   sizeof(bFuncs),
   FD_PLUGIN_INTERFACE_VERSION,
   baculaRegisterEvents,
   baculaGetValue,
   baculaSetValue,
   baculaJobMsg,
   baculaDebugMsg
};


/*
 * Create a plugin event 
 */
void generate_plugin_event(JCR *jcr, bEventType eventType, void *value)     
{
   bEvent event;
   Plugin *plugin;
   int i = 0;

   if (!plugin_list) {
      return;                         /* Return if no plugins loaded */
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   event.eventType = eventType;

   Dmsg2(dbglvl, "plugin_ctx=%p JobId=%d\n", jcr->plugin_ctx_list, jcr->JobId);

   /* Pass event to every plugin */
   foreach_alist(plugin, plugin_list) {
      bRC rc;
      rc = plug_func(plugin)->handlePluginEvent(&plugin_ctx_list[i++], &event, value);
      if (rc != bRC_OK) {
         break;
      }
   }

   return;
}

/*   
 * Sequence of calls for a backup:
 * 1. plugin_save() here is called with ff_pkt
 * 2. we find the plugin requested on the command string
 * 3. we generate a bEventBackupCommand event to the specified plugin
 *    and pass it the command string.
 * 4. we make a startPluginBackup call to the plugin, which gives
 *    us the data we need in save_pkt
 * 5. we call Bacula's save_file() subroutine to save the specified
 *    file.  The plugin will be called at pluginIO() to supply the
 *    file data.
 *
 * Sequence of calls for restore:
 *   See subroutine plugin_name_stream() below.
 */
int plugin_save(JCR *jcr, FF_PKT *ff_pkt, bool top_level)
{
   Plugin *plugin;
   int i = 0;
   int len;
   char *p;
   char *cmd = ff_pkt->top_fname;
   struct save_pkt sp;
   bEvent event;

   if (!plugin_list) {
      return 1;                            /* Return if no plugins loaded */
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   event.eventType = bEventBackupCommand;

   /* Handle plugin command here backup */
   Dmsg1(100, "plugin cmd=%s\n", cmd);
   if (!(p = strchr(cmd, ':'))) {
      Jmsg1(jcr, M_ERROR, 0, "Malformed plugin command: %s\n", cmd);
      goto bail_out;
   }
   len = p - cmd;
   if (len <= 0) {
      goto bail_out;
   }

   foreach_alist(plugin, plugin_list) {
      Dmsg3(100, "plugin=%s cmd=%s len=%d\n", plugin->file, cmd, len);
      if (strncmp(plugin->file, cmd, len) != 0) {
         i++;
         continue;
      }
      Dmsg1(100, "Command plugin = %s\n", cmd);
      /* Send the backup command */
      if (plug_func(plugin)->handlePluginEvent(&plugin_ctx_list[i], &event, cmd) != bRC_OK) {
         goto bail_out;
      }
      /* Loop getting filenames to backup then saving them */
      while (!job_canceled(jcr)) { 
         memset(&sp, 0, sizeof(sp));
         sp.type = FT_REG;
         sp.portable = true;
         sp.cmd = cmd;
         Dmsg3(000, "startBackup st_size=%p st_blocks=%p sp=%p\n", &sp.statp.st_size, &sp.statp.st_blocks,
                &sp);
         /* Get the file save parameters */
         if (plug_func(plugin)->startBackupFile(&plugin_ctx_list[i], &sp) != bRC_OK) {
            goto bail_out;
         }
         jcr->plugin_ctx = &plugin_ctx_list[i];
         jcr->plugin = plugin;
         jcr->plugin_sp = &sp;
         ff_pkt = jcr->ff;
         ff_pkt->fname = sp.fname;
         ff_pkt->type = sp.type;
         memcpy(&ff_pkt->statp, &sp.statp, sizeof(ff_pkt->statp));
         Dmsg1(000, "Save_file: file=%s\n", ff_pkt->fname);
         save_file(jcr, ff_pkt, true);
         if (plug_func(plugin)->endBackupFile(&plugin_ctx_list[i]) != bRC_More) {
            goto bail_out;
         }
      }
   }
   Jmsg1(jcr, M_ERROR, 0, "Command plugin \"%s\" not found.\n", cmd);

bail_out:
   return 1;
}

/* 
 * Send plugin name start/end record to SD
 */
bool send_plugin_name(JCR *jcr, BSOCK *sd, bool start)
{
   int stat;
   struct save_pkt *sp = (struct save_pkt *)jcr->plugin_sp;
  
   Dmsg1(000, "send_plugin_name=%s\n", sp->cmd);
   if (!sd->fsend("%ld %d 0", jcr->JobFiles+1, STREAM_PLUGIN_NAME)) {
     Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
           sd->bstrerror());
     return false;
   }
   Dmsg1(000, "send: %s\n", sd->msg);

   if (start) {
      stat = sd->fsend("%ld 1 %d %s%c", jcr->JobFiles+1, sp->portable, sp->cmd, 0);
   } else {
      stat = sd->fsend("%ld 0");
   }
   if (!stat) {
      Jmsg1(jcr, M_FATAL, 0, _("Network send error to SD. ERR=%s\n"),
            sd->bstrerror());
         return false;
   }
   Dmsg1(000, "send: %s\n", sd->msg);
   sd->signal(BNET_EOD);            /* indicate end of plugin name data */
   return true;
}

/*
 * Plugin name stream found during restore.  The record passed in
 *  argument name was generated in send_plugin_name() above.
 */
void plugin_name_stream(JCR *jcr, char *name)    
{
   char *p = name;
   char *cmd;
   bool start, portable;
   Plugin *plugin;
   int len;
   int i = 0;
   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;

   Dmsg1(100, "Read plugin stream string=%s\n", name);
   skip_nonspaces(&p);             /* skip over jcr->JobFiles */
   skip_spaces(&p);
   start = *p == '1';
   skip_nonspaces(&p);             /* skip start/end flag */
   skip_spaces(&p);
   portable = *p == '1';
   skip_nonspaces(&p);             /* skip portable flag */
   skip_spaces(&p);
   cmd = p;
    
   /* Check for restore end */
   if (!start) {
      /*
       * If end of restore, notify plugin, then clear flags   
       */
      plugin = (Plugin *)jcr->plugin;
      plug_func(plugin)->endRestoreFile(&plugin_ctx_list[i]);
      jcr->plugin_ctx = NULL;
      jcr->plugin = NULL;
      goto bail_out;
   }
      
   /*
    * After this point, we are dealing with a restore start
    */

   Dmsg1(100, "plugin restore cmd=%s\n", cmd);
   if (!(p = strchr(cmd, ':'))) {
      Jmsg1(jcr, M_ERROR, 0, "Malformed plugin command: %s\n", cmd);
      goto bail_out;
   }
   len = p - cmd;
   if (len <= 0) {
      goto bail_out;
   }

   /*
    * Search for correct plugin as specified on the command 
    */
   foreach_alist(plugin, plugin_list) {
      bEvent event;
      Dmsg3(100, "plugin=%s cmd=%s len=%d\n", plugin->file, cmd, len);
      if (strncmp(plugin->file, cmd, len) != 0) {
         i++;
         continue;
      }
      Dmsg1(100, "Restore Command plugin = %s\n", cmd);
      event.eventType = bEventRestoreCommand;     
      if (plug_func(plugin)->handlePluginEvent(&plugin_ctx_list[i], 
            &event, cmd) != bRC_OK) {
         goto bail_out;
      }
      jcr->plugin_ctx = &plugin_ctx_list[i];
      jcr->plugin = plugin;
      goto bail_out;
   }
bail_out:
   return;
}

/*
 * Tell the plugin to create the file.  Return values are
 *
 *  CF_ERROR    -- error
 *  CF_SKIP     -- skip processing this file
 *  CF_EXTRACT  -- extract the file (i.e.call i/o routines)
 *  CF_CREATED  -- created, but no content to extract (typically directories)
 *
 */
int plugin_create_file(JCR *jcr, ATTR *attr, BFILE *bfd, int replace)
{
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   Plugin *plugin = (Plugin *)jcr->plugin;
   struct restore_pkt rp;
   struct io_pkt io;

   if (!set_cmd_plugin(bfd, jcr)) {
      return CF_ERROR;
   }
   rp.stream = attr->stream;
   rp.data_stream = attr->data_stream;
   rp.type = attr->type;
   rp.file_index = attr->file_index;
   rp.LinkFI = attr->LinkFI;
   rp.uid = attr->uid;
   rp.statp = attr->statp;                /* structure assignment */
   rp.attrEx = attr->attrEx;
   rp.ofname = attr->ofname;
   rp.olname = attr->olname;
   rp.where = jcr->where;
   rp.RegexWhere = jcr->RegexWhere;
   rp.replace = jcr->replace;
   if (plug_func(plugin)->createFile(plugin_ctx, &rp) != bRC_OK) {
      return CF_ERROR;
   }
   io.func = IO_OPEN;
   io.count = 0;
   io.buf = NULL;
   io.mode = 0777 & attr->statp.st_mode;
   io.flags = O_WRONLY;
   if (plug_func(plugin)->pluginIO(plugin_ctx, &io) != bRC_OK) {
      return CF_ERROR;
   }
   return CF_EXTRACT;
}

/*
 * Reset the file attributes after all file I/O is done -- this allows
 *  the previous access time/dates to be set properly, and it also allows
 *  us to properly set directory permissions.
 */
bool plugin_set_attributes(JCR *jcr, ATTR *attr, BFILE *ofd)
{
   return true;
}

void load_fd_plugins(const char *plugin_dir)
{
   if (!plugin_dir) {
      return;
   }

   plugin_list = New(alist(10, not_owned_by_alist));
   load_plugins((void *)&binfo, (void *)&bfuncs, plugin_dir, plugin_type);

   /* Plug entry points called from findlib */
   plugin_bopen  = my_plugin_bopen;
   plugin_bclose = my_plugin_bclose;
   plugin_bread  = my_plugin_bread;
   plugin_bwrite = my_plugin_bwrite;
   plugin_blseek = my_plugin_blseek;

}

/*
 * Create a new instance of each plugin for this Job
 */
void new_plugins(JCR *jcr)
{
   Plugin *plugin;
   int i = 0;

   if (!plugin_list) {
      return;
   }

   int num = plugin_list->size();

   if (num == 0) {
      return;
   }

   jcr->plugin_ctx_list = (void *)malloc(sizeof(bpContext) * num);

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   Dmsg2(dbglvl, "Instantiate plugin_ctx=%p JobId=%d\n", plugin_ctx_list, jcr->JobId);
   foreach_alist(plugin, plugin_list) {
      /* Start a new instance of each plugin */
      plugin_ctx_list[i].bContext = (void *)jcr;
      plugin_ctx_list[i].pContext = NULL;
      plug_func(plugin)->newPlugin(&plugin_ctx_list[i++]);
   }
}

/*
 * Free the plugin instances for this Job
 */
void free_plugins(JCR *jcr)
{
   Plugin *plugin;
   int i = 0;

   if (!plugin_list) {
      return;
   }

   bpContext *plugin_ctx_list = (bpContext *)jcr->plugin_ctx_list;
   Dmsg2(dbglvl, "Free instance plugin_ctx=%p JobId=%d\n", plugin_ctx_list, jcr->JobId);
   foreach_alist(plugin, plugin_list) {
      /* Free the plugin instance */
      plug_func(plugin)->freePlugin(&plugin_ctx_list[i++]);
   }
   free(plugin_ctx_list);
   jcr->plugin_ctx_list = NULL;
}

static int my_plugin_bopen(JCR *jcr, const char *fname, int flags, mode_t mode)
{
   Plugin *plugin = (Plugin *)jcr->plugin;
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   struct io_pkt io;
   Dmsg0(000, "plugin_bopen\n");
   io.func = IO_OPEN;
   io.count = 0;
   io.buf = NULL;
   io.mode = mode;
   io.flags = flags;
   plug_func(plugin)->pluginIO(plugin_ctx, &io);
   return io.status;
}

static int my_plugin_bclose(JCR *jcr)
{
   Plugin *plugin = (Plugin *)jcr->plugin;
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   struct io_pkt io;
   Dmsg0(000, "plugin_bclose\n");
   io.func = IO_CLOSE;
   io.count = 0;
   io.buf = NULL;
   plug_func(plugin)->pluginIO(plugin_ctx, &io);
   return io.status;
}

static ssize_t my_plugin_bread(JCR *jcr, void *buf, size_t count)
{
   Plugin *plugin = (Plugin *)jcr->plugin;
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   struct io_pkt io;
   Dmsg0(000, "plugin_bread\n");
   io.func = IO_READ;
   io.count = count;
   io.buf = (char *)buf;
   plug_func(plugin)->pluginIO(plugin_ctx, &io);
   return (ssize_t)io.status;
}

static ssize_t my_plugin_bwrite(JCR *jcr, void *buf, size_t count)
{
   Plugin *plugin = (Plugin *)jcr->plugin;
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   struct io_pkt io;
   Dmsg0(000, "plugin_bwrite\n");
   io.func = IO_WRITE;
   io.count = count;
   io.buf = (char *)buf;
   plug_func(plugin)->pluginIO(plugin_ctx, &io);
   return (ssize_t)io.status;
}

static boffset_t my_plugin_blseek(JCR *jcr, boffset_t offset, int whence)
{
   Plugin *plugin = (Plugin *)jcr->plugin;
   bpContext *plugin_ctx = (bpContext *)jcr->plugin_ctx;
   struct io_pkt io;
   Dmsg0(000, "plugin_bseek\n");
   io.func = IO_SEEK;
   io.offset = offset;
   io.whence = whence;
   plug_func(plugin)->pluginIO(plugin_ctx, &io);
   return (boffset_t)io.offset;
}

/* ==============================================================
 *
 * Callbacks from the plugin
 *
 * ==============================================================
 */
static bRC baculaGetValue(bpContext *ctx, bVariable var, void *value)
{
   JCR *jcr = (JCR *)(ctx->bContext);
// Dmsg1(dbglvl, "bacula: baculaGetValue var=%d\n", var);
   if (!value) {
      return bRC_Error;
   }
// Dmsg1(dbglvl, "Bacula: jcr=%p\n", jcr); 
   switch (var) {
   case bVarJobId:
      *((int *)value) = jcr->JobId;
      Dmsg1(dbglvl, "Bacula: return bVarJobId=%d\n", jcr->JobId);
      break;
   case bVarFDName:
      *((char **)value) = my_name;
      Dmsg1(dbglvl, "Bacula: return my_name=%s\n", my_name);
      break;
   case bVarLevel:
   case bVarType:
   case bVarClient:
   case bVarJobName:
   case bVarJobStatus:
   case bVarSinceTime:
      break;
   }
   return bRC_OK;
}

static bRC baculaSetValue(bpContext *ctx, bVariable var, void *value)
{
   Dmsg1(dbglvl, "bacula: baculaSetValue var=%d\n", var);
   return bRC_OK;
}

static bRC baculaRegisterEvents(bpContext *ctx, ...)
{
   va_list args;
   uint32_t event;

   va_start(args, ctx);
   while ((event = va_arg(args, uint32_t))) {
      Dmsg1(dbglvl, "Plugin wants event=%u\n", event);
   }
   va_end(args);
   return bRC_OK;
}

static bRC baculaJobMsg(bpContext *ctx, const char *file, int line,
  int type, time_t mtime, const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[2000];
   JCR *jcr = (JCR *)(ctx->bContext);

   va_start(arg_ptr, fmt);
   bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   Jmsg(jcr, type, mtime, "%s", buf);
   return bRC_OK;
}

static bRC baculaDebugMsg(bpContext *ctx, const char *file, int line,
  int level, const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[2000];

   va_start(arg_ptr, fmt);
   bvsnprintf(buf, sizeof(buf), fmt, arg_ptr);
   va_end(arg_ptr);
   d_msg(file, line, level, "%s", buf);
   return bRC_OK;
}

#ifdef TEST_PROGRAM

int     (*plugin_bopen)(JCR *jcr, const char *fname, int flags, mode_t mode) = NULL;
int     (*plugin_bclose)(JCR *jcr) = NULL;
ssize_t (*plugin_bread)(JCR *jcr, void *buf, size_t count) = NULL;
ssize_t (*plugin_bwrite)(JCR *jcr, void *buf, size_t count) = NULL;
boffset_t (*plugin_blseek)(JCR *jcr, boffset_t offset, int whence) = NULL;

int save_file(JCR *jcr, FF_PKT *ff_pkt, bool top_level)
{
   return 0;
}

bool set_cmd_plugin(BFILE *bfd, JCR *jcr)
{
   return true;
}

int main(int argc, char *argv[])
{
   char plugin_dir[1000];
   JCR mjcr1, mjcr2;
   JCR *jcr1 = &mjcr1;
   JCR *jcr2 = &mjcr2;

   strcpy(my_name, "test-fd");
    
   getcwd(plugin_dir, sizeof(plugin_dir)-1);
   load_fd_plugins(plugin_dir);

   jcr1->JobId = 111;
   new_plugins(jcr1);

   jcr2->JobId = 222;
   new_plugins(jcr2);

   generate_plugin_event(jcr1, bEventJobStart, (void *)"Start Job 1");
   generate_plugin_event(jcr1, bEventJobEnd);
   generate_plugin_event(jcr2, bEventJobStart, (void *)"Start Job 2");
   free_plugins(jcr1);
   generate_plugin_event(jcr2, bEventJobEnd);
   free_plugins(jcr2);

   unload_plugins();

   Dmsg0(dbglvl, "bacula: OK ...\n");
   close_memory_pool();
   sm_dump(false);
   return 0;
}

#endif /* TEST_PROGRAM */