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

#include <libsyncml/syncml.h>
#include <libsyncml/http_server.h>
#include <libsyncml/http_client.h>

#include <libsyncml/sml_auth.h>
#include <libsyncml/sml_ds_server.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

typedef struct SmlPluginEnv {
	OSyncMember *member;
	GMainContext *context;
	GMainLoop *loop;
	SmlTransport *tsp;
	SmlAuthenticator *auth;
	SmlSession *session;
	char *username;
	char *password;
	SmlBool useStringtable;
	
	SmlDsServer *contactserver;
	char *contact_url;
} SmlPluginEnv;

#include <stdlib.h>

#endif //_SYNCML_PLUGIN_H
