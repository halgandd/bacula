/*
 *
 *   Bacula Gnome Tray Monitor
 *
 *     Nicolas Boichat, August MMIV
 *
 *     Version $Id$
 */

/*
   Copyright (C) 2004 Kern Sibbald and John Walker

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "tray-monitor.h"

#include "eggstatusicon.h"

#include "generic.xpm"

/* Imported functions */
int authenticate_director(JCR *jcr, MONITOR *monitor, DIRRES *director);
int authenticate_file_daemon(JCR *jcr, MONITOR *monitor, CLIENT* client);
int authenticate_storage_daemon(JCR *jcr, MONITOR *monitor, STORE* store);

/* Forward referenced functions */
void writecmd(monitoritem* item, const char* command);
int docmd(monitoritem* item, const char* command, GSList** list);
void getstatus(monitoritem* item, int current, GString** str);

/* Static variables */
static char *configfile = NULL;
static MONITOR *monitor;
static POOLMEM *args;
static JCR jcr;
static int nitems = 0;
static int fullitem = 0; //Item to be display in detailled status window
static int lastupdated = -1; //Last item updated
static monitoritem items[32];

/* Data received from DIR/FD/SD */
static char OKqstatus[]   = "%c000 OK .status\n";
static char DotStatusJob[] = "JobId=%d JobStatus=%c JobErrors=%d\n";

/* UI variables and functions */

extern "C" {
static gboolean fd_read(gpointer data);
static gboolean blink(gpointer data);
}

void trayMessage(const char *fmt,...);
void updateStatusIcon(monitoritem* item);
void changeStatusMessage(monitoritem* item, const char *fmt,...);
static const char** generateXPM(stateenum newstate, stateenum oldstate);

/* Callbacks */
static void TrayIconActivate(GtkWidget *widget, gpointer data);
static void TrayIconExit(unsigned int activateTime, unsigned int button);
static void TrayIconPopupMenu(unsigned int button, unsigned int activateTime);
static void MonitorAbout(GtkWidget *widget, gpointer data);
static void MonitorRefresh(GtkWidget *widget, gpointer data);
static void IntervalChanged(GtkWidget *widget, gpointer data);
static void DaemonChanged(GtkWidget *widget, monitoritem* data);
static gboolean delete_event(GtkWidget *widget, GdkEvent  *event, gpointer   data);

static guint timerTag;
static EggStatusIcon *mTrayIcon;
static GtkWidget *mTrayMenu;
static GtkWidget *window;
static GtkWidget *textview;
static GtkTextBuffer *buffer;
static GtkWidget *timeoutspinner;
char** xpm_generic_var;
static gboolean blinkstate = TRUE;

#define CONFIG_FILE "./tray-monitor.conf"   /* default configuration file */

static void usage()
{
   fprintf(stderr, _(
"Copyright (C) 2000-2004 Kern Sibbald and John Walker\n"
"Written by Nicolas Boichat (2004)\n"
"\nVersion: " VERSION " (" BDATE ") %s %s %s\n\n"
"Usage: tray-monitor [-c config_file] [-d debug_level]\n"
"       -c <file>     set configuration file to file\n"
"       -dnn          set debug level to nn\n"
"       -t            test - read configuration and exit\n"
"       -?            print this message.\n"
"\n"), HOST_OS, DISTNAME, DISTVER);
}

static GtkWidget *new_image_button(const gchar *stock_id,
				   const gchar *label_text) {
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *image;

    button = gtk_button_new();

    box = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(box), 2);
    image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_BUTTON);
    label = gtk_label_new(label_text);

    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 3);

    gtk_widget_show(image);
    gtk_widget_show(label);

    gtk_widget_show(box);

    gtk_container_add(GTK_CONTAINER(button), box);

    return button;
}

/*********************************************************************
 *
 *	   Main Bacula Tray Monitor -- User Interface Program
 *
 */
int main(int argc, char *argv[])
{
   int ch, i;
   bool test_config = false;
   DIRRES* dird;
   CLIENT* filed;
   STORE* stored;

   init_stack_dump();
   my_name_is(argc, argv, "tray-monitor");
   textdomain("bacula");
   init_msg(NULL, NULL);
   working_directory = "/tmp";
   args = get_pool_memory(PM_FNAME);

   struct sigaction sigignore;
   sigignore.sa_flags = 0;
   sigignore.sa_handler = SIG_IGN;
   sigfillset(&sigignore.sa_mask);
   sigaction(SIGPIPE, &sigignore, NULL);

   gtk_init (&argc, &argv);

   while ((ch = getopt(argc, argv, "bc:d:th?f:s:")) != -1) {
      switch (ch) {
      case 'c':                    /* configuration file */
	 if (configfile != NULL) {
	    free(configfile);
	 }
	 configfile = bstrdup(optarg);
	 break;

      case 'd':
	 debug_level = atoi(optarg);
	 if (debug_level <= 0) {
	    debug_level = 1;
	 }
	 break;

      case 't':
	 test_config = true;
	 break;

      case 'h':
      case '?':
      default:
	 usage();
	 exit(1);
      }
   }
   argc -= optind;
   //argv += optind;

   if (argc) {
      usage();
      exit(1);
   }

   if (configfile == NULL) {
      configfile = bstrdup(CONFIG_FILE);
   }

   parse_config(configfile);

   LockRes();
   nitems = 0;
   foreach_res(monitor, R_MONITOR) {
      nitems++;
   }

   if (nitems != 1) {
      Emsg2(M_ERROR_TERM, 0,
	 _("Error: %d Monitor resource defined in %s. You must define one and only one Monitor resource.\n"), nitems, configfile);
   }

   nitems = 0;
   foreach_res(dird, R_DIRECTOR) {
      items[nitems].type = R_DIRECTOR;
      items[nitems].resource = dird;
      items[nitems].D_sock = NULL;
      items[nitems].state = warn;
      items[nitems].oldstate = warn;
      nitems++;
   }
   foreach_res(filed, R_CLIENT) {
      items[nitems].type = R_CLIENT;
      items[nitems].resource = filed;
      items[nitems].D_sock = NULL;
      items[nitems].state = warn;
      items[nitems].oldstate = warn;
      nitems++;
   }
   foreach_res(stored, R_STORAGE) {
      items[nitems].type = R_STORAGE;
      items[nitems].resource = stored;
      items[nitems].D_sock = NULL;
      items[nitems].state = warn;
      items[nitems].oldstate = warn;
      nitems++;
   }
   UnlockRes();

   if (nitems == 0) {
      Emsg1(M_ERROR_TERM, 0, _("No Client, Storage nor Director resource defined in %s\n"
"Without that I don't how to get status from the File, Storage or Director Daemon :-(\n"), configfile);
   }

   if (test_config) {
      exit(0);
   }

   //Copy the content of xpm_generic in xpm_generic_var to be able to modify it
   g_assert((xpm_generic_var = (char**)g_malloc(sizeof(xpm_generic))));
   for (i = 0; i < (int)(sizeof(xpm_generic)/sizeof(const char*)); i++) {
      g_assert((xpm_generic_var[i] = (char*)g_malloc(strlen(xpm_generic[i])*sizeof(char))));
      strcpy(xpm_generic_var[i], xpm_generic[i]);
   }

   (void)WSA_Init();                /* Initialize Windows sockets */

   LockRes();
   monitor = (MONITOR*)GetNextRes(R_MONITOR, (RES *)NULL);
   UnlockRes();

   if ((monitor->RefreshInterval < 1) || (monitor->RefreshInterval > 600)) {
      Emsg2(M_ERROR_TERM, 0, _("Invalid refresh interval defined in %s\n"
"This value must be greater or equal to 1 second and less or equal to 10 minutes (read value: %d).\n"), configfile, monitor->RefreshInterval);
   }

   GdkPixbuf* pixbuf = gdk_pixbuf_new_from_xpm_data(generateXPM(warn, warn));
   // This should be ideally replaced by a completely libpr0n-based icon rendering.
   mTrayIcon = egg_status_icon_new_from_pixbuf(pixbuf);
   g_signal_connect(G_OBJECT(mTrayIcon), "activate", G_CALLBACK(TrayIconActivate), NULL);
   g_signal_connect(G_OBJECT(mTrayIcon), "popup-menu", G_CALLBACK(TrayIconPopupMenu), NULL);
   g_object_unref(G_OBJECT(pixbuf));

   mTrayMenu = gtk_menu_new();

   GtkWidget *entry;

   entry = gtk_menu_item_new_with_label("Open status window...");
   g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(TrayIconActivate), NULL);
   gtk_menu_shell_append(GTK_MENU_SHELL(mTrayMenu), entry);

   gtk_menu_shell_append(GTK_MENU_SHELL(mTrayMenu), gtk_separator_menu_item_new());

   entry = gtk_menu_item_new_with_label("Exit");
   g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(TrayIconExit), NULL);
   gtk_menu_shell_append(GTK_MENU_SHELL(mTrayMenu), entry);

   gtk_widget_show_all(mTrayMenu);

   timerTag = g_timeout_add( 1000*monitor->RefreshInterval/nitems, fd_read, NULL );

   g_timeout_add( 1000, blink, NULL );

   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

   gtk_window_set_title(GTK_WINDOW(window), "Bacula tray monitor");

   g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
   //g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

   gtk_container_set_border_width(GTK_CONTAINER(window), 10);

   GtkWidget* vbox = gtk_vbox_new(FALSE, 10);

   GtkWidget* daemon_table = gtk_table_new((nitems*2)+2, 3, FALSE);

   gtk_table_set_col_spacings(GTK_TABLE(daemon_table), 8);

   GtkWidget* separator = gtk_hseparator_new();
   gtk_table_attach_defaults(GTK_TABLE(daemon_table), separator, 0, 3, 0, 1);

   GString *str;
   GSList *group = NULL;
   GtkWidget* radio;
   GtkWidget* align;

   for (int i = 0; i < nitems; i++) {
      switch (items[i].type) {
      case R_DIRECTOR:
	 str = g_string_new(((DIRRES*)(items[i].resource))->hdr.name);
	 g_string_append(str, _(" (DIR)"));
	 break;
      case R_CLIENT:
	 str = g_string_new(((CLIENT*)(items[i].resource))->hdr.name);
	 g_string_append(str, _(" (FD)"));
	 break;
      case R_STORAGE:
	 str = g_string_new(((STORE*)(items[i].resource))->hdr.name);
	 g_string_append(str, _(" (SD)"));
	 break;
      default:
	 continue;
      }

      radio = gtk_radio_button_new_with_label(group, str->str);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), i == 0);
      g_signal_connect(G_OBJECT(radio), "toggled", G_CALLBACK(DaemonChanged), &(items[i]));

      pixbuf = gdk_pixbuf_new_from_xpm_data(generateXPM(warn, warn));
      items[i].image = gtk_image_new_from_pixbuf(pixbuf);

      items[i].label =  gtk_label_new(_("Unknown status."));
      align = gtk_alignment_new(0.0, 0.5, 0.0, 1.0);
      gtk_container_add(GTK_CONTAINER(align), items[i].label);

      gtk_table_attach(GTK_TABLE(daemon_table), radio, 0, 1, (i*2)+1, (i*2)+2,
	 GTK_FILL, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
      gtk_table_attach(GTK_TABLE(daemon_table), items[i].image, 1, 2, (i*2)+1, (i*2)+2,
	 GTK_FILL, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
      gtk_table_attach(GTK_TABLE(daemon_table), align, 2, 3, (i*2)+1, (i*2)+2,
	 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);

      separator = gtk_hseparator_new();
      gtk_table_attach_defaults(GTK_TABLE(daemon_table), separator, 0, 3, (i*2)+2, (i*2)+3);

      group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio));
   }

   gtk_box_pack_start(GTK_BOX(vbox), daemon_table, FALSE, FALSE, 0);

   textview = gtk_text_view_new();

   buffer = gtk_text_buffer_new(NULL);

   gtk_text_buffer_set_text(buffer, "", -1);

   PangoFontDescription *font_desc = pango_font_description_from_string ("Fixed 10");
   gtk_widget_modify_font(textview, font_desc);
   pango_font_description_free(font_desc);

   gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 20);
   gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 20);

   gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);

   gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview), buffer);

   gtk_box_pack_start(GTK_BOX(vbox), textview, TRUE, TRUE, 0);

   GtkWidget* hbox = gtk_hbox_new(FALSE, 10);

   GtkWidget* hbox2 = gtk_hbox_new(FALSE, 0);
   GtkWidget* label = gtk_label_new(_("Refresh interval in seconds: "));
   gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, FALSE, 0);
   GtkAdjustment *spinner_adj = (GtkAdjustment *) gtk_adjustment_new (monitor->RefreshInterval, 1.0, 600.0, 1.0, 5.0, 5.0);
   timeoutspinner = gtk_spin_button_new (spinner_adj, 1.0, 0);
   g_signal_connect(G_OBJECT(timeoutspinner), "value-changed", G_CALLBACK(IntervalChanged), NULL);
   gtk_box_pack_start(GTK_BOX(hbox2), timeoutspinner, TRUE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(hbox), hbox2, TRUE, FALSE, 0);

   GtkWidget* button = new_image_button("gtk-refresh", _("Refresh now"));
   g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(MonitorRefresh), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);

   button = new_image_button("gtk-help", _("About"));
   g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(MonitorAbout), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);

   button = new_image_button("gtk-close", _("Close"));
   g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_hide), G_OBJECT(window));
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);

   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

   gtk_container_add(GTK_CONTAINER (window), vbox);

   gtk_widget_show_all(vbox);

   gtk_main();

   for (i = 0; i < nitems; i++) {
      if (items[i].D_sock) {
	 writecmd(&items[i], "quit");
	 bnet_sig(items[i].D_sock, BNET_TERMINATE); /* send EOF */
	 bnet_close(items[i].D_sock);
      }
   }

   free_pool_memory(args);
   (void)WSACleanup();		     /* Cleanup Windows sockets */

   //Free xpm_generic_var
   for (i = 0; i < (int)(sizeof(xpm_generic)/sizeof(const char*)); i++) {
      g_free(xpm_generic_var[i]);
   }
   g_free(xpm_generic_var);

   return 0;
}

static void MonitorAbout(GtkWidget *widget, gpointer data) {
#if HAVE_GTK_2_4
   GtkWidget* about = gtk_message_dialog_new_with_markup(GTK_WINDOW(window),GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _(
      "<span size='x-large' weight='bold'>Bacula Tray Monitor</span>\n\n"
      "Copyright (C) 2004 Kern Sibbald and John Walker\n"
      "Written by Nicolas Boichat\n"
      "\n<small>Version: " VERSION " (" BDATE ") %s %s %s</small>"
   ), HOST_OS, DISTNAME, DISTVER);
#else
   GtkWidget* about = gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, _(
      "Bacula Tray Monitor\n\n"
      "Copyright (C) 2004 Kern Sibbald and John Walker\n"
      "Written by Nicolas Boichat\n"
      "\nVersion: " VERSION " (" BDATE ") %s %s %s"
   ), HOST_OS, DISTNAME, DISTVER);
#endif
   gtk_dialog_run(GTK_DIALOG(about));
   gtk_widget_destroy(about);
}

static void MonitorRefresh(GtkWidget *widget, gpointer data) {
   for (int i = 0; i < nitems; i++) {
      fd_read(NULL);
   }
}

static gboolean delete_event( GtkWidget *widget,
			      GdkEvent  *event,
			      gpointer   data ) {
   gtk_widget_hide(window);
   return TRUE; /* do not destroy the window */
}

static void TrayIconActivate(GtkWidget *widget, gpointer data) {
    gtk_widget_show(window);
}

static void TrayIconPopupMenu(unsigned int activateTime, unsigned int button) {
  gtk_menu_popup(GTK_MENU(mTrayMenu), NULL, NULL, NULL, NULL, 1, 0);
  gtk_widget_show_all(mTrayMenu);
}

static void TrayIconExit(unsigned int activateTime, unsigned int button) {
   gtk_main_quit();
}

static void IntervalChanged(GtkWidget *widget, gpointer data) {
   g_source_remove(timerTag);
   timerTag = g_timeout_add(
      (guint)(
	 gtk_spin_button_get_value(GTK_SPIN_BUTTON(timeoutspinner))*1000/nitems
      ), fd_read, NULL );
}

static void DaemonChanged(GtkWidget *widget, monitoritem* data) {
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
      fullitem = -1;
      for (int i = 0; i < nitems; i++) {
	 if (data == &(items[i])) {
	    fullitem = i;
	    break;
	 }
      }
      g_return_if_fail(fullitem != -1);

      int oldlastupdated = lastupdated;
      lastupdated = fullitem-1;
      fd_read(NULL);
      lastupdated = oldlastupdated;
   }
}

static int authenticate_daemon(monitoritem* item, JCR *jcr) {
   switch (item->type) {
   case R_DIRECTOR:
      return authenticate_director(jcr, monitor, (DIRRES*)item->resource);
      break;
   case R_CLIENT:
      return authenticate_file_daemon(jcr, monitor, (CLIENT*)item->resource);
      break;
   case R_STORAGE:
      return authenticate_storage_daemon(jcr, monitor, (STORE*)item->resource);
      break;
   default:
      printf("Error, currentitem is not a Client or a Storage..\n");
      gtk_main_quit();
      return FALSE;
   }
}

static gboolean blink(gpointer data) {
   blinkstate = !blinkstate;
   for (int i = 0; i < nitems; i++) {
      updateStatusIcon(&items[i]);
   }
   updateStatusIcon(NULL);
   return true;
}

static gboolean fd_read(gpointer data) {
   GtkTextBuffer *newbuffer = gtk_text_buffer_new(NULL);
   GtkTextIter start, stop, nstart, nstop;

   GSList *list, *it;

   GString *strlast, *strcurrent;

   lastupdated++;
   if (lastupdated == nitems) {
      lastupdated = 0;
   }

   if (lastupdated == fullitem) {
      if (items[lastupdated].type == R_DIRECTOR)
	 docmd(&items[lastupdated], "status Director\n", &list);
      else
	 docmd(&items[lastupdated], "status\n", &list);

      it = list->next;
      do {
	 gtk_text_buffer_get_end_iter(newbuffer, &stop);
	 gtk_text_buffer_insert (newbuffer, &stop, ((GString*)it->data)->str, -1);
	 if (it->data) g_string_free((GString*)it->data, TRUE);
      } while ((it = it->next) != NULL);

      /* Keep the selection if necessary */
      if (gtk_text_buffer_get_selection_bounds(buffer, &start, &stop)) {
	 gtk_text_buffer_get_iter_at_offset(newbuffer, &nstart, gtk_text_iter_get_offset(&start));
	 gtk_text_buffer_get_iter_at_offset(newbuffer, &nstop,  gtk_text_iter_get_offset(&stop ));

   #if HAVE_GTK_2_4
	 gtk_text_buffer_select_range(newbuffer, &nstart, &nstop);
   #else
	 gtk_text_buffer_move_mark(newbuffer, gtk_text_buffer_get_mark(newbuffer, "insert"), &nstart);
	 gtk_text_buffer_move_mark(newbuffer, gtk_text_buffer_get_mark(newbuffer, "selection_bound"), &nstop);
   #endif
      }

      g_object_unref(buffer);

      buffer = newbuffer;
      gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview), buffer);
   }

   getstatus(&items[lastupdated], 1, &strcurrent);
   getstatus(&items[lastupdated], 0, &strlast);
   updateStatusIcon(&items[lastupdated]);

   changeStatusMessage(&items[lastupdated], "Current job: %s\nLast job: %s", strcurrent->str, strlast->str);

   updateStatusIcon(NULL);

   g_string_free(strcurrent, TRUE);
   g_string_free(strlast, TRUE);

   return 1;
}

void getstatus(monitoritem* item, int current, GString** str) {
   GSList *list, *it;
   stateenum ret = error;
   int jobid = 0, joberrors = 0;
   char jobstatus = JS_ErrorTerminated;
   char num;
   int k;

   *str = g_string_sized_new(128);

   if (current) {
      if (item->type == R_DIRECTOR)
	 docmd(&items[lastupdated], ".status dir current\n", &list);
      else
	 docmd(&items[lastupdated], ".status current\n", &list);
   }
   else {
      if (item->type == R_DIRECTOR)
	 docmd(&items[lastupdated], ".status dir last\n", &list);
      else
	 docmd(&items[lastupdated], ".status last\n", &list);
   }

   it = list->next;
   if ((it == NULL) || (sscanf(((GString*)it->data)->str, OKqstatus, &num) != 1)) {
      g_string_append_printf(*str, ".status error : %s", (it == NULL) ? "" : ((GString*)it->data)->str);
      while (((*str)->str[(*str)->len-1] == '\n') || ((*str)->str[(*str)->len-1] == '\r')) {
	 g_string_set_size(*str, (*str)->len-1);
      }
      ret = error;
   }
   else if ((it = it->next) == NULL) {
      if (current) {
	 g_string_append(*str, _("No current job."));
      }
      else {
	 g_string_append(*str, _("No last job."));
      }
      ret = idle;
   }
   else if ((k = sscanf(((GString*)it->data)->str, DotStatusJob, &jobid, &jobstatus, &joberrors)) == 3) {
      switch (jobstatus) {
      case JS_Created:
	 ret = (joberrors > 0) ? warn : running;
	 g_string_append_printf(*str, _("Job status: Created (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 break;
      case JS_Running:
	 ret = (joberrors > 0) ? warn : running;
	 g_string_append_printf(*str, _("Job status: Running (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 break;
      case JS_Blocked:
	 g_string_append_printf(*str, _("Job status: Blocked (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_Terminated:
	 g_string_append_printf(*str, _("Job status: Terminated (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = (joberrors > 0) ? warn : idle;
	 break;
      case JS_ErrorTerminated:
	 g_string_append_printf(*str, _("Job status: Terminated in error (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = error;
	 break;
      case JS_Error:
	 ret = (joberrors > 0) ? warn : running;
	 g_string_append_printf(*str, _("Job status: Error (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 break;
      case JS_FatalError:
	 g_string_append_printf(*str, _("Job status: Fatal error (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = error;
	 break;
      case JS_Differences:
	 g_string_append_printf(*str, _("Job status: Verify differences (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_Canceled:
	 g_string_append_printf(*str, _("Job status: Canceled (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitFD:
	 g_string_append_printf(*str, _("Job status: Waiting on File daemon (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitSD:
	 g_string_append_printf(*str, _("Job status: Waiting on the Storage daemon (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitMedia:
	 g_string_append_printf(*str, _("Job status: Waiting for new media (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitMount:
	 g_string_append_printf(*str, _("Job status: Waiting for Mount (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitStoreRes:
	 g_string_append_printf(*str, _("Job status: Waiting for storage resource (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitJobRes:
	 g_string_append_printf(*str, _("Job status: Waiting for job resource (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitClientRes:
	 g_string_append_printf(*str, _("Job status: Waiting for Client resource (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitMaxJobs:
	 g_string_append_printf(*str, _("Job status: Waiting for maximum jobs (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitStartTime:
	 g_string_append_printf(*str, _("Job status: Waiting for start time (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      case JS_WaitPriority:
	 g_string_append_printf(*str, _("Job status: Waiting for higher priority jobs to finish (%d error%s)"), joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      default:
	 g_warning("Unknown job status %c.", jobstatus);
	 g_string_append_printf(*str, _("Job status: Unknown(%c) (%d error%s)"), jobstatus, joberrors, (joberrors > 1) ? "s" : "");
	 ret = warn;
	 break;
      }
   }
   else {
      fprintf(stderr, "Bad scan : '%s' %d\n", (it == NULL) ? "" : ((GString*)it->data)->str, k);
      ret = error;
   }

   it = list;
   do {
      if (it->data) g_string_free((GString*)it->data, TRUE);
   } while ((it = it->next) != NULL);

   g_slist_free(list);

   if (current) {
      item->state = ret;
   }
   else {
      item->oldstate = ret;
   }
}

int docmd(monitoritem* item, const char* command, GSList** list) {
   int stat;
   GString* str = NULL;

   *list = g_slist_alloc();

   //str = g_string_sized_new(64);

   if (!item->D_sock) {
      memset(&jcr, 0, sizeof(jcr));

      DIRRES* dird;
      CLIENT* filed;
      STORE* stored;

      switch (item->type) {
      case R_DIRECTOR:
	 dird = (DIRRES*)item->resource;
	 trayMessage("Connecting to Director %s:%d\n", dird->address, dird->DIRport);
	 changeStatusMessage(item, "Connecting to Director %s:%d", dird->address, dird->DIRport);
	 item->D_sock = bnet_connect(NULL, 0, 0, "Director daemon", dird->address, NULL, dird->DIRport, 0);
	 jcr.dir_bsock = item->D_sock;
	 break;
      case R_CLIENT:
	 filed = (CLIENT*)item->resource;
	 trayMessage("Connecting to Client %s:%d\n", filed->address, filed->FDport);
	 changeStatusMessage(item, "Connecting to Client %s:%d", filed->address, filed->FDport);
	 item->D_sock = bnet_connect(NULL, 0, 0, "File daemon", filed->address, NULL, filed->FDport, 0);
	 jcr.file_bsock = item->D_sock;
	 break;
      case R_STORAGE:
	 stored = (STORE*)item->resource;
	 trayMessage("Connecting to Storage %s:%d\n", stored->address, stored->SDport);
	 changeStatusMessage(item, "Connecting to Storage %s:%d", stored->address, stored->SDport);
	 item->D_sock = bnet_connect(NULL, 0, 0, "Storage daemon", stored->address, NULL, stored->SDport, 0);
	 jcr.store_bsock = item->D_sock;
	 break;
      default:
	 printf("Error, currentitem is not a Client, a Storage or a Director..\n");
	 gtk_main_quit();
	 return 0;
      }

      if (item->D_sock == NULL) {
	 g_slist_append(*list, g_string_new("Cannot connect to daemon.\n"));
	 changeStatusMessage(item, "Cannot connect to daemon.");
	 item->state = error;
	 item->oldstate = error;
	 return 0;
      }

      if (!authenticate_daemon(item, &jcr)) {
	 str = g_string_sized_new(64);
	 g_string_printf(str, "ERR=%s\n", item->D_sock->msg);
	 g_slist_append(*list, str);
	 item->state = error;
	 item->oldstate = error;
	 changeStatusMessage(item, "Authentication error : %s", item->D_sock->msg);
	 item->D_sock = NULL;
	 return 0;
      }

      switch (item->type) {
      case R_DIRECTOR:
	 trayMessage("Opened connection with Director daemon.\n");
	 changeStatusMessage(item, "Opened connection with Director daemon.");
	 break;
      case R_CLIENT:
	 trayMessage("Opened connection with File daemon.\n");
	 changeStatusMessage(item, "Opened connection with File daemon.");
	 break;
      case R_STORAGE:
	 trayMessage("Opened connection with Storage daemon.\n");
	 changeStatusMessage(item, "Opened connection with Storage daemon.");
	 break;
      default:
	 printf("Error, currentitem is not a Client, a Storage or a Director..\n");
	 gtk_main_quit();
	 return 0;
	 break;
      }

      if (item->type == R_DIRECTOR) { /* Read connection messages... */
	 GSList *list, *it;
	 docmd(item, "", &list); /* Usually invalid, but no matter */
	 it = list;
	 do {
	    if (it->data) g_string_free((GString*)it->data, TRUE);
	 } while ((it = it->next) != NULL);

	 g_slist_free(list);
      }
   }

   if (command[0] != 0)
      writecmd(item, command);

   while(1) {
      if ((stat = bnet_recv(item->D_sock)) >= 0) {
	 g_slist_append(*list, g_string_new(item->D_sock->msg));
      }
      else if (stat == BNET_SIGNAL) {
	 if (item->D_sock->msglen == BNET_EOD) {
	    //fprintf(stderr, "<< EOD >>\n");
	    return 1;
	 }
	 else if (item->D_sock->msglen == BNET_PROMPT) {
	    //fprintf(stderr, "<< PROMPT >>\n");
	    g_slist_append(*list, g_string_new("<< Error: BNET_PROMPT signal received. >>\n"));
	    return 0;
	 }
	 else if (item->D_sock->msglen == BNET_HEARTBEAT) {
	    bnet_sig(item->D_sock, BNET_HB_RESPONSE);
	    g_slist_append(*list, g_string_new("<< Heartbeat signal received, answered. >>\n"));
	 }
	 else {
	    str = g_string_sized_new(64);
	    g_string_printf(str, "<< Unexpected signal received : %s >>\n", bnet_sig_to_ascii(item->D_sock));
	    g_slist_append(*list, str);
	 }
      }
      else { /* BNET_HARDEOF || BNET_ERROR */
	 g_slist_append(*list, g_string_new("<ERROR>\n"));
	 item->D_sock = NULL;
	 item->state = error;
	 item->oldstate = error;
	 changeStatusMessage(item, "Error : BNET_HARDEOF or BNET_ERROR");
	 //fprintf(stderr, "<< ERROR >>\n");
	 return 0;
      }

      if (is_bnet_stop(item->D_sock)) {
	 g_string_append_printf(str, "<STOP>\n");
	 item->D_sock = NULL;
	 item->state = error;
	 item->oldstate = error;
	 changeStatusMessage(item, "Error : Connection closed.");
	 //fprintf(stderr, "<< STOP >>\n");
	 return 0;            /* error or term */
      }
   }
}

void writecmd(monitoritem* item, const char* command) {
   if (item->D_sock) {
      item->D_sock->msglen = strlen(command);
      pm_strcpy(&item->D_sock->msg, command);
      bnet_send(item->D_sock);
   }
}

/* Note: Does not seem to work either on Gnome nor KDE... */
void trayMessage(const char *fmt,...) {
   char buf[512];
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   bvsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);

   fprintf(stderr, buf);

   egg_tray_icon_send_message(egg_status_icon_get_tray_icon(mTrayIcon), 5000, (const char*)&buf, -1);
}

void changeStatusMessage(monitoritem* item, const char *fmt,...) {
   char buf[512];
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   bvsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);

   gtk_label_set_text(GTK_LABEL(item->label), buf);
}

void updateStatusIcon(monitoritem* item) {
   const char** xpm;

   if (item == NULL) {
      /* For the current status, select the two worse for blinking,
	 but never blink a D_Sock == NULL error with idle. */
      stateenum state1, state2, oldstate;
      gboolean onenull = FALSE;
      state1 = idle;
      state2 = idle;
      oldstate = idle;
      for (int i = 0; i < nitems; i++) {
	 if (items[i].D_sock == NULL) onenull = TRUE;
	 if (items[i].state >= state1) {
	    state2 = state1;
	    state1 = items[i].state;
	 }
	 else if (items[i].state > state2) {
	    state2 = items[i].state;
	 }
	 if (items[i].oldstate > oldstate) oldstate = items[i].oldstate;
      }

      if ((onenull == TRUE) && (state2 == idle)) {
	 state2 = error;
      }

      xpm = generateXPM(blinkstate ? state1 : state2, oldstate);
   }
   else {
      if ((blinkstate) && (item->D_sock != NULL)) {
	 if (item->state > 1) { //Warning or error while running
	    xpm = generateXPM(running, item->oldstate);
	 }
	 else {
	    xpm = generateXPM(idle, item->oldstate);
	 }
      }
      else {
	 xpm = generateXPM(item->state, item->oldstate);
      }
   }

   GdkPixbuf* pixbuf = gdk_pixbuf_new_from_xpm_data(xpm);
   if (item == NULL) {
      egg_status_icon_set_from_pixbuf(mTrayIcon, pixbuf);
      gtk_window_set_icon(GTK_WINDOW(window), pixbuf);
   }
   else {
      gtk_image_set_from_pixbuf(GTK_IMAGE(item->image), pixbuf);
   }
}

/* Note: result should not be stored, as it is a reference to xpm_generic_var */
static const char** generateXPM(stateenum newstate, stateenum oldstate) {
   char* address = &xpm_generic_var[xpm_generic_first_color][xpm_generic_column];
   switch (newstate) {
   case error:
      strcpy(address, "ff0000");
      break;
   case idle:
      strcpy(address, "ffffff");
      break;
   case running:
      strcpy(address, "00ff00");
      break;
   case warn:
      strcpy(address, "ffff00");
      break;
   }

   address = &xpm_generic_var[xpm_generic_second_color][xpm_generic_column];
   switch (oldstate) {
   case error:
      strcpy(address, "ff0000");
      break;
   case idle:
      strcpy(address, "ffffff");
      break;
   case running:
      strcpy(address, "00ff00");
      break;
   case warn:
      strcpy(address, "ffff00");
      break;
   }

   return (const char**)xpm_generic_var;
}
