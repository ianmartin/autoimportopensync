#include "palm_sync.h"
#include "xml.h"
#include <string.h>

GtkWidget *wnd_options = NULL;
palm_connection *conn = NULL;

void messageBox(GtkMessageType msg_type, GtkButtonsType btn_type, char *message, ...)
{
	va_list arglist;
	char buffer[1024];
	va_start(arglist, message);
	vsprintf(buffer, message, arglist);
	GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)wnd_options, GTK_DIALOG_DESTROY_WITH_PARENT, msg_type, btn_type, buffer);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	va_end(arglist);
}

static const guint speedList[] = {9600, 19200, 38400, 57600, 115200, 0};
static const gchar *typeList[] = {"Serial", "USB", "Network", "IrDA", NULL};
int realTypeList[] = {0, 1, 4, 2};

void fill_type_menu (GtkOptionMenu *option_menu, int type)
{
	gint i = 0, n = 0;
	GtkWidget *menu ,*menu_item;

	menu = gtk_menu_new ();

	while (typeList[i]) {
		menu_item = gtk_menu_item_new_with_label (typeList[i]);
		gtk_widget_show (menu_item);
		gtk_object_set_data (GTK_OBJECT (menu_item), "type", GINT_TO_POINTER (realTypeList[i]));
		gtk_menu_append (GTK_MENU (menu),menu_item);
		if (realTypeList[i] == type)
			n = i;
		i++;
	}
	gtk_option_menu_set_menu (option_menu, menu);
	gtk_option_menu_set_history (option_menu, n);
}

void fill_speed_menu (GtkOptionMenu *option_menu, guint default_speed)
{
	gint i = 0, n = 3;
	GtkWidget *menu ,*menu_item;
	gchar buf[20];

	g_return_if_fail (option_menu != NULL);
	g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

	menu = gtk_menu_new ();

	while (speedList[i] != 0) {
		g_snprintf (buf,sizeof (buf),"%d",speedList[i]);
		menu_item = gtk_menu_item_new_with_label (buf);
		gtk_widget_show (menu_item);
		gtk_object_set_data (GTK_OBJECT (menu_item), "speed", GINT_TO_POINTER (speedList[i]));
		gtk_menu_append (GTK_MENU (menu),menu_item);

		if (speedList[i] == default_speed)
			n = i;
		i++;
	}
	gtk_option_menu_set_menu (option_menu, menu);
	gtk_option_menu_set_history (option_menu, n);
}

int set_palm_connection(void)
{
	GtkEntry *txt_sockaddr = (GtkEntry *)lookup_widget(wnd_options, "txt_sockaddr");
	GtkEntry *txt_timeout = (GtkEntry *)lookup_widget(wnd_options, "txt_timeout");
	GtkEntry *txt_id = (GtkEntry *)lookup_widget(wnd_options, "txt_id");
	GtkEntry *txt_username = (GtkEntry *)lookup_widget(wnd_options, "txt_username");

	char *statefile = NULL;
	GtkWidget *item;


	if (!strlen(gtk_entry_get_text(txt_sockaddr))) {
		messageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No device given");
		return 1;
	}

	if (!strlen(gtk_entry_get_text(txt_timeout))) {
		messageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No timeout given");
		return 1;
	}

	statefile = g_strdup(conn->statefile);
	free(conn);
	conn = malloc(sizeof(palm_connection));
	memset(conn, 0, sizeof(conn));

	strcpy(conn->username, gtk_entry_get_text(txt_username));
	conn->id = atoi(gtk_entry_get_text(txt_id));
	conn->sockaddr =  strdup(gtk_entry_get_text(txt_sockaddr));
	conn->timeout =  atoi(gtk_entry_get_text(txt_timeout));
	conn->handle = NULL;
	conn->speed = 56700;
	conn->conntype = 0;
	conn->popup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(wnd_options, "chk_popup")));
	memset(conn->statefile, 0, sizeof(conn->statefile));
	strcpy(conn->statefile, statefile);
	g_free(statefile);

	if (gtk_toggle_button_get_active((GtkToggleButton *)lookup_widget(wnd_options, "opt_sync"))) {
		conn->mismatch = 0;
	} else if (gtk_toggle_button_get_active((GtkToggleButton *)lookup_widget(wnd_options, "opt_ask"))) {
		conn->mismatch = 1;
	} else if (gtk_toggle_button_get_active((GtkToggleButton *)lookup_widget(wnd_options, "opt_abort"))) {
		conn->mismatch = 2;
	}

	if (!strcmp(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry), "Errors Only")) {
		conn->debuglevel = 0;
	}
	if (!strcmp(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry), "Errors and Warnings")) {
		conn->debuglevel = 1;
	}
	if (!strcmp(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry), "Information")) {
		conn->debuglevel = 2;
	}
	if (!strcmp(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry), "Debug")) {
		conn->debuglevel = 3;
	}
	if (!strcmp(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry), "Full Debug")) {
		conn->debuglevel = 4;
	}

	item = gtk_option_menu_get_menu (GTK_OPTION_MENU (lookup_widget(wnd_options, "om_speed")));
	item = gtk_menu_get_active (GTK_MENU (item));
	conn->speed = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "speed"));

	item = gtk_option_menu_get_menu (GTK_OPTION_MENU (lookup_widget(wnd_options, "om_type")));
	item = gtk_menu_get_active (GTK_MENU (item));
	conn->conntype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (item), "type"));

	sscanf(gtk_entry_get_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_codepage"))->entry), "%s", conn->codepage);

	return 0;
}

GtkWidget* open_option_window(sync_pair *pair, connection_type type)
{
	char id[1024];
	char timeout[1024];

	//Create and show the Options window
	wnd_options = create_wnd_options();
	gtk_widget_show (wnd_options);

	//Create a new connection
	conn = malloc(sizeof(palm_connection));
	memset(conn, 0, sizeof(conn));
	conn->debuglevel = 0;
	conn->handle = NULL;
	sprintf(conn->statefile, "%s/%ssettings", sync_get_datapath(pair), (type==CONNECTION_TYPE_LOCAL?"local":"remote"));

	if (!load_palm_settings(conn)) {
		snprintf(id, 1024, "%i", conn->id);
		sprintf(timeout, "%i", conn->timeout);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_id"), id);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_username"), conn->username);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_sockaddr"), conn->sockaddr);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_timeout"), timeout);

		switch (conn->debuglevel) {
			case 0:
				gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry, "Errors Only");
				break;
			case 1:
				gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry, "Errors and Warnings");
				break;
			case 2:
				gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry, "Information");
				break;
			case 3:
				gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry, "Debug");
				break;
			case 4:
				gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_debug"))->entry, "Full Debug");
				break;
		}

		switch (conn->mismatch) {
			case 0:
				gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget(wnd_options, "opt_sync"), TRUE);
				break;
			case 1:
				gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget(wnd_options, "opt_ask"), TRUE);
				break;
			case 2:
				gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget(wnd_options, "opt_abort"), TRUE);
				break;
		}

		if (conn->popup) {
			gtk_toggle_button_set_active ((GtkToggleButton *)lookup_widget(wnd_options, "chk_popup"), TRUE);
		}

		fill_speed_menu ((GtkOptionMenu *)lookup_widget(wnd_options, "om_speed"), conn->speed);
		fill_type_menu ((GtkOptionMenu *)lookup_widget(wnd_options, "om_type"), conn->conntype);
	
		gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_codepage"))->entry, conn->codepage);
	} else {
		fill_speed_menu ((GtkOptionMenu *)lookup_widget(wnd_options, "om_speed"), 57600);
		fill_type_menu ((GtkOptionMenu *)lookup_widget(wnd_options, "om_type"), 0);
		gtk_entry_set_text((GtkEntry *)((GtkCombo *)lookup_widget(wnd_options, "cmb_codepage"))->entry, "cp1252 (Latin) Standard");
	}

	return(wnd_options);
}

void on_wnd_options_destroy (GtkObject *object, gpointer user_data)
{
	sync_plugin_window_closed();
}


void on_btn_cancel_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(wnd_options);
	wnd_options = NULL;
}

void on_btn_ok_clicked (GtkButton *button, gpointer user_data)
{
	if (set_palm_connection()) {
		return;
	}

	save_palm_settings(conn);

	gtk_widget_destroy(wnd_options);
	wnd_options = NULL;
}

void
on_btn_getUsername_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	struct PilotUser User;
	char id[1024];

	if (set_palm_connection()) {
		return;
	}

	if (connectDevice(conn, FALSE, TRUE)) {
		return;
	}

	if (dlp_ReadUserInfo(conn->socket, &User) >= 0) {
		if (User.userID == 0)
			strcpy(User.username, "");
		palm_debug(conn, 2, "User: %s, %i\n", User.username, User.userID);
		snprintf(id, 1024, "%i", (int)User.userID);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_id"), id);
		gtk_entry_set_text((GtkEntry *)lookup_widget(wnd_options, "txt_username"), g_convert(User.username, strlen(User.username), "utf8", "cp1252", NULL, NULL, NULL));
	} else {
		palm_debug(conn, 0, "Unable to read UserInfo");
	}

	dlp_EndOfSync(conn->socket, 0);

	if(conn->socket) {
		pi_close(conn->socket);
	}
	conn->socket = 0;
}


void
on_btn_setUsername_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	struct PilotUser User;
	char *username = NULL;
	username = strdup(gtk_entry_get_text((GtkEntry *)lookup_widget(wnd_options, "txt_username")));

	if (!strlen(username)) {
		messageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Please enter a new username");
		return;
	}

	if (!strlen(gtk_entry_get_text((GtkEntry *)lookup_widget(wnd_options, "txt_id"))) || atoi(gtk_entry_get_text((GtkEntry *)lookup_widget(wnd_options, "txt_id"))) == 0) {
		messageBox(GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Please enter a id except 0");
		return;
	}

	if (set_palm_connection()) {
		return;
	}

	if (connectDevice(conn, FALSE, TRUE)) {
		return;
	}

	if (dlp_ReadUserInfo(conn->socket, &User) >= 0) {
		strcpy(User.username, g_convert(username, strlen(username), "cp1252" ,"utf8", NULL, NULL, NULL));
		User.userID = atoi(gtk_entry_get_text((GtkEntry *)lookup_widget(wnd_options, "txt_id")));
		if (dlp_WriteUserInfo(conn->socket, &User) < 0) {
			palm_debug(conn, 0, "Unable to write UserInfo");
		} else {
			palm_debug(conn, 2, "Done writing new UserInfo");
		}
	} else {
		palm_debug(conn, 0, "Unable to read UserInfo");
	}

	dlp_EndOfSync(conn->socket, 0);

	if(conn->socket) {
		pi_close(conn->socket);
	}
	conn->socket = 0;
}
