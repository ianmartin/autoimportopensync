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

#include "palm_todo.h"
#include "palm_contact.h"
#include "palm_event.h"
#include "palm_note.h"

#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>

#define CATCOUNT 16

#ifdef OLD_PILOT_LINK
#define pi_buffer(b) b
#else
#define pi_buffer(b) b->data
#endif

typedef enum PSyncError {
	PSYNC_NO_ERROR = 0,
	PSYNC_ERROR_NOT_FOUND = 1,
	PSYNC_ERROR_OTHER = 2,
} PSyncError;

static PSyncError _psyncCheckReturn(int sd, int ret, OSyncError **error)
{
#ifdef OLD_PILOT_LINK
	if (ret == dlpErrNotFound) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "File not found");
		return PSYNC_ERROR_NOT_FOUND;
	} else if (ret < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%i", ret);
		return PSYNC_ERROR_OTHER;
	}
#else
	if (ret == PI_ERR_DLP_PALMOS) {
		int pierr = pi_palmos_error(sd);
		if (pierr == dlpErrNotFound) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "File not found");
			return PSYNC_ERROR_NOT_FOUND;
		}
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%i", ret);
		osync_trace(TRACE_INTERNAL, "Encountered a palm os error %i", pierr);
		return PSYNC_ERROR_OTHER;
	} else if (ret < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%i", ret);
		return PSYNC_ERROR_OTHER;
	}
#endif
	return PSYNC_NO_ERROR;
}

void psyncDBClose(PSyncDatabase *db)
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

PSyncDatabase *psyncDBOpen(PSyncEnv *env, char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, name, error);
	
	struct DBInfo dbInfo;
	memset(&dbInfo, 0, sizeof(struct DBInfo));

	if (env->currentDB) {
		if (strcmp(env->currentDB->name, name) != 0) {
			//We have another DB open. close it first
			osync_trace(TRACE_INTERNAL, "Other db open, closing first");
			psyncDBClose(env->currentDB);
		} else {
			//It was already open
			osync_trace(TRACE_EXIT, "%s: Already opened", __func__);
			return env->currentDB;
		}
	}

 	//Search it
	/*r = dlp_FindDBInfo(env->socket, 0, 0, name, 0, 0, &dbInfo);
	if (r < 0) {
		osync_error_set(error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to locate %s. Assuming it has been reset (%i)", name, r);
		goto error;
	}*/

	PSyncDatabase *db = osync_try_malloc0(sizeof(PSyncDatabase), error);
	if (!db)
		goto error;
	db->env = env;
#ifndef OLD_PILOT_LINK
	db->buffer = pi_buffer_new(65536);
#endif
	
	//open it
	int ret = dlp_OpenDB(env->socket, 0, dlpOpenReadWrite, name, &(db->handle));
	if (_psyncCheckReturn(env->socket, ret, error) != PSYNC_NO_ERROR) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open %s: %s", name, osync_error_print(error));
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

PSyncEntry *psyncDBGetEntryByID(PSyncDatabase *db, unsigned long id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %ld, %p)", __func__, db, id, error);
	
	PSyncEntry *entry = osync_try_malloc0(sizeof(PSyncEntry), error);
	if (!entry)
		goto error;
	
	entry->id = id;
	entry->db = db;
#ifndef OLD_PILOT_LINK
	entry->buffer = pi_buffer_new(65536);
	if (!entry->buffer)
		goto error_free_entry;
#endif
	
#ifdef OLD_PILOT_LINK
	int ret = dlp_ReadRecordById(db->env->socket, db->handle, id,
		entry->buffer, &entry->index, &entry->size, &entry->attr,
		&entry->category);
#else
	int ret = dlp_ReadRecordById(db->env->socket, db->handle, id,
		entry->buffer, &entry->index, &entry->attr, &entry->category);
#endif
	PSyncError err = _psyncCheckReturn(db->env->socket, ret, error);
	if (err == PSYNC_ERROR_OTHER) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get next entry: %s", osync_error_print(error));
		goto error_free_buffer;
	} else if (err == PSYNC_ERROR_NOT_FOUND) {
		osync_error_unref(error);
		goto error_free_buffer;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
	return entry;

error_free_buffer:;
#ifndef OLD_PILOT_LINK
		pi_buffer_free(entry->buffer);
error_free_entry:
#endif
	g_free(entry);
error:
	if (osync_error_is_set(error))
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	else
		osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return NULL;
}

PSyncEntry *psyncDBGetNthEntry(PSyncDatabase *db, int nth, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, db, nth, error);
	
	PSyncEntry *entry = osync_try_malloc0(sizeof(PSyncEntry), error);
	if (!entry)
		goto error;
	
	entry->index = nth;
	entry->db = db;
#ifndef OLD_PILOT_LINK
	entry->buffer = pi_buffer_new(65536);
	if (!entry->buffer)
		goto error_free_entry;
#endif
	
#ifdef OLD_PILOT_LINK
	int ret = dlp_ReadRecordByIndex(db->env->socket, db->handle, nth,
		entry->buffer, &entry->id, &entry->size, &entry->attr,
		&entry->category);
#else
	int ret = dlp_ReadRecordByIndex(db->env->socket, db->handle, nth,
		entry->buffer, &entry->id, &entry->attr, &entry->category);
#endif
	PSyncError err = _psyncCheckReturn(db->env->socket, ret, error);
	if (err == PSYNC_ERROR_OTHER) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get next entry: %s", osync_error_print(error));
		goto error_free_buffer;
	} else if (err == PSYNC_ERROR_NOT_FOUND) {
		osync_error_unref(error);
		goto error_free_buffer;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
	return entry;

error_free_buffer:;
#ifndef OLD_PILOT_LINK
		pi_buffer_free(entry->buffer);
error_free_entry:
#endif
	g_free(entry);
error:
	if (osync_error_is_set(error))
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	else
		osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return NULL;
}

PSyncEntry *psyncDBGetNextModified(PSyncDatabase *db, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, db, error);
	
	PSyncEntry *entry = osync_try_malloc0(sizeof(PSyncEntry), error);
	if (!entry)
		goto error;
	
	entry->db = db;
#ifndef OLD_PILOT_LINK
	entry->buffer = pi_buffer_new(65536);
	if (!entry->buffer)
		goto error_free_entry;
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

	PSyncError err = _psyncCheckReturn(db->env->socket, ret, error);
	if (err == PSYNC_ERROR_OTHER) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get next entry: %s", osync_error_print(error));
		goto error_free_buffer;
	} else if (err == PSYNC_ERROR_NOT_FOUND) {
		osync_error_unref(error);
		goto error_free_buffer;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, entry);
	return entry;

error_free_buffer:;
#ifndef OLD_PILOT_LINK
		pi_buffer_free(entry->buffer);
error_free_entry:
#endif
	g_free(entry);
error:
	if (osync_error_is_set(error))
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	else
		osync_trace(TRACE_EXIT, "%s: Not Found", __func__);
	return NULL;
}

osync_bool psyncDBDelete(PSyncDatabase *db, int id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, db, id, error);
	
	int ret = dlp_DeleteRecord(db->env->socket, db->handle, 0,  id);
	if (_psyncCheckReturn(db->env->socket, ret, error) != PSYNC_NO_ERROR) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to delete file: %s", osync_error_print(error));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool psyncDBWrite(PSyncDatabase *db, PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, db, entry, error);
	
	/* Do not use sizeof (entry->buffer) as this will give 
	   the size of the allocated buffer instead of size of
	   the *packed* buffer 
	*/
#ifndef OLD_PILOT_LINK
	int ret = dlp_WriteRecord(db->env->socket, db->handle, entry->attr, entry->id, entry->category, ((pi_buffer_t *)entry->buffer)->data, ((pi_buffer_t *)entry->buffer)->used, 0);
#else
	int ret = dlp_WriteRecord(db->env->socket, db->handle, entry->attr, entry->id, entry->category, entry->buffer, entry->size, 0);
#endif
	if (_psyncCheckReturn(db->env->socket, ret, error) != PSYNC_NO_ERROR) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to write file: %s", osync_error_print(error));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool psyncDBAdd(PSyncDatabase *db, PSyncEntry *entry, unsigned long *id, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p %p)", __func__, db, entry, id, error);
	
	/* Do not use sizeof (entry->buffer) as this will give 
	   the size of the allocated buffer instead of size of
	   the *packed* buffer 
	*/
	int ret;
	
	if ((entry == NULL) || (entry->buffer == NULL)) {
		osync_trace(TRACE_EXIT, "%s: Skipping null entry!", __func__);
		return TRUE;
	}

#ifdef OLD_PILOT_LINK
	ret = dlp_WriteRecord(db->env->socket, db->handle, 0, 0, entry->category, entry->buffer, entry->size, id);
#else
	ret = dlp_WriteRecord(db->env->socket, db->handle, 0, 0, entry->category, ((pi_buffer_t*)entry->buffer)->data, ((pi_buffer_t*)entry->buffer)->used, id);
#endif
	if (_psyncCheckReturn(db->env->socket, ret, error) != PSYNC_NO_ERROR) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to add file: %s", osync_error_print(error));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

const char *psyncDBCategoryFromId(PSyncDatabase *db, int id, OSyncError **error)
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

int psyncDBCategoryToId(PSyncDatabase *db, const char *name, OSyncError **error)
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
	if ((listen_sd = pi_socket (PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP)) < 0) {
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

#ifdef OLD_PILOT_LINK
	if (env->conntype != PILOT_DEVICE_NETWORK && env->conntype != PILOT_DEVICE_IRDA)
		pi_close(listen_sd);
#endif

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

	//OSyncError *error = NULL;
	//if (_connectDevice(env, 1, &error))
		//osync_member_request_synchronization(env->member);
	//else
		//osync_error_unref(&error);
		
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

static void psyncThreadStart(PSyncEnv *env, OSyncPluginInfo *info)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, info);
	
	GMainContext *context = osync_plugin_info_get_loop(info);
	
	GSource *source = g_timeout_source_new(5000);
	g_source_set_callback(source, _psyncPing, env, NULL);
	g_source_attach(source, context);
	
	//FIXME also polls at the end of a hot sync ... 
	source = g_timeout_source_new(1000);
	g_source_set_callback(source, _psyncPoll, env, NULL);
	g_source_attach(source, context);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool psyncSettingsParse(PSyncEnv *env, const char *config, OSyncError **error)
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

	doc = xmlParseMemory(config, strlen(config));

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

unsigned long psyncUidGetID(const char *uid, OSyncError **error)
{
	unsigned long id = 0;
	if (sscanf(uid, "uid-%*[^-]-%ld", &id) != 1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse uid %s", uid);
		return 0;
	}
	osync_trace(TRACE_INTERNAL, "Got id %ld", id);
	if (!id)
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Found 0 ID");
	
	return id;
}

static void psyncConnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncError *error = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	PSyncEnv *env = (PSyncEnv *)data;
	int ret;

	//now connect with the palm
	if (!_connectDevice(env, env->timeout, &error))
		goto error;

	//check the user
	ret = dlp_ReadUserInfo(env->socket, &env->user);
	osync_trace(TRACE_INTERNAL, "test %i", ret);
	if (_psyncCheckReturn(env->socket, ret, &error) != PSYNC_NO_ERROR) {
		//osync_error_update(&error, "Unable to read UserInfo: %s", osync_error_print(&error));
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
		//osync_member_set_slow_sync(env->member, "data", TRUE);
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
	
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void psyncSyncDone(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	PSyncEnv *env = (PSyncEnv *)data;

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
}

static void psyncDisconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	PSyncEnv *env = (PSyncEnv *)data;

	if (env->currentDB)
		psyncDBClose(env->currentDB);
		
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

static void *psyncInitialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	PSyncEnv *env = osync_try_malloc0(sizeof(PSyncEnv), error);
	if (!env)
		goto error;
		
	if (!psyncSettingsParse(env, osync_plugin_info_get_config(info), error))
		goto error_free_env;
	
	if (!psyncContactInitialize(env, info, error))
		goto error_free_env;
	
	/* Add the main sink */
	OSyncObjTypeSink *sink = osync_objtype_sink_new(NULL, error);
	if (!sink)
		goto error_free_env;
	
	/* All sinks have the same functions of course */
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = psyncConnect;
	functions.disconnect = psyncDisconnect;
	functions.sync_done = psyncSyncDone;
	
	osync_objtype_sink_set_functions(sink, functions, NULL);
	osync_plugin_info_add_objtype(info, sink);

	psyncThreadStart(env, info);
	
	osync_trace(TRACE_EXIT, "%s, %p", __func__, env);
	return (void *)env;

error_free_env:
	g_free(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void psyncFinalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	PSyncEnv *env = (PSyncEnv *)data;

	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool psyncDiscover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	PSyncEnv *env = (PSyncEnv *)data;
	
	osync_objtype_sink_set_available(env->contact_sink, TRUE);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "palm-sync");
	osync_plugin_set_longname(plugin, "Palm Synchronization Plugin");
	osync_plugin_set_description(plugin, "Plugin to synchronize Palm devices");
	
	osync_plugin_set_initialize(plugin, psyncInitialize);
	osync_plugin_set_finalize(plugin, psyncFinalize);
	osync_plugin_set_discover(plugin, psyncDiscover);
	
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);
	
	return TRUE;
	
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
