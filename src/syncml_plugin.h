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
 
#ifndef _SYNCML_PLUGIN_H
#define _SYNCML_PLUGIN_H

#include <opensync/opensync.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <libsyncml/syncml.h>

#include <libsyncml/obex_client.h>

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
	char *identifier;
	SmlNotificationVersion version;
	osync_bool useWbxml;
	char *username;
	char *password;
	SmlBool useStringtable;
	SmlBool onlyReplace;
	SmlTransportObexClientType type;
	
	OSyncMember *member;
	GMainContext *context;
	GMainLoop *loop;
	
	SmlTransport *tsp;
	SmlAuthenticator *auth;
	SmlDevInfAgent *agent;
	SmlManager *manager;
	
	SmlDsSession *contactSession;
	SmlDsSession *calendarSession;
	SmlSession *session;
	
	SmlDsServer *contactserver;
	char *contact_url;
	
	SmlDsServer *calendarserver;
	char *calendar_url;
	
	SmlDsServer *taskserver;
	char *task_url;
	
	OSyncContext *connectCtx;
	OSyncContext *getChangesCtx;
	OSyncContext *commitCtx;
	OSyncContext *disconnectCtx;
} SmlPluginEnv;

#endif //_SYNCML_PLUGIN_H
