/*
 * libopensync-palm-plugin - A palm plugin for opensync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * 
 */

#include "palm_sync.h"
#include "palm_format.h"

#define CATCOUNT 16

#ifdef OLD_PILOT_LINK
#define pi_buffer(b) b
#else
#define pi_buffer(b) b->data
#endif

static void _psyncDBClose(PSyncDatabase *db)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, db);
	
	dlp_CloseDB(db->env->socket, db->handle);
	db->env->currentDB = NULL;
	g_free(db->name);
#ifndef OLD_PILOT_LINK
	pi_buffer_free(db->buffer);
#endif
	g_free(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static PSyncDatabase *_psyncDBOpen(PSyncEnv *env, char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, name, error);
	
	struct DBInfo dbInfo;
	memset(&dbInfo, 0, sizeof(struct DBInfo));

	if (env->currentDB) {
		if (strcmp(env->currentDB->name, name) != 0) {
			//We have another DB open. close it first
			osync_trace(TRACE_INTERNAL, "Other db open, closing first");
			_psyncDBClose(env->currentDB);
		} else {
			//It was already open
			osync_trace(TRACE_EXIT, "%s: Already opened", __func__);
			return env->currentDB;
		}
	}

 	//Search it
	if (dlp_FindDBInfo(env->socket, 0, 0, name, 0, 0, &dbInfo) < 0) {
		osync_error_set(error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to locate %s. Assuming it has been reset", name);
		goto error;
	}

	PSyncDatabase *db = osync_try_malloc0(sizeof(PSyncDatabase), error);
	if (!db)
		goto error;
	db->env = env;
#ifndef OLD_PILOT_LINK
	db->buffer = pi_buffer_new(65536);
#endif
	
	//open it
	if (dlp_OpenDB(env->socket, 0, dlpOpenReadWrite, name, &(db->handle)) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open %s", name);
		goto error_free_db;
	}

#ifdef OLD_PILOT_LINK
	db->size = dlp_ReadAppBlock(env->socket, db->handle, 0, db->buffer, 0xffff);
#else
	db->size = dlp_ReadAppBlock(env->socket, db->handle, 0, 0xffff, db->buffer);
#endif
	if (db->size < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to read %s", name);
		goto error_free_db;
	}
	
	if (unpack_CategoryAppInfo(&(db->cai), pi_buffer(db->buffer), db->size) <= 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "unpack_AddressAppInfo failed");
		goto error;
	}
	
	env->currentDB = db;
	db->name = g_strdup(name);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, db);
	return db;

error_free_db:
#ifndef OLD_PILOT_LINK
	pi_buffer_free(db->buffer);
#endif
	g_free(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static PSyncEntry *_psyncDBGetNthEntry(PSyncDatabase *db, int nth)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, db, nth);
	
	PSyncEntry *entry = osync_try_malloc0(sizeof(PSyncEntry), NULL);
	if (!entry) {
		osync_trace(TRACE_EXIT_ERROR, "No more memory");
		return NULL;
	}
	
	entry->index = nth;
	entry->db = db;
#ifndef OLD_PILOT_LINK
	entry->buffer = pi_buffer_new(65536);
#endif
	
#ifdef OLD_PILOT_LINK
	int ret = dlp_ReadRecordByIndex(db->env->socket, db->handle, nth,
		entry->buffer, &entry->id, &entry->size, &entry->attr,
		&entry->category);
#else
	int ret = dlp_ReadRecordByIndex(db->env->socket, db->handle, nth,
		entry->buffer, &entry->id, &entry->attr, &entry->category);
#endif

	if (ret < 0) {
#ifndef OLD_PILOT_LINK
		pi_buffer_free(entry->buffer);
#endif
		g_free(entry);
		osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
	return entry;
}

static PSyncEntry *_psyncDBGetNextModified(PSyncDatabase *db)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, db);
	
	PSyncEntry *entry = osync_try_malloc0(sizeof(PSyncEntry), NULL);
	if (!entry) {
		osync_trace(TRACE_EXIT_ERROR, "No more memory");
		return NULL;
	}
	
	entry->db = db;
#ifndef OLD_PILOT_LINK
	entry->buffer = pi_buffer_new(65536);
#endif

#ifdef OLD_PILOT_LINK	
	int ret = dlp_ReadNextModifiedRec(db->env->socket, db->handle,
		entry->buffer, &entry->id, &entry->index, &entry->size,
		&entry->attr, &entry->category);
#else
	int ret = dlp_ReadNextModifiedRec(db->env->socket, db->handle,
		entry->buffer, &entry->id, &entry->index, &entry->attr,
		&entry->category);
#endif

	if (ret < 0) {
#ifndef OLD_PILOT_LINK
		pi_buffer_free(entry->buffer);
#endif
		g_free(entry);
		osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
	return entry;
}

static osync_bool _psyncDBDelete(PSyncDatabase *db, int id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, db, id, error);
	
	int ret = dlp_DeleteRecord(db->env->socket, db->handle, 0,  id);
	if (ret < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to delete file: %i", ret);
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _psyncDBWrite(PSyncDatabase *db, PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, db, entry, error);
	
	/* Do not use sizeof (entry->buffer) as this will give 
	   the size of the allocated buffer instead of size of
	   the *packed* buffer 
	*/
	int ret = dlp_WriteRecord(db->env->socket, db->handle, entry->attr, entry->id, entry->category, entry->buffer, entry->size, 0);
	if (ret < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to write file: %i", ret);
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _psyncDBAdd(PSyncDatabase *db, PSyncEntry *entry, unsigned long *id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p %p)", __func__, db, entry, id, error);
	
	/* Do not use sizeof (entry->buffer) as this will give 
	   the size of the allocated buffer instead of size of
	   the *packed* buffer 
	*/
	int ret = dlp_WriteRecord(db->env->socket, db->handle, 0, 0, entry->category, entry->buffer, entry->size, id);
	if (ret < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to add file: %i", ret);
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static const char *_psyncDBCategoryFromId(PSyncDatabase *db, int id, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, db, id, error);

	if (id > CATCOUNT || id < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid id %i", id);
		goto error;
	}
		
	const char *ret = db->cai.name[id];

	osync_trace(TRACE_EXIT, "%s: %s", __func__, ret);
	return ret;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static int _psyncDBCategoryToId(PSyncDatabase *db, const char *name, OSyncError **error)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, db, name, error);
	
	int i = 0;
	for (i = 0; i < CATCOUNT; i++) {
		if (db->cai.name[i][0] != '\0') {
			osync_trace(TRACE_INTERNAL, "remote: cat %d [%s] ID %d renamed %d", i, db->cai.name[i], db->cai.ID[i], db->cai.renamed[i]);
			if (!strcmp(db->cai.name[i], name)) {
				osync_trace(TRACE_EXIT, "%s: %i", __func__, i);
				return i;
			}
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return 0;
}

static osync_bool _connectDevice(PSyncEnv *env, unsigned int timeout, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, env, timeout, error);
	int listen_sd = 0;
	char *rate_buf = NULL;
	int ret = 0;

#ifdef OLD_PILOT_LINK
	struct pi_sockaddr addr;
#endif

	if (env->socket) {
		osync_trace(TRACE_EXIT, "%s: Already connected", __func__);
		return TRUE;
	}

	if (env->conntype != PILOT_DEVICE_NETWORK) {
		rate_buf = g_strdup_printf("PILOTRATE=%i", env->speed);
		osync_trace(TRACE_INTERNAL, "setting PILOTRATE=%i", env->speed);
		putenv(rate_buf);
		g_free(rate_buf);
	}

	osync_trace(TRACE_INTERNAL, "Creating socket");
	if (!(listen_sd = pi_socket (PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to create listen sock");
		goto error;
	}

#ifdef OLD_PILOT_LINK
	addr.pi_family = PI_AF_PILOT;

	strcpy(addr.pi_device, env->sockaddr);

	struct pi_sockaddr *tmpaddr = &addr;
#endif
	
	osync_trace(TRACE_INTERNAL, "Binding socket");

#ifdef OLD_PILOT_LINK
	if ((ret = pi_bind(listen_sd, (struct sockaddr*)tmpaddr, sizeof (addr))) < 0) {
#else
	if ((ret = pi_bind(listen_sd, env->sockaddr)) < 0) {
#endif
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to bind to pilot: %i", ret);
		goto error_free_listen;
	}
	
	osync_trace(TRACE_INTERNAL, "Starting to listen");
	if (pi_listen (listen_sd, 1) != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to listen: %s", strerror (errno));
		goto error_free_listen;
	}

	osync_trace(TRACE_INTERNAL, "Accepting connection");
	env->socket = pi_accept_to(listen_sd, 0, 0, timeout * 1000);
	if (env->socket == -1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to accept: %s", strerror (errno));
		goto error_free_listen;
	}

	osync_trace(TRACE_INTERNAL, "Done");
	pi_close(listen_sd);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_listen:
	pi_close(listen_sd);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static gboolean _psyncPoll(gpointer data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = (PSyncEnv *)data;
	
	if (env->socket > 0) {
		osync_trace(TRACE_EXIT, "%s: Already have a socket", __func__);
		return TRUE;
	}
	
	OSyncError *error = NULL;
	if (_connectDevice(env, 1, &error))
		osync_member_request_synchronization(env->member);
	else
		osync_error_free(&error);
		
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static gboolean _psyncPing(gpointer data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = (PSyncEnv *)data;

	if (!env->socket) {
		osync_trace(TRACE_EXIT, "%s: No socket yet", __func__);
		return TRUE;
	}
	
	if (pi_tickle(env->socket) < 0) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Error", __func__);
		return TRUE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void psyncThreadStart(PSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	
	GMainContext *context = osync_member_get_loop(env->member);
	
	GSource *source = g_timeout_source_new(5000);
	g_source_set_callback(source, _psyncPing, env, NULL);
	g_source_attach(source, context);
	
	source = g_timeout_source_new(1000);
	g_source_set_callback(source, _psyncPoll, env, NULL);
	g_source_attach(source, context);
}

static osync_bool psyncSettingsParse(PSyncEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	//set defaults
	env->sockaddr = g_strdup("/dev/pilot");
	env->username = g_strdup("");
	env->codepage = g_strdup("cp1252");
	env->id = 0;
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

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char *)xmlNodeGetContent(cur);
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

static void *psyncInitialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);

	PSyncEnv *env = osync_try_malloc0(sizeof(PSyncEnv), error);
	if (!env)
		goto error;
		
	char *configdata = NULL;
	int configsize = 0;
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto error_free_env;
	}
	
	if (!psyncSettingsParse(env, configdata, configsize, error))
		goto error_free_config;
	
	env->member = member;
	g_free(configdata);
	
	psyncThreadStart(env);
	
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

	//now connect with the palm
	if (!_connectDevice(env, env->timeout, &error))
		goto error;

	//check the user
	if (dlp_ReadUserInfo(env->socket, &env->user) < 0) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to read UserInfo");
		goto error;
	}
	
	if (env->user.userID == 0)
		strcpy(env->user.username, "");
		
	osync_trace(TRACE_INTERNAL, "User: %s, %i\n", env->user.username, env->user.userID);
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
	
	if (env->user.lastSyncPC == 0) {
		//Device has been reseted
		osync_trace(TRACE_INTERNAL, "Detected that the Device has been reset");
		osync_member_set_slow_sync(env->member, "data", TRUE);
	}
	
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

static OSyncChange *_psyncTodoCreate(PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, entry, error);
	PSyncDatabase *db = entry->db;
	
	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	/* Set the uid */
	char *uid = g_strdup_printf("uid-ToDoDB-%ld", entry->id);
	osync_change_set_uid(change, uid);
	g_free(uid);
	
	/* Set the format */
	osync_change_set_objformat_string(change, "palm-todo");
	
	if ((entry->attr &  dlpRecAttrDeleted) || (entry->attr & dlpRecAttrArchived)) {
		if ((entry->attr & dlpRecAttrArchived)) {
			osync_trace(TRACE_INTERNAL, "Archieved");
		}
		//we have a deleted record
		osync_change_set_changetype(change, CHANGE_DELETED);
	} else {
		/* Create the object data */
		PSyncTodoEntry *todo = osync_try_malloc0(sizeof(PSyncTodoEntry), error);
		if (!todo)
			goto error_free_change;
		todo->codepage = g_strdup(db->env->codepage);
		
		osync_trace(TRACE_INTERNAL, "Starting to unpack entry %i", db->size);
		unpack_ToDo(&(todo->todo), entry->buffer, db->size);
	    const char *catname = _psyncDBCategoryFromId(entry->db, entry->category, NULL);
	    if (catname)
			todo->categories = g_list_append(todo->categories, g_strdup(catname));
			
		osync_change_set_data(change, (void *)todo, sizeof(PSyncTodoEntry), TRUE);
		
		if (entry->attr & dlpRecAttrDirty) {
			osync_change_set_changetype(change, CHANGE_MODIFIED);
		} else {
			osync_change_set_changetype(change, CHANGE_UNKNOWN);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;
	
error_free_change:
	osync_change_free(change);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool _psyncTodoGetChangeInfo(OSyncContext *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	
	if (!(db = _psyncDBOpen(env, "ToDoDB", error)))
		goto error;
	
	if (osync_member_get_slow_sync(env->member, "todo") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		
		int n;
		for (n = 0; (entry = _psyncDBGetNthEntry(db, n)); n++) {
			osync_trace(TRACE_INTERNAL, "Got all recored with id %ld", entry->id);
			
			OSyncChange *change = _psyncTodoCreate(entry, error);
			if (!change)
				goto error;
			
			osync_change_set_changetype(change, CHANGE_ADDED);
			osync_context_report_change(ctx, change);
		}
	} else {
		while ((entry = _psyncDBGetNextModified(db))) {
			OSyncChange *change = _psyncTodoCreate(entry, error);
			if (!change)
				goto error;
			
			osync_context_report_change(ctx, change);
		}
	}
	
	if (osync_error_is_set(error))
		goto error_close_db;
	
	_psyncDBClose(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_close_db:
	_psyncDBClose(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool psyncTodoCommit(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	PSyncTodoEntry *todo = NULL;
	OSyncError *error = NULL;
	unsigned long id = 0;

	//open the DB
	if (!(db = _psyncDBOpen(env, "ToDoDB", &error)))
		goto error;
	
	todo = (PSyncTodoEntry *)osync_change_get_data(change);
			
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			//Get the id
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);

			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, sizeof(entry->buffer));
	
			if (!_psyncDBWrite(db, entry, &error))
				goto error;
				
			break;
		case CHANGE_ADDED:
			//Add a new entry
			osync_trace(TRACE_INTERNAL, "Find category");
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			GList *c = NULL;
			for (c = todo->categories; c; c = c->next) {
				osync_trace(TRACE_INTERNAL, "searching category %s\n", c->data);
				entry->category = _psyncDBCategoryToId(db, c->data, NULL);
				if (entry->category != 0) {
					osync_trace(TRACE_INTERNAL, "Found category %i\n", entry->category);
					break;
				}
			}
			
			osync_trace(TRACE_INTERNAL, "Adding new entry");
			
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, sizeof(entry->buffer));
	
			if (!_psyncDBAdd(db, entry, &id, &error))
				goto error;
			
			//Make the new uid
			char *uid = g_strdup_printf("uid-ToDoDB-%ld", id);
			osync_change_set_uid(change, uid);
			g_free(uid);
			break;
		case CHANGE_DELETED:
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);
		
			if (!_psyncDBDelete(db, id, &error))
				goto error;
				
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

static OSyncChange *_psyncContactCreate(PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, entry, error);
	PSyncDatabase *db = entry->db;

	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	char *uid = g_strdup_printf("uid-AddressDB-%ld", entry->id);
	osync_change_set_uid(change, uid);
	g_free(uid);
	
	osync_change_set_objformat_string(change, "palm-contact");
	
	if ((entry->attr &  dlpRecAttrDeleted) || (entry->attr & dlpRecAttrArchived)) {
		if ((entry->attr & dlpRecAttrArchived)) {
			osync_trace(TRACE_INTERNAL, "Archieved");
		}
		//we have a deleted record
		osync_change_set_changetype(change, CHANGE_DELETED);
	} else {
		/* Create the object data */
		PSyncContactEntry *contact = osync_try_malloc0(sizeof(PSyncContactEntry), error);
		if (!contact)
			goto error_free_change;
		contact->codepage = g_strdup(db->env->codepage);
		
		osync_trace(TRACE_INTERNAL, "Starting to unpack entry %i", db->size);
		unpack_Address(&(contact->address), entry->buffer, db->size);
	    const char *catname = _psyncDBCategoryFromId(entry->db, entry->category, NULL);
	    if (catname)
			contact->categories = g_list_append(contact->categories, g_strdup(catname));
		
		osync_change_set_data(change, (void *)contact, sizeof(PSyncContactEntry), TRUE);
		
		if (entry->attr & dlpRecAttrDirty)  {
			osync_change_set_changetype(change, CHANGE_MODIFIED);
		} else {
			osync_change_set_changetype(change, CHANGE_UNKNOWN);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;

error_free_change:
	osync_change_free(change);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool _psyncContactGetChangeInfo(OSyncContext *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	
	if (!(db = _psyncDBOpen(env, "AddressDB", error)))
		goto error;
	
	if (osync_member_get_slow_sync(env->member, "contact") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		
		int n;
		for (n = 0; (entry = _psyncDBGetNthEntry(db, n)); n++) {
			osync_trace(TRACE_INTERNAL, "Got all recored with id %ld", entry->id);
			
			OSyncChange *change = _psyncContactCreate(entry, error);
			if (!change)
				goto error;
			
			osync_change_set_changetype(change, CHANGE_ADDED);
			osync_context_report_change(ctx, change);
		}
	} else {
		while ((entry = _psyncDBGetNextModified(db))) {
			OSyncChange *change = _psyncContactCreate(entry, error);
			if (!change)
				goto error;
			
			osync_context_report_change(ctx, change);
		}
	}
	
	if (osync_error_is_set(error))
		goto error_close_db;
	
	_psyncDBClose(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_close_db:
	_psyncDBClose(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*
		if (osync_error_get_type(&error) == OSYNC_ERROR_FILE_NOT_FOUND) {
			if ((dbCreated == FALSE) && !strcmp(database, "DatebookDB")) {
				//Unlock our Mutex so the palm does not die will the messagebox is open
				dbCreated = TRUE;
				if (dlp_CreateDB(env->socket, 1684108389, 1145132097, 0, 8, 0, "DatebookDB", &dbhandle) < 0) {
					dlp_AddSyncLogEntry(env->socket, "Unable to create Calendar.\n");
					osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to create Calendar");
					goto error;
				}
				env->database = dbhandle;
				dlp_AddSyncLogEntry(env->socket, "Created Calendar.\n");
				osync_trace(TRACE_INTERNAL, "Created Calendar.");
			}
		} else {*/

static osync_bool psyncContactCommit(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	PSyncContactEntry *contact = NULL;
	OSyncError *error = NULL;
	unsigned long id = 0;

	//open the DB
	if (!(db = _psyncDBOpen(env, "AddressDB", &error)))
		goto error;
	
	contact = (PSyncContactEntry *)osync_change_get_data(change);
			
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			//Get the id
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);
			
			PSyncEntry *orig_entry = _psyncDBGetNthEntry(db, id);
			if (!orig_entry)
				goto error;
			
			PSyncContactEntry *orig_contact = osync_try_malloc0(sizeof(PSyncContactEntry), &error);
			if (!orig_contact)
				goto error;
		
			unpack_Address(&(orig_contact->address), orig_entry->buffer, db->size);
				
			if ((orig_contact->address.showPhone) > 4)
				orig_contact->address.showPhone = 0;
			contact->address.showPhone = orig_contact->address.showPhone;

			g_free(orig_entry);
			g_free(orig_contact);

			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			entry->size = pack_Address(&(contact->address), entry->buffer, sizeof(entry->buffer));
	
			if (!_psyncDBWrite(db, entry, &error))
				goto error;
				
			break;
		case CHANGE_ADDED:
			//Add a new entry
			osync_trace(TRACE_INTERNAL, "Find category");
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			GList *c = NULL;
			for (c = contact->categories; c; c = c->next) {
				osync_trace(TRACE_INTERNAL, "searching category %s\n", c->data);
				entry->category = _psyncDBCategoryToId(db, c->data, NULL);
				if (entry->category != 0) {
					osync_trace(TRACE_INTERNAL, "Found category %i\n", entry->category);
					break;
				}
			}
			
			osync_trace(TRACE_INTERNAL, "Adding new entry");
			
			entry->size = pack_Address(&(contact->address), entry->buffer, sizeof(entry->buffer));
	
			
			if (!_psyncDBAdd(db, entry, &id, &error))
				goto error;
			
			//Make the new uid
			char *uid = g_strdup_printf("uid-AddressDB-%ld", id);
			osync_change_set_uid(change, uid);
			g_free(uid);
			break;
		case CHANGE_DELETED:
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);
		
			if (!_psyncDBDelete(db, id, &error))
				goto error;
				
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

static OSyncChange *_psyncEventCreate(PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, entry, error);
	PSyncDatabase *db = entry->db;
	
	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	char *uid = g_strdup_printf("uid-DatebookDB-%ld", entry->id);
	osync_change_set_uid(change, uid);
	g_free(uid);
	
	osync_change_set_objformat_string(change, "palm-event");
	
	if ((entry->attr &  dlpRecAttrDeleted) || (entry->attr & dlpRecAttrArchived)) {
		if ((entry->attr & dlpRecAttrArchived)) {
			osync_trace(TRACE_INTERNAL, "Archieved");
		}
		//we have a deleted record
		osync_change_set_changetype(change, CHANGE_DELETED);
	} else {
		
		PSyncEventEntry *event = osync_try_malloc0(sizeof(PSyncEventEntry), error);
		if (!event)
			goto error_free_change;
		event->codepage = g_strdup(db->env->codepage);
		
		osync_trace(TRACE_INTERNAL, "Starting to unpack entry %i", db->size);
		unpack_Appointment(&(event->appointment), entry->buffer, db->size);
	    const char *catname = _psyncDBCategoryFromId(entry->db, entry->category, NULL);
	    if (catname)
			event->categories = g_list_append(event->categories, g_strdup(catname));
		
		osync_change_set_data(change, (void *)event, sizeof(PSyncEventEntry), TRUE);
	
		if (entry->attr & dlpRecAttrDirty) {
			osync_change_set_changetype(change, CHANGE_MODIFIED);
		} else {
			osync_change_set_changetype(change, CHANGE_UNKNOWN);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;

error_free_change:
	osync_change_free(change);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool _psyncEventGetChangeInfo(OSyncContext *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	
	if (!(db = _psyncDBOpen(env, "DatebookDB", error)))
		goto error;
	
	if (osync_member_get_slow_sync(env->member, "event") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		
		int n;
		for (n = 0; (entry = _psyncDBGetNthEntry(db, n)); n++) {
			osync_trace(TRACE_INTERNAL, "Got all recored with id %ld", entry->id);
			
			OSyncChange *change = _psyncEventCreate(entry, error);
			if (!change)
				goto error;
			
			osync_change_set_changetype(change, CHANGE_ADDED);
			osync_context_report_change(ctx, change);
		}
	} else {
		while ((entry = _psyncDBGetNextModified(db))) {
			OSyncChange *change = _psyncEventCreate(entry, error);
			if (!change)
				goto error;
			
			osync_context_report_change(ctx, change);
		}
	}
	
	if (osync_error_is_set(error))
		goto error_close_db;
	
	_psyncDBClose(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_close_db:
	_psyncDBClose(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool psyncEventCommit(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	PSyncEventEntry *event = NULL;
	OSyncError *error = NULL;
	unsigned long id = 0;

	//open the DB
	if (!(db = _psyncDBOpen(env, "DatebookDB", &error)))
		goto error;
	
	event = (PSyncEventEntry *)osync_change_get_data(change);
			
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			//Get the id
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);

			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			entry->size = pack_Appointment(&(event->appointment), entry->buffer, sizeof(entry->buffer));
	
			if (!_psyncDBWrite(db, entry, &error))
				goto error;
				
			break;
		case CHANGE_ADDED:
			//Add a new entry
			osync_trace(TRACE_INTERNAL, "Find category");
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			GList *c = NULL;
			for (c = event->categories; c; c = c->next) {
				osync_trace(TRACE_INTERNAL, "searching category %s\n", c->data);
				entry->category = _psyncDBCategoryToId(db, c->data, NULL);
				if (entry->category != 0) {
					osync_trace(TRACE_INTERNAL, "Found category %i\n", entry->category);
					break;
				}
			}
			
			osync_trace(TRACE_INTERNAL, "Adding new entry");
			
			entry->size = pack_Appointment(&(event->appointment), entry->buffer, sizeof(entry->buffer));
	
			if (!_psyncDBAdd(db, entry, &id, &error))
				goto error;
			
			//Make the new uid
			char *uid = g_strdup_printf("uid-DatebookDB-%ld", id);
			osync_change_set_uid(change, uid);
			g_free(uid);
			break;
		case CHANGE_DELETED:
			sscanf(osync_change_get_uid(change), "uid-%*[^-]-%ld", &id);
			osync_trace(TRACE_INTERNAL, "id %ld", id);
		
			if (!_psyncDBDelete(db, id, &error))
				goto error;
				
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

static void psyncGetChangeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;

	osync_trace(TRACE_INTERNAL, "Opening conduit");
	dlp_OpenConduit(env->socket);

	if (!_psyncTodoGetChangeInfo(ctx, &error))
		goto error;
	
	if (!_psyncContactGetChangeInfo(ctx, &error))
		goto error;
		
	if (!_psyncEventGetChangeInfo(ctx, &error))
		goto error;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void psyncDone(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);

	PSyncDatabase *db = NULL;
	OSyncError *error = NULL;

	if ((db = _psyncDBOpen(env, "AddressDB", &error))) {
		osync_trace(TRACE_INTERNAL, "Reseting Sync Flags for AddressDB");
		dlp_ResetSyncFlags(env->socket, db->handle);
		dlp_CleanUpDatabase(env->socket, db->handle);
		_psyncDBClose(db);
	}
	
	if ((db = _psyncDBOpen(env, "ToDoDB", &error))) {
		osync_trace(TRACE_INTERNAL, "Reseting Sync Flags for ToDoDB");
		dlp_ResetSyncFlags(env->socket, db->handle);
		dlp_CleanUpDatabase(env->socket, db->handle);
		_psyncDBClose(db);
	}
	
	if ((db = _psyncDBOpen(env, "DatebookDB", &error))) {
		osync_trace(TRACE_INTERNAL, "Reseting Sync Flags for DatebookDB");
		dlp_ResetSyncFlags(env->socket, db->handle);
		dlp_CleanUpDatabase(env->socket, db->handle);
		_psyncDBClose(db);
	}

	//Set the log and sync entries on the palm
	dlp_AddSyncLogEntry(env->socket, "Sync Successfull\n");
	dlp_AddSyncLogEntry(env->socket, "Thank you for using\n");
	dlp_AddSyncLogEntry(env->socket, "OpenSync");

	env->user.lastSyncPC = 1;
	env->user.lastSyncDate = time(NULL);
	env->user.successfulSyncDate = time(NULL);
	if (dlp_WriteUserInfo(env->socket, &env->user) < 0) {
		osync_trace(TRACE_INTERNAL, "Unable to write UserInfo");
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void psyncDisconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);

	if (env->currentDB)
		_psyncDBClose(env->currentDB);
		
	dlp_EndOfSync(env->socket, 0);
	osync_trace(TRACE_INTERNAL, "Done syncing");

	if (env->socket) {
		pi_close(env->socket);
		env->socket = 0;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void psyncFinalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = (PSyncEnv *)data;

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
	osync_plugin_set_commit_objformat(info, "contact", "palm-contact", psyncContactCommit);
	
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "palm-todo", NULL);
	osync_plugin_set_commit_objformat(info, "todo", "palm-todo", psyncTodoCommit);
	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "palm-event", NULL);
	osync_plugin_set_commit_objformat(info, "event", "palm-event", psyncEventCommit);
}
