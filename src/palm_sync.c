#include "palm_sync.h"
#include "vcard.h"
#include "xml.h"

static GMutex *piMutex = NULL;
gboolean dbCreated = FALSE;
gboolean tryConnecting = TRUE;
int dialogShowing = 0;
//GtkWidget *dialogConnecting = NULL;
//extern void async_add_pairlist_log(sync_pair *pair, char* logstring, sync_log_type type);


/***********************************************************************
 *
 * Function:    piMutex_create
 *
 * Summary:   initialize the global mutex to protect the libpisock
 *
 ***********************************************************************/
void piMutex_create()
{
	g_assert (piMutex == NULL);
	piMutex = g_mutex_new ();
}

/***********************************************************************
 *
 * Function:    palm_debug
 *
 * Summary:   used to write Debug messages where leve = 0 is the highest
 *			(error) and 4 is the lowest (full debug)
 *
 ***********************************************************************/
void palm_debug(palm_connection *conn, int level, char *message, ...)
{
	va_list arglist;
	char buffer[4096];
	int debuglevel = conn->debuglevel;

	if (level > debuglevel) { return; }
	va_start(arglist, message);
	vsprintf(buffer, message, arglist);

	switch (level) {
		case 0:
			//Error
			printf("[palm-sync] ERROR: %s\n", buffer);
			break;
		case 1:
			//Warning
			printf("[palm-sync] WARNING: %s\n", buffer);
			break;
		case 2:
			//Information
			printf("[palm-sync] INFORMATION: %s\n",  buffer);
			break;
		case 3:
			//debug
			printf("[palm-sync] DEBUG: %s\n", buffer);
			break;
		case 4:
			//fulldebug
			printf("[palm-sync] FULL DEBUG: %s\n", buffer);
			break;
	}
	va_end(arglist);
}

gboolean pingfunc(gpointer data)
{
	palm_connection *conn = data;

	if (!conn->socket)
		return FALSE;

	if (g_mutex_trylock(piMutex) == FALSE) {
		palm_debug(conn, 3, "Ping: Mutex locked!");
		return TRUE;
	}

	if (pi_tickle(conn->socket) < 0) {
		palm_debug(conn, 1, "Ping: Error");
	} else {
		palm_debug(conn, 3, "Ping");
	}

	g_mutex_unlock(piMutex);
	return TRUE;
}

/*
void cancelButton(void *data)
{
	printf("Cancel\n");
	if (data)
		gtk_widget_destroy((GtkWidget *)data);
	tryConnecting = FALSE;
}

gboolean showDialogConnecting(void *data)
{
	dialogConnecting = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, "Please connect device and press the HotSync button or cancel the operation.");
	gtk_signal_connect (GTK_OBJECT (dialogConnecting), "response", G_CALLBACK (cancelButton), dialogConnecting);
	gtk_widget_show(dialogConnecting);
	return FALSE;
}*/

int connectDevice(palm_connection *conn, gboolean block, gboolean popup)
{
	struct pi_sockaddr addr;
	int listen_sd, pf;
	char rate_buf[128];
	long timeout;

	if (conn->conntype != PILOT_DEVICE_NETWORK) {
		g_snprintf (rate_buf,128,"PILOTRATE=%i", conn->speed);
		palm_debug(conn, 2, "setting PILOTRATE=%i", conn->speed);
		putenv (rate_buf);
	}

	switch (conn->conntype) {
		case PILOT_DEVICE_SERIAL:
			pf = PI_PF_PADP;
			break;
		case PILOT_DEVICE_USB_VISOR:
			pf = PI_PF_NET;
			break;
		case PILOT_DEVICE_IRDA:
			pf = PI_PF_PADP;
			break;
		case PILOT_DEVICE_NETWORK:
			pf = PI_PF_NET;
			break;
		default:
			pf = PI_PF_DLP;
			break;
	}

	if (!(listen_sd = pi_socket (PI_AF_PILOT, PI_SOCK_STREAM, pf))) {
		palm_debug(conn, 0, "pi_socket: %s", strerror (errno));
		return -1;
	}

	addr.pi_family = PI_AF_PILOT;

	if (conn->conntype == PILOT_DEVICE_NETWORK) {
		conn->sockaddr = "net:any";
	}

	strcpy (addr.pi_device, conn->sockaddr);

	if (pi_bind (listen_sd, (struct sockaddr*)&addr, sizeof (addr)) == -1) {
		palm_debug(conn, 0, "Unable to bind to pilot");
		pi_close(listen_sd);
		return -2;
	}

	if (pi_listen (listen_sd, 1) != 0) {
		palm_debug(conn, 0, "pi_listen: %s", strerror (errno));
		pi_close(listen_sd);
		return -3;
	}

	//sync_set_pair_status(conn->handle, "Press \"Hotsync\" now.");

	tryConnecting = TRUE;
	if (popup) {
		//g_idle_add(showDialogConnecting, NULL);
	}

	if (!block) {
		timeout = 0;
		while (tryConnecting) {
			conn->socket = pi_accept_to(listen_sd, 0, 0, 100);
			timeout += 100;
			if (timeout > conn->timeout * 1000 && conn->timeout > 0) {
				palm_debug(conn, 1, "pi_accept_to: timeout");
				palm_debug(conn, 1, "pi_accept_to: timeout was %i secs", conn->timeout);
				pi_close(listen_sd);
				//if (popup && dialogConnecting)
				//	gtk_widget_destroy(dialogConnecting);
				return -4;
			}
			if (conn->socket == -1) {
				//while (gtk_events_pending ())
				//	gtk_main_iteration();
				continue;
			}
			if (conn->socket < -1) {
				palm_debug(conn, 0, "Unable to accept() listen socket");
				pi_close(listen_sd);
				//if (popup && dialogConnecting)
				//	gtk_widget_destroy(dialogConnecting);
				return -5;
			}
			//if (popup && dialogConnecting)
			//	gtk_widget_destroy(dialogConnecting);
			break;
		}
	} else {
		conn->socket = pi_accept_to(listen_sd, 0, 0, conn->timeout * 1000);
		if (conn->socket == -1) {
			palm_debug(conn, 1, "pi_accept_to: %s", strerror (errno));
			palm_debug(conn, 1, "pi_accept_to: timeout was %i secs", conn->timeout);
			pi_close(listen_sd);
			return -6;
		}
	}

	pi_close(listen_sd);
	if (!tryConnecting)
		return -7;
	return 0;
}

/*
gboolean showDialogMismatch(void *data)
{
	GtkWidget *dialog = NULL;
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, (char *)data);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		dialogShowing = 1;
	} else {
		dialogShowing = 2;
	}
	gtk_widget_destroy (dialog);
	return FALSE;

}*/

char *get_config(char *path)
{
	char buffer[1024];
	char *filename = NULL;
	
	filename = g_strdup_printf ("%s/palm-sync.conf", path);
	FILE *file = fopen(filename, "r");
	if (!file)
		return NULL;
	fgets(buffer, 255, file);
	fclose(file);
	g_free(filename);
	return g_strdup(buffer);
}

void store_config(char *data, char *path)
{
	char *filename = NULL;
	filename = g_strdup_printf ("%s/palm-sync.conf", path);
	FILE *file = fopen(filename, "w");
	fputs(data, file);
	fclose(file);
	g_free(filename);
}

palm_connection *initialize(OSyncMember *member, char *path)
{
	palm_connection *conn = g_malloc0(sizeof(palm_connection));
	char *filename = NULL;
	filename = g_strdup_printf ("%s/file-sync.conf", path);
	load_palm_settings(conn, filename);
	conn->member = member;
	return conn;
}

void connect(palm_connection *conn, OSyncContext *context){
	struct PilotUser User;
	gchar *txt;

	palm_debug(conn, 3, "start: sync_connect");

	//now connect with the palm
	if (connectDevice(conn, FALSE, conn->popup)) {
		goto failed;
	}

	//check the user
	if (dlp_ReadUserInfo(conn->socket, &User) >= 0) {
		if (User.userID == 0)
			strcpy(User.username, "");
		palm_debug(conn, 2, "User: %s, %i\n", User.username, User.userID);
		if (strcmp(User.username, conn->username) || User.userID != conn->id) {
			//Id or username mismatch
			switch (conn->mismatch) {
				case 0:
					break;
				case 1:
					//ask
					dialogShowing = 0;
					printf("The username \"%s\" or the ID %i on the device  did not match the username or ID from this syncpair.\nPress \"Cancel\" to abort the synchronization.\n\"OK\" to sync anyway", User.username, (int)User.userID);
					//g_idle_add(showDialogMismatch, txt);
					//while (!dialogShowing) {
					//	usleep(100000);
					//}
					//if (dialogShowing == 2) {
					//	palm_debug(conn, 0, "Sync aborted because of User mismatch");
					//	goto failed;
					//}
					break;
				case 2:
					palm_debug(conn, 0, "Sync aborted because of User mismatch");
					goto failed;
					break;
			}
		}
	} else {
		palm_debug(conn, 0, "Unable to read UserInfo");
		goto failed;
	}

	//init the mutex
	piMutex_create();
	//Add the ping function every 5 secs
	g_timeout_add(5000, pingfunc, conn);

	srand(time(NULL));
	palm_debug(conn, 3, "end: sync_connect");
	osync_context_report_success(context);
	return;

	failed:
		if(conn->socket) {
			dlp_EndOfSync(conn->socket, 0);
			pi_close(conn->socket);
		}
		conn->socket = 0;
		osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory");
}

/***********************************************************************
*
* Function:    CloseDB
*
* Summary:   closes the currently open DB and unsets the conn variables
*
 ***********************************************************************/
void CloseDB(palm_connection *conn)
{
	dlp_CloseDB(conn->socket, conn->database);
	strcpy(conn->databasename, "");
	conn->database = 0;
}

/***********************************************************************
*
* Function:    openDB
*
* Summary:   tries to open a given DB. if it is already open it does nothing.
*			if another db is opened it closes this one first.
*
* Returns:	0 on success
*			-1 on unable to find DB
*			-2 on unable to open DB
*
 ***********************************************************************/
int openDB(palm_connection *conn, char *name)
{
	struct DBInfo dbInfo;
	memset(&dbInfo, 0, sizeof(struct DBInfo));

	if (conn->database) {
		if (strcmp(conn->databasename, name) != 0) {
			//We have another DB open. close it first
			palm_debug(conn, 2, "OpenDB called, closing %s first", conn->databasename);
			CloseDB(conn);
		} else {
			//It was already open
			return 0;
		}
	}

 	//Search it
	if (dlp_FindDBInfo(conn->socket, 0, 0, name, 0, 0, &dbInfo) < 0) {
		palm_debug(conn, 1, "Unable to locate %s. Assuming it has been reset", name);
		return -1;
	}

	//open it
	if (dlp_OpenDB(conn->socket, 0, dlpOpenReadWrite, name, &conn->database) < 0) {
		palm_debug(conn, 0, "Unable to open %s", name);
		conn->database = 0;
		return -2;
	}

	palm_debug(conn, 2, "Successfully opened %s", name);
	strcpy(conn->databasename, name);

	return 0;
}


#define CATCOUNT 16
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/***********************************************************************
 *
 * Function:    get_category_name_from_id
 *
 * Summary:   gets the installed categories as a GList
 *
 ***********************************************************************/
gchar *get_category_name_from_id(palm_connection *conn, int id)
{	
	int size;
	unsigned char buf[65536];
	struct CategoryAppInfo cai;
	int r;

	if (id == 0)
		return NULL;
	
	/* buffer size passed in cannot be any larger than 0xffff */
	size = dlp_ReadAppBlock(conn->socket, conn->database, 0, buf, min(sizeof(buf), 0xFFFF));
	if (size<=0) {
		palm_debug(conn, 0, "Error reading appinfo block\n");
		return NULL;
	}

	r = unpack_CategoryAppInfo(&cai, buf, size);
	if ((r <= 0) || (size <= 0)) {
		palm_debug(conn, 0, "unpack_AddressAppInfo failed %s %d\n", __FILE__, __LINE__);
		return NULL;
	}
   
	return g_strdup(cai.name[id]);
}

/***********************************************************************
 *
 * Function:    get_category_id_from_name
 *
 * Summary:   gets the installed categories as a GList
 *
 ***********************************************************************/
int get_category_id_from_name(palm_connection *conn, char *name)
{	
	int size;
	unsigned char buf[65536];
	struct CategoryAppInfo cai;
	int r, i;

	if (!name)
		return 0;
	
	/* buffer size passed in cannot be any larger than 0xffff */
	size = dlp_ReadAppBlock(conn->socket, conn->database, 0, buf, min(sizeof(buf), 0xFFFF));
	if (size<=0) {
		palm_debug(conn, 0, "Error reading appinfo block\n");
		return 0;
	}

	r = unpack_CategoryAppInfo(&cai, buf, size);
	if ((r <= 0) || (size <= 0)) {
		palm_debug(conn, 0, "unpack_AddressAppInfo failed %s %d\n", __FILE__, __LINE__);
		return 0;
	}
   
	for (i = 0; i < CATCOUNT; i++) {
		if (cai.name[i][0] != '\0') {
			palm_debug(conn, 3, "remote: cat %d [%s] ID %d renamed %d\n", i, cai.name[i], cai.ID[i], cai.renamed[i]);
			if (!strcmp(cai.name[i], name)) {
				return i;
			}
		}
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    get_changes
 *
 * Summary:   gets called by Multisync. returns a GList with all the changes requested
 *
 ***********************************************************************/
void get_changeinfo(palm_connection *conn, OSyncContext *call)
{
	int l, n, ret, i;
	unsigned char buffer[65536];
	recordid_t id=0;
	int index, size, attr, category;
	palm_entry entry;
	struct  PilotUser User;
	char *database;
	OSyncChangeFormat format;

	//Lock our Mutex
	g_mutex_lock(piMutex);

	dlp_OpenConduit(conn->socket);

	//Loop over everything for every type
	for (i = 0; i < 3; i++) {
		//Addressbook
		if (i == 0) {
			format = OSYNC_VCARD;
			database = "AddressDB";
		} else if (i == 1) {
			format = OSYNC_VCALENDAR;
			database = "DatebookDB";
		} else if (i == 2) {
			format = OSYNC_VTODO;
			database = "ToDoDB";
		} else {
			continue;
		}

		//Open the database
		ret = openDB(conn, database);
		if (ret == -1) {
			//database does not exist. we definetly need a resync for that
			//FIXME
		}
		if (ret != 0) {
			continue;
		}
		l = dlp_ReadAppBlock(conn->socket, conn->database, 0, buffer, 0xffff);


		//Now we choose if we want everything or just the modified stuff FIXME
		/*if (newdbs & type) {
			for (n = 0; dlp_ReadRecordByIndex(conn->socket, conn->database, n, buffer, &id, &size, &attr, &category) >= 0; n++) {
				//we have a record
				unpackEntry(&entry, buffer, l, type);
				entry.category = get_category_name_from_id(conn, category);
				snprintf(entry.uid, 1024, "uid-%s-%ld", database, id);
				palm_debug(conn, 2, "NEWDBS: Found a record on palm: %s", entry.uid);
				changes = g_list_append(changes, add_changed(conn, &entry, SYNC_OBJ_ADDED));
			}
		} else {*/
		while((ret = dlp_ReadNextModifiedRec(conn->socket, conn->database, buffer, &id, &index, &size, &attr, &category)) >= 0) {
			OSyncChange *change = osync_change_new();
			osync_change_set_uid(change, g_strdup_printf("uid-%s-%ld", database, id));
			osync_change_set_format(change, format);
			
			if ((attr &  dlpRecAttrDeleted) || (attr & dlpRecAttrArchived)) {
				if ((attr & dlpRecAttrArchived)) {
					palm_debug(conn, 2, "Archieved\n");
				}
				//we have a deleted record
				osync_change_set_changetype(change, CHANGE_DELETED);
			}

			if (attr & dlpRecAttrDirty) {
				//We have a modified record
				osync_change_set_data(change, buffer, l);
				osync_change_set_changetype(change, CHANGE_MODIFIED);

				palm_debug(conn, 2, "Found a modified record on palm: %s", entry.uid);
			}
			osync_context_report_change(call, change);
			
		}
	}
	
	palm_debug(conn, 2, "Done searching for changes");

	if (dlp_ReadUserInfo(conn->socket, &User) >= 0) {
		if (User.lastSyncPC == 0) {
			//Device has been reseted
			palm_debug(conn, 3, "Detected that the Device has been reset");
			//chinfo->newdbs = SYNC_OBJECT_TYPE_ANY;
		}
	}

	osync_call_set_success(call);
	//palm_debug(conn, 2, "Found %i changes", g_list_length(changes));

	osync_call_answer(call);
	//Unlock our Mutex
	g_mutex_unlock(piMutex);
}
 
/***********************************************************************
 *
 * Function:    syncobj_modify
 *
 * Summary:   opens the database and tries to modify the entry in the given db
 *
 ***********************************************************************/
void add_change(palm_connection *conn, OSyncChange *change, OSyncContext *context)
{
	int l, ret;
	unsigned char buffer[65536];
	unsigned char orig_buffer[65536];
	palm_entry entry;
	palm_entry orig_entry;
	recordid_t id=0;
	int size;
	int attr, category;
	int category_new;
	char *database = NULL;
	int dbhandle;
	entry.catID = 0;
	
	//Lock our Mutex
	g_mutex_lock(piMutex);

	palm_debug(conn, 2, "start: syncobj_modify");
	//palm_debug(conn, 3, "uid: %s\nCOMP: %s\n", uid, comp);

	/*if (!comp) {
		sync_set_requestfailed(conn->handle);
		//Unlock our Mutex
		g_mutex_unlock(piMutex);
		return;
	}*/

	//detect the db needed
	switch (osync_change_get_format(change)) {
		case OSYNC_VCARD:
			database = "AddressDB";
			break;
		case OSYNC_VCALENDAR:
			database = "DatebookDB";
			break;
		case OSYNC_VTODO:
			database = "ToDoDB";
			break;
		default:
			printf("Unsupported\n");
	}

	palm_debug(conn, 2, "Detected vcard to belong to %s", database);

	//open the DB
	ret = openDB(conn, database);
	if (ret == -2) {
		osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unasble to open directory");
		palm_debug(conn, 1, "Unable to modify entry: Unable to open DB %s", database);
		//Unlock our Mutex
		g_mutex_unlock(piMutex);
		return;
	}
	if (ret == -1 && (dbCreated == FALSE) && !strcmp(database, "DatebookDB")) {
		//Unlock our Mutex so the palm does not die will the messagebox is open
		dbCreated = TRUE;
		if (dlp_CreateDB(conn->socket, 1684108389, 1145132097, 0, 8, 0, "DatebookDB", &dbhandle) < 0) {
			dlp_AddSyncLogEntry(conn->socket, "Unable to create Calendar.\n");
			palm_debug(conn, 0, "Unable to create Calendar");
			g_mutex_unlock(piMutex);
			osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unsasble to open directory");
			return;
		}
		conn->database = dbhandle;
		dlp_AddSyncLogEntry(conn->socket, "Created Calendar.\n");
		palm_debug(conn, 2, "Created Calendar.");
	}

	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			//Modify a entry
			sscanf(osync_change_get_uid(change), "uid-%[^-]-%ld", database, &id);
			if (dlp_ReadRecordById(conn->socket, conn->database, id, orig_buffer, NULL, &size, &attr, &category) < 0) {
				palm_debug(conn, 1, "Unable to find entry i want to modify");
				osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unsasble to open directory");
				//Unlock our Mutex
				g_mutex_unlock(piMutex);
				return;
			}
			
			//Now retrieve the info about "show in list"
/*			if (!strcmp(database, "AddressDB")) {	
				unpackEntry(&orig_entry, orig_buffer, l, objtype);
				palm_debug(conn, 2, "Using orig \"show in list\": %i", orig_entry.address.showPhone);
				if ((orig_entry.address.showPhone) > 4)
					orig_entry.address.showPhone = 0;
				entry.address.showPhone = orig_entry.address.showPhone;
				//Repack it
				l = pack_Address(&entry.address, buffer, sizeof(buffer));
			}*/
			
			ret = dlp_WriteRecord(conn->socket, conn->database, attr, id, entry.catID, buffer, l, 0);
			if (ret < 0) {
				palm_debug(conn, 0, "Unable to modify entry");
				osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unsasble to open directory");
				//Unlock our Mutex
				g_mutex_unlock(piMutex);
				return;
			}
			break;
		case CHANGE_ADDED:
			//Add a new entry
			palm_debug(conn, 2, "Adding new entry");
			id = 0;
			if (dlp_WriteRecord(conn->socket, conn->database, 0, 0, entry.catID, buffer, l, &id) < 0) {
				palm_debug(conn, 0, "Unable to add new entry");
				osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unsasble to open directory");
				//Unlock our Mutex
				g_mutex_unlock(piMutex);
				return;
			}
			//Make the new uid
			osync_change_set_uid(change, g_strdup_printf("uid-%s-%ld", database, id));
			break;
		case CHANGE_DELETED:
			sscanf(osync_change_get_uid(change), "uid-%[^-]-%ld", database, &id);
		
			if (dlp_DeleteRecord(conn->socket, conn->database, 0,  id) < 0) {
				osync_context_report_error(context, OSYNC_ERROR_FILE_NOT_FOUND, "Unsasble to open directory");
				//Unlock our Mutex
				g_mutex_unlock(piMutex);
				return;
			}
	}

	//Unlock our Mutex
	g_mutex_unlock(piMutex);
	osync_context_report_success(context);
}

/***********************************************************************
 *
 * Function:    sync_done
 *
 * Summary:		gets called by Multisync. opens each DB, reset the sync flags
 *				and write some information to the Palm
 *
 ***********************************************************************/
void sync_done(palm_connection *conn, gboolean success)
{
        struct  PilotUser User;
	int i = 0, ret;
	char *database = NULL;

	//Lock our Mutex
	g_mutex_lock(piMutex);

	if (success == TRUE) {
		//reset change counters
		for (i = 0; i < 3; i++) {
			switch (i) {
				case 0:
					database = "AddressDB";
					break;
				case 1:
					database = "DatebookDB";
					break;
				case 2:
					database = "ToDoDB";
					break;
			}

			if (openDB(conn, database) == 0) {
				palm_debug(conn, 2, "Reseting Sync Flags for %s", database);
				dlp_ResetSyncFlags(conn->socket, conn->database);
				dlp_CleanUpDatabase(conn->socket, conn->database);
				CloseDB(conn);
			}
		}

		//Set the log and sync entries on the palm
		dlp_AddSyncLogEntry(conn->socket, "Sync Successfull\n");
		dlp_AddSyncLogEntry(conn->socket, "Thank you for using\n");
		dlp_AddSyncLogEntry(conn->socket, "Multisync");

		ret = dlp_ReadUserInfo(conn->socket, &User);
		if (ret >= 0) {
			if (User.userID == 0)
				strcpy(User.username, "");
			User.lastSyncPC = 1;
			User.lastSyncDate = time(NULL);
			User.successfulSyncDate = time(NULL);
			if (dlp_WriteUserInfo(conn->socket, &User) < 0) {
				palm_debug(conn, 0, "Unable to write UserInfo");
			} else {
				palm_debug(conn, 2, "Done writing new UserInfo");
			}
		} else {
			palm_debug(conn, 0, "Unable to read UserInfo: %i, %s", ret, dlp_strerror(ret));
		}
	}

	dbCreated = FALSE;

	dlp_EndOfSync(conn->socket, 0);
	palm_debug(conn, 2, "Done syncing");

	//Unlock our Mutex
	g_mutex_unlock(piMutex);
}

/***********************************************************************
 *
 * Function:    sync_disconnect
 *
 * Summary:		gets called by Multisync. Closes the connection with the Palm
 *
 ***********************************************************************/
void disconnect(palm_connection *conn, OSyncContext *context)
{
	//Lock our Mutex
	g_mutex_lock(piMutex);

	if(conn->socket) {
		pi_close(conn->socket);
	}
	conn->socket = 0;
	osync_context_report_success(context);

	//Unlock our Mutex
	g_mutex_unlock(piMutex);
	//Free it
	g_mutex_free(piMutex);
	piMutex = NULL;
}

int type() {
	return 5;
}

char *name() {
	return "palm-sync";
}
