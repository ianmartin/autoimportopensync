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
	_osync_debug(client, "CLI", 3, "Creating new client %p", client);
	client->member = member;
	osync_member_set_data(member, client);
	client->engine = engine;
	engine->clients = g_list_append(engine->clients, client);
	client->incoming = itm_queue_new();
	
	client->fl_connected = osync_flag_new(NULL);
	client->fl_sent_changes = osync_flag_new(engine->cmb_sent_changes);
	client->fl_done = osync_flag_new(NULL);
	client->fl_finished = osync_flag_new(engine->cmb_finished);
	//client->fl_reset = osync_flag_new(engine->cmb_reset);
	
    return client;
}

void osync_client_free(OSyncClient *client)
{
	_osync_debug(client, "CLI", 3, "Freeing client");
	itm_queue_free(client->incoming);
	g_main_loop_unref(client->memberloop);

	osync_flag_free(client->fl_connected);
	osync_flag_free(client->fl_sent_changes);
	osync_flag_free(client->fl_done);
	osync_flag_free(client->fl_finished);

	g_free(client);
}

void *osync_client_message_sink(OSyncMember *member, const char *name, void *data, osync_bool synchronous)
{
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	if (!synchronous) {
		ITMessage *message = itm_message_new_signal(client, "PLUGIN_MESSAGE");
		_osync_debug(client, "CLI", 3, "Sending message %p PLUGIN_MESSAGE for message", message, name);
		itm_message_set_data(message, "data", data);
		itm_message_set_data(message, "name", g_strdup(name));
		itm_queue_send(engine->incoming, message);
		return NULL;
	} else {
		return engine->plgmsg_callback(engine, client, name, data, engine->plgmsg_userdata);
	}	
}

void osync_client_changes_sink(OSyncMember *member, OSyncChange *change)
{
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "NEW_CHANGE");
	itm_message_set_data(message, "change", change);
	itm_queue_send(engine->incoming, message);
}

void osync_client_sync_alert_sink(OSyncMember *member)
{
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "SYNC_ALERT");
	itm_queue_send(engine->incoming, message);
}

void message_callback(OSyncMember *member, ITMessage *message, OSyncError **error)
{
	OSyncClient *client = osync_member_get_data(member);
	ITMessage *reply = NULL;

	if (!osync_error_is_set(error)) {
		reply = itm_message_new_methodreply(client, message);
		_osync_debug(client, "CLI", 3, "Member is replying with message %p to message %p with no error", reply, message);
	} else {
		reply = itm_message_new_errorreply(client, message);
		itm_message_set_error(reply, (*error)->message, (*error)->type);
		_osync_debug(client, "CLI", 3, "Member is replying with message %p to message %p with error %i", reply, message, (*error)->type);
		osync_error_free(error);
	}
	itm_message_move_data(message, reply);
	itm_message_send_reply(reply);
}

void client_message_handler(OSyncEngine *sender, ITMessage *message, OSyncClient *client)
{
	_osync_debug(client, "CLI", 3, "Messages handler called for message %p from engine", message);

	if (itm_message_is_methodcall(message, "CONNECT")) {
		osync_member_connect(client->member, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_methodcall(message, "GET_CHANGES")) {
		osync_member_get_changeinfo(client->member, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_methodcall(message, "COMMIT_CHANGE")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_member_commit_change(client->member, change, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_methodcall(message, "SYNC_DONE")) {
		osync_member_sync_done(client->member, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_methodcall(message, "DISCONNECT")) {
		osync_member_disconnect(client->member, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_methodcall(message, "GET_DATA")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_member_get_change_data(client->member, change, (OSyncEngCallback)message_callback, message);
		return;
	}
	
	if (itm_message_is_signal(message, "CALL_PLUGIN")) {
		char *function = itm_message_get_data(message, "function");
		void *data = itm_message_get_data(message, "data");
		OSyncError *error = NULL;
		osync_member_call_plugin(client->member, function, data, &error);
		if (osync_error_is_set(&error))
			message_callback(client->member, message, &error);
		return;
	}
	
	_osync_debug(client, "CLI", 0, "Unknown message \"%s\"\n", itm_message_get_msgname(message));
	g_assert_not_reached();
}

void osync_client_call_plugin(OSyncClient *client, char *function, void *data)
{
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(engine, "CALL_PLUGIN");
	_osync_debug(client, "CLI", 3, "Sending message %p CALL_PLUGIN for function %s", message, function);
	itm_message_set_data(message, "data", data);
	itm_message_set_data(message, "function", g_strdup(function));
	itm_queue_send(client->incoming, message);
}

/*
void osync_client_call_plugin_with_reply(OSyncClient *client, char *function, void *data, void ( *replyhandler)(OSyncEngine *, OSyncClient *, void *, OSyncError *), int timeout)
{
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(engine, "CALL_PLUGIN");
	_osync_debug(client, "CLI", 3, "Sending message %p CALL_PLUGIN for function %s", message, function);
	itm_message_set_data(message, "data", data);
	itm_message_set_data(message, "function", g_strdup(function));
	itm_queue_send_with_reply(client->incoming, message);
}*/

osync_bool osync_client_init(OSyncClient *client, OSyncError **error)
{
	_osync_debug(client, "CLI", 3, "Starting client mainloop");
	GMainContext *context = g_main_context_new();
	client->memberloop = g_main_loop_new(context, FALSE);
	
	//Set the callback functions
	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(client->member);
	functions->rf_change = osync_client_changes_sink;
	functions->rf_message = osync_client_message_sink;
	functions->rf_sync_alert = osync_client_sync_alert_sink;

	//Start the queue
	itm_queue_set_message_handler(client->incoming, (ITMessageHandler)client_message_handler, client);
	itm_queue_setup_with_gmainloop(client->incoming, context);

	//Call the init function
	if (!osync_member_initialize(client->member, error))
		return FALSE;
	
	_osync_debug(client, "CLI", 3, "Spawning new client thread");
	g_thread_create ((GThreadFunc)g_main_loop_run, client->memberloop, TRUE, NULL);
	return TRUE;
}

void osync_client_finalize(OSyncClient *client)
{
	_osync_debug(client, "CLI", 3, "Finalizing client");
	g_main_loop_quit(client->memberloop);
	osync_member_finalize(client->member);
	osync_client_free(client);
}
