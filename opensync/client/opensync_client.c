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
#include "opensync-client.h"
#include "opensync_client_internals.h"

typedef struct callContext {
	OSyncClient *client;
	OSyncMessage *message;
} callContext;

static OSyncContext *_create_context(OSyncClient *client, OSyncMessage *message, OSyncContextCallbackFn callback, OSyncError **error)
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
	
	g_free(baton);
}

/*static void _osync_client_changes_sink(OSyncPlugin *plugin, OSyncChange *change, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, change, user_data);
	context *ctx = (context *)user_data;
	PluginProcess *pp = ctx->pp;
	OSyncMessage *orig = ctx->message;

	OSyncError *error = NULL;

	if (osync_message_is_answered(orig)) {
		osync_change_free(change);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NEW_CHANGE, 0, &error);
	if (!message)
		process_error_shutdown(pp, &error);

	osync_marshal_change(message, change);

	osync_message_write_long_long_int(message, osync_member_get_id(member));

	if (!osync_queue_send_message(pp->outgoing, NULL, message, &error))
		process_error_shutdown(pp, &error);

	osync_trace(TRACE_EXIT, "%s", __func__);
}*/

static void _osync_client_connect_callback(void *data, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	OSyncError *locerror = NULL;
	callContext *baton = data;

	OSyncMessage *message = baton->message;
	OSyncClient *client = baton->client;

	OSyncMessage *reply = NULL;
	if (!osync_error_is_set(&error)) {
		reply = osync_message_new_reply(message, &locerror);
		//Send connect specific reply data
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

static osync_bool _osync_client_handle_initialize(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = NULL;
	char *enginepipe = NULL;
	char *pluginname = NULL;
	char *plugindir = NULL;
	char *configdir = NULL;
	char *formatdir = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);
	
	osync_message_read_string(message, &enginepipe);
	osync_message_read_string(message, &formatdir);
	osync_message_read_string(message, &plugindir);
	osync_message_read_string(message, &pluginname);
	osync_message_read_string(message, &configdir);
	
	osync_trace(TRACE_INTERNAL, "enginepipe %s, plugin %s", enginepipe, pluginname);
		
	/* First we connect the engine pipe if necessary*/
	if (enginepipe) {
		client->outgoing = osync_queue_new(enginepipe, error);
		if (!client->outgoing)
			goto error;
		
		osync_trace(TRACE_INTERNAL, "connecting to engine");
		
		if (!osync_queue_connect(client->outgoing, OSYNC_QUEUE_SENDER, error))
			goto error;
		
		osync_trace(TRACE_INTERNAL, "done connecting to engine");
	}
	
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

	client->plugin_info = osync_plugin_info_new(error);
	if (!client->plugin_info)
		goto error;
	
	osync_plugin_info_set_configdir(client->plugin_info, configdir);
	osync_plugin_info_set_loop(client->plugin_info, client->context);

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
	g_free(formatdir);
	
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
	g_free(formatdir);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_finalize(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);
	
	osync_plugin_finalize(client->plugin, client->plugin_data);
	
	osync_plugin_unref(client->plugin);
	client->plugin = NULL;
	
	osync_plugin_env_free(client->plugin_env);
	client->plugin_env = NULL;
	
	osync_plugin_info_unref(client->plugin_info);
	client->plugin_info = NULL;
	
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
	int i = 0;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	if (!osync_plugin_discover(client->plugin, client->plugin_data, client->plugin_info, error))
		goto error;

	reply = osync_message_new_reply(message, error);
	if (!reply)
		goto error;

	if (osync_plugin_info_get_sink(client->plugin_info))
		osync_message_write_int(reply, 1);
	else
		osync_message_write_int(reply, 0);

	int numobjs = osync_plugin_info_num_objtypes(client->plugin_info);
	osync_message_write_int(reply, numobjs);
	
	for (i = 0; i < numobjs; i++) {
		OSyncObjTypeSink *sink = osync_plugin_info_nth_objtype(client->plugin_info, i);
		if (osync_objtype_sink_is_available(sink)) {
			if (!osync_marshal_objtype_sink(reply, sink, error))
				goto error_free_message;
		}
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
	osync_message_read_string(message, &objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype)
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
	else
		sink = osync_plugin_info_get_sink(client->plugin_info);
		
	if (!sink) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
		g_free(objtype);
		goto error;
	}
	g_free(objtype);
	
	OSyncContext *context = _create_context(client, message, _osync_client_connect_callback, error);
	if (!context)
		goto error;
	
	osync_objtype_sink_connect(sink, client->plugin_data, client->plugin_info, context);
	
	osync_context_unref(context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_client_handle_disconnect(OSyncClient *client, OSyncMessage *message, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, message, error);

	char *objtype = NULL;
	osync_message_read_string(message, &objtype);
	osync_trace(TRACE_INTERNAL, "Searching sink for %s", objtype);
	
	OSyncObjTypeSink *sink = NULL;
	if (objtype)
		sink = osync_plugin_info_find_objtype(client->plugin_info, objtype);
	else
		sink = osync_plugin_info_get_sink(client->plugin_info);
		
	if (!sink) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
		g_free(objtype);
		goto error;
	}
	g_free(objtype);
	
	OSyncContext *context = _create_context(client, message, _osync_client_disconnect_callback, error);
	if (!context)
		goto error;
	
	osync_objtype_sink_disconnect(sink, client->plugin_data, client->plugin_info, context);
		
	osync_context_unref(context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void _osync_client_message_handler(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncClient *client = user_data;

	OSyncError *error = NULL;

	osync_trace(TRACE_INTERNAL, "plugin received command %i", osync_message_get_command(message));

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
			break;
	
		case OSYNC_MESSAGE_COMMIT_CHANGE:
			/*OSyncChange *change;
	  		osync_demarshal_change(message, member->group->conv_env, &change);
			osync_change_set_member(change, member);
		  	osync_member_commit_change(member, change, (OSyncEngCallback)message_callback, ctx);*/
			break;
			
		case OSYNC_MESSAGE_SYNC_DONE:
			/*ctx = g_malloc0(sizeof(context));
			ctx->pp = pp;
			ctx->message = message;
			osync_message_ref(message);
	  		osync_member_sync_done(member, (OSyncEngCallback)message_callback, ctx);*/
			break;
			
	  	case OSYNC_MESSAGE_COMMITTED_ALL:
			/*ctx = g_malloc0(sizeof(context));
			ctx->pp = pp;
			ctx->message = message;
			osync_message_ref(message);
	  		osync_member_committed_all(member, (OSyncEngCallback)message_callback, ctx);*/
			break;
	
		/*case OSYNC_MESSAGE_READ_CHANGE:
			osync_demarshal_change( queue, &change, &error );
			osync_member_read_change(client->member, change, (OSyncEngCallback)message_callback, message);
			osync_trace(TRACE_EXIT, "message_handler");
			break;
		*/
	
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

void osync_client_ref(OSyncClient *client)
{
	osync_assert(client);
	
	g_atomic_int_inc(&(client->ref_count));
}

void osync_client_unref(OSyncClient *client)
{
	osync_assert(client);
	
	if (g_atomic_int_dec_and_test(&(client->ref_count))) {
		osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
		
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
	osync_queue_connect(client->incoming, OSYNC_QUEUE_RECEIVER, NULL);
	
	return FALSE;
}


osync_bool osync_client_run_external(OSyncClient *client, char *pipe_path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, client, pipe_path, error);
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

static gboolean osyncClientStopCallback(gpointer data)
{
	OSyncClient *client = data;
	
	/* First, we disconnect our incoming queue. This will generate a HUP on the remote
	 * side. We dont disconnect our outgoing queue yet, since we have to make sure that
	 * all data is read. Only the listener should disconnect a pipe! */
	osync_queue_disconnect(client->incoming, NULL);

	/* We now wait until the other side disconnect our outgoing queue */
	while (osync_queue_is_connected(client->outgoing)) { usleep(100); }
	
	/* Now we can safely disconnect our outgoing queue */
	osync_queue_disconnect(client->outgoing, NULL);

	/* now we can quit the main loop */
	g_main_loop_quit(client->syncloop);
	return FALSE;
}

void osync_client_shutdown(OSyncClient *client)
{
	GSource *source = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	osync_assert(client);
	
	if (client->syncloop) {
		if (g_main_loop_is_running(client->syncloop)) {
			source = g_idle_source_new();
			g_source_set_callback(source, osyncClientStopCallback, client, NULL);
			g_source_attach(source, client->context);
			
			g_source_unref(source);
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
	if (message)
		osync_queue_send_message(client->outgoing, NULL, message, NULL);

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
