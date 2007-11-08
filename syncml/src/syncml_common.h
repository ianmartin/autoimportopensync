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

#include <libsyncml/obex_client.h>
#include <libsyncml/http_server.h>
#include <libsyncml/http_client.h>

#include <libsyncml/sml_auth.h>
#include <libsyncml/sml_devinf_obj.h>
#include <libsyncml/sml_ds_server.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

struct commitContext {
	OSyncContext *context;
	OSyncChange *change;
};

typedef struct SmlPluginEnv {
	char *path;
	unsigned int interface;
	char *bluetoothAddress;
	int bluetoothChannel;
	char *identifier;
	SmlNotificationVersion version;
	osync_bool useWbxml;
	char *username;
	char *password;
	SmlBool useStringtable;
	SmlBool onlyReplace;
	SmlBool onlyLocaltime;
	SmlTransportObexClientType type;
	unsigned int port;
	char *url;
	
	unsigned int recvLimit;
	unsigned int maxObjSize;

	SmlBool gotFinal;
	SmlBool gotDisconnect;
	SmlBool tryDisconnect;

	
	OSyncMember *member;
	char *anchor_path;

	GSource *source;
	GSourceFuncs *source_functions;

	GMainContext *context;
	GMainLoop *loop;
	
	SmlTransport *tsp;
	SmlAuthenticator *auth;
	SmlDevInfAgent *agent;
	SmlManager *manager;
	SmlSession *session;
	
	OSyncContext *connectCtx;

	SmlNotification *san;

	GList *databases;

	int num;

	GList *eventEntries;
	unsigned int numEventEntries;

	osync_bool isConnected;

	SmlAuthType authType;
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

	osync_bool gotChanges;
	osync_bool finalChanges; 

	OSyncContext *getChangesCtx;
	OSyncContext *commitCtx;
	OSyncContext *disconnectCtx;

} SmlDatabase;

extern SmlBool _recv_alert(
			SmlDsSession *dsession, 
			SmlAlertType type, 
			const char *last, 
			const char *next, 
			void *userdata);

extern void _manager_event(
			SmlManager *manager, 
			SmlManagerEventType type, 
			SmlSession *session, 
			SmlError *error, 
			void *userdata);

extern gboolean _sessions_prepare(GSource *source, gint *timeout_);

extern gboolean _sessions_check(GSource *source);

extern gboolean _sessions_dispatch(
			GSource *source, 
			GSourceFunc callback, 
			gpointer user_data);

extern void get_changeinfo(
			void *data, 
			OSyncPluginInfo *info, 
			OSyncContext *ctx);

extern void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

extern void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

extern void batch_commit(
			void *data, 
			OSyncPluginInfo *info, 
			OSyncContext *ctx, 
			OSyncContext **contexts, 
			OSyncChange **changes);

extern void _ds_alert(SmlDsSession *dsession, void *userdata);

extern void _verify_user(
			SmlAuthenticator *auth, 
			const char *username, 
			const char *password, 
			void *userdata, 
			SmlErrorType *reply);

extern const char *_objtype_to_contenttype(const char *objtype);

extern void finalize(void *data);

#endif //_SYNCML_COMMON_H
