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
#include <glib.h>
#include <opensync/opensync_support.h>
#include "opensync/opensync_message_internals.h"
#include "opensync/opensync_queue_internals.h"

#include "engine_internals.h"
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

 
/*! @brief This function can be used to receive GET_ENTRY command replies
 * 
 * See OSyncMessageHandler
 * 
 */
void _get_changes_reply_receiver(OSyncMessage *message, OSyncClient *sender)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, sender);
	OSyncEngine *engine = sender->engine;
	
	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("ENG", 1, "Get changes command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_GET_CHANGES_ERROR, &error);
		osync_error_update(&engine->error, "Unable to read from one of the members");
		osync_flag_unset(sender->fl_sent_changes);
		//osync_flag_set(sender->fl_finished);
		osync_flag_set(sender->fl_done);
		/*
		 * FIXME: For now we want to stop the engine if
		 * one of the member didnt connect yet. Later it should
		 * be that if >= 2 members connect, the sync should continue
		 */
		osync_flag_set(engine->fl_stop);
		
	} else {
		osync_status_update_member(engine, sender, MEMBER_SENT_CHANGES, NULL);
		osync_flag_set(sender->fl_sent_changes);
	}
	
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_get_changes_reply_receiver");
}

/*! @brief This function can be used to receive CONNECT command replies
 *
 * See OSyncMessageHandler
 * 
 */
void _connect_reply_receiver(OSyncMessage *message, OSyncClient *sender)
{
	osync_trace(TRACE_ENTRY, "_connect_reply_receiver(%p, %p)", message, sender);
	
	printf("connect reply %i\n", osync_message_is_error(message));
	OSyncEngine *engine = sender->engine;
	
	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("ENG", 1, "Connect command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_CONNECT_ERROR, &error);
		osync_error_update(&engine->error, "Unable to connect one of the members");
		osync_flag_unset(sender->fl_connected);
		osync_flag_set(sender->fl_finished);
		osync_flag_set(sender->fl_sent_changes);
		osync_flag_set(sender->fl_done);
		/*
		 * FIXME: For now we want to stop the engine if
		 * one of the member didnt connect yet. Later it should
		 * be that if >= 2 members connect, the sync should continue
		 */
		osync_flag_set(engine->fl_stop);
		
	} else {
		osync_status_update_member(engine, sender, MEMBER_CONNECTED, NULL);
		osync_flag_set(sender->fl_connected);	
	}

	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_connect_reply_receiver");
}

void _sync_done_reply_receiver(OSyncMessage *message, OSyncClient *sender)
{
	osync_trace(TRACE_ENTRY, "_sync_done_reply_receiver(%p, %p)", message, sender);

	OSyncEngine *engine = sender->engine;
	
	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("ENG", 1, "Sync done command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_SYNC_DONE_ERROR, &error);
		osync_error_update(&engine->error, "Unable to finish the sync for one of the members");
	}
	
	osync_flag_set(sender->fl_done);
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_sync_done_reply_receiver");
}

void _committed_all_reply_receiver(OSyncMessage *message, OSyncClient *sender)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, sender);

	OSyncEngine *engine = sender->engine;
	
	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("ENG", 1, "Committed all command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_COMMITTED_ALL_ERROR, &error);
		osync_error_update(&engine->error, "Unable to write changes to one of the members");
	} else
		osync_status_update_member(engine, sender, MEMBER_COMMITTED_ALL, NULL);
	
	osync_flag_set(sender->fl_committed_all);
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void _disconnect_reply_receiver(OSyncMessage *message, OSyncClient *sender)
{
	osync_trace(TRACE_ENTRY, "_disconnect_reply_receiver(%p, %p)", message, sender);
	
	OSyncEngine *engine = sender->engine;

	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_debug("ENG", 1, "Sync done command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_DISCONNECT_ERROR, &error);
	} else
		osync_status_update_member(engine, sender, MEMBER_DISCONNECTED, NULL);
			
	osync_flag_unset(sender->fl_connected);
	osync_flag_set(sender->fl_finished);
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_disconnect_reply_receiver");
}

void _get_change_data_reply_receiver(OSyncClient *sender, OSyncMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_get_change_data_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	//OSyncMappingEntry *entry = osync_message_get_data(message, "entry");
	
	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", osync_error_print(&error));
		//osync_status_update_change(engine, entry->change, CHANGE_RECV_ERROR, &error);
		osync_error_update(&engine->error, "Unable to read one or more objects");
		
		//FIXME Do we need to do anything here?
		//osync_flag_unset(entry->fl_has_data);
	} else {
		//osync_flag_set(entry->fl_has_data);
		//osync_status_update_change(engine, entry->change, CHANGE_RECEIVED, NULL);
	}
	
	//osync_change_save(entry->change, TRUE, NULL);
	//osengine_mappingentry_decider(engine, entry);
	osync_trace(TRACE_EXIT, "_get_change_data_reply_receiver");
}

void _read_change_reply_receiver(OSyncClient *sender, OSyncMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_read_change_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	/*OSyncMappingEntry *entry = osync_message_get_data(message, "entry");
	
	osync_flag_detach(entry->fl_read);
	
	osync_flag_unset(entry->mapping->fl_solved);
	osync_flag_unset(entry->mapping->fl_chkconflict);
	osync_flag_unset(entry->mapping->fl_multiplied);
	
	if (osync_change_get_changetype(entry->change) == CHANGE_DELETED)
		osync_flag_set(entry->fl_deleted);
	
	osync_flag_set(entry->fl_has_info);
	osync_flag_unset(entry->fl_synced);
	
	osync_change_save(entry->change, TRUE, NULL);
	
	osync_status_update_change(engine, entry->change, CHANGE_RECEIVED, NULL);
	
	osengine_mappingentry_decider(engine, entry);*/
	osync_trace(TRACE_EXIT, "_read_change_reply_receiver");
}

void _commit_change_reply_receiver(OSyncMessage *message, OSyncMappingEntry *entry)
{
	osync_trace(TRACE_ENTRY, "_commit_change_reply_receiver(%p, %p)", message, entry);
	OSyncEngine *engine = entry->client->engine;

	if (osync_message_is_error(message)) {
		OSyncError *error = osync_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", osync_error_print(&error));
		osync_status_update_change(engine, entry->change, CHANGE_WRITE_ERROR, &error);
		OSyncError *maperror = NULL;
		osync_error_duplicate(&maperror, &error);
		osync_status_update_mapping(engine, entry->mapping, MAPPING_WRITE_ERROR, &maperror);
		osync_error_update(&engine->error, "Unable to write one or more objects");
		
		//FIXME Do we need to do anything here?
		osync_flag_unset(entry->fl_dirty);
		osync_flag_set(entry->fl_synced);
	} else {
		osync_status_update_change(engine, entry->change, CHANGE_SENT, NULL);
		osync_flag_unset(entry->fl_dirty);
		osync_flag_set(entry->fl_synced);
	}
	
	if (osync_change_get_changetype(entry->change) == CHANGE_DELETED)
		osync_flag_set(entry->fl_deleted);
	
	osync_change_reset(entry->change);
	
	OSyncError *error = NULL;
	osync_change_save(entry->change, TRUE, &error);
	
	osengine_mappingentry_decider(engine, entry);
	osync_trace(TRACE_EXIT, "_commit_change_reply_receiver");
}

OSyncClient *osync_client_new(OSyncEngine *engine, OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, member, error);
	OSyncClient *client = osync_try_malloc0(sizeof(OSyncClient), error);
	if (!client)
		goto error;
		
	client->member = member;
	osync_member_set_data(member, client);
	client->engine = engine;
	engine->clients = g_list_append(engine->clients, client);
	
	char *name = g_strdup_printf("%s/pluginpipe", osync_member_get_configdir(member));
	client->incoming = osync_queue_new(name, TRUE, error);
	g_free(name);
	if (!client->incoming)
		goto error_free_client;
		
	client->fl_connected = osync_flag_new(engine->cmb_connected);
	client->fl_sent_changes = osync_flag_new(engine->cmb_sent_changes);
	client->fl_done = osync_flag_new(NULL);
	client->fl_committed_all = osync_flag_new(engine->cmb_committed_all_sent);
	client->fl_finished = osync_flag_new(engine->cmb_finished);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, client);
    return client;

error_free_client:
	g_free(client);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_client_reset(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	osync_flag_set_state(client->fl_connected, FALSE);
	osync_flag_set_state(client->fl_sent_changes, FALSE);
	osync_flag_set_state(client->fl_done, FALSE);
	osync_flag_set_state(client->fl_finished, FALSE);
	osync_flag_set_state(client->fl_committed_all, FALSE);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_client_free(OSyncClient *client)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, client);
	osync_queue_free(client->incoming);
	
	osync_flag_free(client->fl_connected);
	osync_flag_free(client->fl_sent_changes);
	osync_flag_free(client->fl_done);
	osync_flag_free(client->fl_finished);
	osync_flag_free(client->fl_committed_all);

	g_free(client);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void *osync_client_message_sink(OSyncMember *member, const char *name, void *data, osync_bool synchronous)
{
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	if (!synchronous) {
		/*OSyncMessage *message = itm_message_new_signal(client, "PLUGIN_MESSAGE");
		osync_debug("CLI", 3, "Sending message %p PLUGIN_MESSAGE for message %s", message, name);
		itm_message_set_data(message, "data", data);
		itm_message_set_data(message, "name", g_strdup(name));
		itm_queue_send(engine->incoming, message);*/
		return NULL;
	} else {
		return engine->plgmsg_callback(engine, client, name, data, engine->plgmsg_userdata);
	}	
}

OSyncPluginTimeouts osync_client_get_timeouts(OSyncClient *client)
{
	return osync_plugin_get_timeouts(osync_member_get_plugin(client->member));
}

void osync_client_call_plugin(OSyncClient *client, char *function, void *data, OSyncPluginReplyHandler replyhandler, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, client, function, data, replyhandler, userdata);
	
	/*OSyncEngine *engine = client->engine;
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
	
	itm_queue_send(client->incoming, message);*/
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_client_get_changes(OSyncClient *target, OSyncEngine *sender, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, target, sender, error);
	
	osync_flag_changing(target->fl_sent_changes);
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_GET_CHANGES, 0, error);
	if (!message)
		goto error;
		
	osync_message_set_handler(message, (OSyncMessageHandler)_get_changes_reply_receiver, target);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	if (!osync_queue_send_message_with_timeout(target->incoming, sender->incoming, message, timeouts.get_changeinfo_timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*void osync_client_get_change_data(OSyncEngine *sender, OSyncMappingEntry *entry)
{
	osync_flag_changing(entry->fl_has_data);
	OSyncMessage *message = osync_message_new_methodcall(sender, "GET_DATA");
	osync_message_set_handler(message, sender->incoming, (OSyncMessageHandler)_get_change_data_reply_receiver, sender);
	osync_message_set_data(message, "change", entry->change);
	osync_message_set_data(message, "entry", entry);
	osync_debug("ENG", 3, "Sending get_entry message %p to client %p", message, entry->client);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(entry->client);
	osync_queue_send_with_timeout(entry->client->incoming, message, timeouts.get_data_timeout, sender);
}

void osync_client_read_change(OSyncEngine *sender, OSyncMappingEntry *entry)
{
	//osync_flag_changing(entry->fl_has_data);
	OSyncMessage *message = osync_message_new_methodcall(sender, "READ_CHANGE");
	osync_message_set_handler(message, sender->incoming, (OSyncMessageHandler)_read_change_reply_receiver, sender);
	osync_message_set_data(message, "change", entry->change);
	osync_message_set_data(message, "entry", entry);
	osync_debug("ENG", 3, "Sending read_change message %p to client %p", message, entry->client);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(entry->client);
	osync_queue_send_with_timeout(entry->client->incoming, message, timeouts.read_change_timeout, sender);
}*/

osync_bool osync_client_connect(OSyncClient *target, OSyncEngine *sender, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, target, sender, error);
	
	osync_flag_changing(target->fl_connected);
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_CONNECT, 0, error);
	if (!message)
		goto error;
		
	osync_message_set_handler(message, (OSyncMessageHandler)_connect_reply_receiver, target);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	if (!osync_queue_send_message_with_timeout(target->incoming, sender->incoming, message, timeouts.connect_timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_commit_change(OSyncClient *target, OSyncEngine *sender, OSyncMappingEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, target, sender, entry);
	osync_trace(TRACE_INTERNAL, "Committing change with uid %s, changetype %i, data %p, size %i, objtype %s and format %s from member %lli", osync_change_get_uid(entry->change), osync_change_get_changetype(entry->change), osync_change_get_data(entry->change), osync_change_get_datasize(entry->change), osync_change_get_objtype(entry->change) ? osync_objtype_get_name(osync_change_get_objtype(entry->change)) : "None", osync_change_get_objformat(entry->change) ? osync_objformat_get_name(osync_change_get_objformat(entry->change)) : "None", osync_member_get_id(entry->client->member));
	
	osync_flag_changing(entry->fl_dirty);
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_COMMIT_CHANGE, 0, error);
	if (!message)
		goto error;

	osync_marshal_change(message, entry->change);

	osync_message_set_handler(message, (OSyncMessageHandler)_commit_change_reply_receiver, entry);
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(entry->client);
	
	if (!osync_queue_send_message_with_timeout(target->incoming, sender->incoming, message, timeouts.commit_timeout, error))
		goto error_free_message;

	osync_message_unref(message);

	g_assert(osync_flag_is_attached(entry->fl_committed) == TRUE);
	osync_flag_detach(entry->fl_committed);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_sync_done(OSyncClient *target, OSyncEngine *sender, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, target, sender, error);

	osync_flag_changing(target->fl_done);
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_SYNC_DONE, 0, error);
	if (!message)
		goto error;

	osync_message_set_handler(message, (OSyncMessageHandler)_sync_done_reply_receiver, target);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	if (!osync_queue_send_message_with_timeout(target->incoming, sender->incoming, message, timeouts.sync_done_timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_committed_all(OSyncClient *target, OSyncEngine *sender, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, target, sender);

	osync_flag_changing(target->fl_committed_all);

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_COMMITTED_ALL, 0, error);
	if (!message)
		goto error;
		
	osync_message_set_handler(message, (OSyncMessageHandler)_committed_all_reply_receiver, target);
	
	//OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	/*FIXME: Add timeout to committed_all message */
	if (!osync_queue_send_message(target->incoming, sender->incoming, message, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_disconnect(OSyncClient *target, OSyncEngine *sender, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, target, sender);

	osync_flag_changing(target->fl_connected);

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_DISCONNECT, 0, error);
	if (!message)
		goto error;
		
	osync_message_set_handler(message, (OSyncMessageHandler)_disconnect_reply_receiver, target);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	if (!osync_queue_send_message_with_timeout(target->incoming, sender->incoming, message, timeouts.disconnect_timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
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

osync_bool osync_client_spawn(OSyncClient *client, OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, engine, error);
	
	if (!osync_queue_exists(client->incoming) || !osync_queue_is_alive(client->incoming)) {
		pid_t cpid = fork();
		if (cpid == 0) {
			printf("About to exec osplugin\n");
			char *memberstring = g_strdup_printf("%lli", osync_member_get_id(client->member));
			execlp("osplugin", "osplugin", osync_group_get_name(engine->group), memberstring, NULL);
			
			printf("unable to exec\n");
			exit(1);
		}

		client->child_pid = cpid;
		
		while (!osync_queue_exists(client->incoming)) {
			osync_trace(TRACE_INTERNAL, "Waiting for other side to create fifo");
			usleep(500000);
		}
		
		osync_trace(TRACE_INTERNAL, "Queue was created");
	}
		
	if (!osync_queue_connect(client->incoming, O_WRONLY, error))
		goto error;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, error);
	if (!message)
		goto error_disconnect;
	
	osync_message_write_string(message, engine->incoming->name);
	
	if (!osync_queue_send_message(client->incoming, NULL, message, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_message:
	osync_message_unref(message);
error_disconnect:
	osync_queue_disconnect(client->incoming, NULL);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_init(OSyncClient *client, OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, client, engine, error);
	
	OSyncMessage *reply = NULL;
	while (!(reply = osync_queue_get_message(engine->incoming)))
		usleep(10000);
	
	osync_trace(TRACE_INTERNAL, "Reply received");
		printf("reply received %i\n", reply->cmd);
	if (reply->cmd != OSYNC_MESSAGE_REPLY) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong answer");
		goto error_free_reply;
	}
	
	osync_message_unref(reply);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_reply:
	osync_message_unref(reply);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_finalize(OSyncClient *client, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, client, error);

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_FINALIZE, 0, error);
	if (!message)
		goto error;

	if (!osync_queue_send_message(client->incoming, NULL, message, error))
		goto error_free_message;
	
	osync_message_unref(message);

	int status;
	if (waitpid(client->child_pid, &status, 0) == -1)
		goto error;

	if (!WIFEXITED(status))
		osync_trace(TRACE_INTERNAL, "Child has exited abnormally");
	else if (WEXITSTATUS(status) != 0)
		osync_trace(TRACE_INTERNAL, "Child has returned non-zero exit status (%d)", WEXITSTATUS(status));

	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}
