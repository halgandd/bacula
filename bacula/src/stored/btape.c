/*
 *
 *   Bacula Tape manipulation program
 *
 *    Has various tape manipulation commands -- mostly for
 *    use in determining how tapes really work.
 *
 *     Kern Sibbald, April MM
 *
 *   Note, this program reads stored.conf, and will only
 *     talk to devices that are configured.
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

#include "bacula.h"
#include "stored.h"


/* External subroutines */
extern void free_config_resources();

/* Exported variables */
int quit = 0;
char buf[100000];
int bsize = TAPE_BSIZE;
char VolName[100];

DEVICE *dev = NULL;
DEVRES *device = NULL;

	    
/* Forward referenced subroutines */
static void do_tape_cmds();
static void helpcmd();
static void scancmd();
static void rewindcmd();
static void clearcmd();
static void wrcmd();
static void eodcmd();
static int find_device_res();


#define CONFIG_FILE "stored.conf"

static char *configfile;
static char cmd[1000];
static int signals = TRUE;
static int default_tape = FALSE;

static JCR *jcr;


static void usage();
static void terminate_btape(int sig);
int get_cmd(char *prompt);

static void my_free_jcr(JCR *jcr)
{
   return;
}

int write_dev(DEVICE *dev, char *buf, size_t len) 
{
   Emsg0(M_ABORT, 0, "write_dev not implemented.\n");
   return 0;
}

int read_dev(DEVICE *dev, char *buf, size_t len)
{
   Emsg0(M_ABORT, 0, "read_dev not implemented.\n");
   return 0;
}


/*********************************************************************
 *
 *	   Main Bacula Pool Creation Program
 *
 */
int main(int argc, char *argv[])
{
   int ch;

   /* Sanity checks */
   if (TAPE_BSIZE % DEV_BSIZE != 0 || TAPE_BSIZE / DEV_BSIZE == 0) {
      Emsg2(M_ABORT, 0, "Tape block size (%d) not multiple of system size (%d)\n",
	 TAPE_BSIZE, DEV_BSIZE);
   }
   if (TAPE_BSIZE != (1 << (ffs(TAPE_BSIZE)-1))) {
      Emsg1(M_ABORT, 0, "Tape block size (%d) is not a power of 2\n", TAPE_BSIZE);
   }

   printf("Tape block granularity is %d bytes.\n", TAPE_BSIZE);

   while ((ch = getopt(argc, argv, "c:d:st?")) != -1) {
      switch (ch) {
         case 'c':                    /* specify config file */
	    if (configfile != NULL) {
	       free(configfile);
	    }
	    configfile = bstrdup(optarg);
	    break;

         case 'd':                    /* set debug level */
	    debug_level = atoi(optarg);
	    if (debug_level <= 0) {
	       debug_level = 1; 
	    }
	    break;

         case 't':
	    default_tape = TRUE;
	    break;

         case 's':
	    signals = FALSE;
	    break;

         case '?':
	 default:
	    helpcmd();
	    exit(0);

      }  
   }
   argc -= optind;
   argv += optind;


   my_name_is(argc, argv, "btape");
   init_msg(NULL, NULL);
   
   if (signals) {
      init_signals(terminate_btape);
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   daemon_start_time = time(NULL);

   parse_config(configfile);


   /* See if we can open a device */
   if (argc) {
      if (!(dev = init_dev(NULL, *argv))) {
	 usage();
	 exit(1);
      }
   }

   /* Try default device */
   if (!dev && default_tape) {
      dev = init_dev(NULL, DEFAULT_TAPE_DRIVE);
   }

   if (dev) {
      if (!find_device_res()) {
	 exit(1);
      }
      if (!open_device(dev)) {
         Pmsg1(0, "Warning could not open device. ERR=%s", strerror_dev(dev));
	 term_dev(dev);
	 dev = NULL;
      }
   }

   jcr = new_jcr(sizeof(JCR), my_free_jcr);
   jcr->VolSessionId = 1;
   jcr->VolSessionTime = (uint32_t)time(NULL);
   jcr->NumVolumes = 1;


   Dmsg0(200, "Do tape commands\n");
   do_tape_cmds();
  
   terminate_btape(0);
   return 0;
}

static void terminate_btape(int stat)
{

   sm_check(__FILE__, __LINE__, False);
   if (configfile) {
      free(configfile);
   }
   free_config_resources();

   if (dev) {
      term_dev(dev);
   }

   if (debug_level > 10)
      print_memory_pool_stats(); 

   free_jcr(jcr);
   jcr = NULL;

   term_msg();
   close_memory_pool(); 	      /* free memory in pool */

   sm_dump(False);
   exit(stat);
}

void quitcmd()
{
   quit = 1;
}

/*
 * Get a new device name 
 *  Normally given on the command line
 */
static void devicecmd()
{
   if (dev) {
      term_dev(dev);
      dev = NULL;
   }
   if (!get_cmd("Enter Device Name: ")) {
      return;
   }
   dev = init_dev(NULL, cmd);
   if (dev) {
      if (!find_device_res()) {
	 return;
      }
      if (!open_device(dev)) {
         Pmsg1(0, "Device open failed. ERR=%s\n", strerror_dev(dev));
      }
   } else {
      Pmsg0(0, "Device init failed.\n");                          
   }
}

/*
 * Write a label to the tape   
 */
static void labelcmd()
{
   DEVRES *device;
   int found = 0;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }

   LockRes();
   for (device=NULL; (device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device)); ) {
      if (strcmp(device->device_name, dev->dev_name) == 0) {
	 jcr->device = device;	      /* Arggg a bit of duplication here */
	 device->dev = dev;
	 dev->device = device;
	 found = 1;
	 break;
      }
   } 
   UnlockRes();
   if (!found) {
      Pmsg2(0, "Could not find device %s in %s\n", dev->dev_name, configfile);
      return;
   }

   if (!get_cmd("Enter Volume Name: ")) {
      return;
   }
	 
   if (!(dev->state & ST_OPENED)) {
      if (!open_device(dev)) {
         Pmsg1(0, "Device open failed. ERR=%s\n", strerror_dev(dev));
      }
   }
   write_volume_label_to_dev(jcr, device, cmd, "Default");
}

/*
 * Read the tape label	 
 */
static void readlabelcmd()
{
   int save_debug_level = debug_level;
   int stat;
   DEV_BLOCK *block;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   block = new_block(dev);
   stat = read_dev_volume_label(jcr, dev, block);
   switch (stat) {
      case VOL_NO_LABEL:
         Pmsg0(0, "Volume has no label.\n");
	 break;
      case VOL_OK:
         Pmsg0(0, "Volume label read correctly.\n");
	 break;
      case VOL_IO_ERROR:
         Pmsg1(0, "I/O error on device: ERR=%s", strerror_dev(dev));
	 break;
      case VOL_NAME_ERROR:
         Pmsg0(0, "Volume name error\n");
	 break;
      case VOL_CREATE_ERROR:
         Pmsg1(0, "Error creating label. ERR=%s", strerror_dev(dev));
	 break;
      case VOL_VERSION_ERROR:
         Pmsg0(0, "Volume version error.\n");
	 break;
      case VOL_LABEL_ERROR:
         Pmsg0(0, "Bad Volume label type.\n");
	 break;
      default:
         Pmsg0(0, "Unknown error.\n");
	 break;
   }

   debug_level = 20;
   dump_volume_label(dev); 
   debug_level = save_debug_level;
   free_block(block);
}


/*
 * Search for device resource that corresponds to 
 * device name on command line (or default).
 *	 
 * Returns: 0 on failure
 *	    1 on success
 */
static int find_device_res()
{
   int found = 0;

   LockRes();
   for (device=NULL; (device=(DEVRES *)GetNextRes(R_DEVICE, (RES *)device)); ) {
      if (strcmp(device->device_name, dev->dev_name) == 0) {
	 device->dev = dev;
	 dev->capabilities = device->cap_bits;
	 found = 1;
	 break;
      }
   } 
   UnlockRes();
   if (!found) {
      Pmsg2(0, "Could not find device %s in %s\n", dev->dev_name, configfile);
      return 0;
   }
   Pmsg1(0, "Using device: %s\n", dev->dev_name);
   return 1;
}

/*
 * Load the tape should have prevously been taken
 * off line, otherwise this command is not necessary.
 */
static void loadcmd()
{

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if (!load_dev(dev)) {
      Pmsg1(0, "Bad status from load. ERR=%s\n", strerror_dev(dev));
   } else
      Pmsg1(0, "Loaded %s\n", dev_name(dev));
}

/*
 * Rewind the tape.   
 */
static void rewindcmd()
{
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if (!rewind_dev(dev)) {
      Pmsg1(0, "Bad status from rewind. ERR=%s\n", strerror_dev(dev));
      clrerror_dev(dev, -1);
   } else
      Pmsg1(0, "Rewound %s\n", dev_name(dev));
}

/*
 * Clear any tape error   
 */
static void clearcmd()
{
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   clrerror_dev(dev, -1);
}

/*
 * Write and end of file on the tape   
 */
static void weofcmd()
{
   int stat;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if ((stat = weof_dev(dev, 1)) < 0) {
      Pmsg2(0, "Bad status from weof %d. ERR=%s\n", stat, strerror_dev(dev));
      return;
   } else {
      Pmsg1(0, "Wrote EOF to %s\n", dev_name(dev));
   }
}


/* Go to the end of the medium -- raw command	
 * The idea was orginally that the end of the Bacula
 * medium would be flagged differently. This is not
 * currently the case. So, this is identical to the
 * eodcmd().
 */
static void eomcmd()
{
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if (!eod_dev(dev)) {
      Pmsg1(0, "Bad status from eod. ERR=%s\n", strerror_dev(dev));
      return;
   } else {
      Pmsg0(0, "Moved to end of media\n");
   }
}

/*
 * Go to the end of the media (either hardware determined
 *  or defined by two eofs.
 */
static void eodcmd()
{
   eomcmd();
}

/*
 * Backspace file   
 */
static void bsfcmd()
{
   int stat;
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if ((stat=bsf_dev(dev, 1)) < 0) {
      Pmsg1(0, "Bad status from bsf. ERR=%s\n", strerror(errno));
   } else {
      Pmsg0(0, "Back spaced one file.\n");
   }
}

/*
 * Backspace record   
 */
static void bsrcmd()
{
   int stat;
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if ((stat=bsr_dev(dev, 1)) < 0) {
      Pmsg1(0, "Bad status from bsr. ERR=%s\n", strerror(errno));
   } else {
      Pmsg0(0, "Back spaced one record.\n");
   }
}

/*
 * List device capabilities as defined in the 
 *  stored.conf file.
 */
static void capcmd()
{
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   Pmsg0(0, "Device capabilities: ");
   printf("%sEOF ", dev->capabilities & CAP_EOF ? "" : "!");
   printf("%sBSR ", dev->capabilities & CAP_BSR ? "" : "!");
   printf("%sBSF ", dev->capabilities & CAP_BSF ? "" : "!");
   printf("%sFSR ", dev->capabilities & CAP_FSR ? "" : "!");
   printf("%sFSF ", dev->capabilities & CAP_FSF ? "" : "!");
   printf("%sEOM ", dev->capabilities & CAP_EOM ? "" : "!");
   printf("%sREM ", dev->capabilities & CAP_REM ? "" : "!");
   printf("%sRACCESS ", dev->capabilities & CAP_RACCESS ? "" : "!");
   printf("%sAUTOMOUNT ", dev->capabilities & CAP_AUTOMOUNT ? "" : "!");
   printf("%sLABEL ", dev->capabilities & CAP_LABEL ? "" : "!");
   printf("%sANONVOLS ", dev->capabilities & CAP_ANONVOLS ? "" : "!");
   printf("%sALWAYSOPEN ", dev->capabilities & CAP_ALWAYSOPEN ? "" : "!");
   printf("\n");
}

/*
 * Test writting larger and larger records.  
 * This is a torture test for records.
 */
static void rectestcmd()
{
   DEV_BLOCK *block;
   DEV_RECORD *rec;
   int i, blkno = 0;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }

   Pmsg0(0, "Test writting larger and larger records.\n\
This is a torture test for records.\nI am going to write\n\
larger and larger records. It will stop when the record size\n\
plus the header exceeds the block size (by default about 64K\n");


   get_cmd("Do you want to continue? (y/n): ");
   if (cmd[0] != 'y') {
      Pmsg0(000, "Command aborted.\n");
      return;
   }

   sm_check(__FILE__, __LINE__, False);
   block = new_block(dev);
   rec = new_record();

   for (i=1; i<500000; i++) {
      rec->data = check_pool_memory_size(rec->data, i);
      memset(rec->data, i & 0xFF, i);
      rec->data_len = i;
      sm_check(__FILE__, __LINE__, False);
      if (write_record_to_block(block, rec)) {
	 empty_block(block);
	 blkno++;
         Pmsg2(0, "Block %d i=%d\n", blkno, i);
      } else {
	 break;
      }
      sm_check(__FILE__, __LINE__, False);
   }
   free_record(rec);
   free_block(block);
   sm_check(__FILE__, __LINE__, False);
}

/* 
 * This is a general test of Bacula's functions
 *   needed to read and write the tape.
 */
static void testcmd()
{
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   Pmsg0(0, "Append files test.\n\n\
I'm going to write one record  in file 0,\n\
                   two records in file 1,\n\
             and three records in file 2\n\n");
   rewindcmd();
   wrcmd();
   weofcmd();	   /* end file 0 */
   wrcmd();
   wrcmd();
   weofcmd();	   /* end file 1 */
   wrcmd();
   wrcmd();
   wrcmd();
   weofcmd();	  /* end file 2 */
//   weofcmd();
   rewindcmd();
   Pmsg0(0, "Now moving to end of media.\n");
   eodcmd();
   Pmsg2(0, "\nWe should be in file 3. I am at file %d. This is %s\n\n", 
      dev->file, dev->file == 3 ? "correct!" : "NOT correct!!!!");

   Pmsg0(0, "\nNow I am going to attempt to append to the tape.\n");
   wrcmd(); 
   weofcmd();
//   weofcmd();
   rewindcmd();
   scancmd();
   Pmsg0(0, "The above scan should have four files of:\n\
One record, two records, three records, and one record respectively.\n\n");


   Pmsg0(0, "Append block test.\n\n\
I'm going to write a block, an EOF, rewind, go to EOM,\n\
then backspace over the EOF and attempt to append a\
second block in the first file.\n\n");
   rewindcmd();
   wrcmd();
   weofcmd();
//   weofcmd();
   rewindcmd();
   eodcmd();
   Pmsg2(0, "We should be at file 1. I am at EOM File=%d. This is %s\n",
      dev->file, dev->file == 1 ? "correct!" : "NOT correct!!!!");
   Pmsg0(0, "Doing backspace file.\n");
   bsfcmd();
   Pmsg0(0, "Write second block, hoping to append to first file.\n");
   wrcmd();
   weofcmd();
   rewindcmd();
   Pmsg0(0, "Done writing, scanning results\n");
   scancmd();
   Pmsg0(0, "The above should have one file of two blocks.\n");
}


static void fsfcmd()
{
   int stat;
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if ((stat=fsf_dev(dev, 1)) < 0) {
      Pmsg2(0, "Bad status from fsf %d. ERR=%s\n", stat, strerror_dev(dev));
      return;
   }
   Pmsg0(0, "Forward spaced one file.\n");
}

static void fsrcmd()
{
   int stat;
   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if ((stat=fsr_dev(dev, 1)) < 0) {
      Pmsg2(0, "Bad status from fsr %d. ERR=%s\n", stat, strerror_dev(dev));
      return;
   }
   Pmsg0(0, "Forward spaced one record.\n");
}

static void rdcmd()
{
#ifdef xxxxx
   int stat;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   if (!read_dev(dev, buf, 512*126)) {
      Pmsg1(0, "Bad status from read. ERR=%s\n", strerror_dev(dev));
      return;
   }
   Pmsg1(10, "Read %d bytes\n", stat);
#else
   printf("Rdcmd no longer implemented.\n");
#endif
}


static void wrcmd()
{
   DEV_BLOCK *block;
   DEV_RECORD *rec;
   int i;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   sm_check(__FILE__, __LINE__, False);
   block = new_block(dev);
   rec = new_record();

   i = 32001;
   rec->data = (char *) check_pool_memory_size(rec->data, i);
   memset(rec->data, i & 0xFF, i);
   rec->data_len = i;
   sm_check(__FILE__, __LINE__, False);
   if (!write_record_to_block(block, rec)) {
      Pmsg0(0, "Error writing record to block.\n"); 
      return;
   }
   if (!write_block_to_dev(dev, block)) {
      Pmsg0(0, "Error writing block to device.\n"); 
      return;
   } else {
      Pmsg1(0, "Wrote one record of %d bytes.\n",
	 ((i+TAPE_BSIZE-1)/TAPE_BSIZE) * TAPE_BSIZE);
   }

   sm_check(__FILE__, __LINE__, False);
   free_record(rec);
   free_block(block);
   sm_check(__FILE__, __LINE__, False);
   Pmsg0(0, "Wrote block to device.\n");
}


/*
 * Scan tape by reading block by block. Report what is
 * on the tape.
 */
static void scancmd()
{
   int stat;
   int blocks, tot_blocks, tot_files;
   int block_size;
   uint64_t bytes;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   blocks = block_size = tot_blocks = 0;
   bytes = 0;
   if (dev->state & ST_EOT) {
      Pmsg0(0, "End of tape\n");
      return; 
   }
   update_pos_dev(dev);
   tot_files = dev->file;
   for (;;) {
      if ((stat = read(dev->fd, buf, sizeof(buf))) < 0) {
	 clrerror_dev(dev, -1);
         Mmsg2(&dev->errmsg, "read error on %s. ERR=%s.\n",
	    dev->dev_name, strerror(dev->dev_errno));
         Pmsg2(0, "Bad status from read %d. ERR=%s\n", stat, strerror_dev(dev));
	 if (blocks > 0)
            printf("%d block%s of %d bytes in file %d\n",        
                    blocks, blocks>1?"s":"", block_size, dev->file);
	 return;
      }
      Dmsg1(200, "read status = %d\n", stat);
/*    sleep(1); */
      if (stat != block_size) {
	 update_pos_dev(dev);
	 if (blocks > 0) {
            printf("%d block%s of %d bytes in file %d\n", 
                 blocks, blocks>1?"s":"", block_size, dev->file);
	    blocks = 0;
	 }
	 block_size = stat;
      }
      if (stat == 0) {		      /* EOF */
	 update_pos_dev(dev);
         printf("End of File mark.\n");
	 /* Two reads of zero means end of tape */
	 if (dev->state & ST_EOF)
	    dev->state |= ST_EOT;
	 else {
	    dev->state |= ST_EOF;
	    dev->file++;
	 }
	 if (dev->state & ST_EOT) {
            printf("End of tape\n");
	    break;
	 }
      } else {			      /* Got data */
	 dev->state &= ~ST_EOF;
	 blocks++;
	 tot_blocks++;
	 bytes += stat;
      }
   }
   update_pos_dev(dev);
   tot_files = dev->file - tot_files;
   printf("Total files=%d, blocks=%d, bytes = %" lld "\n", tot_files, tot_blocks, bytes);
}

static void statcmd()
{
   int stat = 0;
   int debug;
   uint32_t status;

   if (!dev) {
      Pmsg0(0, "No device: Use device command.\n");
      return;
   }
   debug = debug_level;
   debug_level = 30;
   if (!status_dev(dev, &status)) {
      Pmsg2(0, "Bad status from status %d. ERR=%s\n", stat, strerror_dev(dev));
   }
#ifdef xxxx
   dump_volume_label(dev);
#endif
   debug_level = debug;
}



struct cmdstruct { char *key; void (*func)(); char *help; }; 
static struct cmdstruct commands[] = {
 {"bsf",        bsfcmd,       "backspace file"},
 {"bsr",        bsrcmd,       "backspace record"},
 {"cap",        capcmd,       "list device capabilities"},
 {"clear",      clearcmd,     "clear tape errors"},
 {"device",     devicecmd,    "specify the tape device name"},
 {"eod",        eodcmd,       "go to end of Bacula data for append"},
 {"test",       testcmd,      "General test Bacula tape functions"},
 {"eom",        eomcmd,       "go to the physical end of medium"},
 {"fsf",        fsfcmd,       "forward space a file"},
 {"fsr",        fsrcmd,       "forward space a record"},
 {"help",       helpcmd,      "print this command"},
 {"label",      labelcmd,     "write a Bacula label to the tape"},
 {"load",       loadcmd,      "load a tape"},
 {"quit",       quitcmd,      "quit btape"},   
 {"rd",         rdcmd,        "read tape"},
 {"readlabel",  readlabelcmd, "read and print the Bacula tape label"},
 {"rectest",    rectestcmd,   "test record handling functions"},
 {"rewind",     rewindcmd,    "rewind the tape"},
 {"scan",       scancmd,      "read tape block by block to EOT and report"}, 
 {"status",     statcmd,      "print tape status"},
 {"weof",       weofcmd,      "write an EOF on the tape"},
 {"wr",         wrcmd,        "write a single record of 2048 bytes"}, 
	     };
#define comsize (sizeof(commands)/sizeof(struct cmdstruct))

static void
do_tape_cmds()
{
   unsigned int i;
   int found;

   while (get_cmd("*")) {
      sm_check(__FILE__, __LINE__, False);
      found = 0;
      for (i=0; i<comsize; i++)       /* search for command */
	 if (fstrsch(cmd,  commands[i].key)) {
	    (*commands[i].func)();    /* go execute command */
	    found = 1;
	    break;
	 }
      if (!found)
         Pmsg1(0, "%s is an illegal command\n", cmd);
      if (quit)
	 break;
   }
}

static void helpcmd()
{
   unsigned int i;
   usage();
   printf("  Command    Description\n  =======    ===========\n");
   for (i=0; i<comsize; i++)
      printf("  %-10s %s\n", commands[i].key, commands[i].help);
   printf("\n");
}

static void usage()
{
   fprintf(stderr,
"\nVersion: " VERSION " (" DATE ")\n\n"
"Usage: btape [-c config_file] [-d debug_level] [device_name]\n"
"       -c <file>   set configuration file to file\n"
"       -dnn        set debug level to nn\n"
"       -s          turn off signals\n"
"       -t          open the default tape device\n"
"       -?          print this message.\n"  
"\n");

}

/*	
 * Get next input command from terminal.  This
 * routine is REALLY primitive, and should be enhanced
 * to have correct backspacing, etc.
 */
int 
get_cmd(char *prompt)
{
   int i = 0;
   int ch;
   fprintf(stdout, prompt);

   /* We really should turn off echoing and pretty this
    * up a bit.
    */
   cmd[i] = 0;
   while ((ch = fgetc(stdin)) != EOF) { 
      if (ch == '\n') {
	 strip_trailing_junk(cmd);
	 return 1;
      } else if (ch == 4 || ch == 0xd3 || ch == 0x8) {
	 if (i > 0)
	    cmd[--i] = 0;
	 continue;
      } 
	 
      cmd[i++] = ch;
      cmd[i] = 0;
   }
   quit = 1;
   return 0;
}
