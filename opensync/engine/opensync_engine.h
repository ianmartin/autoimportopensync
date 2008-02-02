/*
 * libopensync - A synchronization engine for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007       Daniel Gollub <dgollub@suse.de>
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
 
#ifndef OPENSYNC_ENGINE_H_
#define OPENSYNC_ENGINE_H_

typedef enum {
	OSYNC_ENGINE_COMMAND_CONNECT = 1,
	OSYNC_ENGINE_COMMAND_READ = 2,
	OSYNC_ENGINE_COMMAND_WRITE = 3,
	OSYNC_ENGINE_COMMAND_SYNC_DONE = 4,
	OSYNC_ENGINE_COMMAND_DISCONNECT = 5,
	OSYNC_ENGINE_COMMAND_SOLVE = 6,
	OSYNC_ENGINE_COMMAND_DISCOVER = 7
} OSyncEngineCmd;

typedef enum {
	OSYNC_ENGINE_STATE_UNINITIALIZED,
	OSYNC_ENGINE_STATE_INITIALIZED,
	OSYNC_ENGINE_STATE_WAITING,
	OSYNC_ENGINE_STATE_CONNECTING,
	OSYNC_ENGINE_STATE_READING,
	OSYNC_ENGINE_STATE_WRITING,
	OSYNC_ENGINE_STATE_DISCONNECTING
} OSyncEngineState;

typedef enum {
	OSYNC_ENGINE_EVENT_CONNECTED = 1,
	OSYNC_ENGINE_EVENT_ERROR = 2,
	OSYNC_ENGINE_EVENT_READ = 3,
	OSYNC_ENGINE_EVENT_WRITTEN = 4,
	OSYNC_ENGINE_EVENT_SYNC_DONE = 5,
	OSYNC_ENGINE_EVENT_DISCONNECTED = 6,
	OSYNC_ENGINE_EVENT_SUCCESSFUL = 7,
	OSYNC_ENGINE_EVENT_END_CONFLICTS = 8,
	OSYNC_ENGINE_EVENT_PREV_UNCLEAN = 9
} OSyncEngineEvent;

typedef enum {
	OSYNC_CLIENT_EVENT_CONNECTED = 1,
	OSYNC_CLIENT_EVENT_ERROR = 2,
	OSYNC_CLIENT_EVENT_READ = 3,
	OSYNC_CLIENT_EVENT_WRITTEN = 4,
	OSYNC_CLIENT_EVENT_SYNC_DONE = 5,
	OSYNC_CLIENT_EVENT_DISCONNECTED = 6,
	OSYNC_CLIENT_EVENT_DISCOVERED = 7
} OSyncMemberEvent;

typedef enum {
	OSYNC_CHANGE_EVENT_READ = 1,
	OSYNC_CHANGE_EVENT_WRITTEN = 2,
	OSYNC_CHANGE_EVENT_ERROR = 3
} OSyncChangeEvent;

typedef enum {
	OSYNC_MAPPING_EVENT_SOLVED = 1,
	//OSYNC_MAPPING_EVENT_WRITTEN = 2,
	OSYNC_MAPPING_EVENT_ERROR = 3
} OSyncMappingEvent;


/*! @brief Struct for the member status callback
 * @ingroup OSEnginePublic
 */
typedef struct OSyncMemberUpdate {
	/** The type of the status update */
	OSyncMemberEvent type;
	char *objtype;
	/** The member for which the status update is */
	OSyncMember *member;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncMemberUpdate;

/*! @brief Struct for the change status callback
 * @ingroup OSEnginePublic
 */
typedef struct OSyncChangeUpdate {
	/** The type of the status update */
	OSyncChangeEvent type;
	/** The change for which the status update is */
	OSyncChange *change;
	/** The id of the member which sent this change */
	OSyncMember *member;
	/** The id of the mapping to which this change belongs if any */
	int mapping_id;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncChangeUpdate;

/*! @brief Struct for the mapping status callback
 * @ingroup OSEnginePublic
 */
typedef struct OSyncMappingUpdate {
	/** The type of the status update */
	OSyncMappingEvent type;
	/** If the mapping was already solved, this will have the id if the winning entry */
	long long int winner;
	/** The mapping for which the status update is */
	OSyncMapping *mapping;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncMappingUpdate;

/*! @brief Struct for the engine status callback
 * @ingroup OSEnginePublic
 */
typedef struct OSyncEngineUpdate {
	/** The type of the status update */
	OSyncEngineEvent type;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncEngineUpdate;

OSYNC_EXPORT OSyncEngine *osync_engine_new(OSyncGroup *group, OSyncError **error);
OSYNC_EXPORT OSyncEngine *osync_engine_ref(OSyncEngine *engine);
OSYNC_EXPORT void osync_engine_unref(OSyncEngine *engine);

OSYNC_EXPORT void osync_engine_set_plugindir(OSyncEngine *engine, const char *dir);
OSYNC_EXPORT void osync_engine_set_formatdir(OSyncEngine *engine, const char *dir);

OSYNC_EXPORT OSyncGroup *osync_engine_get_group(OSyncEngine *engine);
OSYNC_EXPORT OSyncArchive *osync_engine_get_archive(OSyncEngine *engine);

OSYNC_EXPORT osync_bool osync_engine_initialize(OSyncEngine *engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_finalize(OSyncEngine *engine, OSyncError **error);

OSYNC_EXPORT osync_bool osync_engine_synchronize(OSyncEngine *engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_synchronize_and_block(OSyncEngine *engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_wait_sync_end(OSyncEngine *engine, OSyncError **error);

OSYNC_EXPORT osync_bool osync_engine_discover(OSyncEngine *engine, OSyncMember *member, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_discover_and_block(OSyncEngine *engine, OSyncMember *member, OSyncError **error);

//OSYNC_EXPORT void osync_engine_pause(OSyncEngine *engine);
//OSYNC_EXPORT void osync_engine_abort(OSyncEngine *engine);

//OSYNC_EXPORT void osync_engine_one_iteration(OSyncEngine *engine);
//OSYNC_EXPORT void osync_engine_flag_manual(OSyncEngine *engine);

typedef struct OSyncMappingEngine OSyncMappingEngine;

//typedef void *(* osync_message_cb) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
typedef void (* osync_conflict_cb) (OSyncEngine *, OSyncMappingEngine *, void *);
typedef void (* osync_status_change_cb) (OSyncChangeUpdate *, void *);
typedef void (* osync_status_mapping_cb) (OSyncMappingUpdate *, void *);
typedef void (* osync_status_member_cb) (OSyncMemberUpdate *, void *);
typedef void (* osync_status_engine_cb) (OSyncEngineUpdate *, void *);

/* OSYNC_EXPORT void osync_engine_set_message_callback(OSyncEngine *engine, osync_message_cb callback, void *user_data);*/
OSYNC_EXPORT void osync_engine_set_conflict_callback(OSyncEngine *engine, osync_conflict_cb callback, void *user_data);
OSYNC_EXPORT void osync_engine_set_changestatus_callback(OSyncEngine *engine, osync_status_change_cb callback, void *user_data);
OSYNC_EXPORT void osync_engine_set_mappingstatus_callback(OSyncEngine *engine, osync_status_mapping_cb callback, void *user_data);
OSYNC_EXPORT void osync_engine_set_enginestatus_callback(OSyncEngine *engine, osync_status_engine_cb callback, void *user_data);
OSYNC_EXPORT void osync_engine_set_memberstatus_callback(OSyncEngine *engine, osync_status_member_cb callback, void *user_data);

OSYNC_EXPORT void osync_engine_event(OSyncEngine *engine, OSyncEngineEvent event);
OSYNC_EXPORT osync_bool osync_engine_check_get_changes(OSyncEngine *engine);

OSYNC_EXPORT int osync_engine_num_proxies(OSyncEngine *engine);
OSYNC_EXPORT OSyncClientProxy *osync_engine_nth_proxy(OSyncEngine *engine, int nth);
OSYNC_EXPORT OSyncClientProxy *osync_engine_find_proxy(OSyncEngine *engine, OSyncMember *member);

OSYNC_EXPORT int osync_engine_num_objengine(OSyncEngine *engine);
OSYNC_EXPORT OSyncObjEngine *osync_engine_nth_objengine(OSyncEngine *engine, int nth);
OSYNC_EXPORT OSyncObjEngine *osync_engine_find_objengine(OSyncEngine *engine, const char *objtype);

OSYNC_EXPORT osync_bool osync_engine_mapping_solve(OSyncEngine *engine, OSyncMappingEngine *mapping_engine, OSyncChange *change, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_mapping_duplicate(OSyncEngine *engine, OSyncMappingEngine *mapping_engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_mapping_ignore_conflict(OSyncEngine *engine, OSyncMappingEngine *mapping_engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_engine_mapping_use_latest(OSyncEngine *engine, OSyncMappingEngine *mapping_engine, OSyncError **error);

#endif /*OPENSYNC_ENGINE_H_*/
