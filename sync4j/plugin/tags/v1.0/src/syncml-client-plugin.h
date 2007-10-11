/*
 * Copyright (C) 2006 Michael Kolmodin
 * Copyright (C) 2007 Michael Unterkalmsteiner, <michael.unterkalmsteiner@stud-inf.unibz.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef  SYNCML_CLIENT_PLUGIN
#define  SYNCML_CLIENT_PLUGIN

#include "Sync4jClient.h"

#define LOGDIR		"."
#define LOGFILE		"syncml.log"
#define LOGLEVEL	LOG_LEVEL_DEBUG

extern "C" {	
	osync_bool get_sync_info(OSyncPluginEnv* env, OSyncError** error);
	osync_bool smc_discover(void* data, OSyncPluginInfo* info, OSyncError** error);
	void* smc_initialize(OSyncPlugin* plugin, OSyncPluginInfo* info, OSyncError** error);
	void smc_os_connect(void* data, OSyncPluginInfo* info, OSyncContext* ctx);
	void smc_get_changes(void* data, OSyncPluginInfo* info, OSyncContext* ctx);
	void smc_commit_change(void* data, OSyncPluginInfo* info, OSyncContext* ctx, OSyncChange* change);
	void smc_committed_all(void* data, OSyncPluginInfo* info, OSyncContext* ctx);
	void smc_sync_done(void* data, OSyncPluginInfo* info, OSyncContext* ctx);
	void smc_disconnect(void* data, OSyncPluginInfo* info, OSyncContext* ctx);
	void smc_finalize(void* data);
	int get_version(void);
}


#endif
