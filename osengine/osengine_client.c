/*
 * libosengine - A synchronization engine for the opensync framework
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
 
#include "engine.h"
#include "engine_internals.h"

OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member)
{
	OSyncClient *client = g_malloc0(sizeof(OSyncClient));
	osync_debug("CLI", 3, "Creating new client %p", client);
	client->member = member;
	osync_member_set_data(member, client);
	client->engine = engine;
	engine->clients = g_list_append(engine->clients, client);
	client->incoming = itm_queue_new();
	
	client->context = g_main_context_new();
	client->memberloop = g_main_loop_new(client->context, FALSE);
	
	client->started_mutex = g_mutex_new();
	client->started = g_cond_new();
	
	client->fl_connected = osync_flag_new(engine->cmb_connected);
	client->fl_sent_changes = osync_flag_new(engine->cmb_sent_changes);
	client->fl_done = osync_flag_new(NULL);
	client->fl_committed_all = osync_flag_new(engine->cmb_committed_all_sent);
	client->fl_finished = osync_flag_new(engine->cmb_finished);
	
    return client;
}

void osync_client_reset(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	osync_flag_set_state(client->fl_connected, FALSE);
	osync_flag_set_state(client->fl_sent_changes, FALSE);
	osync_flag_set_state(client->fl_done, FALSE);
	osync_flag_set_state(client->fl_finished, FALSE);
	osync_flag_set_state(client->fl_committed_all, FALSE);
	itm_queue_flush(client->incoming);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_client_free(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "osync_client_free(%p)", client);
	itm_queue_free(client->incoming);
	g_main_loop_unref(client->memberloop);
	g_main_context_unref(client->context);
	
	g_mutex_free(client->started_mutex);
	g_cond_free(client->started);
	
	osync_flag_free(client->fl_connected);
	osync_flag_free(client->fl_sent_changes);
	osync_flag_free(client->fl_done);
	osync_flag_free(client->fl_finished);
	osync_flag_free(client->fl_committed_all);

	g_free(client);
	osync_trace(TRACE_EXIT, "osync_client_free");
}

void *osync_client_message_sink(OSyncMember *member, const char *name, void *data, osync_bool synchronous)
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
}

void osync_client_changes_sink(OSyncMember *member, OSyncChange *change, void *user_data)
{
	ITMessage *orig = (ITMessage *)user_data;
	
	if (itm_message_is_answered(orig))
		return; //FIXME How do we free the change here?
	
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "NEW_CHANGE");
	itm_message_set_data(message, "change", change);
	itm_message_reset_timeout(orig);
	itm_queue_send(engine->incoming, message);
}

void osync_client_sync_alert_sink(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "SYNC_ALERT");
	itm_queue_send(engine->incoming, message);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void message_callback(OSyncMember *member, ITMessage *message, OSyncError **error)
{
	OSyncClient *client = osync_member_get_data(member);
	ITMessage *reply = NULL;

	//FIXME This has to be protected by a mutex
	if (itm_message_is_answered(message) == TRUE)
		return;

	if (!osync_error_is_set(error)) {
		reply = itm_message_new_methodreply(client, message);
		osync_debug("CLI", 4, "Member is replying with message %p to message %p:\"%s\" with no error", reply, message, message->msgname);
	} else {
		reply = itm_message_new_errorreply(client, message);
		itm_message_set_error(reply, *error);
		osync_debug("CLI", 1, "Member is replying with message %p to message %p:\"%s\" with error %i: %s", reply, message, message->msgname, osync_error_get_type(error), osync_error_print(error));
	}
	
	itm_message_move_data(message, reply);
	itm_message_send_reply(reply);
	itm_message_set_answered(message);
}

void client_message_handler(OSyncEngine *sender, ITMessage *message, OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "client_message_handler(%p, %p, %p)", sender, message, client);
	osync_debug("CLI", 3, "Client message handler called for message \"%s\"", message->msgname);

	if (itm_message_is_methodcall(message, "CONNECT")) {
		osync_member_connect(client->member, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "GET_CHANGES")) {
		osync_member_get_changeinfo(client->member, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "COMMIT_CHANGE")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_member_commit_change(client->member, change, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "SYNC_DONE")) {
		osync_member_sync_done(client->member, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "DISCONNECT")) {
		osync_member_disconnect(client->member, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "GET_DATA")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_member_get_change_data(client->member, change, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}

	if (itm_message_is_methodcall(message, "COMMITTED_ALL")) {
		osync_member_committed_all(client->member, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	if (itm_message_is_methodcall(message, "READ_CHANGE")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_member_read_change(client->member, change, (OSyncEngCallback)message_callback, message);
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}

	if (itm_message_is_signal(message, "CALL_PLUGIN")) {
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
		osync_trace(TRACE_EXIT, "client_message_handler");
		return;
	}
	
	osync_debug("CLI", 0, "Unknown message \"%s\"\n", itm_message_get_msgname(message));
	osync_trace(TRACE_EXIT_ERROR, "client_message_handler: Unknown message");
	g_assert_not_reached();
}

OSyncPluginTimeouts osync_client_get_timeouts(OSyncClient *client)
{
	return osync_plugin_get_timeouts(osync_member_get_plugin(client->member));
}

static gboolean startupfunc(gpointer data)
{
	OSyncClient *client = data;
	osync_trace(TRACE_INTERNAL, "+++++++++ This is the client #%lli (%s plugin) of group %s +++++++++", osync_member_get_id(client->member), osync_member_get_pluginname(client->member), osync_group_get_name(client->engine->group));
	
	g_mutex_lock(client->started_mutex);
	g_cond_signal(client->started);
	g_mutex_unlock(client->started_mutex);
	return FALSE;
}

static void _recv_plugin_answer(OSyncEngine *engine, ITMessage *message, void *userdata)
{	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, message, userdata);
	OSyncPluginCallContext *ctx = userdata;
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		ctx->handler(NULL, ctx->userdata, error);
	} else {
		void *replydata = itm_message_get_data(message, "reply");
		ctx->handler(replydata, ctx->userdata, NULL);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_client_call_plugin(OSyncClient *client, char *function, void *data, OSyncPluginReplyHandler replyhandler, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, client, function, data, replyhandler, userdata);
	
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_methodcall(engine, "CALL_PLUGIN");
	itm_message_set_data(message, "data", data);
	itm_message_set_data(message, "function", g_strdup(function));
	
	if (replyhandler) {
		OSyncPluginCallContext *ctx = g_malloc0(sizeof(OSyncPluginCallContext));
		ctx->handler = replyhandler;
		ctx->userdata = userdata;
		itm_message_set_handler(message, engine->incoming, (ITMessageHandler)_recv_plugin_answer, ctx);

		itm_message_set_data(message, "want_reply", GINT_TO_POINTER(1));
	} else
		itm_message_set_data(message, "want_reply", GINT_TO_POINTER(0));
	
	itm_queue_send(client->incoming, message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*
void osync_client_call_plugin_with_reply(OSyncClient *client, char *function, void *data, void ( *replyhandler)(OSyncEngine *, OSyncClient *, void *, OSyncError *), int timeout)
{
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(engine, "CALL_PLUGIN");
	osync_debug("CLI", 3, "Sending message %p CALL_PLUGIN for function %s", message, function);
	itm_message_set_data(message, "data", data);
	itm_message_set_data(message, "function", g_strdup(function));
	itm_queue_send_with_reply(client->incoming, message);
}*/

osync_bool osync_client_init(OSyncClient *client, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_client_init(%p, %p)", client, error);
	
	//Set the callback functions
	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(client->member);
	functions->rf_change = osync_client_changes_sink;
	functions->rf_message = osync_client_message_sink;
	functions->rf_sync_alert = osync_client_sync_alert_sink;

	//Start the queue
	itm_queue_set_message_handler(client->incoming, (ITMessageHandler)client_message_handler, client);
	itm_queue_setup_with_gmainloop(client->incoming, client->context);
	osync_member_set_loop(client->member, client->context);

	//Call the init function
	if (!osync_member_initialize(client->member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "osync_client_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	g_mutex_lock(client->started_mutex);
	GSource *idle = g_idle_source_new();
	g_source_set_callback(idle, startupfunc, client, NULL);
    g_source_attach(idle, client->context);
	osync_trace(TRACE_INTERNAL, "Waiting for startup");
	client->thread = g_thread_create ((GThreadFunc)g_main_loop_run, client->memberloop, TRUE, NULL);
	g_cond_wait(client->started, client->started_mutex);
	osync_trace(TRACE_INTERNAL, "startup done!");
	g_mutex_unlock(client->started_mutex);
	
	osync_trace(TRACE_EXIT, "osync_client_init");
	client->is_initialized = TRUE;
	return TRUE;
}

void osync_client_finalize(OSyncClient *client)
{
	if (!client->is_initialized)
		return;
	
	osync_debug("CLI", 3, "Finalizing client");
	g_main_loop_quit(client->memberloop);
	g_thread_join(client->thread);
	
	osync_member_finalize(client->member);
	
	itm_queue_flush(client->incoming);
}
