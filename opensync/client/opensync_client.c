/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-context.h"
#include "opensync-plugin.h"

#include "opensync-ipc.h"
#include "ipc/opensync_serializer_internals.h"

#include "opensync-format.h"
#include "opensync-version.h"
#include "opensync-merger.h"

#include "opensync-client.h"
#include "opensync_client_internals.h"
#include "opensync_client_private.h"

#ifdef OPENSYNC_UNITTESTS
#include "plugin/opensync_plugin_info_internals.h"
#endif

typedef struct callContext {
	OSyncClient *client;
	OSyncMessage *message;
	OSyncChange *change;
} callContext;

static OSyncContext *_create_context(OSyncClient *client, OSyncMessage *message, OSyncContextCallbackFn callback, OSyncChange *change, OSyncError **error)
{
	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;
	
	callContext *baton = osync_try_malloc0(sizeof(callContext), error);
	if (!baton)
		goto error_free_context;
	
	baton->client = client;
	osync_client_ref(baton->client);
	
	baton->message = message;
	osync_message_ref(message);
	
	baton->change = change;
	if (baton->change)
		osync_change_ref(baton->change);
		
	osync_context_set_callback(context, callback, baton);
	return context;
	
error_free_context:
	osync_context_unref(context);
error:
	return FALSE;
}

static void _free_baton(callContext *baton)
{
	osync_client_unref(baton->client);
	osync_message_unref(baton->message);
	
	if (baton->change)
		osync_change_unref(baton->change);
	
	g_free(baton);
}

static void _osync_client_connect_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	char *objtype = NULL;
	osync_message_read_string(message, &objtype);
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		// objtype sink (e.g. event, contact, ...)
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else {
		// main sink
		sink = osync_plugin_info_get_sink(client->plugin_info);
	}
	int slowsync = osync_objtype_sink_get_slowsync(sink);
	osync_trace(TRACE_INTERNAL, "%s: slowsync %i", __func__, slowsync);

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		if (!reply)
			goto error;

		//Send connect specific reply data
		osync_message_write_int(reply, slowsync);

	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_disconnect_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_get_changes_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		//Send get_changes specific reply data
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_change_callback(OSyncChange *change, void *data)
{
	callContext *baton = data;
	OSyncError *locerror = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, data);
	
	OSyncClient *client = baton->client;
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NEW_CHANGE, 0, &locerror);
	if (!message)
		goto error;

	if (!osync_marshal_change(message, change, &locerror))
		goto error_free_message;

	if (!osync_queue_send_message(client->outgoing, NULL, message, &locerror))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(message);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_ignored_conflict_callback(OSyncChange *change, void *data)
{
	callContext *baton = data;
	OSyncError *locerror = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, data);
	
	OSyncClient *client = baton->client;
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_READ_CHANGE, 0, &locerror);
	if (!message)
		goto error;

	if (!osync_marshal_change(message, change, &locerror))
		goto error_free_message;

	if (!osync_queue_send_message(client->outgoing, NULL, message, &locerror))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(message);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_read_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	osync_trace(TRACE_INTERNAL, "ignored chnaged: %p", baton->change);

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		if (!reply)
			goto error;

		//Send get_changes specific reply data
		osync_message_write_string(reply, osync_change_get_uid(baton->change));
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));


	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;

	_osync_client_ignored_conflict_callback(baton->change, baton);

	_free_baton(baton);

	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_commit_change_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		if (!reply)
			goto error;

		//Send get_changes specific reply data
		osync_message_write_string(reply, osync_change_get_uid(baton->change));
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_committed_all_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		//Send get_changes specific reply data
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_sync_done_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		//Send get_changes specific reply data
	} else {
		reply = osync_message_new_errorreply(message, error, &locerror);
	}
	if (!reply)
		goto error;
	osync_trace(TRACE_INTERNAL, "Reply id %lli", osync_message_get_id(reply));

	_free_baton(baton);
	
	if (!osync_queue_send_message(client->outgoing, NULL, reply, &locerror))
		goto error_free_message;
	
	osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error_free_message:
	osync_message_unref(reply);
error:
	_free_baton(baton);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static osync_bool _osync_client_handle_initialize(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = NULL;
	char *enginepipe = NULL;
	char *pluginname = NULL;
	char *plugindir = NULL;
	char *groupname = NULL;
	char *configdir = NULL;
	char *formatdir = NULL;
	int haspluginconfig = 0;
	OSyncPluginConfig *config = NULL;
	OSyncQueue *outgoing = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);
	
	osync_message_read_string(message, &enginepipe);
	osync_message_read_string(message, &formatdir);
	osync_message_read_string(message, &plugindir);
	osync_message_read_string(message, &pluginname);
	osync_message_read_string(message, &groupname);
	osync_message_read_string(message, &configdir);
	osync_message_read_int(message, &haspluginconfig);

	if (haspluginconfig && !osync_demarshal_pluginconfig(message, &config, error))
		goto error;
	
	osync_trace(TRACE_INTERNAL, "enginepipe %s, formatdir %s, plugindir %s, pluginname %s", enginepipe, formatdir, plugindir, pluginname);
		
	/* First we connect the engine pipe if necessary*/
	if (enginepipe) {
		outgoing = osync_queue_new(enginepipe, error);
		if (!outgoing)
			goto error;
		
		osync_trace(TRACE_INTERNAL, "connecting to engine");
		
		if (!osync_queue_connect(outgoing, OSYNC_QUEUE_SENDER, error)) {
			osync_queue_free(outgoing);
			goto error;
		}
		
		osync_client_set_outgoing_queue(client, outgoing);
		osync_trace(TRACE_INTERNAL, "done connecting to engine");
	}
	
	if (!client->plugin) {
		client->plugin_env = osync_plugin_env_new(error);
		if (!client->plugin_env)
			goto error;
		
		if (!osync_plugin_env_load(client->plugin_env, plugindir, error))
			goto error;
	
		client->plugin = osync_plugin_env_find_plugin(client->plugin_env, pluginname);
		if (!client->plugin) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plugin %s", pluginname);
			goto error;
		}
		osync_plugin_ref(client->plugin);
	}
	
	client->format_env = osync_format_env_new(error);
	if (!client->format_env)
		goto error;
	
	if (!osync_format_env_load_plugins(client->format_env, formatdir, error))
		goto error;
	
	client->plugin_info = osync_plugin_info_new(error);
	if (!client->plugin_info)
		goto error;
	
	osync_plugin_info_set_configdir(client->plugin_info, configdir);
	osync_plugin_info_set_loop(client->plugin_info, client->context);
	osync_plugin_info_set_format_env(client->plugin_info, client->format_env);
	osync_plugin_info_set_groupname(client->plugin_info, groupname);

	if (config)
		osync_plugin_info_set_config(client->plugin_info, config);
	
#ifdef OPENSYNC_UNITTESTS
	long long int memberid;
	osync_message_read_long_long_int(message, &memberid); // Introduced (only) for testing/debugging purpose (mock-sync)
	client->plugin_info->memberid = memberid;
#endif	

	/* Enable active sinks */
	OSyncList *r = NULL;

	if (config)
		r = osync_plugin_config_get_resources(config);

	for (; r; r = r->next) {
		OSyncPluginResource *res = r->data;
		OSyncObjTypeSink *sink;

		/* Don't add disabled Resources to PluginInfo objtypes list: #817 */
		if (!osync_plugin_resource_is_enabled(res))
			continue;

		const char *objtype = osync_plugin_resource_get_objtype(res); 
		/* Check for ObjType sink */
		if (!(sink = osync_plugin_info_find_objtype(client->plugin_info, objtype))) {
			sink = osync_objtype_sink_new(objtype, error);
			if (!sink)
				goto error_finalize;

			osync_plugin_info_add_objtype(client->plugin_info, sink);
			osync_objtype_sink_unref(sink);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Duplicate sink objtype \"%s\" configured in plugin %s", objtype, pluginname);
			goto error;
		}

		const char *preferred_format = osync_plugin_resource_get_preferred_format(res);
		osync_objtype_sink_set_preferred_format(sink, preferred_format); 
		OSyncList *o = osync_plugin_resource_get_objformat_sinks(res);
		for (; o; o = o->next) {
			OSyncObjFormatSink *format_sink = (OSyncObjFormatSink *) o->data; 
			osync_objtype_sink_add_objformat_sink(sink, format_sink);
		}
	}
	
	client->plugin_data = osync_plugin_initialize(client->plugin, client->plugin_info, error);
	if (!client->plugin_data)
		goto error;

	reply = osync_message_new_reply(message, error);
	if (!reply)
		goto error_finalize;

	if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
		goto error_free_message;
	
	osync_message_unref(reply);
		
	g_free(enginepipe);
	g_free(pluginname);
	g_free(configdir);
	g_free(plugindir);
	g_free(groupname);
	g_free(formatdir);
	
	if (config)
		osync_plugin_config_unref(config);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_message:
	osync_message_unref(reply);
error_finalize:
	osync_plugin_finalize(client->plugin, client->plugin_data);
error:
	g_free(enginepipe);
	g_free(pluginname);
	g_free(configdir);
	g_free(plugindir);
	g_free(groupname);
	g_free(formatdir);

	if (config)
		osync_plugin_config_unref(config);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_finalize(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);
	
	if (client->plugin) {
		if (client->plugin_data)
			osync_plugin_finalize(client->plugin, client->plugin_data);
		
		osync_plugin_unref(client->plugin);
		client->plugin = NULL;
	}
	
	if (client->plugin_env) {
		osync_plugin_env_free(client->plugin_env);
		client->plugin_env = NULL;
	}
	
	if (client->plugin_info) {
		osync_plugin_info_unref(client->plugin_info);
		client->plugin_info = NULL;
	}
	
	if (client->format_env) {
		osync_format_env_free(client->format_env);
		client->format_env = NULL;
	}
	
	if (!client->outgoing) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No outgoing queue yet");
		goto error;
	}
	
	OSyncMessage *reply = osync_message_new_reply(message, NULL);
	if (!reply)
		goto error;

	if (!osync_queue_send_message(client->outgoing, NULL, reply, NULL))
		goto error_free_message;
	
	osync_message_unref(reply);
		
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_message:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_discover(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = NULL;
	unsigned int i = 0;
	OSyncPluginConfig *config = osync_plugin_info_get_config(client->plugin_info);
	OSyncList *res = osync_plugin_config_get_resources(config);
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	if (!osync_plugin_discover(client->plugin, client->plugin_data, client->plugin_info, error))
		goto error;

	reply = osync_message_new_reply(message, error);
	if (!reply)
		goto error;

	if (osync_plugin_info_get_main_sink(client->plugin_info))
		osync_message_write_int(reply, 1);
	else
		osync_message_write_int(reply, 0);

	unsigned int numobjs = osync_plugin_info_num_objtypes(client->plugin_info);
	unsigned int avail = 0;
	for (i = 0; i < numobjs; i++) {
		OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(client->plugin_info, i);
		if (osync_objtype_sink_is_available(sink)) {
			avail++;
		}
	}

	osync_message_write_uint(reply, avail);
	
	for (i = 0; i < numobjs; i++) {
		OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(client->plugin_info, i);
		if (osync_objtype_sink_is_available(sink)) {
			if (!osync_marshal_objtype_sink(reply, sink, error))
				goto error_free_message;
		}
	}

	OSyncVersion *version = osync_plugin_info_get_version(client->plugin_info);
	if (version) {
		osync_message_write_int(reply, 1);
		osync_message_write_string(reply, osync_version_get_plugin(version));
		osync_message_write_string(reply, osync_version_get_priority(version));
		osync_message_write_string(reply, osync_version_get_vendor(version));
		osync_message_write_string(reply, osync_version_get_modelversion(version));
		osync_message_write_string(reply, osync_version_get_firmwareversion(version));
		osync_message_write_string(reply, osync_version_get_softwareversion(version));
		osync_message_write_string(reply, osync_version_get_hardwareversion(version));
		osync_message_write_string(reply, osync_version_get_identifier(version));
	}else
		osync_message_write_int(reply, 0);
	
	/* Report detected capabilities */
	OSyncCapabilities *capabilities = osync_plugin_info_get_capabilities(client->plugin_info);
	if (capabilities) {
		char* buffer;
		int size;
		osync_message_write_int(reply, 1);
		if(!osync_capabilities_assemble(capabilities, &buffer, &size))
			goto error_free_message;
		osync_message_write_string(reply, buffer);
		g_free(buffer);
	}else
		osync_message_write_int(reply, 0);

	/* Report, detected Resources */
	res = osync_plugin_config_get_resources(config);
	unsigned int num_res = osync_list_length(res);

	osync_message_write_uint(reply, num_res);
	for (; res; res = res->next) {
		OSyncPluginResource *resource = res->data;
		if (!osync_marshal_pluginresource(reply, resource, error))
			goto error_free_message;
	}

	if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
		goto error_free_message;
	
	osync_message_unref(reply);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_message:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_connect(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	char *objtype = NULL;
	int slowsync;
	OSyncMessage *reply = NULL;

	/* The message content is read and the objtype is written back immediately
	 * because the connect callback needs a safe way to determine the correct
	 * sink. The current sink of the plugin info is usually not correct
	 * because the connect functions work asynchronous and the last sink
	 * (usually the main sink) is the current sink.
	 */
	osync_message_read_string(message, &objtype);
	osync_message_read_int(message, &slowsync);
	osync_message_write_string(message, objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		/* SlowSync dummy value for connect reply message handler. */
		osync_message_write_int(reply, FALSE);

		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		/* set slowsync.
		   otherwise disable slowsync - to avoid slowsyncs every time with the same initiliazed engine
		   without finalizing the engine the next sync with the same engine would be again a slow-sync.
		   (unittest: sync - testcases: sync_easy_new_del, sync_easy_new_mapping) */
		if (slowsync)
			osync_objtype_sink_set_slowsync(sink, TRUE);
		else
			osync_objtype_sink_set_slowsync(sink, FALSE);

		OSyncContext *context = _create_context(client, message, _osync_client_connect_callback, NULL, error);
		if (!context)
			goto error;
		
		osync_plugin_info_set_sink(client->plugin_info, sink);
		osync_objtype_sink_connect(sink, client->plugin_data, client->plugin_info, context);

		osync_context_unref(context);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_disconnect(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	char *objtype = NULL;
	OSyncMessage *reply = NULL;
	
	osync_message_read_string(message, &objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		OSyncContext *context = _create_context(client, message, _osync_client_disconnect_callback, NULL, error);
		if (!context)
			goto error;
		
		osync_plugin_info_set_sink(client->plugin_info, sink);
		osync_objtype_sink_disconnect(sink, client->plugin_data, client->plugin_info, context);
	
		osync_context_unref(context);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_get_changes(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	int slowsync;
	char *objtype = NULL;
	OSyncMessage *reply = NULL;
	
	osync_message_read_string(message, &objtype);
	osync_message_read_int(message, &slowsync);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s (slowsync: %i)", objtype, slowsync);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		/* set slowsync (again) if the slow-sync got requested during the connect() call
		   of a member - and not during frontend/engine itself (e.g. anchor mismatch). */
		if (slowsync)
			osync_objtype_sink_set_slowsync(sink, TRUE);
		else
			osync_objtype_sink_set_slowsync(sink, FALSE);

		OSyncContext *context = _create_context(client, message, _osync_client_get_changes_callback, NULL, error);
		if (!context)
			goto error;
		osync_context_set_changes_callback(context, _osync_client_change_callback);
		
		osync_plugin_info_set_sink(client->plugin_info, sink);

		osync_objtype_sink_get_changes(sink, client->plugin_data, client->plugin_info, context);
	
		osync_context_unref(context);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_read_change(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	const char *objtype = NULL;
	OSyncMessage *reply = NULL;
	
	OSyncChange *change = NULL;
	
	if (!osync_demarshal_change(message, &change, client->format_env, error))
		goto error;
		
	osync_trace(TRACE_INTERNAL, "Change %p", change);

	objtype = osync_data_get_objtype(osync_change_get_data(change));

	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			goto error;
		}
	} else {
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
	}
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		OSyncContext *context = _create_context(client, message, _osync_client_read_callback, change, error);
		if (!context)
			goto error;
		
		osync_plugin_info_set_sink(client->plugin_info, sink);

		osync_objtype_sink_read_change(sink, client->plugin_data, client->plugin_info, change, context);
	
		osync_context_unref(context);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static osync_bool _osync_client_handle_commit_change(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	OSyncChange *change = NULL;
	
	if (!osync_demarshal_change(message, &change, client->format_env, error))
		goto error;
		
	osync_trace(TRACE_INTERNAL, "Change %p", change);
	
	OSyncData *data = osync_change_get_data(change);
	
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", osync_data_get_objtype(data));
	
	OSyncObjTypeSink *sink = NULL;
	sink = osync_plugin_info_find_objtype(client->plugin_info, osync_data_get_objtype(data));
	
	if (!sink) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", osync_data_get_objtype(data));
		osync_change_unref(change);
		goto error;
	}
		
	OSyncContext *context = _create_context(client, message, _osync_client_commit_change_callback, change, error);
	if (!context)
		goto error;
		
	osync_plugin_info_set_sink(client->plugin_info, sink);
	osync_objtype_sink_commit_change(sink, client->plugin_data, client->plugin_info, change, context);
	
	osync_change_unref(change);
	osync_context_unref(context);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_committed_all(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	char *objtype = NULL;
	OSyncMessage *reply = NULL;
	
	osync_message_read_string(message, &objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		OSyncContext *context = _create_context(client, message, _osync_client_committed_all_callback, NULL, error);
		if (!context)
			goto error;
		
		osync_plugin_info_set_sink(client->plugin_info, sink);
		osync_objtype_sink_committed_all(sink, client->plugin_data, client->plugin_info, context);
	
		osync_context_unref(context);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_sync_done(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	char *objtype = NULL;
	OSyncMessage *reply = NULL;
	
	osync_message_read_string(message, &objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype) {
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
		
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
			g_free(objtype);
			goto error;
		}
		
		g_free(objtype);
	} else
		sink = osync_plugin_info_get_main_sink(client->plugin_info);
		
	if (!sink) {
		reply = osync_message_new_reply(message, error);
		if (!reply)
			goto error;
		
		if (!osync_queue_send_message(client->outgoing, NULL, reply, error))
			goto error_free_reply;
		
		osync_message_unref(reply);
	} else {
		OSyncContext *context = _create_context(client, message, _osync_client_sync_done_callback, NULL, error);
		if (!context)
			goto error;
		
		osync_plugin_info_set_sink(client->plugin_info, sink);
		osync_objtype_sink_sync_done(sink, client->plugin_data, client->plugin_info, context);
	
		osync_context_unref(context);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_reply:
	osync_message_unref(reply);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void _osync_client_message_handler(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncClient *client = user_data;

	OSyncError *error = NULL;

	osync_trace(TRACE_INTERNAL, "plugin received command %i (%s)", osync_message_get_command(message), osync_message_get_commandstr(message));

	switch (osync_message_get_command(message)) {
		case OSYNC_MESSAGE_NOOP:
		case OSYNC_MESSAGE_REPLY:
	  	case OSYNC_MESSAGE_ERRORREPLY:
	  	case OSYNC_MESSAGE_NEW_CHANGE:
	  	case OSYNC_MESSAGE_SYNCHRONIZE:
		case OSYNC_MESSAGE_ENGINE_CHANGED:
		case OSYNC_MESSAGE_MAPPING_CHANGED:
		case OSYNC_MESSAGE_MAPPINGENTRY_CHANGED:
			//Ignore these. They dont have any meaning to the client
			break;
		case OSYNC_MESSAGE_QUEUE_ERROR:
		case OSYNC_MESSAGE_ERROR:
		case OSYNC_MESSAGE_QUEUE_HUP:
			/* Handle disconnect here */
			break;
			
		case OSYNC_MESSAGE_INITIALIZE:
			if (!_osync_client_handle_initialize(client, message, &error))
				goto error;
			break;
			
		case OSYNC_MESSAGE_FINALIZE:
			if (!_osync_client_handle_finalize(client, message, &error))
				goto error;
			break;
			
		case OSYNC_MESSAGE_DISCOVER:
			if (!_osync_client_handle_discover(client, message, &error))
				goto error;
			break;
			
		case OSYNC_MESSAGE_CONNECT:
			if (!_osync_client_handle_connect(client, message, &error))
				goto error;
			break;
	
		case OSYNC_MESSAGE_DISCONNECT:
			if (!_osync_client_handle_disconnect(client, message, &error))
				goto error;
			break;
			
		case OSYNC_MESSAGE_GET_CHANGES:
			if (!_osync_client_handle_get_changes(client, message, &error))
				goto error;
			break;
	
		case OSYNC_MESSAGE_COMMIT_CHANGE:
			if (!_osync_client_handle_commit_change(client, message, &error))
				goto error;
			break;
			
		case OSYNC_MESSAGE_SYNC_DONE:
			if (!_osync_client_handle_sync_done(client, message, &error))
				goto error;
			break;
			
	  	case OSYNC_MESSAGE_COMMITTED_ALL:
			if (!_osync_client_handle_committed_all(client, message, &error))
				goto error;
			break;
	
		case OSYNC_MESSAGE_READ_CHANGE:
			if (!_osync_client_handle_read_change(client, message, &error))
				goto error;
			break;
	
		case OSYNC_MESSAGE_CALL_PLUGIN:
			/*
			char *function = itm_message_get_data(message, "function");
			void *data = itm_message_get_data(message, "data");
			OSyncError *error = NULL;
			void *replydata = osync_member_call_plugin(client->member, function, data, &error);
	
			if (itm_message_get_data(message, "want_reply")) {
				ITMessage *reply = NULL;
				if (!osync_error_is_set(&error)) {
					reply = itm_message_new_methodreply(client, message);
					itm_message_set_data(message, "reply", replydata);
				} else {
					reply = itm_message_new_errorreply(client, message);
					itm_message_set_error(reply, error);
				}
	
				itm_message_send_reply(reply);
			}
			*/
			break;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:;
	if (!client->outgoing) {
		client->thread = NULL;
		osync_client_shutdown(client);
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to notify parent. no outgoing queue: %s", __func__, osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}

	OSyncError *locerror = NULL;
	OSyncMessage *errorreply = osync_message_new_errorreply(message, error, &locerror);
	if (!errorreply) {
		osync_client_error_shutdown(client, locerror);
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT_ERROR, "%s: Error while sending error: %s", __func__, osync_error_print(&locerror));
		osync_error_unref(&locerror);
		return;
	}

	if (!osync_queue_send_message(client->outgoing, NULL, errorreply, &locerror)) {
		osync_client_error_shutdown(client, locerror);
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT_ERROR, "%s: Error while sending error: %s", __func__, osync_error_print(&locerror));
		osync_error_unref(&locerror);
		return;
	}

	osync_message_unref(errorreply);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

/** This function takes care of the messages received on the outgoing (sending)
 * queue. The only messages we can receive there, are HUPs or ERRORs. */
static void _osync_client_hup_handler(OSyncMessage *message, void *user_data)
{
	OSyncClient *client = user_data;
	OSyncError *error = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);

	osync_trace(TRACE_INTERNAL, "plugin received command %i on sending queue", osync_message_get_command(message));

	if (osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_ERROR) {
		/* Houston, we have a problem */
	} else if (osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP) {
		/* The remote side disconnected. So we can now disconnect as well and then
		 * shutdown */
		if (!osync_queue_disconnect(client->outgoing, &error))
			osync_error_unref(&error);
		
		if (!osync_queue_disconnect(client->incoming, &error))
			osync_error_unref(&error);
		
		if (client->syncloop) {
			g_main_loop_quit(client->syncloop);
		}
	} else {
		/* This should never ever happen */
		osync_trace(TRACE_ERROR, "received neither a hup, nor a error on a sending queue...");
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

OSyncClient *osync_client_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncClient *client = osync_try_malloc0(sizeof(OSyncClient), error);
	if (!client) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	client->ref_count = 1;
	client->context = g_main_context_new();
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, client);
	return client;
}

OSyncClient *osync_client_ref(OSyncClient *client)
{
	osync_assert(client);
	
	g_atomic_int_inc(&(client->ref_count));

	return client;
}

void osync_client_unref(OSyncClient *client)
{
	osync_assert(client);
	
	if (g_atomic_int_dec_and_test(&(client->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
		
		if(client->disconnectThread) {
			g_thread_join(client->disconnectThread);
			client->disconnectThread = NULL;
		}

		if (client->incoming) {
			if (osync_queue_is_connected(client->incoming))
				osync_queue_disconnect(client->incoming, NULL);
			osync_queue_remove(client->incoming, NULL);
			osync_queue_free(client->incoming);
		}
	
		if (client->outgoing) {
			if (osync_queue_is_connected(client->outgoing))
				osync_queue_disconnect(client->outgoing, NULL);
			osync_queue_free(client->outgoing);
		}
		
		if (client->plugin)
			osync_plugin_unref(client->plugin);
		
		if (client->thread)
			osync_thread_free(client->thread);
		
		g_free(client);
		
		osync_trace(TRACE_EXIT, "%s", __func__);
	}
}

void osync_client_set_incoming_queue(OSyncClient *client, OSyncQueue *incoming)
{
	osync_queue_set_message_handler(incoming, _osync_client_message_handler, client);
	osync_queue_setup_with_gmainloop(incoming, client->context);
	client->incoming = incoming;
}

void osync_client_set_outgoing_queue(OSyncClient *client, OSyncQueue *outgoing)
{
	osync_queue_set_message_handler(outgoing, _osync_client_hup_handler, client);
	osync_queue_setup_with_gmainloop(outgoing, client->context);
	client->outgoing = outgoing;
}

void osync_client_run_and_block(OSyncClient *client)
{
	client->syncloop = g_main_loop_new(client->context, TRUE);
	g_main_loop_run(client->syncloop);
}

osync_bool osync_client_run(OSyncClient *client, OSyncError **error)
{
	client->thread = osync_thread_new(client->context, error);
	if (!client->thread)
		return FALSE;
		
	osync_thread_start(client->thread);
	
	return TRUE;
}

static gboolean osyncClientConnectCallback(gpointer data)
{
	OSyncClient *client = data;
	osync_trace(TRACE_INTERNAL, "About to connect to the incoming queue");
	
	/* We now connect to our incoming queue */
	if (!osync_queue_connect(client->incoming, OSYNC_QUEUE_RECEIVER, NULL))
		return TRUE;
	
	return FALSE;
}


osync_bool osync_client_run_external(OSyncClient *client, char *pipe_path, OSyncPlugin *plugin, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p)", __func__, client, pipe_path, plugin, error);
	/* Create connection pipes **/
	OSyncQueue *incoming = osync_queue_new(pipe_path, error);
	if (!incoming)
		goto error;
	
	if (!osync_queue_create(incoming, error))
		goto error_free_queue;
	
	osync_client_set_incoming_queue(client, incoming);
	
	client->thread = osync_thread_new(client->context, error);
	if (!client->thread)
		goto error_remove_queue;
	
	osync_thread_start(client->thread);
	
	client->plugin = plugin;
	osync_plugin_ref(client->plugin);
	
	GSource *source = NULL;
	
	source = g_idle_source_new();
	g_source_set_callback(source, osyncClientConnectCallback, client, NULL);
	g_source_attach(source, client->context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_remove_queue:
	osync_queue_remove(incoming, NULL);
error_free_queue:
	osync_queue_free(incoming);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static gboolean osyncClientDisconnectCallback(gpointer data)
{
	OSyncClient *client = data;
	
	/* First, we disconnect our incoming queue. This will generate a HUP on the remote
	 * side. We dont disconnect our outgoing queue yet, since we have to make sure that
	 * all data is read. Only the listener should disconnect a pipe! */
	osync_queue_disconnect(client->incoming, NULL);

	if (client->outgoing) {
		/* We now wait until the other side disconnect our outgoing queue */
		while (osync_queue_is_connected(client->outgoing)) { g_usleep(100); }

		/* Gives some time if anyone wants to grab the HUP message from the outgoing part of the pipe */
		g_usleep(200);	
		/* Now we can safely disconnect our outgoing queue */
		osync_queue_disconnect(client->outgoing, NULL);
	}
	
	return FALSE;
}

/* The aim of this thread is to avoid blocking the all the engine event sources with the osyncClientDisconnectCallback internal loop */
static void client_disconnect_workerthread(gpointer data) 
{
	OSyncClient *client = data;
	GMainContext *context = g_main_context_new();
	GSource *source = NULL;
	source = g_idle_source_new();
	g_source_set_callback(source, osyncClientDisconnectCallback, client, NULL);
	g_source_attach(source, context);
	OSyncThread *thread = osync_thread_new(context, NULL);
	osync_thread_start(thread);

	osync_thread_stop(thread);
	osync_thread_free(thread);
	thread = NULL;	
	g_source_unref(source);
}

	
void osync_client_disconnect(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	
	client->disconnectThread = g_thread_create((GThreadFunc)client_disconnect_workerthread, client, TRUE, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_client_shutdown(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	osync_assert(client);
	
	osync_client_disconnect(client);
			
	if (client->syncloop) {
		if (g_main_loop_is_running(client->syncloop)) {

			/* now we can quit the main loop */
			g_main_loop_quit(client->syncloop);
		}
		
		g_main_loop_unref(client->syncloop);
		client->syncloop = NULL;
	} else if (client->thread) {
		osync_thread_stop(client->thread);
		osync_thread_free(client->thread);
		client->thread = NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_client_error_shutdown(OSyncClient *client, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, client, error);

	OSyncMessage *message = osync_message_new_error(error, NULL);
	if (message) {
		osync_queue_send_message(client->outgoing, NULL, message, NULL);
		osync_message_unref(message);
	}

	osync_client_shutdown(client);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*void *osync_client_message_sink(OSyncMember *member, const char *name, void *data, osync_bool synchronous)
{
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	if (!synchronous) {

		ITMessage *message = itm_message_new_signal(client, "PLUGIN_MESSAGE");
		osync_debug("CLI", 3, "Sending message %p PLUGIN_MESSAGE for message %s", message, name);
		itm_message_set_data(message, "data", data);
		itm_message_set_data(message, "name", g_strdup(name));
		itm_queue_send(engine->incoming, message);

		return NULL;
	} else {
		return engine->plgmsg_callback(engine, client, name, data, engine->plgmsg_userdata);
	}

  return NULL;
}*/
