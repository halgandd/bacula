/*
 * Second generation Storage daemon.
 *
 * It accepts a number of simple commands from the File daemon
 * and acts on them. When a request to append data is made,
 * it opens a data channel and accepts data from the
 * File daemon. 
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

#include "bacula.h"
#include "stored.h"

/* Imported functions */


/* Forward referenced functions */
void terminate_stored(int sig);
static void check_config();
static void *director_thread(void *arg);

#define CONFIG_FILE "bacula-sd.conf"  /* Default config file */


/* Global variables exported */


struct s_shm *shm;		      /* memory shared with children */
BSHM bshm;			      /* shared memory control packet */


/* This is our own global resource */
static STORES *me;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t VolSessionId = 0;
uint32_t VolSessionTime;

static char *configfile;
static int foreground = 0;

static workq_t dird_workq;	      /* queue for processing connections */
static workq_t filed_workq;	      /* queue for processing connections */


static void usage()
{
   fprintf(stderr, _(
"\nVersion: " VERSION " (" DATE ")\n\n"
"Usage: stored [-s -f ] [-c config_file] [-d debug_level]  [config_file]\n"
"        -c <file>   use <file> as configuration file\n"
"        -dnn        set debug level to nn\n"
"        -f          run in foreground (for debugging)\n"
"        -s          no signals (for debugging)\n"
"        -t          test - read config and exit\n"
"        -?          print this message.\n"
"\n"));
   exit(1);
}

/********************************************************************* 
 *
 *  Main Bacula Unix Storage Daemon
 *
 */
int main (int argc, char *argv[])
{
   int ch, i;
   int no_signals = FALSE;
   int test_config = FALSE;
   DEVRES *device;
   pthread_t dirid;
   int status;

   init_stack_dump();
   my_name_is(argc, argv, "stored");
   daemon_start_time = time(NULL);
   memset(&last_job, 0, sizeof(last_job));

   /* Sanity checks */
   if (TAPE_BSIZE % DEV_BSIZE != 0 || TAPE_BSIZE / DEV_BSIZE == 0) {
      Emsg2(M_ABORT, 0, "Tape block size (%d) not multiple of system size (%d)\n",
	 TAPE_BSIZE, DEV_BSIZE);
   }
   if (TAPE_BSIZE != (1 << (ffs(TAPE_BSIZE)-1))) {
      Emsg1(M_ABORT, 0, "Tape block size (%d) is not a power of 2\n", TAPE_BSIZE);
   }

   while ((ch = getopt(argc, argv, "c:d:fst?")) != -1) {
      switch (ch) {
         case 'c':                    /* configuration file */
	    if (configfile != NULL) {
	       free(configfile);
	    }
	    configfile = bstrdup(optarg);
	    break;

         case 'd':                    /* debug level */
	    debug_level = atoi(optarg);
	    if (debug_level <= 0) {
	       debug_level = 1; 
	    }
	    break;

         case 'f':                    /* run in foreground */
	    foreground = TRUE;
	    break;

         case 's':                    /* no signals */
	    no_signals = TRUE;
	    break;

         case 't':
	    test_config = TRUE;
	    break;

         case '?':
	 default:
	    usage();

      }  
   }
   argc -= optind;
   argv += optind;

   if (argc) {
      if (configfile != NULL) {
	 free(configfile);
      }
      configfile = bstrdup(*argv);
      argc--; 
      argv++;
   }
   if (argc)
      usage();

   if (!no_signals) {
      init_signals(terminate_stored);
   }


   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   init_msg(NULL, NULL);
   parse_config(configfile);
   check_config();

   bshm.size = 0;
   if (test_config) {
      terminate_stored(0);
   }

   if (!foreground) {
      daemon_start();		      /* become daemon */
      init_stack_dump();	      /* pick up new pid */
   }

   /*  ****FIXME**** clean this up */
   /* Create and attach to shared memory. This is a
    * hold over from the days of child processes. 
    * Note, in reality all memory is shared. This
    * is just a global buffer for the device packets.
    */
   shm = (s_shm *) malloc(sizeof(struct s_shm));
   /* Zero shared memory */
   memset(shm, 0, sizeof(struct s_shm));

   /* Ensure that Volume Session Time and Id are both
    * set and are both non-zero.
    */
   VolSessionTime = (long)daemon_start_time;
   if (VolSessionTime == 0) { /* paranoid */
      Emsg0(M_ABORT, 0, _("Volume Session Time is ZERO!\n"));
   }

   LockRes();
   for (device=NULL,i=0;  (device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device)); i++) {
      if (i >= MAX_DEVICES) {
	 UnlockRes();
         Emsg1(M_ABORT, 0, _("Too many Device Resources. Max=%d\n"), MAX_DEVICES);
      }
      Dmsg1(90, "calling init_dev %s\n", device->device_name);
      device->dev = init_dev(&shm->dev[i], device->device_name);
      /* Copy some attributes from the Device Resource to the DEV structure */
      if (device->dev) {
	 device->dev->capabilities = device->cap_bits;
	 device->dev->min_block_size = device->min_block_size;
	 device->dev->max_block_size = device->max_block_size;
	 device->dev->max_volume_jobs = device->max_volume_jobs;
	 device->dev->max_volume_files = device->max_volume_files;
	 device->dev->max_volume_size = device->max_volume_size;
	 device->dev->max_file_size = device->max_file_size;
	 device->dev->volume_capacity = device->volume_capacity;
	 device->dev->max_rewind_wait = device->max_rewind_wait;
	 device->dev->device = device;
      }
      Dmsg1(10, "Init done %s\n", device->device_name);
      if (!device->dev) {
         Emsg1(M_ERROR, 0, _("Could not initialize %s\n"), device->device_name);
      }
      if (device->cap_bits & CAP_ALWAYSOPEN) {
         Dmsg1(20, "calling open_device %s\n", device->device_name);
	 if (!open_device(device->dev)) {
            Emsg1(M_ERROR, 0, _("Could not open device %s\n"), device->device_name);
	 }
      }
      if (device->cap_bits & CAP_AUTOMOUNT && device->dev && 
	  device->dev->state & ST_OPENED) {
	 DEV_BLOCK *block;
	 JCR *jcr;
	 block = new_block(device->dev);
	 jcr = new_jcr(sizeof(JCR), stored_free_jcr);
	 switch (read_dev_volume_label(jcr, device->dev, block)) {
	    case VOL_OK:
	       break;
	    default:
               Emsg1(M_WARNING, 0, _("Could not mount device %s\n"), device->device_name);
	       break;
	 }
	 free_jcr(jcr);
	 free_block(block);
      }
   } 
   UnlockRes();
   device = NULL;

   start_watchdog();		      /* start watchdog thread */

   /*
    * Here we support either listening on one port or on two ports
    */
   if (me->SDDport == 0 || me->SDDport == me->SDport) {
      /* Single server used for Director and File daemon */
      bnet_thread_server(me->SDport, 20, &dird_workq, connection_request);
   } else {
      /* Start the Director server */
      set_thread_concurrency(10);
      if ((status=pthread_create(&dirid, NULL, director_thread, 	
	   (void *)me->SDport)) != 0) {
         Emsg1(M_ABORT, 0, _("Cannot create Director thread: %s\n"), strerror(status));
      }
      /* Start File daemon server */
      bnet_thread_server(me->SDDport, 10, &filed_workq, connection_from_filed);
      /* never returns */
   }

   exit(1);			      /* to keep compiler quiet */
}

static void *director_thread(void *arg)
{
   int dir_port = (int)arg;
   pthread_detach(pthread_self());
   bnet_thread_server(dir_port, 10, &dird_workq, connection_request);
   return NULL;
}

/* Return a new Session Id */
uint32_t newVolSessionId()
{
   uint32_t Id;

   P(mutex);
   VolSessionId++;
   Id = VolSessionId;
   V(mutex);
   return Id;
}

/* Check Configuration file for necessary info */
static void check_config()
{
   struct stat stat_buf; 

   LockRes();
   me = (STORES *)GetNextRes(R_STORAGE, NULL);
   if (!me) {
      UnlockRes();
      Emsg1(M_ABORT, 0, _("No Storage resource defined in %s. Cannot continue.\n"),
	 configfile);
   }

   my_name_is(0, (char **)NULL, me->hdr.name);	   /* Set our real name */

   if (GetNextRes(R_STORAGE, (RES *)me) != NULL) {
      UnlockRes();
      Emsg1(M_ABORT, 0, _("Only one Storage resource permitted in %s\n"), 
	 configfile);
   }
   if (GetNextRes(R_DIRECTOR, NULL) == NULL) {
      UnlockRes();
      Emsg1(M_ABORT, 0, _("No Director resource defined in %s. Cannot continue.\n"),
	 configfile);
   }
   if (GetNextRes(R_DEVICE, NULL) == NULL){
      UnlockRes();
      Emsg1(M_ABORT, 0, _("No Device resource defined in %s. Cannot continue.\n"),
	   configfile);
   }

   UnlockRes();

   if (!me->working_directory) {
      Emsg1(M_ABORT, 0, _("No Working Directory defined in %s. Cannot continue.\n"),
	 configfile);
   }
   if (stat(me->working_directory, &stat_buf) != 0) {
      Emsg1(M_ABORT, 0, _("Working Directory: %s not found. Cannot continue.\n"),
	 me->working_directory);
   }
   if (!S_ISDIR(stat_buf.st_mode)) {
      Emsg1(M_ABORT, 0, _("Working Directory: %s is not a directory. Cannot continue.\n"),
	 me->working_directory);
   }
   working_directory = me->working_directory;
}

/* Clean up and then exit */
void terminate_stored(int sig)
{
   static int in_here = FALSE;
   DEVRES *device;

   if (in_here) {		      /* prevent loops */
      exit(1);
   }
   in_here = TRUE;

   stop_watchdog();

   Dmsg0(200, "In terminate_stored()\n");

   LockRes();
   for (device=NULL; (device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device)); ) {
      if (device->dev) {
	 term_dev(device->dev);
      }
   } 
   UnlockRes();

   if (configfile)
   free(configfile);
   free_config_resources();

   if (debug_level > 10)
      print_memory_pool_stats();
   close_memory_pool();

   if (shm)
   free(shm);
   term_msg();

   sm_dump(False);		      /* dump orphaned buffers */
   exit(1);
}
