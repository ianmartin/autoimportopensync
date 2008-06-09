/*
 * syncml plugin - A syncml plugin for OpenSync
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */
 
#ifndef _SYNCML_COMMON_H
#define _SYNCML_COMMON_H
//#include <config.h>

#include <opensync/opensync.h>

#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync-version.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <libsyncml/syncml.h>

#include <libsyncml/sml_auth.h>
#include <libsyncml/sml_devinf_obj.h>
#include <libsyncml/sml_ds_server.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

typedef struct SmlPluginEnv {
	char *bluetoothAddress;
	char *bluetoothChannel;
	char *atCommand;
	char *atManufacturer;
	char *atModel;
	char *identifier;
	SmlNotificationVersion version;
	osync_bool useWbxml;
	char *username;
	char *password;
	SmlBool useStringtable;
	SmlBool onlyReplace;
	SmlBool onlyLocaltime;
	SmlTransportConnectionType type;
	char *port;
	char *url;
	char *proxy;
	char *cafile;
	
	unsigned int recvLimit;
	unsigned int maxObjSize;

	SmlBool gotDisconnect;
	SmlBool tryDisconnect;
	SmlBool doReconnect;
	
	OSyncPluginInfo *pluginInfo;
	char *anchor_path;
	char *devinf_path;

	GSource *source;
	GSourceFuncs *source_functions;

	GMainContext *context;
	
	SmlTransport *tsp;
	SmlAuthenticator *auth;
	SmlDevInf *devinf;
	SmlDevInf *remote_devinf;
	SmlDevInfAgent *agent;
	SmlManager *manager;
	SmlSession *session;
	
	SmlNotification *san;

	GList *databases;
	GList *ignoredDatabases;
	char *sessionUser;

	int num;

	SmlAuthType authType;
	osync_bool fakeDevice;
	SmlProtocolVersion syncmlVersion;
        char *fakeManufacturer;
        char *fakeModel;
        char *fakeSoftwareVersion;

	osync_bool isConnected;
	OSyncContext *connectCtx;
	OSyncContext *disconnectCtx;
	GMutex *connectMutex;

	/* This function pointer is necessary to start the second OMA DS session
	 * if the synchronization uses an OMA DS client.
	 */
	OSyncSinkConnectFn connectFunction;

	GMutex *managerMutex;
} SmlPluginEnv;

typedef struct SmlDatabase {
	SmlPluginEnv *env;
	SmlDsSession *session;
	SmlDsServer *server;
	OSyncObjFormat *objformat;
	char *objformat_name;
	OSyncObjTypeSink *sink;
	char *objtype;	
	char *url;

	SmlDsSessionAlertCb dsSessionCallback;

	OSyncChange **syncChanges;
	OSyncContext **syncContexts;
	osync_bool syncReceived;
	osync_bool gotChanges;
	unsigned int pendingChanges;
	unsigned int pendingCommits;

	OSyncContext *syncModeCtx;
	OSyncContext *getChangesCtx;
	OSyncContext *commitCtx;
} SmlDatabase;

struct commitContext {
	OSyncContext *context;
	OSyncChange *change;
	SmlDatabase *database;
};

gboolean _sessions_prepare(GSource *source, gint *timeout_);

gboolean _sessions_check(GSource *source);

gboolean _sessions_dispatch(
			GSource *source, 
			GSourceFunc callback, 
			gpointer user_data);

void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

SmlBool send_sync_message(
                        SmlDatabase *database,
                        void *func_ptr,
                        OSyncError **oserror);

osync_bool syncml_config_parse_database(
			SmlPluginEnv *env,
			xmlNode *cur,
			OSyncError **error);

osync_bool init_objformat(
			OSyncPluginInfo *info,
			SmlDatabase *database,
			OSyncError **error);

SmlBool flush_session_for_all_databases(
			SmlPluginEnv *env,
			SmlBool activeDatabase,
			SmlError **error);

SmlDatabase *get_database_from_plugin_info(OSyncPluginInfo *info);

/* this is a helper function which adds an object to a GList */
/* the function guarantees that an object exists only once in */
/* this GList. No double entrees. */
GList *g_list_add(GList *databases, void *database);

void initEnv(SmlPluginEnv *env);

void finalize(void *data);

void safe_cfree(char **address);
void safe_free(gpointer *address);

void report_success_on_context(OSyncContext **ctx);
void report_error_on_context(OSyncContext **ctx, OSyncError **error, osync_bool cleanupError);

// FIXME: This is only a fast fix for SuSE.
// FIXME: Perhaps the functions should be renamed.
OSyncChangeType _to_osync_changetype(SmlChangeType type);
void set_session_user(SmlPluginEnv *env, const char* user);
unsigned int get_num_changes(OSyncChange **changes);

#endif //_SYNCML_COMMON_H
