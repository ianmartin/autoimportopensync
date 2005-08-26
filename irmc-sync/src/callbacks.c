#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "gui.h"

extern irmc_connection *irmcconn;

gboolean
on_optionwin_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  option_window_closed();
  return FALSE;
}


void
on_infook_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  ok_infodialog();
}


gboolean
on_infodialog_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  close_infodialog();
  return FALSE;
}


void
on_searchbutton_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  search_bt_devices();
}


void
on_unitlist_select_row                 (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_object_set_data (GTK_OBJECT (clist), "selected", (gpointer) row);
}



void
on_listok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  device_selected();

}


void
on_listcancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  device_canceled();
}


void
on_okbutton_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  option_window_ok();
}


void
on_cancelbutton_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  option_window_cancel();
}


void
on_testcablebutton_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_testconnbutton_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  spawn_conn_test();
}


void
on_irsearchbutton_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  search_ir_devices();
}




void
on_unitlist_row_activated              (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
  device_selected();
}


void
on_sonyericssoncable_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  irmcconn->cabletype = IRMC_CABLE_ERICSSON;
}


void
on_siemenscable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  irmcconn->cabletype = IRMC_CABLE_SIEMENS;
}

