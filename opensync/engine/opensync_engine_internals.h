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

struct OSyncEngine {
	int ref_count;
	/** The opensync group **/
	OSyncGroup *group;
	
	char *engine_path;
	char *plugin_dir;
	
	OSyncEngineState state;
	
	void (* conflict_callback) (OSyncEngine *, OSyncMapping *, void *);
	void *conflict_userdata;
	/*void (* changestat_callback) (OSyncEngine *, OSyncChangeUpdate *, void *);
	void *changestat_userdata;
	void (* mebstat_callback) (OSyncMemberUpdate *, void *);
	void *mebstat_userdata;
	void (* engstat_callback) (OSyncEngine *, OSyncEngineUpdate *, void *);
	void *engstat_userdata;
	void (* mapstat_callback) (OSyncMappingUpdate *, void *);
	void *mapstat_userdata;*/
	void *(* plgmsg_callback) (OSyncEngine *, OSyncClient *, const char *, void *, void *);
	void *plgmsg_userdata;
	
	/** The g_main_loop of this engine **/
	OSyncThread *thread;
	GMainContext *context;
	
	GCond* syncing;
	GMutex* syncing_mutex;
	
	GCond* started;
	GMutex* started_mutex;
	
	GList *proxies;
	
	osync_bool man_dispatch;
	osync_bool allow_sync_alert;
	
	OSyncError *error;
};

#endif /*OPENSYNC_ENGINE_INTERNALS_H_*/
