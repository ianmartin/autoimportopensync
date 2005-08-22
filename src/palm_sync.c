#include "palm_sync.h"

osync_bool psyncSettingsParse(PSyncEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	//set defaults
	env->sockaddr = g_strdup("/dev/pilot");
	env->username = g_strdup("");
	env->codepage = g_strdup("cp1252");
	env->id = 0;
	env->debuglevel = 0;
	env->conntype = 0;
	env->speed = 57600;
	env->timeout = 2;
	env->mismatch = 1;
	env->popup = 0;

	doc = xmlParseMemory(config, size);

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		goto error;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		goto error_free_doc;
	}

	if (xmlStrcmp(cur->name, "config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"sockaddr")) {
				g_free(env->sockaddr);
				env->sockaddr = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
				g_free(env->username);
				env->username = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"codepage")) {
				g_free(env->codepage);
				env->codepage = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"timeout")) {
				env->timeout = atoi(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
				env->conntype = atoi(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"speed")) {
				env->speed = atoi(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"id")) {
				env->id = atoi(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"popup")) {
				env->popup = atoi(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"mismatch")) {
				env->mismatch = atoi(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	if (env->conntype == PILOT_DEVICE_NETWORK) {
		if (env->sockaddr)
			g_free(env->sockaddr);
		env->sockaddr = "net:any";
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_doc:
		xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static GMutex *piMutex = NULL;
gboolean dbCreated = FALSE;
gboolean tryConnecting = TRUE;

/** @brief initialize the global mutex to protect the libpisock
 *
 */
void piMutex_create()
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);
	
	g_assert (piMutex == NULL);
	piMutex = g_mutex_new ();
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

gboolean pingfunc(gpointer data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = data;

	if (!env->socket)
		return FALSE;

	if (g_mutex_trylock(piMutex) == FALSE) {
		osync_trace(TRACE_EXIT, "Ping: Mutex locked!");
		return TRUE;
	}

	if (pi_tickle(env->socket) < 0) {
		osync_trace(TRACE_INTERNAL, "Ping: Error");
	} else {
		osync_trace(TRACE_INTERNAL, "Ping");
	}

	g_mutex_unlock(piMutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static osync_bool _connectDevice(PSyncEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	struct pi_sockaddr addr;
	int listen_sd, pf;
	char *rate_buf = NULL;
	//long timeout;

	if (env->conntype != PILOT_DEVICE_NETWORK) {
		rate_buf = g_strdup_printf("PILOTRATE=%i", env->speed);
		osync_trace(TRACE_INTERNAL, "setting PILOTRATE=%i", env->speed);
		putenv(rate_buf);
		g_free(rate_buf);
	}

	switch (env->conntype) {
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
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create listen sock");
		goto error;
	}

	addr.pi_family = PI_AF_PILOT;

	strcpy(addr.pi_device, env->sockaddr);

	struct pi_sockaddr *tmpaddr = &addr;
	if (pi_bind(listen_sd, (struct sockaddr*)tmpaddr, sizeof (addr)) == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to bind to pilot");
		goto error_free_listen;
	}
	
	if (pi_listen (listen_sd, 1) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to listen: %s", strerror (errno));
		goto error_free_listen;
	}

	env->socket = pi_accept_to(listen_sd, 0, 0, env->timeout * 1000);
	if (env->socket == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to accept: %s", strerror (errno));
		goto error_free_listen;
	}

	pi_close(listen_sd);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_listen:
	pi_close(listen_sd);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void *psyncInitialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);

	PSyncEnv *env = osync_try_malloc0(sizeof(PSyncEnv), error);
	if (!env)
		goto error;
		
	char *configdata = NULL;
	unsigned int configsize = 0;
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto error_free_env;
	}
	
	if (!psyncSettingsParse(env, configdata, configsize, error))
		goto error_free_config;
	
	env->member = member;
	g_free(configdata);
	
	osync_trace(TRACE_EXIT, "%s, %p", __func__, env);
	return (void *)env;

error_free_config:
	g_free(configdata);
error_free_env:
	g_free(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void psyncConnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;
	struct PilotUser User;

	//now connect with the palm
	if (!_connectDevice(env, &error))
		goto error;

	//check the user
	if (dlp_ReadUserInfo(env->socket, &User) >= 0) {
		if (User.userID == 0)
			strcpy(User.username, "");
			
		osync_trace(TRACE_INTERNAL, "User: %s, %i\n", User.username, User.userID);
		/*if (strcmp(User.username, conn->username) || User.userID != conn->id) {
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
		}*/
	} else {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to read UserInfo");
		goto error;
	}

	//init the mutex
	piMutex_create();
	
	//Add the ping function every 5 secs
	//g_timeout_add(5000, pingfunc, conn);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (env->socket) {
		dlp_EndOfSync(env->socket, 0);
		pi_close(env->socket);
		env->socket = 0;
	}
	
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/***********************************************************************
*
* Function:    CloseDB
*
* Summary:   closes the currently open DB and unsets the conn variables
*
 ***********************************************************************/
static void _closeDB(PSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	
	dlp_CloseDB(env->socket, env->database);
	strcpy(env->databasename, "");
	env->database = 0;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool _openDB(PSyncEnv *env, char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, name, error);
	
	struct DBInfo dbInfo;
	memset(&dbInfo, 0, sizeof(struct DBInfo));

	if (env->database) {
		if (strcmp(env->databasename, name) != 0) {
			//We have another DB open. close it first
			_closeDB(env);
		} else {
			//It was already open
			osync_trace(TRACE_EXIT, "%s: Already opened", __func__);
			return TRUE;
		}
	}

 	//Search it
	if (dlp_FindDBInfo(env->socket, 0, 0, name, 0, 0, &dbInfo) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to locate %s. Assuming it has been reset", name);
		goto error;
	}

	//open it
	if (dlp_OpenDB(env->socket, 0, dlpOpenReadWrite, name, &env->database) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open %s", name);
		env->database = 0;
		goto error;
	}

	g_free(env->databasename);
	env->databasename = g_strdup(name);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
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
char *get_category_name_from_id(PSyncEnv *env, int id, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, env, id, error);
	
	int size;
	unsigned char buf[65536];
	struct CategoryAppInfo cai;
	int r;

	if (id == 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Id was null");
		goto error;
	}
	
	/* buffer size passed in cannot be any larger than 0xffff */
	size = dlp_ReadAppBlock(env->socket, env->database, 0, buf, min(sizeof(buf), 0xFFFF));
	if (size <= 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error reading appinfo block");
		goto error;
	}

	r = unpack_CategoryAppInfo(&cai, buf, size);
	if ((r <= 0) || (size <= 0)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "unpack_AddressAppInfo failed");
		goto error;
	}

	char *ret = g_strdup(cai.name[id]);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, ret);
	return ret;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/***********************************************************************
 *
 * Function:    get_category_id_from_name
 *
 * Summary:   gets the installed categories as a GList
 *
 ***********************************************************************/
int get_category_id_from_name(PSyncEnv *env, const char *name, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, name, error);
	
	int size;
	unsigned char buf[65536];
	struct CategoryAppInfo cai;
	int r, i;

	if (!name) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Name was null");
		goto error;
	}
	
	/* buffer size passed in cannot be any larger than 0xffff */
	size = dlp_ReadAppBlock(env->socket, env->database, 0, buf, min(sizeof(buf), 0xFFFF));
	if (size<=0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error reading appinfo block");
		goto error;
	}

	r = unpack_CategoryAppInfo(&cai, buf, size);
	if ((r <= 0) || (size <= 0)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "unpack_AddressAppInfo failed");
		goto error;
	}
   
	for (i = 0; i < CATCOUNT; i++) {
		if (cai.name[i][0] != '\0') {
			osync_trace(TRACE_INTERNAL, "remote: cat %d [%s] ID %d renamed %d", i, cai.name[i], cai.ID[i], cai.renamed[i]);
			if (!strcmp(cai.name[i], name)) {
				osync_trace(TRACE_EXIT, "%s: %i", i);
				return i;
			}
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return 0;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return 0;
}

void psyncGetChangeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	
	int l, ret, i;
	unsigned char buffer[65536];
	recordid_t id=0;
	int index, size, attr, category;
	palm_entry entry;
	struct  PilotUser User;
	char *database;
	const char *format = NULL;
	
	//Lock our Mutex
	g_mutex_lock(piMutex);

	dlp_OpenConduit(env->socket);

	//Loop over everything for every type
	for (i = 0; i < 3; i++) {
		//Addressbook
		if (i == 0) {
			format = "palm-contact";
			database = "AddressDB";
		} else if (i == 1) {
			format = "palm-event";
			database = "DatebookDB";
		} else if (i == 2) {
			format = "palm-todo";
			database = "ToDoDB";
		} else {
			continue;
		}

		//Open the database
		SmlError *error = NULL;
		if (!_openDB(env, database, &error)) {
			//database does not exist. we definetly need a resync for that
			//FIXME
			continue;
		}
		
		l = dlp_ReadAppBlock(env->socket, env->database, 0, buffer, 0xffff);


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
		while((ret = dlp_ReadNextModifiedRec(env->socket, env->database, buffer, &id, &index, &size, &attr, &category)) >= 0) {
			OSyncChange *change = osync_change_new();
			osync_change_set_uid(change, g_strdup_printf("uid-%s-%ld", database, id));
			osync_change_set_objformat_string(change, format);
			
			if ((attr &  dlpRecAttrDeleted) || (attr & dlpRecAttrArchived)) {
				if ((attr & dlpRecAttrArchived)) {
					osync_trace(TRACE_INTERNAL, "Archieved");
				}
				//we have a deleted record
				osync_change_set_changetype(change, CHANGE_DELETED);
			}

			if (attr & dlpRecAttrDirty) {
				//We have a modified record
				osync_change_set_data(change, buffer, l, TRUE);
				osync_change_set_changetype(change, CHANGE_MODIFIED);

				osync_trace(TRACE_INTERNAL, "Found a modified record on palm: %s", entry.uid);
			}
			osync_context_report_change(ctx, change);
			
		}
	}
	
	osync_trace(TRACE_INTERNAL, "Done searching for changes");

	if (dlp_ReadUserInfo(env->socket, &User) >= 0) {
		if (User.lastSyncPC == 0) {
			//Device has been reseted
			osync_trace(TRACE_INTERNAL, "Detected that the Device has been reset");
			//chinfo->newdbs = SYNC_OBJECT_TYPE_ANY;
		}
	}

	g_mutex_unlock(piMutex);

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

/*error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));*/
}

void psyncCommitChange(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	
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

	//detect the db needed
	if (!strcmp(osync_change_get_objformat(change), "palm-contact")) {
		database = "AddressDB";
	} else if (!strcmp(osync_change_get_objformat(change), "palm-event")) {
		database = "DatebookDB";
	} else if (!strcmp(osync_change_get_objformat(change), "palm-todo")) {
		database = "ToDoDB";
	} else {
		osync_error_set(

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
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/***********************************************************************
 *
 * Function:    sync_done
 *
 * Summary:		gets called by Multisync. opens each DB, reset the sync flags
 *				and write some information to the Palm
 *
 ***********************************************************************/
void psyncDone(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	
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

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/***********************************************************************
 *
 * Function:    sync_disconnect
 *
 * Summary:		gets called by Multisync. Closes the connection with the Palm
 *
 ***********************************************************************/
void psyncDisconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
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

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void psyncFinalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = (filesyncinfo *)data;

	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->name = "palm-sync";
	info->longname = "Palm Synchronization Plugin";
	info->description = "Plugin to synchronize Palm devices";
	info->version = 1;
	
	info->functions.initialize = psyncInitialize;
	info->functions.connect = psyncConnect;
	info->functions.sync_done = psyncDone;
	info->functions.disconnect = psyncDisconnect;
	info->functions.finalize = psyncFinalize;
	info->functions.get_changeinfo = psyncGetChangeinfo;

	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "palm-contact", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "palm-contact", psyncCommitChange);
}
