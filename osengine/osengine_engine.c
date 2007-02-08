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

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>

#include <opensync/opensync_support.h>
#include "opensync/opensync_message_internals.h"
#include "opensync/opensync_queue_internals.h"
#include "opensync/opensync_format_internals.h"

#include "engine_internals.h"
#include <opensync/opensync_user_internals.h>

OSyncMappingEntry *osengine_mappingtable_find_entry(OSyncMappingTable *table, const char *uid, const char *objtype, long long int memberid);
/**
 * @defgroup OSEnginePrivate OpenSync Engine Private API
 * @ingroup PrivateAPI
 * @brief The internals of the multisync engine
 * 
 */

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

void _new_change_receiver(OSyncEngine *engine, OSyncClient *client, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, engine, client, change);

	OSyncError *error = NULL;
	OSyncChangeType change_type = osync_change_get_changetype(change);
	OSyncFormatEnv *format_env = osync_group_get_format_env(engine->group);
	OSyncObjType *objtype = osync_change_get_objtype(change);
	const char* uid = osync_change_get_uid(change);
	OSyncObjFormat *objformat = osync_change_get_objformat(change);

	osync_change_set_member(change, client->member);

	osync_trace(TRACE_INTERNAL, "Handling new change with uid %s, changetype %i, objtype %s and format %s from member %lli", uid, change_type, 
	objtype ? osync_objtype_get_name(objtype) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None",
	osync_member_get_id(client->member));


	/**
	 * first we need to detect the objtype because we use
	 * uid + objtype as identifier for an entry.
	 * Special case is file as objformat... we must not change 
	 * the objtype with format file
	 **/
	if ( (change_type != CHANGE_DELETED) &&
	     (osync_change_has_data(change))) {
		osync_bool is_file_objformat = FALSE;
		if(objformat)
			is_file_objformat = 
				((!strcmp(objformat->name, "file"))?(TRUE):(FALSE));
		if ( (!objtype) || (!objformat) ||
		     (!strcmp(osync_objtype_get_name(objtype), "data")) ||
		     (!strcmp(objformat->name, "plain"))) {
			objtype = osync_change_detect_objtype_full(format_env, change, &error);
		}
		if (objtype) {
			osync_trace(TRACE_INTERNAL, "Detected the object to be of type %s", osync_objtype_get_name(objtype));
			/**we must not change with file as format*/
			if(!is_file_objformat)
			{
				osync_change_set_objtype(change, objtype);
			}
			/**
			 * do not use CHANGE_MODIFIED if slowsync or (change not
			 * exist before if not filesync)
			 **/
			if ( ( (osync_group_get_slow_sync(engine->group,
				 osync_objtype_get_name(objtype))) || 
			       ( (!is_file_objformat) &&
				 (!osengine_mappingtable_find_entry(
					engine->maptable, uid,
					osync_objtype_get_name(objtype),
					osync_member_get_id(client->member))) ) 
			      ) && (change_type == CHANGE_MODIFIED) ){
				osync_change_set_changetype(change, CHANGE_ADDED);
				change_type = osync_change_get_changetype(change);
			}
		}
	} else 
		if (change_type == CHANGE_DELETED){
			/**
			 * we need to handle the special delete case where objtype 
			 * is data and no uid with objtype data exists from this
			 * member	
			 **/
			if ( !objtype ||
			     (( !strcmp(osync_objtype_get_name(objtype), "data") ) &&
			     ( !osengine_mappingtable_find_entry(
					engine->maptable, uid,
				 osync_objtype_get_name(objtype),
				osync_member_get_id(client->member)) )) ){

				OSyncMappingEntry *entry = 
					osengine_mappingtable_find_entry(
						engine->maptable, uid, NULL,
						osync_member_get_id(client->member)
					);
				if (entry) {
					osync_change_set_objtype(change,
						 osync_change_get_objtype(
							entry->change));
					objtype=osync_change_get_objtype(change);
				} else {
					osync_error_set(&error, OSYNC_ERROR_GENERIC,
						 "Could not find one entry with UID=%s to delete.", uid);
					goto error;
				}
			}
		} else {
			osync_trace(TRACE_INTERNAL, "Change has no data!");
		}
	
	osync_trace(TRACE_INTERNAL, "Handling new change with uid %s, changetype %i, data %p, size %i, objtype %s and format %s from member %lli", uid, change_type, osync_change_get_data(change), osync_change_get_datasize(change), objtype ? osync_objtype_get_name(objtype) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None", osync_member_get_id(client->member));

	if (!objtype){
		if (!osync_error_is_set(&error)) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC,
				"ObjType not set for uid %s.", uid);
		} else {
			osync_error_update(&error,
				"ObjType not set for uid %s.", uid);
		}
		goto error;
	}
	
	
	OSyncMappingEntry *entry = osengine_mappingtable_store_change(engine->maptable, change);
	change = entry->change;
	if (!osync_change_save(change, TRUE, &error)) {
		osync_error_duplicate(&engine->error, &error);
		osync_status_update_change(engine, change, CHANGE_RECV_ERROR, &error);
		osync_error_update(&engine->error, "Unable to receive one or more objects");
		osync_flag_unset(entry->fl_has_data);
		goto error;
	}
	
	osync_group_remove_changelog(engine->group, change, &error);
	
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
		osync_flag_unset(entry->mapping->fl_chkconflict);
		osync_flag_unset(entry->mapping->fl_multiplied);
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
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
	return;
}

OSyncClient *osengine_get_client(OSyncEngine *engine, long long int memberId)
{
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		if (osync_member_get_id(client->member) == memberId)
			return client;
	}
	return NULL;
}


void send_engine_changed(OSyncEngine *engine)
{
	if (!engine->is_initialized)
		return;

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_ENGINE_CHANGED, 0, NULL);
	/*FIXME: Handle errors here */

	osync_debug("ENG", 4, "Sending message %p:\"ENGINE_CHANGED\"", message);
	osync_queue_send_message(engine->commands_to_self, NULL, message, NULL);
}

void send_mapping_changed(OSyncEngine *engine, OSyncMapping *mapping)
{
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_MAPPING_CHANGED, sizeof(long long), NULL);
	osync_message_write_long_long_int(message, mapping->id);
	/*FIXME: Handle errors here */

	osync_queue_send_message(engine->commands_to_self, NULL, message, NULL);
	/*FIXME: Handle errors here, too */
}

void send_mappingentry_changed(OSyncEngine *engine, OSyncMappingEntry *entry)
{
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_MAPPINGENTRY_CHANGED, sizeof(long long)*2, NULL);

	/*FIXME: don't pass a pointer through the messaging system */
	long long ptr = (long long)(long)entry;
	osync_message_write_long_long_int(message, ptr);
	/*FIXME: Handle errors here */

	osync_queue_send_message(engine->commands_to_self, NULL, message, NULL);
	/*FIXME: Handle errors here, too */
}

/*! @brief The queue message handler of the engine
 * 
 * @param sender The Client who sent this message
 * @param message The message
 * @param engine The engine
 * 
 */
static void engine_message_handler(OSyncMessage *message, OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "engine_message_handler(%p:%lli-%i, %p)", message, message->id1, message->id2, engine);
	
	OSyncChange *change = NULL;
			
	osync_trace(TRACE_INTERNAL, "engine received command %i", osync_message_get_command(message));
	
	switch (osync_message_get_command(message)) {
		case OSYNC_MESSAGE_SYNCHRONIZE:
			osync_trace(TRACE_INTERNAL, "all deciders");
			osengine_client_all_deciders(engine);
			break;
		case OSYNC_MESSAGE_NEW_CHANGE:
			osync_demarshal_change(message, osync_group_get_format_env(engine->group), &change);
			
			long long int member_id = 0;
			osync_message_read_long_long_int(message, &member_id);
			OSyncClient *sender = osengine_get_client(engine, member_id);
			
			_new_change_receiver(engine, sender, change);
			break;
		case OSYNC_MESSAGE_ENGINE_CHANGED:
			osengine_client_all_deciders(engine);
			osengine_mapping_all_deciders(engine);
			GList *u;
			for (u = engine->maptable->unmapped; u; u = u->next) {
				OSyncMappingEntry *unmapped = u->data;
				send_mappingentry_changed(engine, unmapped);
			}
			break;
		case OSYNC_MESSAGE_MAPPING_CHANGED:
		{
			long long id;
			osync_message_read_long_long_int(message, &id);
			/*FIXME: check errors by read_long_long_int */
			OSyncMapping *mapping = osengine_mappingtable_mapping_from_id(engine->maptable, id);
			
			if (!g_list_find(engine->maptable->mappings, mapping)) {
				osync_trace(TRACE_EXIT, "%s: Mapping %p is dead", __func__, mapping);
				return;
			}
			
			osengine_mapping_decider(engine, mapping);
		}
		break;
		case OSYNC_MESSAGE_MAPPINGENTRY_CHANGED:
		{
			long long ptr;
			osync_message_read_long_long_int(message, &ptr);
			OSyncMappingEntry *entry = (OSyncMappingEntry*)(long)ptr;
			
			if (!g_list_find(engine->maptable->entries, entry) && !g_list_find(engine->maptable->unmapped, entry)) {
				osync_trace(TRACE_EXIT, "%s: Entry %p is dead", __func__, entry);
				return;
			}
			
			osengine_mappingentry_decider(engine, entry);
		}
		break;
		case OSYNC_MESSAGE_SYNC_ALERT:
			if (engine->allow_sync_alert)
				osync_flag_set(engine->fl_running);
			else
				osync_trace(TRACE_INTERNAL, "Sync Alert not allowed");
		break;

		default:
			break;
	}
	
	/*TODO: Implement handling of the messages listed below, on commented code */

	/*	
	if (osync_message_is_signal (message, "CLIENT_CHANGED")) {
		OSyncClient *client = osync_message_get_data(message, "client");
		
		if (!g_list_find(engine->clients, client)) {
			osync_trace(TRACE_EXIT, "%s: Client %p is dead", __func__, client);
			return;
		}
		
		osengine_client_decider(engine, client);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	if (osync_message_is_signal (message, "PLUGIN_MESSAGE")) {
		char *name = osync_message_get_data(message, "name");
		void *data = osync_message_get_data(message, "data");
		engine->plgmsg_callback(engine, sender, name, data, engine->plgmsg_userdata);
		osync_trace(TRACE_EXIT, "engine_message_handler");
		return;
	}
	
	osync_debug("ENG", 0, "Unknown message \"%s\"", osync_message_get_msgname(message));
	osync_trace(TRACE_EXIT_ERROR, "engine_message_handler: Unknown message");
	g_assert_not_reached();*/
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_clients_sent_changes(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_status_update_engine(engine, ENG_ENDPHASE_READ, NULL);
	
	g_mutex_lock(engine->info_received_mutex);
	g_cond_signal(engine->info_received);
	g_mutex_unlock(engine->info_received_mutex);
	
	//Load the old mappings
	osengine_mappingtable_inject_changes(engine->maptable);
	
	send_engine_changed(engine);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_clients_read_all(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);

	send_engine_changed(engine);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_status_end_conflicts(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_status_update_engine(engine, ENG_END_CONFLICTS, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_clients_connected(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_status_update_engine(engine, ENG_ENDPHASE_CON, NULL);
	osengine_client_all_deciders(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_clients_comitted_all(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_status_update_engine(engine, ENG_ENDPHASE_WRITE, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}


/*void send_engine_committed_all(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	
	engine->committed_all_sent = TRUE;
	
	osync_trace(TRACE_INTERNAL, "++++ ENGINE COMMAND: Committed all ++++");
		
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		if (osync_flag_is_not_set(client->fl_committed_all))
			send_committed_all(client, engine);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void trigger_engine_committed_all(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	
	if (osync_flag_is_not_set(engine->cmb_multiplied)) {
		osync_trace(TRACE_EXIT, "%s: Not multiplied yet", __func__);
		return;
	}
	
	send_engine_committed_all(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}*/

static gboolean startupfunc(gpointer data)
{
	OSyncEngine *engine = data;
	osync_trace(TRACE_INTERNAL, "+++++++++ This is the engine of group \"%s\" +++++++++", osync_group_get_name(engine->group));
	
	OSyncError *error = NULL;
	if (!osengine_mappingtable_load(engine->maptable, &error)) {
		osync_error_duplicate(&engine->error, &error);
		osync_status_update_engine(engine, ENG_ERROR, &error);
		osync_error_update(&engine->error, "Unable to connect one of the members");
		osync_flag_set(engine->fl_stop);
	}
	
	g_mutex_lock(engine->started_mutex);
	g_cond_signal(engine->started);
	g_mutex_unlock(engine->started_mutex);
	return FALSE;
}

/*@}*/

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
osync_bool osengine_reset(OSyncEngine *engine, OSyncError **error)
{
	//FIXME Check if engine is running
	osync_trace(TRACE_ENTRY, "osengine_reset(%p, %p)", engine, error);
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_client_reset(client);
	}
	
	osync_flag_set_state(engine->fl_running, FALSE);
	osync_flag_set_state(engine->fl_stop, FALSE);
	osync_flag_set_state(engine->cmb_sent_changes, FALSE);
	osync_flag_set_state(engine->cmb_entries_mapped, TRUE);
	osync_flag_set_state(engine->cmb_synced, TRUE);
	osync_flag_set_state(engine->cmb_chkconflict, TRUE);
	osync_flag_set_state(engine->cmb_finished, FALSE);
	osync_flag_set_state(engine->cmb_connected, FALSE);
	osync_flag_set_state(engine->cmb_read_all, TRUE);
	osync_flag_set_state(engine->cmb_committed_all, TRUE);
	osync_flag_set_state(engine->cmb_committed_all_sent, FALSE);
	
	osync_status_update_engine(engine, ENG_ENDPHASE_DISCON, NULL);
	
	engine->committed_all_sent = FALSE;
	
	osengine_mappingtable_reset(engine->maptable);
	
	if (engine->error) {
		//FIXME We might be leaking memory here
		OSyncError *newerror = NULL;
		osync_error_duplicate(&newerror, &engine->error);
		osync_status_update_engine(engine, ENG_ERROR, &newerror);
		osync_group_set_slow_sync(engine->group, "data", TRUE);
	} else {
		osync_status_update_engine(engine, ENG_SYNC_SUCCESSFULL, NULL);
		osync_group_reset_slow_sync(engine->group, "data");
	}
	
	osync_trace(TRACE_INTERNAL, "engine error is %p", engine->error);
	
	g_mutex_lock(engine->syncing_mutex);
	g_cond_signal(engine->syncing);
	g_mutex_unlock(engine->syncing_mutex);

	osync_trace(TRACE_EXIT, "osengine_reset");
	return TRUE;
}

/* Implementation of g_mkdir_with_parents()
 *
 * This function overwrite the contents of the 'dir' parameter
 */
static int __mkdir_with_parents(char *dir, int mode)
{
	if (g_file_test(dir, G_FILE_TEST_IS_DIR))
		return 0;

	char *slash = strrchr(dir, '/');
	if (slash && slash != dir) {
		/* Create parent directory if needed */

		/* This is a trick: I don't want to allocate a new string
		 * for the parent directory. So, just put a NUL char
		 * in the last slash, and restore it after creating the
		 * parent directory
		 */
		*slash = '\0';
		if (__mkdir_with_parents(dir, mode) < 0)
			return -1;
		*slash = '/';
	}

	if (mkdir(dir, mode) < 0)
		return -1;

	return 0;
}

static int mkdir_with_parents(const char *dir, int mode)
{
	int r;
	char *mydir = strdup(dir);
	if (!mydir)
		return -1;

	r = __mkdir_with_parents(mydir, mode);
	free(mydir);
	return r;
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
OSyncEngine *osengine_new(OSyncGroup *group, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, error);
	
	g_assert(group);
	OSyncEngine *engine = g_malloc0(sizeof(OSyncEngine));
	osync_group_set_data(group, engine);
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	engine->context = g_main_context_new();
	engine->syncloop = g_main_loop_new(engine->context, FALSE);
	engine->group = group;

	OSyncUserInfo *user = osync_user_new(error);
	if (!user)
		goto error;

	char *enginesdir = g_strdup_printf("%s/engines", osync_user_get_confdir(user));
	char *path = g_strdup_printf("%s/enginepipe", enginesdir);

	if (mkdir_with_parents(enginesdir, 0755) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't create engines directory: %s", strerror(errno));
		goto error_free_paths;
	}

	engine->syncing_mutex = g_mutex_new();
	engine->info_received_mutex = g_mutex_new();
	engine->syncing = g_cond_new();
	engine->info_received = g_cond_new();
	engine->started_mutex = g_mutex_new();
	engine->started = g_cond_new();
		
	//Set the default start flags
	engine->fl_running = osync_flag_new(NULL);
	osync_flag_set_pos_trigger(engine->fl_running, (OSyncFlagTriggerFunc)osengine_client_all_deciders, engine, NULL);

	engine->fl_sync = osync_flag_new(NULL);
	engine->fl_stop = osync_flag_new(NULL);
	osync_flag_set_pos_trigger(engine->fl_stop, (OSyncFlagTriggerFunc)osengine_client_all_deciders, engine, NULL);
	
	//The combined flags
	engine->cmb_sent_changes = osync_comb_flag_new(FALSE, FALSE);
	osync_flag_set_pos_trigger(engine->cmb_sent_changes, (OSyncFlagTriggerFunc)trigger_clients_sent_changes, engine, NULL);
	
	engine->cmb_read_all = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_read_all, (OSyncFlagTriggerFunc)trigger_clients_read_all, engine, NULL);
	
	engine->cmb_entries_mapped = osync_comb_flag_new(FALSE, FALSE);
	osync_flag_set_pos_trigger(engine->cmb_entries_mapped, (OSyncFlagTriggerFunc)send_engine_changed, engine, NULL);

	
	engine->cmb_synced = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_synced, (OSyncFlagTriggerFunc)send_engine_changed, engine, NULL);

	
	engine->cmb_finished = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_finished, (OSyncFlagTriggerFunc)osengine_reset, engine, NULL);
	
	engine->cmb_connected = osync_comb_flag_new(FALSE, FALSE);
	osync_flag_set_pos_trigger(engine->cmb_connected, (OSyncFlagTriggerFunc)trigger_clients_connected, engine, NULL);

	engine->cmb_chkconflict = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_chkconflict, (OSyncFlagTriggerFunc)trigger_status_end_conflicts, engine, NULL);
	
	engine->cmb_multiplied = osync_comb_flag_new(FALSE, TRUE);
	
	engine->cmb_committed_all = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_committed_all, (OSyncFlagTriggerFunc)send_engine_changed, engine, NULL);


	engine->cmb_committed_all_sent = osync_comb_flag_new(FALSE, TRUE);
	osync_flag_set_pos_trigger(engine->cmb_committed_all_sent, (OSyncFlagTriggerFunc)trigger_clients_comitted_all, engine, NULL);
	
	osync_flag_set(engine->fl_sync);
	
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		if (!osync_client_new(engine, member, error))
			goto error_free_paths;
	}
	
	engine->maptable = osengine_mappingtable_new(engine);
	
	osync_trace(TRACE_EXIT, "osengine_new: %p", engine);
	return engine;

error_free_paths:
	g_free(path);
	g_free(enginesdir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*! @brief This will free a engine and all resources associated
 * 
 * This will free a engine and all resources associated
 * 
 * @param engine A pointer to the engine, which you want to free
 * 
 */
void osengine_free(OSyncEngine *engine)
{
	osync_trace(TRACE_ENTRY, "osengine_free(%p)", engine);
	
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
	osync_flag_free(engine->cmb_chkconflict);
	osync_flag_free(engine->cmb_finished);
	osync_flag_free(engine->cmb_connected);
	osync_flag_free(engine->cmb_read_all);
	osync_flag_free(engine->cmb_multiplied);
	osync_flag_free(engine->cmb_committed_all);
	osync_flag_free(engine->cmb_committed_all_sent);
	
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
	osync_trace(TRACE_EXIT, "osengine_free");
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
void osengine_set_conflict_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncMapping *, void *), void *user_data)
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
void osengine_set_changestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncChangeUpdate *, void *), void *user_data)
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
void osengine_set_mappingstatus_callback(OSyncEngine *engine, void (* function) (OSyncMappingUpdate *, void *), void *user_data)
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
void osengine_set_enginestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncEngineUpdate *, void *), void *user_data)
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
void osengine_set_memberstatus_callback(OSyncEngine *engine, void (* function) (OSyncMemberUpdate *, void *), void *user_data)
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
void osengine_set_message_callback(OSyncEngine *engine, void *(* function) (OSyncEngine *, OSyncClient *, const char *, void *, void *), void *user_data)
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
osync_bool osengine_init(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osengine_init(%p, %p)", engine, error);
	
	if (engine->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "This engine was already initialized");
		osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	switch (osync_group_lock(engine->group)) {
		case OSYNC_LOCKED:
			osync_error_set(error, OSYNC_ERROR_LOCKED, "Group is locked");
			osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
			return FALSE;
		case OSYNC_LOCK_STALE:
			osync_debug("ENG", 1, "Detected stale lock file. Slow-syncing");
			osync_status_update_engine(engine, ENG_PREV_UNCLEAN, NULL);
			osync_group_set_slow_sync(engine->group, "data", TRUE);
			break;
		default:
			break;
	}
	
	osync_flag_set(engine->cmb_entries_mapped);
	osync_flag_set(engine->cmb_synced);
	engine->allow_sync_alert = TRUE;
	
	//OSyncMember *member = NULL;
	OSyncGroup *group = engine->group;
	
	if (osync_group_num_members(group) < 2) {
		//Not enough members!
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You only configured %i members, but at least 2 are needed", osync_group_num_members(group));
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	engine->is_initialized = TRUE;
	
	osync_trace(TRACE_INTERNAL, "Spawning clients");
	GList *c = NULL;
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		osync_queue_create(client->commands_from_osplugin, NULL);

		if (!osync_client_spawn(client, engine, error)) {
			osync_group_unlock(engine->group, TRUE);
			osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
			return FALSE;
		}

		osync_queue_set_message_handler(client->commands_from_osplugin, (OSyncMessageHandler)engine_message_handler, engine);
		if (!(engine->man_dispatch))
			osync_queue_setup_with_gmainloop(client->commands_from_osplugin, engine->context);
		osync_trace(TRACE_INTERNAL, "opening client queue");
		if (!osync_queue_connect(client->commands_from_osplugin, OSYNC_QUEUE_RECEIVER, 0 )) {
			osync_group_unlock(engine->group, TRUE);
			osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
			return FALSE;
		}
	}
	
	osync_trace(TRACE_INTERNAL, "opening engine queue");
	if (!osync_queue_new_pipes(&engine->commands_from_self, &engine->commands_to_self, error)) {
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
		return FALSE;
	}

	if (!osync_queue_connect(engine->commands_from_self, OSYNC_QUEUE_RECEIVER, 0 )) {
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	if (!osync_queue_connect(engine->commands_to_self, OSYNC_QUEUE_SENDER, 0 )) {
		osync_group_unlock(engine->group, TRUE);
		osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
		return FALSE;
	}
	
	osync_queue_set_message_handler(engine->commands_from_self, (OSyncMessageHandler)engine_message_handler, engine);
	if (!(engine->man_dispatch))
		osync_queue_setup_with_gmainloop(engine->commands_from_self, engine->context);
	
	osync_trace(TRACE_INTERNAL, "initializing clients");
	for (c = engine->clients; c; c = c->next) {
		OSyncClient *client = c->data;
		if (!osync_client_init(client, engine, error)) {
			osengine_finalize(engine);
			osync_group_unlock(engine->group, TRUE);
			osync_trace(TRACE_EXIT_ERROR, "osengine_init: %s", osync_error_print(error));
			return FALSE;
		}
	}
	
	osync_debug("ENG", 3, "Running the main loop");

	//Now we can run the main loop
	//We protect the startup by a g_cond
	g_mutex_lock(engine->started_mutex);
	GSource *idle = g_idle_source_new();
	g_source_set_priority(idle, G_PRIORITY_HIGH);
	g_source_set_callback(idle, startupfunc, engine, NULL);
    g_source_attach(idle, engine->context);
	engine->thread = g_thread_create ((GThreadFunc)g_main_loop_run, engine->syncloop, TRUE, NULL);
	g_cond_wait(engine->started, engine->started_mutex);
	g_mutex_unlock(engine->started_mutex);
	
	osync_trace(TRACE_EXIT, "osengine_init");
	return TRUE;
}

/*! @brief This will finalize a engine
 * 
 * Finalizing a engine will stop all threads and listening server.
 * The engine can be initialized again.
 * 
 * @param engine A pointer to the engine, which will be finalized
 * 
 */
void osengine_finalize(OSyncEngine *engine)
{
	//FIXME check if engine is running
	osync_trace(TRACE_ENTRY, "osengine_finalize(%p)", engine);

	if (!engine->is_initialized) {
		osync_trace(TRACE_EXIT_ERROR, "osengine_finalize: Not initialized");
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
		osync_queue_disconnect(client->commands_from_osplugin, NULL);
		osync_client_finalize(client, NULL);
	}

	osync_queue_disconnect(engine->commands_from_self, NULL);
	osync_queue_disconnect(engine->commands_to_self, NULL);

	osync_queue_free(engine->commands_from_self);
	engine->commands_from_self = NULL;
	osync_queue_free(engine->commands_to_self);
	engine->commands_to_self = NULL;
	
	osengine_mappingtable_close(engine->maptable);
	
	if (engine->error) {
		/* If the error occured during connect, we
		 * dont want to trigger a slow-sync the next
		 * time. In the case the we have a slow-sync
		 * right in the beginning, we also dont remove
		 * the lockfile to trigger a slow-sync again
		 * next time */
		if (!osync_flag_is_set(engine->cmb_connected) && !engine->slowsync)
			osync_group_unlock(engine->group, TRUE);
		else
			osync_group_unlock(engine->group, FALSE);
	} else
		osync_group_unlock(engine->group, TRUE);
	
	engine->is_initialized = FALSE;
	osync_trace(TRACE_EXIT, "osengine_finalize");
}

/*! @brief Starts to synchronize the given OSyncEngine
 *
 * This function synchronizes a given engine. The Engine has to be created
 * from a OSyncGroup before by using osengine_new(). This function will not block
 * 
 * @param engine A pointer to the engine, which will be used to sync
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE. Note that this just says if the sync has been started successfully, not if the sync itself was successfull
 * 
 */
osync_bool osengine_synchronize(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_INTERNAL, "synchronize now");
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	g_assert(engine);
	
	if (!engine->is_initialized) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "osengine_synchronize: Not initialized");
		goto error;
	}
	
	/* We now remember if slow-sync is set right from the start.
	 * If it is, we dont remove the lock file in the case of
	 * a error during connect. */
	if (osync_group_get_slow_sync(engine->group, "data")) {
		engine->slowsync = TRUE;
	} else {
		engine->slowsync = FALSE;
	}
	
	engine->wasted = 0;
	engine->alldeciders = 0;
	
	osync_flag_set(engine->fl_running);
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_SYNCHRONIZE, 0, error);
	if (!message)
		goto error;
	
	if (!osync_queue_send_message(engine->commands_to_self, NULL, message, error))
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

/*! @brief Sets a flag on the engine that the engine should only request the info about sync objects
 *
 * This can be used to see only what has changed. The engine will not request the data itself from
 * the members. Note that some members might not support this behaviour and might send the data anyways.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osengine_flag_only_info(OSyncEngine *engine)
{
	osync_flag_unset(engine->fl_sync);
}

/*! @brief Sets a flag on the engine that the engine should do single stepping (For debugging)
 *
 * This flag can be used to set single stepping on the engine. The engine will pause after each iteration.
 * Use osengine_one_iteration to initialize the next iteration. This is only for debugging purposes.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osengine_flag_manual(OSyncEngine *engine)
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
void osengine_pause(OSyncEngine *engine)
{
	osync_flag_unset(engine->fl_running);
}

/*! @brief Sets a flag on the engine that the engine should do single stepping (For debugging)
 *
 * This flag can be used to set single stepping on the engine. The engine will pause after each iteration.
 * Use osengine_one_iteration to initialize the next iteration. This is only for debugging purposes.
 * 
 * @param engine A pointer to the engine, for which to set the flag
 */
void osengine_abort(OSyncEngine *engine)
{
	osync_flag_set(engine->fl_stop);
}

/*! @brief Allows that the engine can be started by a member
 * 
 * Allow the engine to by started by a member by sending a sync alert.
 * 
 * @param engine The engine
 */
void osengine_allow_sync_alert(OSyncEngine *engine)
{
	engine->allow_sync_alert = TRUE;
}

/*! @brief Do not allow that the engine can be started by a member
 * 
 * Do not allow the engine to by started by a member by sending a sync alert.
 * 
 * @param engine The engine
 */
void osengine_deny_sync_alert(OSyncEngine *engine)
{
	engine->allow_sync_alert = FALSE;
}

/*! @brief This function will synchronize once and block until the sync has finished
 *
 * This can be used to sync a group and wait for the synchronization end. DO NOT USE
 * osengine_wait_sync_end for this as this might introduce a race condition.
 * 
 * @param engine A pointer to the engine, which to sync and wait for the sync end
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise.
 * 
 */
osync_bool osengine_sync_and_block(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	g_mutex_lock(engine->syncing_mutex);
	
	if (!osengine_synchronize(engine, error)) {
		g_mutex_unlock(engine->syncing_mutex);
		goto error;
	}
	
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
	
	if (engine->error) {
		osync_error_duplicate(error, &(engine->error));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
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
osync_bool osengine_wait_sync_end(OSyncEngine *engine, OSyncError **error)
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
void osengine_wait_info_end(OSyncEngine *engine)
{
	g_mutex_lock(engine->info_received_mutex);
	g_cond_wait(engine->info_received, engine->info_received_mutex);
	g_mutex_unlock(engine->info_received_mutex);
}

/*! @brief Does one iteration of the engine (For debugging)
 *
 * @param engine The engine to iterate
 */
void osengine_one_iteration(OSyncEngine *engine)
{
	/*TODO: Reimplement support to stepping mode on engine */
	abort();//osync_queue_dispatch(engine->incoming);
}

/*! @brief Searches for a mapping by its id
 *
 * @param engine The engine
 * @param id The id of the mapping
 * @returns The mapping or NULL if not found
 */
OSyncMapping *osengine_mapping_from_id(OSyncEngine *engine, long long int id)
{
	return osengine_mappingtable_mapping_from_id(engine->maptable, id);
}

/** @}*/
