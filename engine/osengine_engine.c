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
	_osync_debug(engine, "ENG", 3, "Received a reply %p to GET_CHANGES command from client %p", message, sender);

	osync_flag_set(sender->fl_sent_changes);
	osync_client_decider(engine, sender);
}

/*! @brief This function can be used to receive CONNECT command replies
 *
 * See ITMessageHandler
 * 
 */
void _connect_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	_osync_debug(engine, "ENG", 3, "Received a reply %p to CONNECT command from client %p, engine is at %p", message, sender, engine);
	
	if (itm_message_is_error(message)) {
		osync_flag_unset(sender->fl_connected);
		osync_flag_set(sender->fl_finished);
		osync_flag_set(sender->fl_sent_changes);
		osync_flag_set(sender->fl_done);
	} else {
		osync_flag_set(sender->fl_connected);
	}

	osync_client_decider(engine, sender);
	osync_status_update_member(engine, sender, MEMBER_CONNECTED);
}

void _sync_done_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	_osync_debug(engine, "ENG", 3, "Received a reply %p to SYNC_DONE command from client %p", message, sender);
	osync_flag_set(sender->fl_done);
	osync_client_decider(engine, sender);
}

/*! @brief This function can be used to receive DISCONNECT command replies
 *
 * See ITMessageHandler
 * 
 */
void _disconnect_reply_receiver(OSyncClient *sender, ITMessage *message, OSyncEngine *engine)
{
	_osync_debug(engine, "ENG", 3, "Received a reply %p to DISCONNECT command from client %p", message, sender);
	osync_status_update_member(engine, sender, MEMBER_DISCONNECTED);
	osync_flag_unset(sender->fl_connected);
	osync_flag_set(sender->fl_finished);
	osync_client_decider(engine, sender);
}

void send_get_changes(OSyncClient *target, OSyncEngine *sender, osync_bool data)
{
	osync_flag_changing(target->fl_sent_changes);
	ITMessage *message = itm_message_new_methodcall(sender, "GET_CHANGES");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_get_changes_reply_receiver, sender);
	itm_message_set_data(message, "data", (void *)data);
	_osync_debug(sender, "ENG", 3, "Sending get_changes message %p to client %p", message, target);
	itm_queue_send(target->incoming, message);
}

void send_connect(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_connected);
	ITMessage *message = itm_message_new_methodcall(sender, "CONNECT");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_connect_reply_receiver, sender);
	itm_queue_send_with_timeout(target->incoming, message, 15, target);
}

void send_sync_done(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_done);
	ITMessage *message = itm_message_new_methodcall(sender, "SYNC_DONE");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_sync_done_reply_receiver, sender);
	itm_queue_send_with_timeout(target->incoming, message, 60, target);
}

void send_disconnect(OSyncClient *target, OSyncEngine *sender)
{
	osync_flag_changing(target->fl_connected);
	ITMessage *message = itm_message_new_methodcall(sender, "DISCONNECT");
	itm_message_set_handler(message, sender->incoming, (ITMessageHandler)_disconnect_reply_receiver, sender);
	itm_queue_send_with_timeout(target->incoming, message, 60, target);
}

void osync_client_deciders(OSyncEngine *engine)
{
	int i = 0;
	_osync_debug(engine, "ENG", 3, "Calling all client deciders (%i)", g_list_length(engine->clients));
	for (i = 0; i < g_list_length(engine->clients); i++) {
		OSyncClient *client = g_list_nth_data(engine->clients, i);
		send_client_changed(engine, client);
	}
}

void osync_client_decider(OSyncEngine *engine, OSyncClient *client)
{
	_osync_debug(engine, "ENG", 3, "Client decider called for client %p", client);
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_not_set(client->fl_connected) && osync_flag_is_not_set(client->fl_finished)) {
		_osync_debug(engine, "ENG", 2, "Telling client %p to connect", client);
		send_connect(client, engine);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_set(client->fl_connected) && osync_flag_is_not_set(client->fl_sent_changes)) {
		if (osync_flag_is_set(engine->fl_sync)) {
			_osync_debug(engine, "ENG", 2, "Telling client %p to send changes with data", client);
			send_get_changes(client, engine, TRUE);
		} else {
			_osync_debug(engine, "ENG", 2, "Telling client %p to send changes without data", client);
			send_get_changes(client, engine, FALSE);
		}
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_not_set(engine->fl_stop) && osync_flag_is_not_set(client->fl_done) && osync_flag_is_set(client->fl_connected) && osync_flag_is_set(client->fl_sent_changes) && osync_flag_is_set(engine->cmb_sent_changes) && osync_flag_is_set(engine->cmb_synced) && osync_flag_is_set(engine->cmb_entries_mapped)) {
		_osync_debug(engine, "ENG", 2, "Telling client %p to call sync_done", client);
		send_sync_done(client, engine);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(client->fl_done) && osync_flag_is_set(client->fl_connected)) {
		_osync_debug(engine, "ENG", 2, "Telling client %p to disconnect", client);
		send_disconnect(client, engine);
		return;
	}
	
	if (osync_flag_is_set(engine->fl_running) && osync_flag_is_set(engine->fl_stop) && osync_flag_is_set(client->fl_connected)) {
		_osync_debug(engine, "ENG", 2, "Telling client %p to disconnect", client);
		send_disconnect(client, engine);
		return;
	}

	_osync_debug(engine, "ENG", 3, "Waste cycle in client decider %p", client);
}

void handle_new_change(OSyncEngine *engine, OSyncClient *client, OSyncChange *change)
{
	OSyncMember *member = client->member;
	MSyncChangeFlags *flags = NULL;
	OSyncMapping *mapping = NULL;
	
	_osync_debug(engine, "ENG", 2, "Handling new change %p with uid %s and changetype %i with data %p from member %lli", change, osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_member_get_id(client->member));
	
	//Update the change in the correct mapping
	//Save the change
	mapping = osync_change_get_mapping(change);

	if (!mapping)
		osync_member_update_change(member, &change);
			
	if (!(mapping = osync_change_get_mapping(change))) {
		osync_mappingtable_add_unmapped(engine->maptable, change);
		osync_member_add_changeentry(member, change);
		flags = osync_change_get_flags(change);
		osync_flag_attach(flags->fl_mapped, engine->cmb_entries_mapped);
		osync_flag_unset(flags->fl_mapped);
		_osync_debug(engine, "ENG", 3, "+It has no mapping");
	} else {
		_osync_debug(engine, "ENG", 3, "+It has mapping");
		mapping = osync_change_get_mapping(change);
		flags = osync_change_get_flags(change);
		/*FIXME: multisync crashes here if a member
		 * reports two changes with the same uid
		 */
		osync_change_flags_attach(change, mapping);
		osync_flag_set(flags->fl_mapped);
		
	}

	osync_mappingtable_save_change(engine->maptable, change);
	
	if (osync_change_has_data(change)) {
		_osync_debug(engine, "ENG", 3, "+It has data");
		osync_flag_set(flags->fl_has_data);
	} else {
		_osync_debug(engine, "ENG", 3, "+It has no data");
		osync_flag_unset(flags->fl_has_data);
	}
	
	if (osync_change_get_changetype(change) == CHANGE_DELETED)
		osync_flag_set(flags->fl_deleted);
	osync_flag_set(flags->fl_has_info);
	osync_flag_unset(flags->fl_synced);
	osync_status_update_change(engine, change, CHANGE_RECEIVED);
	osync_change_decider(engine, change);
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
	if (itm_message_is_signal (message, "CHANGE_CHANGED")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"ENTRY_CHANGED\"", message);
		OSyncChange *change = itm_message_get_data(message, "change");
		osync_change_decider(engine, change);
		return;
	}
	
	if (itm_message_is_signal (message, "MAPPING_CHANGED")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"MAPPING_CHANGED\"", message);
		OSyncMapping *mapping = itm_message_get_data(message, "mapping");
		osync_mapping_decider(engine, mapping);
		return;
	}
	
	if (itm_message_is_signal (message, "CLIENT_CHANGED")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"CLIENT_CHANGED\" for client %p", message, sender);
		OSyncClient *client = itm_message_get_data(message, "client");
		osync_client_decider(engine, client);
		return;
	}
	
	if (itm_message_is_signal (message, "ENGINE_CHANGED")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"ENGINE_CHANGED\"", message);
		osync_client_deciders(engine);
		osync_mapping_all_deciders(engine);
		int i = 0;
		for (i = 0; i < osync_mappingtable_num_unmapped(engine->maptable); i++) {
			OSyncChange *unmapped = osync_mappingtable_nth_unmapped(engine->maptable, i);
			send_change_changed(unmapped);
		}
		return;
	}
	
	if (itm_message_is_signal (message, "PLUGIN_MESSAGE")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"PLUGIN_MESSAGE\"", message);
		char *name = itm_message_get_data(message, "name");
		void *data = itm_message_get_data(message, "data");
		engine->plgmsg_callback(engine, sender, name, data, engine->plgmsg_userdata);
		return;
	}
	
	if (itm_message_is_signal (message, "NEW_CHANGE")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"NEW_CHANGE\"", message);
		OSyncChange *change = itm_message_get_data(message, "change");
		handle_new_change(engine, sender, change);
		return;
	}
	
	if (itm_message_is_signal (message, "SYNC_ALERT")) {
		_osync_debug(engine, "ENG", 3, "Message handler called for message %p \"SYNC_ALERT\"", message);
		if (engine->allow_sync_alert)
			osync_flag_set(engine->fl_running);
		return;
	}
	
	_osync_debug(engine, "ENG", 0, "Unknown message \"%s\"\n", itm_message_get_msgname(message));
	g_assert_not_reached();
}

void send_engine_changed(OSyncEngine *engine)
{
	ITMessage *message = itm_message_new_signal(NULL, "ENGINE_CHANGED");
	itm_queue_send(engine->incoming, message);
}

void send_client_changed(OSyncEngine *engine, OSyncClient *client)
{
	ITMessage *message = itm_message_new_signal(NULL, "CLIENT_CHANGED");
	itm_message_set_data(message, "client", client);
	itm_queue_send(engine->incoming, message);
}

void trigger_clients_sent_changes(OSyncEngine *engine)
{
	osync_status_update_engine(engine, ENG_ENDPHASE_READ);
	
	g_mutex_lock(engine->info_received_mutex);
	g_cond_signal(engine->info_received);
	g_mutex_unlock(engine->info_received_mutex);

	send_engine_changed(engine);
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
	itm_queue_flush(engine->incoming);
	
	osync_status_update_engine(engine, ENG_ENDPHASE_DISCON);
	
	g_mutex_lock(engine->syncing_mutex);
	g_cond_signal(engine->syncing);
	g_mutex_unlock(engine->syncing_mutex);

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
	g_assert(group);
	OSyncEngine *engine = g_malloc0(sizeof(OSyncEngine));
	osync_group_set_data(group, engine);
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	//Now load the old uid mapping
	engine->maptable = osync_mappingtable_new(group);
	osync_mappingtable_set_dbpath(engine->maptable, osync_group_get_configdir(group));

	engine->group = group;
	engine->incoming = itm_queue_new();
	
	engine->syncing_mutex = g_mutex_new();
	engine->info_received_mutex = g_mutex_new();
	engine->syncing = g_cond_new();
	engine->info_received = g_cond_new();
	
	//Set the default start flags
	engine->fl_running = osync_flag_new(NULL);
	osync_flag_set_pos_trigger(engine->fl_running, (MSyncFlagTriggerFunc)send_engine_changed, engine);
	engine->fl_sync = osync_flag_new(NULL);
	engine->fl_stop = osync_flag_new(NULL);
	
	//The combined flags
	engine->cmb_sent_changes = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_sent_changes, (MSyncFlagTriggerFunc)trigger_clients_sent_changes, engine);
	engine->cmb_entries_mapped = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_entries_mapped, (MSyncFlagTriggerFunc)send_engine_changed, engine);
	engine->cmb_synced = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_synced, (MSyncFlagTriggerFunc)send_engine_changed, engine);
	engine->cmb_finished = osync_comb_flag_new(FALSE);
	osync_flag_set_pos_trigger(engine->cmb_finished, (MSyncFlagTriggerFunc)osync_engine_reset, engine);
	
	osync_flag_set(engine->fl_sync);
	
	_osync_debug(engine, "ENG", 3, "Created a new syncengine!");
	
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
	_osync_debug(engine, "ENG", 3, "Freeing engine %p", engine);
	osync_mappingtable_free(engine->maptable);
	
	itm_queue_free(engine->incoming);

	g_list_free(engine->clients);
	g_main_loop_unref(engine->syncloop);
	
	g_mutex_free(engine->syncing_mutex);
	g_mutex_free(engine->info_received_mutex);
	g_cond_free(engine->syncing);
	g_cond_free(engine->info_received);
	
	osync_flag_free(engine->fl_running);
	osync_flag_free(engine->fl_sync);
	osync_flag_free(engine->fl_stop);
	osync_flag_free(engine->cmb_sent_changes);
	osync_flag_free(engine->cmb_entries_mapped);
	osync_flag_free(engine->cmb_synced);
	osync_flag_free(engine->cmb_finished);
	
	g_free(engine);
}

/*! @brief This will set the conflict handler for the given engine
 * 
 * The conflict handler will be called everytime a conflict occurs
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the conflict
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
 * 
 */
void osync_engine_set_mappingstatus_callback(OSyncEngine *engine, void (* function) (MSyncMappingUpdate *))
{
	engine->mapstat_callback = function;
}

/*! @brief This will set the engine status handler for the given engine
 * 
 * The engine status handler will be called every time the engine is updated (started, stoped etc)
 * 
 * @param engine A pointer to the engine, for which to set the callback
 * @param function A pointer to a function which will receive the engine status
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
 * 
 */
void osync_engine_set_memberstatus_callback(OSyncEngine *engine, void (* function) (MSyncMemberUpdate *))
{
	engine->mebstat_callback = function;
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
	int i = 0;
	if (!(engine->man_dispatch))
		itm_queue_setup_with_gmainloop(engine->incoming, NULL);
	itm_queue_set_message_handler(engine->incoming, (ITMessageHandler)engine_message_handler, engine);
	
	osync_mappingtable_load(engine->maptable);
	
	osync_flag_set(engine->cmb_entries_mapped);
	osync_flag_set(engine->cmb_synced);
	engine->allow_sync_alert = FALSE;
	
	OSyncMember *member = NULL;
	_osync_debug(engine, "ENG", 3, "Spawning all clients");
	OSyncGroup *group = engine->group;
	
	if (osync_group_num_members(group) < 2) {
		//Not enough members!
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You only configured %i members, but at least 2 are needed", osync_group_num_members(group));
		return FALSE;
	}
	
	for (i = 0; i < osync_group_num_members(group); i++) {
		member = osync_group_nth_member(group, i);
		
		//Creating the client
		OSyncClient *client = osync_client_new(engine, member);

		//Starting the client
		if (!osync_client_init(client, error))
			return FALSE;
	}
	
	engine->syncloop = g_main_loop_new(NULL, FALSE);
	_osync_debug(engine, "ENG", 3, "Running the main loop");

	//Now we can run the main loop
	g_thread_create ((GThreadFunc)g_main_loop_run, engine->syncloop, TRUE, NULL);
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
	
	g_assert(engine);
	_osync_debug(engine, "ENG", 3, "finalizing engine %p", engine);
	g_main_loop_quit(engine->syncloop);
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_client_finalize(client);
	}
	osync_mappingtable_close(engine->maptable);
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
	g_assert(engine);
	osync_flag_set(engine->fl_running);
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
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE. Note that this just says if the sync has been started successfully, not if the sync itself was successfull
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
	return TRUE;
}

/*! @brief This function will block until a synchronization has ended
 *
 * This can be used to wait until the synchronization has ended. Note that this function will always
 * block until 1 sync has ended. It can be used before the sync has started, to wait for one auto-sync
 * to end
 * 
 * @param engine A pointer to the engine, for which to wait for the sync end
 */
void osync_engine_wait_sync_end(OSyncEngine *engine)
{
	g_mutex_lock(engine->syncing_mutex);
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
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
