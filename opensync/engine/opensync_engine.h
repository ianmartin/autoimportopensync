/*
 * libosync_engine - A synchronization engine for the opensync framework
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
 
#ifndef OPENSYNC_ENGINE_H_
#define OPENSYNC_ENGINE_H_

typedef enum {
	OSYNC_ENGINE_STATE_UNINITIALIZED,
	OSYNC_ENGINE_STATE_INITIALIZED,
	OSYNC_ENGINE_STATE_WAITING,
	OSYNC_ENGINE_STATE_CONNECTING,
	OSYNC_ENGINE_STATE_READING,
	OSYNC_ENGINE_STATE_WRITING,
	OSYNC_ENGINE_STATE_DISCONNECTING
} OSyncEngineState;

OSyncEngine *osync_engine_new(OSyncGroup *group, OSyncError **error);
void osync_engine_ref(OSyncEngine *engine);
void osync_engine_unref(OSyncEngine *engine);

void osync_engine_set_plugindir(OSyncEngine *engine, const char *dir);

osync_bool osync_engine_initialize(OSyncEngine *engine, OSyncError **error);
osync_bool osync_engine_finalize(OSyncEngine *engine, OSyncError **error);

osync_bool osync_engine_synchronize(OSyncEngine *engine, OSyncError **error);
void osync_engine_pause(OSyncEngine *engine);
void osync_engine_abort(OSyncEngine *engine);
osync_bool osync_engine_wait_sync_end(OSyncEngine *engine, OSyncError **error);

void osync_engine_one_iteration(OSyncEngine *engine);
void osync_engine_flag_manual(OSyncEngine *engine);

/*void osync_engine_set_message_callback(OSyncEngine *engine, void *(* function) (OSyncEngine *, OSyncClient *, const char *, void *, void *), void *user_data);
void osync_engine_set_conflict_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncMapping *, void *), void *user_data);
void osync_engine_set_changestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine*, OSyncChangeUpdate *, void *), void *user_data);
void osync_engine_set_mappingstatus_callback(OSyncEngine *engine, void (* function) (OSyncMappingUpdate *, void *), void *user_data);
void osync_engine_set_enginestatus_callback(OSyncEngine *engine, void (* function) (OSyncEngine *, OSyncEngineUpdate *, void *), void *user_data);
void osync_engine_set_memberstatus_callback(OSyncEngine *engine, void (* function) (OSyncMemberUpdate *, void *), void *user_data);*/

osync_bool osync_engine_sync_and_block(OSyncEngine *engine, OSyncError **error);

#endif /*OPENSYNC_ENGINE_H_*/
