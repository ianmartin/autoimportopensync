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

#include <libsyncml/data_sync_api/defines.h>
#include <libsyncml/data_sync_api/standard.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/* The default limits are taken from libsyncml. */
#define OSYNC_PLUGIN_SYNCML_MAX_MSG_SIZE SML_DEFAULT_MAX_MSG_SIZE
#define OSYNC_PLUGIN_SYNCML_MAX_OBJ_SIZE SML_DEFAULT_MAX_OBJ_SIZE

#define SYNCML_PLUGIN_CONFIG_SANVERSION "SANVersion"
#define SYNCML_PLUGIN_CONFIG_WBXML "WBXML"
#define SYNCML_PLUGIN_CONFIG_ATCOMMAND "ATCommand"
#define SYNCML_PLUGIN_CONFIG_ATMANUFACTURER "ATManufacturer"
#define SYNCML_PLUGIN_CONFIG_ATMODEL "ATModel"
#define SYNCML_PLUGIN_CONFIG_IDENTIFIER "Identifier"

#define SYNCML_PLUGIN_CONFIG_USESTRINGTABLE "UseStringTable"
#define SYNCML_PLUGIN_CONFIG_USETIMEANCHOR "UseTimeAnchor"
#define SYNCML_PLUGIN_CONFIG_ONLYREPLACE "OnlyReplace"
#define SYNCML_PLUGIN_CONFIG_MAXMSGSIZE "MaxMsgSize"
#define SYNCML_PLUGIN_CONFIG_MAXOBJSIZE "MaxObjSize"
#define SYNCML_PLUGIN_CONFIG_ONLYLOCALTIME "OnlyLocaltime"

#define SYNCML_PLUGIN_CONFIG_PATH "Path"
#define SYNCML_PLUGIN_CONFIG_CAFILE "CaFile"
#define SYNCML_PLUGIN_CONFIG_PROXY "Proxy"

#define SYNCML_PLUGIN_CONFIG_FAKE_DEVICE "FakeDevice"
#define SYNCML_PLUGIN_CONFIG_FAKE_MANUFACTURER "FakeManufacturer"
#define SYNCML_PLUGIN_CONFIG_FAKE_MODEL "FakeModel"
#define SYNCML_PLUGIN_CONFIG_FAKE_SOFTWARE_VERSION "FakeSoftwareVersion"

#define SYNCML_PLUGIN_CONFIG_AUTH_TYPE "AuthType"

typedef enum {
	OSYNC_PLUGIN_SYNCML_COMMAND_UNKNOWN,
	OSYNC_PLUGIN_SYNCML_COMMAND_SEND_ALERT,
	OSYNC_PLUGIN_SYNCML_COMMAND_SEND_SYNC,
	OSYNC_PLUGIN_SYNCML_COMMAND_RECV_SYNC,
} OSyncPluginSyncmlDatastoreCommand;

typedef struct SmlPluginEnv {
	SmlDataSyncObject *dsObject1;
	SmlDataSyncObject *dsObject2;
	SmlSessionType sessionType;

	/* libsyncml state management */

	SmlBool abort;
	SmlDataSyncEventType state1;
	SmlDataSyncEventType state2;

	/* opensync state management */

	OSyncContext *connectCtx;
	OSyncContext *disconnectCtx;
	GList *databases;
	unsigned int gotDatabaseCommits; /* only for OMA DS server */

	/* environment data */

	OSyncPluginInfo *pluginInfo;
	char *anchor_path;
	char *devinf_path;

	GSource *source;
	GSourceFuncs *source_functions;

	GMainContext *context;

} SmlPluginEnv;

typedef struct SmlDatabase {
	SmlPluginEnv *env;
	OSyncObjFormat *objformat;
	const char *objformat_name;
	OSyncObjTypeSink *sink;
	const char *objtype;	
	const char *url;
	char *remoteNext;
	char *localNext;

	OSyncChange **syncChanges;
	OSyncContext **syncContexts;
	unsigned int pendingChanges;
	unsigned int pendingCommits;

	OSyncPluginSyncmlDatastoreCommand command;

	OSyncContext *syncModeCtx;
	OSyncContext *getChangesCtx;
	OSyncContext *commitCtx;
} SmlDatabase;

struct commitContext {
	OSyncContext *context;
	OSyncChange *change;
	SmlDatabase *database;
};

osync_bool discover(
		const char *name,
		void *data,
		OSyncPluginInfo *info,
		OSyncError **error);
void *syncml_init(
		SmlSessionType sessionType,
		SmlTransportType tspType,
		OSyncPlugin *plugin,
		OSyncPluginInfo *info,
		OSyncError **oerror);
void syncml_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

osync_bool parse_config(
		SmlTransportType tsp,
		SmlDataSyncObject *dsObject,
		OSyncPluginConfig *config,
		OSyncError **oerror);

SmlDatabase *syncml_config_parse_database(
			SmlPluginEnv *env,
			OSyncPluginResource *res,
			OSyncError **error);

SmlDatabase *get_database_from_plugin_info(OSyncPluginInfo *info);

/* this is a helper function which adds an object to a GList */
/* the function guarantees that an object exists only once in */
/* this GList. No double entrees. */
GList *g_list_add(GList *databases, void *database);

void finalize(void *data);

void safe_cfree(char **address);
void safe_free(gpointer *address);

void report_success_on_context(OSyncContext **ctx);
void report_error_on_context(OSyncContext **ctx, OSyncError **error, osync_bool cleanupError);

// FIXME: This is only a fast fix for SuSE.
// FIXME: Perhaps the functions should be renamed.
OSyncChangeType _to_osync_changetype(SmlChangeType type);
unsigned int get_num_changes(OSyncChange **changes);

SmlDatabase *get_database_from_source(
			SmlPluginEnv *env,
			const char *source,
			SmlError **error);

SmlChangeType _get_changetype(OSyncChange *change);

#endif //_SYNCML_COMMON_H
