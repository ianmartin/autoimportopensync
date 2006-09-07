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
 
#ifndef OPENSYNC_ENGINE_INTERNALS_H_
#define OPENSYNC_ENGINE_INTERNALS_H_

typedef enum {
	OSYNC_ENGINE_SOLVE_DUPLICATE,
	OSYNC_ENGINE_SOLVE_CHOOSE,
	OSYNC_ENGINE_SOLVE_IGNORE,
	OSYNC_ENGINE_SOLVE_USE_LATEST
} OSyncEngineSolveType;

typedef struct OSyncEngineCommand {
	OSyncEngineCmd cmd;
	OSyncMappingEngine *mapping_engine;
	OSyncChange *master;
	OSyncEngineSolveType solve_type;
	OSyncMember *member;
} OSyncEngineCommand;

struct OSyncEngine {
	int ref_count;
	/** The opensync group **/
	OSyncGroup *group;
	OSyncArchive *archive;
	
	char *engine_path;
	char *plugin_dir;
	char *format_dir;
	
	OSyncFormatEnv *formatenv;
	OSyncPluginEnv *pluginenv;
	
	OSyncEngineState state;
	
	osync_conflict_cb conflict_callback;
	void *conflict_userdata;
	
	osync_status_change_cb changestat_callback;
	void *changestat_userdata;
	
	osync_status_member_cb mebstat_callback;
	void *mebstat_userdata;
	
	osync_status_engine_cb engstat_callback;
	void *engstat_userdata;
	
	osync_status_mapping_cb mapstat_callback;
	void *mapstat_userdata;
	
	//void *(* plgmsg_callback) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
	//void *plgmsg_userdata;
	
	/** The g_main_loop of this engine **/
	OSyncThread *thread;
	GMainContext *context;
	
	GAsyncQueue *command_queue;
	GSourceFuncs *command_functions;
	GSource *command_source;
	
	GCond* syncing;
	GMutex* syncing_mutex;
	
	GCond* started;
	GMutex* started_mutex;
	
	GList *proxies;
	GList *object_engines;
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	
	OSyncError *error;
	
	int proxy_connects;
	int proxy_disconnects;
	int proxy_get_changes;
	int proxy_written;
	int proxy_sync_done;
	int proxy_errors;
	
	int obj_errors;
	int obj_connects;
	int obj_disconnects;
	int obj_get_changes;
	int obj_written;
	int obj_sync_done;
	
	osync_bool busy;
	
	GHashTable *internalFormats;
};

#endif /*OPENSYNC_ENGINE_INTERNALS_H_*/
