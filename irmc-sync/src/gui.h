#ifndef GUI_H
#define GUI_H
#include <glib.h>
#include <multisync.h>
#include <irmc_sync.h>
#include <gtk/gtk.h>
  

void show_options(irmc_connection *conn);
GtkWidget* open_option_window(sync_pair *pair, connection_type type);
void option_window_closed(void);
void get_bt_options(irmc_connection *conn);
void optionpreset_selected(GtkMenuItem *menuitem, gpointer user_data);
void connectmedium_selected(GtkMenuItem *menuitem, gpointer user_data);
void show_connectmedium(irmc_connection *irmcconn);
void fetch_gui_data(void);
void option_window_ok(void);
void option_window_cancel(void);
void ok_infodialog(void);
void open_infodialog(char *text, void (*okfunction)(void));
void close_infodialog(void);
void search_bt_devices(void);
gboolean bt_units_found(gpointer data);
void async_find_bt_units(gpointer data);
void spawn_bt_search(void);
void search_ir_devices(void);
gboolean ir_units_found(gpointer data);
void async_find_ir_units(gpointer data);
void spawn_ir_search(void);
void device_selected(void);
void device_canceled(void);
void close_unitdialog(void);
gboolean connection_tested(gpointer data);
void async_test_connection(gpointer data);
void spawn_conn_test(void);
void irmc_async_close_infodialog(void);
gboolean irmc_do_closeinfo(gpointer data);
void irmc_async_slowsync_msg(sync_object_type objtype);
gboolean irmc_do_showinfo(gpointer data);
void irmc_age_changed(GtkAdjustment *adjustment);
void irmc_set_cursor(GtkWidget *window, gboolean busy);

#endif
