/* 
   MultiSync Evolution Plugin - Synchronize Ximian Evolution data
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
 *  $Id: gui.c,v 1.2.2.4 2004/04/16 20:06:26 irix Exp $
 */

#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <multisync.h>
#include <dirent.h>
#include "evolution_sync.h"
#include "evolution_config.h"
#include "gui.h"
#include "interface.h"
#include "support.h"

GtkWidget* evowindow = NULL;
eds_conn_t *evoconn = NULL;
GList *calendarlist = NULL, *todolist = NULL, *addressbooklist = NULL;

GList* read_filelist(GList *list, char *dirname, char *textname, 
                     char *filename, int depth) 
{
  DIR *dir;
  
  if (depth < 0)
    return(list);
  
  if ((dir = opendir(dirname)))
  {
    struct dirent *de;
    while ((de = readdir(dir))) 
    {
      if (de->d_type != DT_DIR) 
      {
        if (!strcmp(de->d_name, filename))
        {
          evo_sync_file *ef = g_malloc0(sizeof(evo_sync_file));
          ef->name = g_strdup(textname);
          ef->path = g_strdup_printf("%s/%s", dirname, de->d_name);
          list = g_list_append(list, ef);
        }
      }
       
      if (de->d_type != DT_REG) 
      {
        char *newdir;
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) 
        {
          newdir = g_strdup_printf("%s/%s", dirname, de->d_name);
          list = read_filelist(list, newdir, de->d_name, filename, depth-1);
          g_free(newdir);
        }
      }
    }
    closedir(dir);
  }
  return(list);
}

void free_filelist(GList *list) 
{
  while (list) 
  {
    GList *first;
    evo_sync_file *ef;
    
    first = g_list_first(list);
    ef = first->data;
    if (ef->name) 
      g_free(ef->name);
    if (ef->path)
      g_free(ef->path);
    list = g_list_remove_link(list, first);
  }
}

void calendar_selected(GtkMenuItem *menuitem, gpointer user_data) 
{
  if (evoconn->calendarpath)
    g_free(evoconn->calendarpath);
  evoconn->calendarpath = g_strdup(user_data);
}

void todo_selected(GtkMenuItem *menuitem, gpointer user_data) 
{
  if (evoconn->todopath)
    g_free(evoconn->todopath);
  evoconn->todopath = g_strdup(user_data);
}

void addressbook_selected(GtkMenuItem *menuitem, gpointer user_data) 
{
  if (evoconn->addressbookpath)
    g_free(evoconn->addressbookpath);
  evoconn->addressbookpath = g_strdup(user_data);
}

GtkWidget* open_option_window(sync_pair *pair, connection_type type) 
{
  if (!evowindow) 
  {
    char *cal_dirname, *addr_dirname, *todo_dirname;
    int n;
    GtkWidget *calendarmenu, *todomenu, *addressbookmenu;
    GtkWidget *menuitem;

    // The current active connection, if exists
    evoconn = g_malloc0(sizeof(eds_conn_t));
    evoconn->sync_pair = pair;
    
    /* 
    FIXME
    evoconn->conntype = type;
    evo_load_state(evoconn); 
    */
    
    evowindow = create_optwin();
    
    cal_dirname = g_strdup_printf("%s/.evolution/calendar", 
                                  g_get_home_dir());
    addr_dirname = g_strdup_printf("%s/.evolution/addressbook", 
                                   g_get_home_dir());
    todo_dirname = g_strdup_printf("%s/.evolution/tasks", 
                                   g_get_home_dir());
    
    calendarlist = read_filelist(NULL, cal_dirname, NULL, "calendar.ics", 5);
    todolist = read_filelist(NULL, todo_dirname, NULL, "tasks.ics", 5);
    addressbooklist = read_filelist(NULL, addr_dirname, NULL, "addressbook.db", 5);
    
    g_free(cal_dirname);
    g_free(addr_dirname);
    g_free(todo_dirname);

    calendarmenu = gtk_menu_new ();
    for (n = 0; n < g_list_length(calendarlist); n++) 
    {
      evo_sync_file *ef = g_list_nth_data(calendarlist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                          GTK_SIGNAL_FUNC (calendar_selected),
                          (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (calendarmenu), menuitem);
      
      if ((evoconn->calendarpath && !strcmp(evoconn->calendarpath, ef->path))
          || (!evoconn->calendarpath && n == 0)) 
      {
        gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
        gtk_menu_set_active (GTK_MENU(calendarmenu), n);
      }
    }
    
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
                              "calendarmenu")), 
                              calendarmenu);
    gtk_widget_show_all(GTK_WIDGET(calendarmenu));

    todomenu = gtk_menu_new ();
    
    for (n = 0; n < g_list_length(todolist); n++) 
    {
      evo_sync_file *ef = g_list_nth_data(todolist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                          GTK_SIGNAL_FUNC (todo_selected),
                          (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (todomenu), menuitem);
      
      if ((evoconn->todopath && !strcmp(evoconn->todopath, ef->path))
          || (!evoconn->todopath && n == 0)) 
      {
        gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
        gtk_menu_set_active (GTK_MENU(todomenu), n);
      }
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
                              "todomenu")), 
                              todomenu);
    gtk_widget_show_all(GTK_WIDGET(todomenu));

    addressbookmenu = gtk_menu_new ();
    
    for (n = 0; n < g_list_length(addressbooklist); n++) 
    {
      evo_sync_file *ef = g_list_nth_data(addressbooklist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                          GTK_SIGNAL_FUNC (addressbook_selected),
                          (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (addressbookmenu), menuitem);
      
      if ((evoconn->addressbookpath && !strcmp(evoconn->addressbookpath, ef->path))
          || (!evoconn->addressbookpath && n == 0)) 
      {
        gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
        gtk_menu_set_active (GTK_MENU(addressbookmenu), n);
      }
    }
    
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
                              "addressbookmenu")), 
                              addressbookmenu);
    gtk_widget_show_all(GTK_WIDGET(addressbookmenu));

    gtk_widget_show (evowindow);
  }
  return(evowindow);
}


void evo2_window_closed(void) 
{
  sync_plugin_window_closed();
  if (evoconn) 
  {
    if (evoconn->calendarpath)
      g_free(evoconn->calendarpath);
    if (evoconn->addressbookpath)
      g_free(evoconn->addressbookpath);
    if (evoconn->todopath)
      g_free(evoconn->todopath);
    g_free(evoconn);
  }
  
  evowindow = NULL;
  evoconn = NULL;
}

void evo2_window_ok(void) 
{
  evo2_save_config(evoconn);

  gtk_widget_destroy(evowindow);
  evo2_window_closed();
}

void evo2_window_cancel(void) 
{
  gtk_widget_destroy(evowindow);
  sync_plugin_window_closed();
  evo2_window_closed();
}

void evo2_display_error(char *msg) 
{
  GtkWidget *evo_msgbox;

  evo_msgbox = gnome_message_box_new (msg,
              GNOME_MESSAGE_BOX_ERROR,
              GNOME_STOCK_BUTTON_OK, NULL);
  gtk_widget_show(evo_msgbox);
}

gboolean evo2_do_display_error(gpointer msg) 
{
  evo_display_error(msg);
  g_free(msg);
  return(FALSE);
}

void evo2_async_display_error(char *msg) 
{
  g_idle_add(evo2_do_display_error, g_strdup(msg));
}
