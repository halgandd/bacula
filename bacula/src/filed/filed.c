/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *  Bacula File Daemon
 *
 *    Kern Sibbald, March MM
 *
 *   Version $Id$
 *
 */

#include "bacula.h"
#include "filed.h"

#ifdef HAVE_PYTHON

#undef _POSIX_C_SOURCE
#include <Python.h>

#include "lib/pythonlib.h"

/* Imported Functions */
extern PyObject *job_getattr(PyObject *self, char *attrname);
extern int job_setattr(PyObject *self, char *attrname, PyObject *value);

#endif /* HAVE_PYTHON */

/* Imported Functions */
extern void *handle_client_request(void *dir_sock);
extern bool parse_fd_config(CONFIG *config, const char *configfile, int exit_code);

/* Forward referenced functions */
void terminate_filed(int sig);
static bool check_resources();

/* Exported variables */
CLIENT *me;                           /* my resource */
bool no_signals = false;
void *start_heap;

#define CONFIG_FILE "bacula-fd.conf" /* default config file */

char *configfile = NULL;
static bool foreground = false;
static workq_t dir_workq;             /* queue of work from Director */
static pthread_t server_tid;
static CONFIG *config;


static int tls_pem_callback(char *buf, int size, const void *userdata)
{
#ifdef HAVE_TLS
   const char *prompt = (const char *)userdata;
# if defined(HAVE_WIN32)
   sendit(prompt);
   if (win32_cgets(buf, size) == NULL) {
      buf[0] = 0;
      return 0;
   } else {
      return strlen(buf);
   }
# else
   char *passwd;

   passwd = getpass(prompt);
   bstrncpy(buf, passwd, size);
   return strlen(buf);
# endif
#else
   buf[0] = 0;
   return 0;
#endif
}

static void usage()
{
   Pmsg3(-1, _(
PROG_COPYRIGHT
"\nVersion: %s (%s)\n\n"
"Usage: bacula-fd [-f -s] [-c config_file] [-d debug_level]\n"
"        -c <file>   use <file> as configuration file\n"
"        -d <nn>     set debug level to <nn>\n"
"        -dt         print timestamp in debug output\n"
"        -f          run in foreground (for debugging)\n"
"        -g          groupid\n"
"        -s          no signals (for debugging)\n"
"        -t          test configuration file and exit\n"
"        -u          userid\n"
"        -v          verbose user messages\n"
"        -?          print this message.\n"
"\n"), 2000, VERSION, BDATE);
   exit(1);
}


/*********************************************************************
 *
 *  Main Bacula Unix Client Program
 *
 */
#if defined(HAVE_WIN32)
#define main BaculaMain
#endif

void *initiate_jobs (void *arg)
{
        static int numdir, numcon;
        static DIRRES *dir = NULL;
        static CONRES *cons = NULL;
        static BSOCK *UA_sock = NULL;
        int stat ;
        int  i,j; 
        utime_t heart_beat;
        JCR jcr;

   pthread_detach(pthread_self());

        (void)WSA_Init();  
                        
        LockRes();
        numdir = 0;
        foreach_res(dir, R_DIRECTOR) {
           numdir++;
        }
        numcon = 0;
        foreach_res(cons, R_CONSOLE) {
           numcon++;
        }
        UnlockRes();

        char buf[1024];

        cons = NULL ;
        for(i = 0; i < numcon ; ++i)
		{
                LockRes();
                cons = (CONRES *)GetNextRes(R_CONSOLE, (RES *)cons);
                UnlockRes();
                dir = NULL ;
                for(j = 0; j < numdir ; ++j)
                {
                        LockRes();
                        dir = (DIRRES *)GetNextRes(R_DIRECTOR, (RES *)dir);
                        UnlockRes();
                        
                        if( strcmp(cons->director,dir->hdr.name))continue;

                        /* Initialize Console TLS context */
                        if (cons && (cons->tls_enable || cons->tls_require)) {
            			/* Generate passphrase prompt */
                               bsnprintf(buf, sizeof(buf), "Passphrase for Console \"%s\" TLS private key: ", cons->hdr.name);

						/* Initialize TLS context:
						 * Args: CA certfile, CA certdir, Certfile, Keyfile,
						 * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer   
						 */
                                cons->tls_ctx = new_tls_context(cons->tls_ca_certfile,
								cons->tls_ca_certdir, cons->tls_certfile,
								cons->tls_keyfile, tls_pem_callback, &buf, NULL, true);
                                if (!cons->tls_ctx) {
                                        Dmsg1(50,"Failed to initialize TLS context for Console \"%s\".\n",dir->hdr.name);
                                        continue ;
                                }
                        }  
                   		/* Initialize Director TLS context */
                        if (dir->tls_enable || dir->tls_require) { 
            			/* Generate passphrase prompt */
                                bsnprintf(buf, sizeof(buf), "Passphrase for Director \"%s\" TLS private key: ", dir->hdr.name);
            
						/* Initialize TLS context:
						 * Args: CA certfile, CA certdir, Certfile, Keyfile,
						 * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
                                dir->tls_ctx = new_tls_context(dir->tls_ca_certfile,
								dir->tls_ca_certdir, dir->tls_certfile,
								dir->tls_keyfile, tls_pem_callback, &buf, NULL, true);
                                if (!dir->tls_ctx) {
                                        Dmsg1(50,"Failed to initialize TLS context for Director \"%s\".\n",dir->hdr.name);
                                        continue ;
                                }
                        }

                        if (dir->heartbeat_interval) {
                                heart_beat = dir->heartbeat_interval;
                        } else if (cons) {
                                heart_beat = cons->heartbeat_interval;
                        } else {
                                heart_beat = 0;
                        }

                        UA_sock = bnet_connect(NULL, 5, 15, heart_beat, "Director daemon", dir->address,NULL, dir->DIRport, 0);
                        if (UA_sock == NULL) {
                                continue ;
                        } 
                        jcr.dir_bsock = UA_sock;

                        if (!authenticate_director(&jcr,cons)) {
                                continue ;
                        } 

         				Dmsg0(40, "Opened connection with Director daemon\n");
         
						char* cmd = NULL;
						int length_cmd;
				 
						for(int index=0; index < cons->jobstostart->size(); ++index){
							length_cmd = strlen("backup ");
							length_cmd += strlen((char*)cons->jobstostart->get(index));
							length_cmd += strlen(" ");
							length_cmd += strlen(cons->director);
							length_cmd += strlen(" fdcalled");
							length_cmd += 2;
							cmd = (char*)malloc(sizeof(char)*(length_cmd));
							bstrncpy(cmd,"backup ",length_cmd);
							bstrncat(cmd,(char*)cons->jobstostart->get(index),length_cmd);
							bstrncat(cmd," ",length_cmd);
							bstrncat(cmd,cons->director,length_cmd);
							bstrncat(cmd," fdcalled",length_cmd);

							Dmsg1(40,"fd> %s \n",cmd);
							bnet_fsend(UA_sock, cmd);
							free(cmd);
							while (stat = bnet_recv(UA_sock),  UA_sock->msglen != BNET_CMD_FAILED && UA_sock->msglen != BNET_CMD_OK ) {
								Dmsg1(40,"fd< %s\n",UA_sock->msg);
							}

							if (UA_sock->msglen == BNET_CMD_OK)
								handle_client_request(UA_sock); 

							while ((stat = bnet_recv(UA_sock)), UA_sock->msglen != BNET_EOD);
						}
						Dmsg0(40,"fd> quit\n");
						bnet_fsend(UA_sock, "quit");

						while ((stat = bnet_recv(UA_sock)) >= 0) {
							Dmsg1(50,"fd< %s",UA_sock->msg);
						}

						if (UA_sock != NULL) {
							  UA_sock->signal(BNET_TERMINATE); 
							  UA_sock->close();
						}
                }
		}
   return NULL;
}

int main (int argc, char *argv[])
{
   int ch;
   bool test_config = false;
   char *uid = NULL;
   char *gid = NULL;
#ifdef HAVE_PYTHON
   init_python_interpreter_args python_args;
#endif /* HAVE_PYTHON */

   start_heap = sbrk(0);
   setlocale(LC_ALL, "");
   bindtextdomain("bacula", LOCALEDIR);
   textdomain("bacula");

   init_stack_dump();
   my_name_is(argc, argv, "bacula-fd");
   init_msg(NULL, NULL);
   daemon_start_time = time(NULL);

   while ((ch = getopt(argc, argv, "c:d:fg:stu:v?")) != -1) {
      switch (ch) {
      case 'c':                    /* configuration file */
         if (configfile != NULL) {
            free(configfile);
         }
         configfile = bstrdup(optarg);
         break;

      case 'd':                    /* debug level */
         if (*optarg == 't') {
            dbg_timestamp = true;
         } else {
            debug_level = atoi(optarg);
            if (debug_level <= 0) {
               debug_level = 1;
            }
         }
         break;

      case 'f':                    /* run in foreground */
         foreground = true;
         break;

      case 'g':                    /* set group */
         gid = optarg;
         break;

      case 's':
         no_signals = true;
         break;

      case 't':
         test_config = true;
         break;

      case 'u':                    /* set userid */
         uid = optarg;
         break;

      case 'v':                    /* verbose */
         verbose++;
         break;

      case '?':
      default:
         usage();

      }
   }
   argc -= optind;
   argv += optind;

   if (argc) {
      if (configfile != NULL)
         free(configfile);
      configfile = bstrdup(*argv);
      argc--;
      argv++;
   }
   if (argc) {
      usage();
   }

   server_tid = pthread_self();
   if (!no_signals) {
      init_signals(terminate_filed);
   } else {
      /* This reduces the number of signals facilitating debugging */
      watchdog_sleep_time = 120;      /* long timeout for debugging */
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   config = new_config_parser();
   parse_fd_config(config, configfile, M_ERROR_TERM);

   if (init_crypto() != 0) {
      Emsg0(M_ERROR, 0, _("Cryptography library initialization failed.\n"));
      terminate_filed(1);
   }

   if (!check_resources()) {
      Emsg1(M_ERROR, 0, _("Please correct configuration file: %s\n"), configfile);
      terminate_filed(1);
   }

   set_working_directory(me->working_directory);

   if (test_config) {
      terminate_filed(0);
   }

   if (!foreground) {
      daemon_start();
      init_stack_dump();              /* set new pid */
   }

   /* Maximum 1 daemon at a time */
   create_pid_file(me->pid_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));
   read_state_file(me->working_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));

   load_fd_plugins(me->plugin_directory);

   drop(uid, gid);

#ifdef BOMB
   me += 1000000;
#endif

#ifdef HAVE_PYTHON
   python_args.progname = me->hdr.name;
   python_args.scriptdir = me->scripts_directory;
   python_args.modulename = "FDStartUp";
   python_args.configfile = configfile;
   python_args.workingdir = me->working_directory;
   python_args.job_getattr = job_getattr;
   python_args.job_setattr = job_setattr;

   init_python_interpreter(&python_args);
#endif /* HAVE_PYTHON */

   int status;
   pthread_t initiate_job_t;
        if(me->initiate_jobs){
                if ((status=pthread_create(&initiate_job_t, NULL, initiate_jobs, NULL)) != 0) {
                   berrno be;
                   Emsg1(M_ABORT, 0, _("Cannot create initiate jobs thread: %s\n"), be.bstrerror(status));
                }
        }

   set_thread_concurrency(10);

   if (!no_signals) {
      start_watchdog();               /* start watchdog thread */
      init_jcr_subsystem();           /* start JCR watchdogs etc. */
   }
   server_tid = pthread_self();

   /* Become server, and handle requests */
   IPADDR *p;
   foreach_dlist(p, me->FDaddrs) {
      Dmsg1(10, "filed: listening on port %d\n", p->get_port_host_order());
   }
   bnet_thread_server(me->FDaddrs, me->MaxConcurrentJobs, &dir_workq, handle_client_request);

   terminate_filed(0);
   exit(0);                           /* should never get here */
}

void terminate_filed(int sig)
{
   static bool already_here = false;

   if (already_here) {
      bmicrosleep(2, 0);              /* yield */
      exit(1);                        /* prevent loops */
   }
   already_here = true;
   debug_level = 0;                   /* turn off debug */
   stop_watchdog();

   bnet_stop_thread_server(server_tid);
   generate_daemon_event(NULL, "Exit");
   unload_plugins();
   write_state_file(me->working_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));
   delete_pid_file(me->pid_directory, "bacula-fd", get_first_port_host_order(me->FDaddrs));

   if (configfile != NULL) {
      free(configfile);
   }

   if (debug_level > 0) {
      print_memory_pool_stats();
   }
   if (config) {
      config->free_resources();
      free(config);
      config = NULL;
   }
   term_msg();
   cleanup_crypto();
   close_memory_pool();               /* release free memory in pool */
   lmgr_cleanup_main();
   sm_dump(false);                    /* dump orphaned buffers */
   exit(sig);
}

/*
* Make a quick check to see that we have all the
* resources needed.
*/
static bool check_resources()
{
   bool OK = true;
   DIRRES *director;
   bool need_tls;

   LockRes();

   me = (CLIENT *)GetNextRes(R_CLIENT, NULL);
   if (!me) {
      Emsg1(M_FATAL, 0, _("No File daemon resource defined in %s\n"
            "Without that I don't know who I am :-(\n"), configfile);
      OK = false;
   } else {
      if (GetNextRes(R_CLIENT, (RES *) me) != NULL) {
         Emsg1(M_FATAL, 0, _("Only one Client resource permitted in %s\n"),
              configfile);
         OK = false;
      }
      my_name_is(0, NULL, me->hdr.name);
      if (!me->messages) {
         me->messages = (MSGS *)GetNextRes(R_MSGS, NULL);
         if (!me->messages) {
             Emsg1(M_FATAL, 0, _("No Messages resource defined in %s\n"), configfile);
             OK = false;
         }
      }
      /* tls_require implies tls_enable */
      if (me->tls_require) {
#ifndef HAVE_TLS
         Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
         OK = false;
#else
         me->tls_enable = true;
#endif
      }
      need_tls = me->tls_enable || me->tls_authenticate;

      if ((!me->tls_ca_certfile && !me->tls_ca_certdir) && need_tls) {
         Emsg1(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
            " or \"TLS CA Certificate Dir\" are defined for File daemon in %s.\n"),
                            configfile);
        OK = false;
      }

      /* If everything is well, attempt to initialize our per-resource TLS context */
      if (OK && (need_tls || me->tls_require)) {
         /* Initialize TLS context:
          * Args: CA certfile, CA certdir, Certfile, Keyfile,
          * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
         me->tls_ctx = new_tls_context(me->tls_ca_certfile,
            me->tls_ca_certdir, me->tls_certfile, me->tls_keyfile,
            NULL, NULL, NULL, true);

         if (!me->tls_ctx) { 
            Emsg2(M_FATAL, 0, _("Failed to initialize TLS context for File daemon \"%s\" in %s.\n"),
                                me->hdr.name, configfile);
            OK = false;
         }
      }

      if (me->pki_encrypt || me->pki_sign) {
#ifndef HAVE_CRYPTO
         Jmsg(NULL, M_FATAL, 0, _("PKI encryption/signing enabled but not compiled into Bacula.\n"));
         OK = false;
#endif
      }

      /* pki_encrypt implies pki_sign */
      if (me->pki_encrypt) {
         me->pki_sign = true;
      }

      if ((me->pki_encrypt || me->pki_sign) && !me->pki_keypair_file) {
         Emsg2(M_FATAL, 0, _("\"PKI Key Pair\" must be defined for File"
            " daemon \"%s\" in %s if either \"PKI Sign\" or"
            " \"PKI Encrypt\" are enabled.\n"), me->hdr.name, configfile);
         OK = false;
      }

      /* If everything is well, attempt to initialize our public/private keys */
      if (OK && (me->pki_encrypt || me->pki_sign)) {
         char *filepath;
         /* Load our keypair */
         me->pki_keypair = crypto_keypair_new();
         if (!me->pki_keypair) {
            Emsg0(M_FATAL, 0, _("Failed to allocate a new keypair object.\n"));
            OK = false;
         } else {
            if (!crypto_keypair_load_cert(me->pki_keypair, me->pki_keypair_file)) {
               Emsg2(M_FATAL, 0, _("Failed to load public certificate for File"
                     " daemon \"%s\" in %s.\n"), me->hdr.name, configfile);
               OK = false;
            }

            if (!crypto_keypair_load_key(me->pki_keypair, me->pki_keypair_file, NULL, NULL)) {
               Emsg2(M_FATAL, 0, _("Failed to load private key for File"
                     " daemon \"%s\" in %s.\n"), me->hdr.name, configfile);
               OK = false;
            }
         }

         /*
          * Trusted Signers. We're always trusted.
          */
         me->pki_signers = New(alist(10, not_owned_by_alist));
         if (me->pki_keypair) {
            me->pki_signers->append(crypto_keypair_dup(me->pki_keypair));
         }

         /* If additional signing public keys have been specified, load them up */
         if (me->pki_signing_key_files) {
            foreach_alist(filepath, me->pki_signing_key_files) {
               X509_KEYPAIR *keypair;

               keypair = crypto_keypair_new();
               if (!keypair) {
                  Emsg0(M_FATAL, 0, _("Failed to allocate a new keypair object.\n"));
                  OK = false;
               } else {
                  if (crypto_keypair_load_cert(keypair, filepath)) {
                     me->pki_signers->append(keypair);

                     /* Attempt to load a private key, if available */
                     if (crypto_keypair_has_key(filepath)) {
                        if (!crypto_keypair_load_key(keypair, filepath, NULL, NULL)) {
                           Emsg3(M_FATAL, 0, _("Failed to load private key from file %s for File"
                              " daemon \"%s\" in %s.\n"), filepath, me->hdr.name, configfile);
                           OK = false;
                        }
                     }

                  } else {
                     Emsg3(M_FATAL, 0, _("Failed to load trusted signer certificate"
                        " from file %s for File daemon \"%s\" in %s.\n"), filepath, me->hdr.name, configfile);
                     OK = false;
                  }
               }
            }
         }

         /*
          * Crypto recipients. We're always included as a recipient.
          * The symmetric session key will be encrypted for each of these readers.
          */
         me->pki_recipients = New(alist(10, not_owned_by_alist));
         if (me->pki_keypair) {
            me->pki_recipients->append(crypto_keypair_dup(me->pki_keypair));
         }


         /* If additional keys have been specified, load them up */
         if (me->pki_master_key_files) {
            foreach_alist(filepath, me->pki_master_key_files) {
               X509_KEYPAIR *keypair;

               keypair = crypto_keypair_new();
               if (!keypair) {
                  Emsg0(M_FATAL, 0, _("Failed to allocate a new keypair object.\n"));
                  OK = false;
               } else {
                  if (crypto_keypair_load_cert(keypair, filepath)) {
                     me->pki_recipients->append(keypair);
                  } else {
                     Emsg3(M_FATAL, 0, _("Failed to load master key certificate"
                        " from file %s for File daemon \"%s\" in %s.\n"), filepath, me->hdr.name, configfile);
                     OK = false;
                  }
               }
            }
         }
      }
   }


   /* Verify that a director record exists */
   LockRes();
   director = (DIRRES *)GetNextRes(R_DIRECTOR, NULL);
   UnlockRes();
   if (!director) {
      Emsg1(M_FATAL, 0, _("No Director resource defined in %s\n"),
            configfile);
      OK = false;
   }

   foreach_res(director, R_DIRECTOR) { 
      /* tls_require implies tls_enable */
      if (director->tls_require) {
#ifndef HAVE_TLS
         Jmsg(NULL, M_FATAL, 0, _("TLS required but not configured in Bacula.\n"));
         OK = false;
         continue;
#else
         director->tls_enable = true;
#endif
      }
      need_tls = director->tls_enable || director->tls_authenticate;

      if (!director->tls_certfile && need_tls) {
         Emsg2(M_FATAL, 0, _("\"TLS Certificate\" file not defined for Director \"%s\" in %s.\n"),
               director->hdr.name, configfile);
         OK = false;
      }

      if (!director->tls_keyfile && need_tls) {
         Emsg2(M_FATAL, 0, _("\"TLS Key\" file not defined for Director \"%s\" in %s.\n"),
               director->hdr.name, configfile);
         OK = false;
      }

      if ((!director->tls_ca_certfile && !director->tls_ca_certdir) && need_tls && director->tls_verify_peer) {
         Emsg2(M_FATAL, 0, _("Neither \"TLS CA Certificate\""
                             " or \"TLS CA Certificate Dir\" are defined for Director \"%s\" in %s."
                             " At least one CA certificate store is required"
                             " when using \"TLS Verify Peer\".\n"),
                             director->hdr.name, configfile);
         OK = false;
      }

      /* If everything is well, attempt to initialize our per-resource TLS context */
      if (OK && (need_tls || director->tls_require)) {
         /* Initialize TLS context:
          * Args: CA certfile, CA certdir, Certfile, Keyfile,
          * Keyfile PEM Callback, Keyfile CB Userdata, DHfile, Verify Peer */
         director->tls_ctx = new_tls_context(director->tls_ca_certfile,
            director->tls_ca_certdir, director->tls_certfile,
            director->tls_keyfile, NULL, NULL, director->tls_dhfile,
            director->tls_verify_peer);

         if (!director->tls_ctx) { 
            Emsg2(M_FATAL, 0, _("Failed to initialize TLS context for Director \"%s\" in %s.\n"),
                                director->hdr.name, configfile);
            OK = false;
         }
      }
   }

   UnlockRes();

   if (OK) {
      close_msg(NULL);                /* close temp message handler */
      init_msg(NULL, me->messages);   /* open user specified message handler */
   }

   return OK;
}
