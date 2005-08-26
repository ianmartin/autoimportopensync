/* 
   MultiSync IrMC Plugin - Synchronize IrMC (mobile) devices
   Copyright (C) 2002-2003 Bo Lincoln <lincoln@lysator.liu.se>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
   SOFTWARE IS DISCLAIMED.
*/

/*
 *  $Id: gui.c,v 1.29 2004/04/06 09:47:22 lincoln Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <pthread.h>

#include "interface.h"
#include "support.h"
#include <multisync.h>
#include "gui.h"
#include "irmc_bluetooth.h"
#include "irmc_sync.h"

GtkWidget* irmcwindow = NULL;
GtkWidget* infodialog = NULL;
GtkWidget* unitdialog = NULL;
void (*infodialogok)(void) = NULL;
pthread_t irmcbtthread;
irmc_connection *irmcconn = NULL;

gpointer (*plugin_function)();
#define CALL_PLUGIN(mod, name, args) (g_module_symbol(mod,name,(gpointer*)&plugin_function)?(*plugin_function)args:NULL)
extern GModule *bluetoothplugin;

void show_options(irmc_connection *conn) {
  char *str;
  GtkAdjustment *adj;
  
  if (bluetoothplugin) 
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "addressentry")),
		       CALL_PLUGIN(bluetoothplugin, "irmc_batostr",
				   (&conn->btunit.bdaddr)));
  str=g_strdup_printf("%d", conn->btchannel);
  gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "channelentry")),
		     str);
  g_free(str);
  gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "irdevnameentry")),
		     conn->irunit.name);
  gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "irdevidentry")),
		     conn->irunit.serial);
  gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "serialportentry")),
		     conn->cabledev);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"serialportradioother")), TRUE);
  if (!strcmp("/dev/ttyS0", conn->cabledev))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"serialportradio0")), TRUE);
  if (!strcmp("/dev/ttyS1", conn->cabledev))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"serialportradio1")), TRUE);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							      "keepdbsizecheck")),
			       conn->commondata.managedbsize);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							      "fakerecurcheck")),
			       conn->commondata.fake_recurring);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							      "fixdstcheck")),
			       conn->fixdst);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							      "donttellsynccheck")),
			       conn->donttellsync);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							      "onlyphonecheck")),
			       conn->onlyphonenumbers);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							       "nooldercheck")),
			       conn->dontacceptold);
  gtk_widget_set_sensitive(lookup_widget(irmcwindow, "ageslider"), 
			   conn->dontacceptold);
  adj = gtk_range_get_adjustment(GTK_RANGE(lookup_widget(irmcwindow, "ageslider")));
  gtk_adjustment_set_value(adj, (float) conn->maximumage);
  gtk_signal_connect (GTK_OBJECT (adj), "value-changed",
		      GTK_SIGNAL_FUNC (irmc_age_changed), NULL);
  irmc_age_changed(adj);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							       "charsetcheck")),
			       conn->translatecharset);
  gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "charsetentry")),
		     conn->charset);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							       "alarmfromirmccheck")),
			       conn->alarmfromirmc);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							       "alarmtoirmccheck")),
			       conn->alarmtoirmc);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,
							       "convertadecheck")),
			       conn->convertade);

}

void irmc_age_changed(GtkAdjustment *adjustment) {
  int val = (int) adjustment->value;
  GtkLabel *label;
  char *text;

  label = GTK_LABEL(gtk_object_get_data(GTK_OBJECT(irmcwindow), 
					"agelabel"));
  irmcconn->maximumage = val;
  text = g_strdup_printf("%d day%s", val, val>1?"s":"");
  gtk_label_set_text(label, text);
  g_free(text);
}


void optionpreset_selected(GtkMenuItem *menuitem, gpointer user_data) {
  client_preset preset = (client_preset) user_data;
  
  switch(preset) {
  case PRESET_T68:
    irmcconn->commondata.managedbsize = TRUE;
    irmcconn->commondata.fake_recurring = TRUE;
    irmcconn->donttellsync = TRUE;
    irmcconn->fixdst = TRUE;
    irmcconn->onlyphonenumbers = TRUE;
    irmcconn->translatecharset = TRUE;
    if (irmcconn->charset)
      g_free(irmcconn->charset);
    irmcconn->charset = g_strdup("ISO8859-1");
    show_options(irmcconn);
    break;
  case PRESET_T39:
    irmcconn->commondata.managedbsize = TRUE;
    irmcconn->commondata.fake_recurring = TRUE;
    irmcconn->donttellsync = TRUE;
    irmcconn->fixdst = TRUE;
    irmcconn->onlyphonenumbers = TRUE;
    irmcconn->translatecharset = TRUE;
    if (irmcconn->charset)
      g_free(irmcconn->charset);
    irmcconn->charset = g_strdup("ISO8859-1");
    show_options(irmcconn);
    break;
  case PRESET_S55:
    irmcconn->commondata.managedbsize = TRUE;
    irmcconn->commondata.fake_recurring = FALSE;
    irmcconn->donttellsync = FALSE;
    irmcconn->fixdst = FALSE;
    irmcconn->onlyphonenumbers = TRUE;
    irmcconn->translatecharset = FALSE;
    show_options(irmcconn);
    break;
  }
}

void irmc_nooldercheck_toggled (GtkToggleButton *togglebutton) {
  irmcconn->dontacceptold = gtk_toggle_button_get_active(togglebutton);
  gtk_widget_set_sensitive(lookup_widget(irmcwindow, "ageslider"), 
			   irmcconn->dontacceptold);  
}  

void connectmedium_selected(GtkMenuItem *menuitem, gpointer user_data) {
  connect_medium medium = (connect_medium) user_data;
  gtk_widget_hide_all(lookup_widget(irmcwindow,"bttable"));
  gtk_widget_hide_all(lookup_widget(irmcwindow,"irdatable"));
  gtk_widget_hide_all(lookup_widget(irmcwindow,"cabletable"));
  irmcconn->connectmedium = medium;
  switch(medium) {
  case MEDIUM_BLUETOOTH:
    gtk_widget_show_all(lookup_widget(irmcwindow,"bttable"));
    break;
  case MEDIUM_IR:
    gtk_widget_show_all(lookup_widget(irmcwindow,"irdatable"));
    break;
  case MEDIUM_CABLE:
    gtk_widget_show_all(lookup_widget(irmcwindow,"cabletable"));
    break;
  }
}


GtkWidget* open_option_window(sync_pair *pair, connection_type type) {
  if (!irmcwindow) {
    GtkWidget *menuitem;
    GtkWidget *connectmenu;

    // The current active connection, if exists
    irmcconn = g_malloc0(sizeof(irmc_connection));
    irmcconn->sync_pair = pair;
    irmcconn->conntype = type;
    // Some defaults
    irmcconn->commondata.managedbsize = TRUE;

    if (bluetoothplugin) 
      irmcconn->connectmedium = MEDIUM_BLUETOOTH;
    else {
#if HAVE_IRDA
      irmcconn->connectmedium = MEDIUM_IR;
#else
      irmcconn->connectmedium = MEDIUM_CABLE;
#endif
    }
    strcpy(irmcconn->cabledev, "/dev/ttyS0");
    irmcconn->cabletype = IRMC_CABLE_ERICSSON;
    load_state(irmcconn);
    
    irmcwindow = create_optionwin();
    gtk_widget_show (irmcwindow);
    connectmenu = gtk_menu_new ();
    menuitem = gtk_menu_item_new_with_label (_("Bluetooth"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (connectmedium_selected),
			(gpointer) MEDIUM_BLUETOOTH);
    if (!bluetoothplugin)
      gtk_widget_set_sensitive(GTK_WIDGET(menuitem), FALSE);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    if (irmcconn->connectmedium==MEDIUM_BLUETOOTH) {
      gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
      gtk_menu_set_active (GTK_MENU(connectmenu), 0);
    }
    menuitem = gtk_menu_item_new_with_label (_("IR"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (connectmedium_selected),
			(gpointer) MEDIUM_IR);
#if !HAVE_IRDA
    gtk_widget_set_sensitive(GTK_WIDGET(menuitem), FALSE);
#endif
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    if (irmcconn->connectmedium==MEDIUM_IR) {
      gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
      gtk_menu_set_active (GTK_MENU(connectmenu), 1);
    }
    menuitem = gtk_menu_item_new_with_label (_("Cable"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (connectmedium_selected),
			(gpointer) MEDIUM_CABLE);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    if (irmcconn->connectmedium==MEDIUM_CABLE) {
      gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
      gtk_menu_set_active (GTK_MENU(connectmenu), 2);
    }
    gtk_widget_show_all(GTK_WIDGET(connectmenu));
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(irmcwindow,
							    "connectmenu")), 
			      connectmenu);
        
    connectmenu = gtk_menu_new ();
    menuitem = gtk_menu_item_new_with_label (_("None"));
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    menuitem = gtk_menu_item_new_with_label (_("Ericsson T39/R520m"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (optionpreset_selected),
			(gpointer) PRESET_T39);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    menuitem = gtk_menu_item_new_with_label (_("SonyEricsson T68i/T610"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (optionpreset_selected),
			(gpointer) PRESET_T68);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    menuitem = gtk_menu_item_new_with_label (_("Siemens S55"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (optionpreset_selected),
			(gpointer) PRESET_S55);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    gtk_widget_show_all(GTK_WIDGET(connectmenu));
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(irmcwindow,
							    "optionpresetmenu")), connectmenu);
			      
    /*connectmenu = gtk_menu_new ();
    menuitem = gtk_menu_item_new_with_label (_("Ericsson / SonyEricsson Phone"));
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (cabletype_selected),
			(gpointer) IRMC_CABLE_ERICSSON);
    gtk_menu_append (GTK_MENU (connectmenu), menuitem);
    gtk_widget_show_all(GTK_WIDGET(connectmenu));
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(irmcwindow,
    "cabletypemenu")), connectmenu);*/

    {
      int cablemenu = 0;
      if (irmcconn->cabletype == IRMC_CABLE_SIEMENS)
	cablemenu = 1;
      gtk_option_menu_set_history (GTK_OPTION_MENU(lookup_widget(irmcwindow, "cablemanumenu")), cablemenu);
    }

    show_options(irmcconn);
  }
  return(irmcwindow);
}

void option_window_closed() {
  sync_plugin_window_closed();
  if (irmcconn)
    g_free(irmcconn);
  irmcwindow = NULL;
}

void fetch_gui_data(void) {
  char *str;
  str=(char*) gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "addressentry")));
  if (bluetoothplugin)
    CALL_PLUGIN(bluetoothplugin, "irmc_strtoba",
		(&irmcconn->btunit.bdaddr, str));
  str=(char*) gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "channelentry")));
  sscanf(str, "%d", &irmcconn->btchannel);
  str=(char*) gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "irdevnameentry")));
  strncpy(irmcconn->irunit.name, str, 31);
  str=(char*) gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "irdevidentry")));
  strncpy(irmcconn->irunit.serial, str, 127);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "serialportradio0")))) 
    strcpy(irmcconn->cabledev, "/dev/ttyS0");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "serialportradio1")))) 
    strcpy(irmcconn->cabledev, "/dev/ttyS1");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "serialportradioother")))) {
    str=(char*) gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "serialportentry")));
    strncpy(irmcconn->cabledev, str, 19);
  }

  irmcconn->commondata.managedbsize = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "keepdbsizecheck")));
  irmcconn->commondata.fake_recurring = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "fakerecurcheck")));
  irmcconn->fixdst = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow, "fixdstcheck")));
  irmcconn->donttellsync = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"donttellsynccheck")));
  irmcconn->onlyphonenumbers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"onlyphonecheck")));
  irmcconn->dontacceptold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"nooldercheck")));
  irmcconn->translatecharset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"charsetcheck")));
  irmcconn->charset=g_strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(irmcwindow, "charsetentry"))));
  irmcconn->alarmfromirmc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"alarmfromirmccheck")));
  irmcconn->alarmtoirmc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"alarmtoirmccheck")));
  irmcconn->convertade = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(irmcwindow,"convertadecheck")));
}


void option_window_ok() {
  fetch_gui_data();
  save_state(irmcconn);

  gtk_widget_destroy(irmcwindow);
  sync_plugin_window_closed();
  if (irmcconn)
    g_free(irmcconn);
  irmcconn = NULL;
  irmcwindow = NULL;
}

void option_window_cancel(void) {
  gtk_widget_destroy(irmcwindow);
  sync_plugin_window_closed();
  if (irmcconn)
    g_free(irmcconn);
  irmcconn = NULL;
  irmcwindow = NULL;
}


void open_infodialog(char *text, void (*okfunction)(void)) {
  if (!infodialog)
    infodialog = create_infodialog();
  gtk_widget_show (infodialog);
  gtk_label_set_text(GTK_LABEL(lookup_widget(infodialog,"infolabel")),text);
  if (okfunction) {
    infodialogok = okfunction;
    gtk_widget_set_sensitive(lookup_widget(infodialog, "infook"), TRUE);
  } else {
    gtk_widget_set_sensitive(lookup_widget(infodialog, "infook"), FALSE);
  }
}

void close_infodialog() {
  if (infodialog)
    gtk_widget_destroy(infodialog);
  infodialog = NULL;
}

gboolean irmc_do_showinfo(gpointer data) {
  char *msg = data;
  open_infodialog(msg, NULL);
  g_free(msg);
  return(FALSE);
}


void irmc_async_slowsync_msg(sync_object_type objtype) {
  char *msg;
  msg = g_strdup_printf("Fetching the full %s database.\nThis may take several minutes.", sync_objtype_as_string(objtype));
  g_idle_add(irmc_do_showinfo, msg);
}

gboolean irmc_do_closeinfo(gpointer data) {
  close_infodialog();
  return(FALSE);
}

void irmc_async_close_infodialog(void) {
  g_idle_add(irmc_do_closeinfo, NULL);
}

void ok_infodialog() {
  close_infodialog();
  if (infodialogok)
    (*infodialogok)();
}

void search_bt_devices() {
  open_infodialog("Please make sure your Bluetooth device\n is discoverable and press OK.", spawn_bt_search);
}

gboolean bt_units_found(gpointer data) {
  GList *units = data;
  char *text;
  int n;
  char *columns[] = { "", NULL };
  GtkListStore *unitstore = 
    g_object_get_data(G_OBJECT(unitdialog), "unitstore");
  GtkTreeSelection *select;

  text = g_strdup_printf("Search done. %d units found.", g_list_length(units));
  gtk_label_set_text(GTK_LABEL(lookup_widget(unitdialog, "listlabel")),
			       text);
  g_free(text);
  irmc_set_cursor(unitdialog, FALSE);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listok"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listcancel"), TRUE);
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW(lookup_widget(unitdialog, "unitlist")));
  for (n = 0; n < g_list_length(units); n++) {
    irmc_bt_unit *irbt = g_list_nth_data(units, n);
    if (irbt) {
      GtkTreeIter iter;
      if (irbt->channel >= 0) {
	columns[0] = g_strdup_printf("%s (%s, channel %d)", irbt->name,
				     irbt->address, irbt->channel);
      } else {
	columns[0] = g_strdup_printf("%s (%s) - No IrMC synchronization", 
				     irbt->name, irbt->address);
      }
      gtk_list_store_append (unitstore, &iter);
      gtk_list_store_set(unitstore, &iter, 0, columns[0], 1, irbt, -1);
      if (n == 0)
	gtk_tree_selection_select_iter(select, &iter);
      g_free(columns[0]);
    }
  }
  g_list_free(units);
  return(FALSE);
}

void async_find_bt_units(gpointer data) {
  if (bluetoothplugin) {
    GList* (*btfind)(void);
    if (g_module_symbol(bluetoothplugin,"find_bt_units",
			(gpointer*) &btfind)) {
      GList *found = (btfind)();
      gtk_idle_add(bt_units_found, found);
    }
  }
}

void irmc_set_cursor(GtkWidget *window, gboolean busy) {
  GdkWindow *gdkwin;
  GdkCursor* cursor;

  gdkwin = window->window;
  if (busy)
    cursor = gdk_cursor_new(GDK_WATCH);
  else
    cursor = gdk_cursor_new(GDK_LEFT_PTR);
  gdk_window_set_cursor(gdkwin, cursor);
  gdk_cursor_destroy(cursor);
}

void spawn_bt_search(void) {
  GtkListStore *unitstore;
  GtkTreeView *unitlist;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column;
  
  if (!unitdialog)
    unitdialog = create_listdialog();
  unitstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  unitlist = GTK_TREE_VIEW(lookup_widget(unitdialog, "unitlist"));  
  gtk_tree_view_set_model(unitlist, GTK_TREE_MODEL(unitstore));
  g_object_set_data(G_OBJECT(unitdialog), "unitstore", unitstore);
  g_object_unref(G_OBJECT(unitstore));
  column = gtk_tree_view_column_new_with_attributes("Devices", renderer, 
						    "text", 0, NULL);
  gtk_tree_view_column_set_min_width(column, 150);
  gtk_tree_view_append_column (unitlist, column);
  gtk_widget_show (unitdialog);
  gtk_label_set_text(GTK_LABEL(lookup_widget(unitdialog, "listlabel")),
			       "Searching...");
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listok"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listcancel"), FALSE);
  irmc_set_cursor(unitdialog, TRUE);
  pthread_create(&irmcbtthread, NULL, 
		 (void *(*) (void *)) async_find_bt_units, NULL);
}

void close_unitdialog(void) {
  if (unitdialog) 
    gtk_widget_destroy(unitdialog);
  unitdialog = NULL;
}

gpointer irmc_get_selected_unit(int pos) {
  GtkTreeSelection *select;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gpointer data = NULL;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW(lookup_widget(unitdialog, "unitlist")));
  if (select && gtk_tree_selection_get_selected (select, &model, &iter)) {
    gtk_tree_model_get (model, &iter, pos, &data, -1);
  }
  return(data);
}

void device_selected(void) {
  switch(irmcconn->connectmedium) {
  case MEDIUM_BLUETOOTH: {
    irmc_bt_unit *irbt = irmc_get_selected_unit(1);
    
    if (irbt) {
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, "addressentry")),
			 irbt->address);
      if (irbt->channel >= 0) {
	char *chan;
	chan = g_strdup_printf("%d", irbt->channel);
	gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, 
						   "channelentry")), chan);
	g_free(chan);
      } else
	gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, 
						   "channelentry")), "");
    }
  } break;
  case MEDIUM_IR: {
    irmc_ir_unit *iru = irmc_get_selected_unit(1);
    if (iru) {
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, 
						 "irdevnameentry")),
			 iru->name);
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(irmcwindow, 
						 "irdevidentry")),
			 iru->serial);
    }
  } break;
  default:
    break;
  }
  close_unitdialog();
}

void device_canceled(void) {
  close_unitdialog();
}

void search_ir_devices() {
  open_infodialog("Please make sure your IR device\n is ready and press OK.", spawn_ir_search);
}

gboolean ir_units_found(gpointer data) {
  GList *units = data;
  char *text;
  int n;
  char *columns[] = { "", NULL };
  GtkListStore *unitstore = 
    g_object_get_data(G_OBJECT(unitdialog), "unitstore");
  GtkTreeSelection *select;

  text = g_strdup_printf("Search done. %d units found.", g_list_length(units));
  gtk_label_set_text(GTK_LABEL(lookup_widget(unitdialog, "listlabel")),
			       text);
  g_free(text);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listok"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listcancel"), TRUE);
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW(lookup_widget(unitdialog, "unitlist")));
  for (n = 0; n < g_list_length(units); n++) {
    irmc_ir_unit *iru = g_list_nth_data(units, n);
    if (iru) {
      GtkTreeIter iter;
      if (sizeof(iru->serial) > 0) {
	columns[0] = g_strdup_printf("%s, serial number %s", iru->name,
				     iru->serial);
      } else {
	columns[0] = g_strdup_printf("%s", iru->name);
      }
      gtk_list_store_append (unitstore, &iter);
      gtk_list_store_set(unitstore, &iter, 0, columns[0], 1, iru, -1);
      if (n == 0)
	gtk_tree_selection_select_iter(select, &iter);
      g_free(columns[0]);
    }
  }
  g_list_free(units);
  return(FALSE);
}

GList* find_irda_units(irmc_connection *conn);

void async_find_ir_units(gpointer data) {
  GList *found = (GList*) find_irda_units(irmcconn);
  gtk_idle_add(ir_units_found, found);
}

void spawn_ir_search(void) {
  GtkListStore *unitstore;
  GtkTreeView *unitlist;
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column;
  
  if (!unitdialog)
    unitdialog = create_listdialog();
  unitstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  unitlist = GTK_TREE_VIEW(lookup_widget(unitdialog, "unitlist"));  
  gtk_tree_view_set_model(unitlist, GTK_TREE_MODEL(unitstore));
  g_object_set_data(G_OBJECT(unitdialog), "unitstore", unitstore);
  g_object_unref(G_OBJECT(unitstore));
  column = gtk_tree_view_column_new_with_attributes("Devices", renderer, 
						    "text", 0, NULL);
  gtk_tree_view_column_set_min_width(column, 150);
  gtk_tree_view_append_column (unitlist, column);
  gtk_widget_show (unitdialog);
  gtk_label_set_text(GTK_LABEL(lookup_widget(unitdialog, "listlabel")),
			       "Searching...");
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listok"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(unitdialog, "listcancel"), FALSE);
  irmc_set_cursor(unitdialog, TRUE);
  pthread_create(&irmcbtthread, NULL, 
		 (void *(*) (void *))async_find_ir_units, NULL);
}


gboolean connection_tested(gpointer data) {
  gboolean success = (gboolean) data;
  if (success) {
    open_infodialog("Connection succeeded!", close_infodialog);
  } else {
    open_infodialog("Connection failed.", close_infodialog);
  }
  return(FALSE);
}

void async_test_connection(gpointer data) {
  gboolean success = sync_test_connection(irmcconn);
  gtk_idle_add(connection_tested, (gpointer) success);
}
  

void spawn_conn_test(void) {
  fetch_gui_data();
  open_infodialog("Testing connection...", NULL);
  pthread_create(&irmcbtthread, NULL, 
		 (void *(*) (void *))async_test_connection, NULL);
}

