#include <gnome.h>


gboolean
on_optionwin_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_infook_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_infodialog_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_searchbutton_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_unitlist_select_row                 (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_okbutton_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancelbutton_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_listok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_listcancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_okbutton_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancelbutton_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_testcablebutton_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_testconnbutton_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_irsearchbutton_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_serialportradio0_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_serialportradio1_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_serialportradioother_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
irmc_nooldercheck_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_unitlist_row_activated              (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);

void
on_sonyericssoncable_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_siemenscable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
