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

static SmlBool _recv_alert(
			SmlDsSession *dsession, 
			SmlAlertType type, 
			const char *last, 
			const char *next, 
			void *userdata);

static void _manager_event(
			SmlManager *manager, 
			SmlManagerEventType type, 
			SmlSession *session, 
			SmlError *error, 
			void *userdata);

static gboolean _sessions_prepare(GSource *source, gint *timeout_);

static gboolean _sessions_check(GSource *source);

static gboolean _sessions_dispatch(
			GSource *source, 
			GSourceFunc callback, 
			gpointer user_data);

static void get_changeinfo(
			void *data, 
			OSyncPluginInfo *info, 
			OSyncContext *ctx);

static void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

static void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

static void batch_commit(
			void *data, 
			OSyncPluginInfo *info, 
			OSyncContext *ctx, 
			OSyncContext **contexts, 
			OSyncChange **changes);

static void _ds_alert(SmlDsSession *dsession, void *userdata);

static void _verify_user(
			SmlAuthenticator *auth, 
			const char *username, 
			const char *password, 
			void *userdata, 
			SmlErrorType *reply);

#endif //_SYNCML_PLUGIN_H
