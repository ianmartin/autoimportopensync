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

/**
 * @defgroup OSyncEnginePrivate OpenSync Engine Internals
 * @ingroup OSEnginePrivate
 * @brief The internals of the engine (communication part)
 * 
 * This gives you an insight in the inner workings of the sync engine,
 * especially the communication part.
 * 
 * 
 */
/*@{*/

/*! @brief This function can be used to receive GET_ENTRY command replies
 * 
 * See ITMessageHandler
 * 
 */
void _get_changes_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_get_changes_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
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
 * See ITMessageHandler
 * 
 */
void _connect_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_connect_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
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

void _sync_done_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_sync_done_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("ENG", 1, "Sync done command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_SYNC_DONE_ERROR, &error);
		osync_error_update(&engine->error, "Unable to finish the sync for one of the members");
	}
	
	osync_flag_set(sender->fl_done);
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_sync_done_reply_receiver");
}

/*! @brief This function can be used to receive DISCONNECT command replies
 *
 * See ITMessageHandler
 * 
 */
void _disconnect_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_disconnect_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_debug("ENG", 1, "Sync done command reply was a error: %s", osync_error_print(&error));
		osync_status_update_member(engine, sender, MEMBER_DISCONNECT_ERROR, &error);
	} else
		osync_status_update_member(engine, sender, MEMBER_DISCONNECTED, NULL);
			
	osync_flag_unset(sender->fl_connected);
	osync_flag_set(sender->fl_finished);
	osengine_client_decider(engine, sender);
	osync_trace(TRACE_EXIT, "_disconnect_reply_receiver");
}

void _new_change_receiver(OSyncEngine *engine, OSyncClient *client, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "_new_change_receiver(%p, %p, %p)", engine, client, change);
	osync_debug("ENG", 2, "Handling new change with uid %s, changetype %i, data %p, format %s and objtype %s from member %lli", osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None", osync_member_get_id(client->member));
	
	OSyncError *error = NULL;
	osync_change_set_member(change, client->member);
	OSyncMappingEntry *entry = osengine_mappingtable_store_change(engine->maptable, change);
	change = entry->change;
	if (!osync_change_save(change, TRUE, &error)) {
		//FIXME Notify user
		osync_trace(TRACE_EXIT_ERROR, "_new_change_receiver");
		return;
	}
	
	//We convert to the common format here to make sure we always pass it
	osync_change_convert_to_common(change, NULL);
	
	if (!entry->mapping) {
		osync_flag_attach(entry->fl_mapped, engine->cmb_entries_mapped);
		osync_flag_unset(entry->fl_mapped);
		osync_debug("ENG", 3, "+It has no mapping");
	} else {
		osync_debug("ENG", 3, "+It has mapping");
		osync_flag_set(entry->fl_mapped);
		osync_flag_unset(entry->mapping->fl_solved);
		
	}
	
	if (osync_change_has_data(change)) {
		osync_debug("ENG", 3, "+It has data");
		osync_flag_set(entry->fl_has_data);
		osync_status_update_change(engine, change, CHANGE_RECEIVED, NULL);
	} else {
		osync_debug("ENG", 3, "+It has no data");
		osync_flag_unset(entry->fl_has_data);
		osync_status_update_change(engine, change, CHANGE_RECEIVED_INFO, NULL);
	}
	
	if (osync_change_get_changetype(change) == CHANGE_DELETED)
		osync_flag_set(entry->fl_deleted);
	
	osync_flag_set(entry->fl_has_info);
	osync_flag_unset(entry->fl_synced);

	osengine_mappingentry_decider(engine, entry);
	
	osync_trace(TRACE_EXIT, "_new_change_receiver");
}

void _get_change_data_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_get_change_data_reply_receiver(%p, %p, %p)", sender, message, engine);
	
	OSyncMappingEntry *entry = itm_message_get_data(message, "entry");
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", osync_error_print(&error));
		osync_status_update_change(engine, entry->change, CHANGE_RECV_ERROR, &error);
		osync_error_update(&engine->error, "Unable to read one or more objects");
		
		//FIXME Do we need to do anything here?
		osync_flag_unset(entry->fl_has_data);
	} else {
		osync_flag_set(entry->fl_has_data);
		osync_status_update_change(engine, entry->change, CHANGE_RECEIVED, NULL);
	}
	
	osync_change_save(entry->change, TRUE, NULL);
	osengine_mappingentry_decider(engine, entry);
	osync_trace(TRACE_EXIT, "_get_change_data_reply_receiver");
}

void _commit_change_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "_commit_change_reply_receiver(%p, %p, %p)", sender, message, engine);
	OSyncMappingEntry *entry = itm_message_get_data(message, "entry");
	
	if (itm_message_is_error(message)) {
		OSyncError *error = itm_message_get_error(message);
		osync_error_duplicate(&engine->error, &error);
		osync_debug("MAP", 1, "Commit change command reply was a error: %s", osync_error_print(&error));
		osync_status_update_change(engine, entry->change, CHANGE_WRITE_ERROR, &error);
		osync_status_update_mapping(engine, entry->mapping, MAPPING_WRITE_ERROR, &error);
		osync_error_update(&engine->error, "Unable to write one or more objects");
		
		//FIXME Do we need to do anything here?
		osync_flag_unset(entry->fl_dirty);
		osync_flag_set(entry->fl_synced);
	} else {
		osync_status_update_change(engine, entry->change, CHANGE_SENT, NULL);
		osync_flag_unset(entry->fl_dirty);
		osync_flag_set(entry->fl_synced);
	}
	
	OSyncError *error = NULL;
	osync_change_save(entry->change, TRUE, &error);
	if (osync_change_get_changetype(entry->change) == CHANGE_DELETED)
		osync_flag_set(entry->fl_deleted);
	
	osengine_mappingentry_decider(engine, entry);
	osync_trace(TRACE_EXIT, "_get_changes_reply_receiver");
}

void send_get_changes(OSyncClient *target, OSyncEngine *sender, osync_bool data)
{
	osync_flag_changing(target->fl_sent_changes);
	ITMessage *message = itm_message_new_methodcall(sender, "GET_CHANGES");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_get_changes_reply_receiver, sender);
	itm_message_set_data(message, "data", (void *)data);
	osync_debug("ENG", 3, "Sending get_changes message %p to client %p", message, target);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	itm_queue_send_with_timeout(target->incoming, message, timeouts.get_changeinfo_timeout, target);
}

void send_get_change_data(OSyncEngine *sender, OSyncMappingEntry *entry)
{
	osync_flag_changing(entry->fl_has_data);
	ITMessage *message = itm_message_new_methodcall(sender, "GET_DATA");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_get_change_data_reply_receiver, sender);
	itm_message_set_data(message, "change", entry->change);
	itm_message_set_data(message, "entry", entry);
	osync_debug("ENG", 3, "Sending get_entry message %p to client %p", message, entry->client);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(entry->client);
	itm_queue_send_with_timeout(entry->client->incoming, message, timeouts.get_data_timeout, sender);
}

void send_connect(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_connected);
	ITMessage *message = itm_message_new_methodcall(sender, "CONNECT");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_connect_reply_receiver, sender);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	itm_queue_send_with_timeout(target->incoming, message, timeouts.connect_timeout, target);
}

void send_commit_change(OSyncEngine *sender, OSyncMappingEntry *entry)
{
	osync_flag_changing(entry->fl_dirty);
	ITMessage *message = itm_message_new_methodcall(sender, "COMMIT_CHANGE");
	itm_message_set_data(message, "change", entry->change);
	itm_message_set_data(message, "entry", entry);
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_commit_change_reply_receiver, sender);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(entry->client);
	itm_queue_send_with_timeout(entry->client->incoming, message, timeouts.commit_timeout, sender);
}

void send_sync_done(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_done);
	ITMessage *message = itm_message_new_methodcall(sender, "SYNC_DONE");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_sync_done_reply_receiver, sender);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	itm_queue_send_with_timeout(target->incoming, message, timeouts.sync_done_timeout, target);
}

void send_disconnect(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_connected);
	ITMessage *message = itm_message_new_methodcall(sender, "DISCONNECT");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_disconnect_reply_receiver, sender);
	
	OSyncPluginTimeouts timeouts = osync_client_get_timeouts(target);
	itm_queue_send_with_timeout(target->incoming, message, timeouts.disconnect_timeout, target);
}

void send_engine_changed(OSyncEngine *engine)
{
	ITMessage *message = itm_message_new_signal(NULL, "ENGINE_CHANGED");
	osync_debug("ENG", 4, "Sending message %p:\"ENGINE_CHANGED\"", message);
	itm_queue_send(engine->incoming, message);
}

void send_client_changed(OSyncEngine *engine, OSyncClient *client)
{
	ITMessage *message = itm_message_new_signal(NULL, "CLIENT_CHANGED");
	osync_debug("ENG", 4, "Sending message %p:\"CLIENT_CHANGED\" for client %p", message, client);
	itm_message_set_data(message, "client", client);
	itm_queue_send(engine->incoming, message);
}

void send_mappingentry_changed(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	ITMessage *message = itm_message_new_signal(NULL, "ENTRY_CHANGED");
	osync_debug("ENG", 4, "Sending message %p:\"ENTRY_CHANGED\" for entry %p", message, entry);
	itm_message_set_data(message, "entry", entry);
	itm_queue_send(engine->incoming, message);
}

void send_mapping_changed(OSyncEngine *engine, OSyncMapping *mapping)
{
	ITMessage *message = itm_message_new_signal(NULL, "MAPPING_CHANGED");
	itm_message_set_data(message, "mapping", mapping);
	itm_queue_send(engine->incoming, message);
}

/*! @brief The queue message handler of the engine
 * 
 * @param sender The Client who sent this message
 * @param message The message
 * @param engine The engine
 * 
 */
void engine_message_handler(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "engine_message_handler(%p, %p:%s, %p)", sender, message, message->msgname, engine);
	
	if (itm_message_is_signal (message, "ENTRY_CHANGED")) {
		OSyncMappingEntry *entry = itm_message_get_data(message, "entry");
		osengine_mappingentry_decider(engine, entry);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "MAPPING_CHANGED")) {
		OSyncMapping *mapping = itm_message_get_data(message, "mapping");
		osengine_mapping_decider(engine, mapping);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "CLIENT_CHANGED")) {
		OSyncClient *client = itm_message_get_data(message, "client");
		osengine_client_decider(engine, client);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "ENGINE_CHANGED")) {
		osengine_client_all_deciders(engine);
		osengine_mapping_all_deciders(engine);
		GList *u;
		for (u = engine->maptable->unmapped; u; u = u->next) {
			OSyncMappingEntry *unmapped = u->data;
			send_mappingentry_changed(engine, unmapped);
		}
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "PLUGIN_MESSAGE")) {
		char *name = itm_message_get_data(message, "name");
		void *data = itm_message_get_data(message, "data");
		engine->plgmsg_callback(engine, sender, name, data, engine->plgmsg_userdata);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "NEW_CHANGE")) {
		OSyncChange *change = itm_message_get_data(message, "change");
		_new_change_receiver(engine, sender, change);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (itm_message_is_signal (message, "SYNC_ALERT")) {
		if (engine->allow_sync_alert)
			osync_flag_set(engine->fl_running);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	osync_debug("ENG", 0, "Unknown message \"%s\"", itm_message_get_msgname(message));
	osync_trace(TRACE_EXIT_ERROR, "engine_message_handler: Unknown message");
	g_assert_not_reached();
}

void trigger_clients_sent_changes(OSyncEngine *engine)
{
	osync_status_update_engine(engine, ENG_ENDPHASE_READ, NULL);
	
	g_mutex_lock(engine->info_received_mutex);
	g_cond_signal(engine->info_received);
	g_mutex_unlock(engine->info_received_mutex);

	send_engine_changed(engine);
}

static gboolean startupfunc(gpointer data)
{
	OSyncEngine *engine = data;
	osync_trace(TRACE_INTERNAL, "+++++++++ This is the engine of group \"%s\" +++++++++", osync_group_get_name(engine->group));
	g_mutex_lock(engine->started_mutex);
	g_cond_signal(engine->started);
	g_mutex_unlock(engine->started_mutex);
	return FALSE;
}

/*@}*/

/**
 * @defgroup PublicAPI Public APIs
 * @brief Available public APIs
 * 
 */

/**
 * @defgroup OSEnginePublic OpenSync Engine API
 * @ingroup PublicAPI
 * @brief The API of the syncengine available to everyone
 * 
 * This gives you an insight in the public API of the opensync sync engine.
 * 
 */
/*@{*/


/*! @brief This will reset the engine to its initial state
 * 
 * This function will reset the engine to its initial state. The engine
 * must not be running at this point.
 * 
 * @param engine A pointer to the engine you want to reset
 * @param error A pointer to a error struct
 * @returns TRUE if command was succcessfull, FALSE otherwise
 * 
 */

osync_bool osync_engine_reset(OSyncEngine *engine, OSyncError **error)
{
	//FIXME Check if engine is running
	osync_trace(TRACE_ENTRY, "osync_engine_reset(%p, %p)", engine, error);
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_flag_set_state(client->fl_connected, FALSE);
		osync_flag_set_state(client->fl_sent_changes, FALSE);
		osync_flag_set_state(client->fl_done, FALSE);
		osync_flag_set_state(client->fl_finished, FALSE);
		itm_queue_flush(client->incoming);
	}
	
	osync_flag_set_state(engine->fl_running, FALSE);
	osync_flag_set_state(engine->fl_stop, FALSE);
	osync_flag_set_state(engine->cmb_sent_changes, FALSE);
	osync_flag_set_state(engine->cmb_entries_mapped, TRUE);
	osync_flag_set_state(engine->cmb_synced, TRUE);
	osync_flag_set_state(engine->cmb_finished, FALSE);
	osync_flag_set_state(engine->cmb_connected, FALSE);
	itm_queue_flush(engine->incoming);
	
	osync_status_update_engine(engine, ENG_ENDPHASE_DISCON, NULL);
	
	osengine_mappingtable_reset(engine->maptable);
	
	if (engine->error) {
		OSyncError *error = NULL;
		osync_error_duplicate(&error, &engine->error);
		osync_status_update_engine(engine, ENG_ERROR, &error);
		osync_error_free(&error);
		osync_group_set_slow_sync(engine->group, "data", TRUE);
	} else {
		osync_status_update_engine(engine, ENG_SYNC_SUCCESSFULL, NULL);
		osync_group_set_slow_sync(engine->group, "data", FALSE);
	}
	
	g_mutex_lock(engine->syncing_mutex);
	g_cond_signal(engine->syncing);
	g_mutex_unlock(engine->syncing_mutex);

	osync_trace(TRACE_EXIT, "osync_engine_reset");
	return TRUE;
}

/*! @brief This will create a new engine for the given group
 * 
 * This will create a new engine for the given group
 * 
 * @param group A pointer to the group, for which you want to create a new engine
 * @param error A pointer to a error struct
 * @returns Pointer to a newly allocated OSyncEngine on success, NULL otherwise
 * 
 */
OSyncEngine *osync_engine_new(OSyncGroup *group, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_engine_new(%p, %p)", group, error);
	
	g_assert(group);
	OSyncEngine *engine = g_malloc0(sizeof(OSyncEngine));
	osync_group_set_data(group, engine);
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	engine->context = g_main_context_new();
	engine->syncloop = g_main_loop_new(engine->context, FALSE);
	engine->group = group;

	engine->incoming = itm_queue_new();
	
	engine->syncing_mutex = g_mutex_new();
	engine->info_received_mutex = g_mutex_new();
	engine->syncing = g_cond_new();
	engine->info_received = g_cond_new();
	engine->started_mutex = g_mutex_new();
	engine->started = g_cond_new();
	
	//Set the default start flags
	engine->fl_running = osync_flag_new(NULL);
	osync_flag_set_pos_trigger(engine->fl_running, (MSyncFlagTriggerFunc)send_engine_changed, engine, NULL);
	engine->fl_sync = osync_flag_new(NULL);
	engine->fl_stop = osync_flag_new(NULL);
	osync_flag_set_pos_trigger(engine->fl_stop, (MSyncFlagTriggerFunc)send_engine_changed, engine, NULL);
	
	//The combined flags
	engine->cmb_sent_changes = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_sent_changes, (MSyncFlagTriggerFunc)trigger_clients_sent_changes, engine, NULL);
	engine->cmb_entries_mapped = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_entries_mapped, (MSyncFlagTriggerFunc)send_engine_changed, engine, NULL);
	engine->cmb_synced = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_synced, (MSyncFlagTriggerFunc)send_engine_changed, engine, NULL);
	engine->cmb_finished = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_finished, (MSyncFlagTriggerFunc)osync_engine_reset, engine, NULL);
	engine->cmb_connected = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_connected, (MSyncFlagTriggerFunc)send_engine_changed, engine, NULL);
	osync_flag_set(engine->fl_sync);
	
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		osync_client_new(engine, member);
	}
	
	engine->maptable = osengine_mappingtable_new(engine);
	
	osync_trace(TRACE_EXIT, "osync_engine_new: %p", engine);
	return engine;
}

/*! @brief This will free a engine and all resources associated
 * 
 * This will free a engine and all resources associated
 * 
 * @param engine A pointer to the engine, which you want to free
 * 
 */
void osync_engine_free(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "osync_engine_free(%p)", engine);
	
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_client_free(client);
	}
	
	osengine_mappingtable_free(engine->maptable);
	engine->maptable = NULL;
	
	osync_flag_free(engine->fl_running);
	osync_flag_free(engine->fl_sync);
	osync_flag_free(engine->fl_stop);
	osync_flag_free(engine->cmb_sent_changes);
	osync_flag_free(engine->cmb_entries_mapped);
	osync_flag_free(engine->cmb_synced);
	osync_flag_free(engine->cmb_finished);
	osync_flag_free(engine->cmb_connected);
	
	itm_queue_flush(engine->incoming);
	itm_queue_free(engine->incoming);
	engine->incoming = NULL;

	g_list_free(engine->clients);
	g_main_loop_unref(engine->syncloop);
	
	g_main_context_unref(engine->context);
	
	g_mutex_free(engine->syncing_mutex);
	g_mutex_free(engine->info_received_mutex);
	g_cond_free(engine->syncing);
	g_cond_free(engine->info_received);
	g_mutex_free(engine->started_mutex);
	g_cond_free(engine->started);
	
	g_free(engine);
	osync_trace(TRACE_EXIT, "osync_engine_free");
}

/*! @brief This will set the conflict handler for the given engine
 * 
 * The conflict handler will be called everytime a conflict occurs
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the conflict
 * @param user_data Pointer to some data that will get passed to the status function as the last argument
 * 
 */
void osync_engine_set_conflict_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncMapping *, void *), void *user_data)
{
	engine->conflict_callback = function;
	engine->conflict_userdata = user_data;
}

/*! @brief This will set the change status handler for the given engine
 * 
 * The change status handler will be called every time a new change is received, written etc
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the change status
 * @param user_data Pointer to some data that will get passed to the status function as the last argument
 * 
 */
void osync_engine_set_changestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, MSyncChangeUpdate *, void *), void *user_data)
{
	engine->changestat_callback = function;
	engine->changestat_userdata = user_data;
}

/*! @brief This will set the mapping status handler for the given engine
 * 
 * The mapping status handler will be called every time a mapping is updated
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the mapping status
 * @param user_data Pointer to some data that will get passed to the status function as the last argument
 * 
 */
void osync_engine_set_mappingstatus_callback(OSyncEngine *engine, void (* function) (MSyncMappingUpdate *, void *), void *user_data)
{
	engine->mapstat_callback = function;
	engine->mapstat_userdata = user_data;
}

/*! @brief This will set the engine status handler for the given engine
 * 
 * The engine status handler will be called every time the engine is updated (started, stoped etc)
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the engine status
 * @param user_data Pointer to some data that will get passed to the status function as the last argument
 * 
 */
void osync_engine_set_enginestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncEngineUpdate *, void *), void *user_data)
{
	engine->engstat_callback = function;
	engine->engstat_userdata = user_data;
}

/*! @brief This will set the member status handler for the given engine
 * 
 * The member status handler will be called every time a member is updated (connects, disconnects etc)
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the member status
 * @param user_data Pointer to some data that will get passed to the status function as the last argument
 * 
 */
void osync_engine_set_memberstatus_callback(OSyncEngine *engine, void (* function) (MSyncMemberUpdate *, void *), void *user_data)
{
	engine->mebstat_callback = function;
	engine->mebstat_userdata = user_data;
}

/*! @brief This will set the callback handler for a custom message
 * 
 * A custom message can be used to communicate with a plugin directly
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the member status
 * @param user_data A pointer to some user data that the callback function will get passed
 * 
 */
void osync_engine_set_message_callback(OSyncEngine *engine, void *(* function) (OSyncEngine *, OSyncClient *, const char *, void *, void *), void *user_data)
{
	engine->plgmsg_callback = function;
	engine->plgmsg_userdata = user_data;
}

/*! @brief This will initialize a engine
 * 
 * After initialization, the engine will be ready to sync. The threads for the engine,
 * the members are started. If one of the members has a listening server, the server will be
 * started and listening.
 * 
 * @param engine A pointer to the engine, which will be initialized
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE.
 * 
 */
osync_bool osync_engine_init(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_engine_init(%p, %p)", engine, error);
	
	if (engine->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "This engine was already initialized");
		osync_trace(TRACE_EXIT_ERROR, "osync_engine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	switch (osync_group_lock(engine->group)) {
		case OSYNC_LOCKED:
			osync_error_set(error, OSYNC_ERROR_LOCKED, "Group is locked");
			osync_trace(TRACE_EXIT_ERROR, "osync_engine_init: %s", osync_error_print(error));
			return FALSE;
		case OSYNC_LOCK_STALE:
			osync_debug("ENG", 1, "Detected stale lock file. Slow-syncing");
			osync_status_update_engine(engine, ENG_PREV_UNCLEAN, NULL);
			osync_group_set_slow_sync(engine->group, "data", TRUE);
			break;
		default:
			break;
	}
	
	if (!(engine->man_dispatch))
		itm_queue_setup_with_gmainloop(engine->incoming, engine->context);
	itm_queue_set_message_handler(engine->incoming, (ITMessageHandler)engine_message_handler, engine);
	
	osync_flag_set(engine->cmb_entries_mapped);
	osync_flag_set(engine->cmb_synced);
	engine->allow_sync_alert = FALSE;
	
	//OSyncMember *member = NULL;
	OSyncGroup *group = engine->group;
	
	if (osync_group_num_members(group) < 2) {
		//Not enough members!
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You only configured %i members, but at least 2 are needed", osync_group_num_members(group));
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osync_engine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	engine->is_initialized = TRUE;
	
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		if (!osync_client_init(client, error)) {
			osync_engine_finalize(engine);
			osync_group_unlock(engine->group, TRUE);
			osync_trace(TRACE_EXIT_ERROR, "osync_engine_init: %s", osync_error_print(error));
			return FALSE;
		}
	}
	
	if (!osengine_mappingtable_load(engine->maptable, error)) {
		osync_engine_finalize(engine);
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osync_engine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	osync_debug("ENG", 3, "Running the main loop");

	//Now we can run the main loop
	//We protect the startup by a g_cond
	g_mutex_lock(engine->started_mutex);
	GSource *idle = g_idle_source_new();
	g_source_set_callback(idle, startupfunc, engine, NULL);
    g_source_attach(idle, engine->context);
	engine->thread = g_thread_create ((GThreadFunc)g_main_loop_run, engine->syncloop, TRUE, NULL);
	g_cond_wait(engine->started, engine->started_mutex);
	g_mutex_unlock(engine->started_mutex);
	
	osync_trace(TRACE_EXIT, "osync_engine_init");
	return TRUE;
}

/*! @brief This will finalize a engine
 * 
 * Finalizing a engine will stop all threads and listening server.
 * The engine can be initialized again.
 * 
 * @param engine A pointer to the engine, which will be finalized
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE.
 * 
 */
void osync_engine_finalize(OSyncEngine *engine)
{
	//FIXME check if engine is running
	osync_trace(TRACE_ENTRY, "osync_engine_finalize(%p)", engine);

	if (!engine->is_initialized) {
		osync_trace(TRACE_EXIT_ERROR, "osync_engine_finalize: Not initialized");
		return;
	}
	
	g_assert(engine);
	osync_debug("ENG", 3, "finalizing engine %p", engine);
	
	if (engine->thread) {
		g_main_loop_quit(engine->syncloop);
		g_thread_join(engine->thread);
	}
	
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_client_finalize(client);
	}
	
	osengine_mappingtable_close(engine->maptable);
	
	itm_queue_flush(engine->incoming);
	
	if (engine->error)
		osync_group_unlock(engine->group, FALSE);
	else
		osync_group_unlock(engine->group, TRUE);
	
	engine->is_initialized = FALSE;
	osync_trace(TRACE_EXIT, "osync_engine_finalize");
}

/*! @brief Starts to synchronize the given OSyncEngine
 *
 * This function synchronizes a given engine. The Engine has to be created
 * from a OSyncGroup before by using osync_engine_new(). This function will not block
 * 
 * @param engine A pointer to the engine, which will be used to sync
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE. Note that this just says if the sync has been started successfully, not if the sync itself was successfull
 * 
 */
osync_bool osync_engine_synchronize(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_engine_synchronize(%p)", engine);
	g_assert(engine);
	if (!engine->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "osync_engine_synchronize: Not initialized");
		osync_trace(TRACE_EXIT_ERROR, "osync_engine_synchronize: %s", osync_error_print(error));
		return FALSE;
	}
	
	osync_flag_set(engine->fl_running);
	osync_trace(TRACE_EXIT, "osync_engine_synchronize");
	return TRUE;
}

/*! @brief Sets a flag on the engine that the engine should only request the info about sync objects
 *
 * This can be used to see only what has changed. The engine will not request the data itself from
 * the members. Note that some members might not support this behaviour and might send the data anyways.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osync_engine_flag_only_info(OSyncEngine *engine)
{
	osync_flag_unset(engine->fl_sync);
}

/*! @brief Sets a flag on the engine that the engine should do single stepping (For debugging)
 *
 * This flag can be used to set single stepping on the engine. The engine will pause after each iteration.
 * Use osync_engine_one_iteration to initialize the next iteration. This is only for debugging purposes.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osync_engine_flag_manual(OSyncEngine *engine)
{
	if (engine->syncloop) {
		g_warning("Unable to flag manual since engine is already initialized\n");
	}
	engine->man_dispatch = TRUE;
}

/*! @brief This will pause the engine
 *
 * This flag can be used to temporarily suspend the engine
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osync_engine_pause(OSyncEngine *engine)
{
	osync_flag_unset(engine->fl_running);
}

/*! @brief Sets a flag on the engine that the engine should do single stepping (For debugging)
 *
 * This flag can be used to set single stepping on the engine. The engine will pause after each iteration.
 * Use osync_engine_one_iteration to initialize the next iteration. This is only for debugging purposes.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osync_engine_abort(OSyncEngine *engine)
{
	osync_flag_set(engine->fl_stop);
}

void osync_engine_allow_sync_alert(OSyncEngine *engine)
{
	engine->allow_sync_alert = TRUE;
}

void osync_engine_deny_sync_alert(OSyncEngine *engine)
{
	engine->allow_sync_alert = FALSE;
}

/*! @brief This function will synchronize once and block until the sync has finished
 *
 * This can be used to sync a group and wait for the synchronization end. DO NOT USE
 * osync_engine_wait_sync_end for this as this might introduce a race condition.
 * 
 * @param engine A pointer to the engine, which to sync and wait for the sync end
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise.
 * 
 */
osync_bool osync_engine_sync_and_block(OSyncEngine *engine, OSyncError **error)
{
	g_mutex_lock(engine->syncing_mutex);
	
	if (!osync_engine_synchronize(engine, error)) {
		g_mutex_unlock(engine->syncing_mutex);
		return FALSE;
	}
	
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
	
	if (engine->error) {
		osync_error_duplicate(error, &(engine->error));
		return FALSE;
	}
	
	return TRUE;
}

/*! @brief This function will block until a synchronization has ended
 *
 * This can be used to wait until the synchronization has ended. Note that this function will always
 * block until 1 sync has ended. It can be used before the sync has started, to wait for one auto-sync
 * to end
 * 
 * @param engine A pointer to the engine, for which to wait for the sync end
 * @param error Return location for the error if the sync was not successfull
 * @returns TRUE on success, FALSE otherwise.
 */
osync_bool osync_engine_wait_sync_end(OSyncEngine *engine, OSyncError **error)
{
	g_mutex_lock(engine->syncing_mutex);
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
	
	if (engine->error) {
		osync_error_duplicate(error, &(engine->error));
		return FALSE;
	}
	return TRUE;
}

/*! @brief This function will block until all change object information has been received
 *
 * This will block until the information and not the data has been received.
 * 
 * @param engine A pointer to the engine, for which to wait for the info
 */
void osync_engine_wait_info_end(OSyncEngine *engine)
{
	g_mutex_lock(engine->info_received_mutex);
	g_cond_wait(engine->info_received, engine->info_received_mutex);
	g_mutex_unlock(engine->info_received_mutex);
}

/*! @brief Does one iteration of the engine (For debugging)
 *
 */
void osync_engine_one_iteration(OSyncEngine *engine)
{
	itm_queue_dispatch(engine->incoming);
}

/*@}*/
