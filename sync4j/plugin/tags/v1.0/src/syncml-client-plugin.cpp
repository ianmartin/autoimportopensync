/*
 *  The SyncML client plugin
 *
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

#include "syncml-client-plugin.h"
#include <opensync/opensync-version.h>

/**
* Return the overall description of this plugin to opensync engine.
*/
osync_bool get_sync_info(OSyncPluginEnv* env, OSyncError** error)
{
    osync_trace(TRACE_ENTRY, "%s", __func__ );
        
    osync_bool success = true;
    OSyncPlugin* plugin = osync_plugin_new(error);
    
    if(plugin) 
    {
    	osync_plugin_set_name(plugin, "syncml-client");
    	osync_plugin_set_longname(plugin, "SyncML client, syncs against Funambol servers");
    	osync_plugin_set_description(plugin, "This opensync plugin syncs data against syncml \
                					servers such as http://www.scheduleworld.com ");
        
    	osync_plugin_set_initialize(plugin, smc_initialize);
    	osync_plugin_set_finalize(plugin, smc_finalize);
    	osync_plugin_set_discover(plugin, smc_discover);
    	
    	osync_plugin_env_register_plugin(env, plugin);
    	osync_plugin_unref(plugin);    
    }
    else 
    {
    	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
    	osync_error_unref(error);
    	success = false;
    }
        
    osync_trace(TRACE_EXIT, "%s", __func__);
    
    return success;
}

/* The discover function of a plugin can be used to specify which of the 
 * sinks in the plugin are currently available, and to declare the compatible
 * device versions for the plugin. It can also be used to set the plugin's 
 * capabilities.
 * 
 * The discover function is optional.
 */
osync_bool smc_discover(void* data, OSyncPluginInfo* info, OSyncError** error) {
	LOG.debug("Begin smc_discover()");
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	Sync4jClient* client = (Sync4jClient*) data;
	vector<SmcSyncSource>& sources = client->getSources();
	vector<SmcSyncSource>::iterator iter;
	
	for(iter = sources.begin(); iter != sources.end(); iter++) {
		iter->setSinkAvailable();
	}
	
	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "syncml-client");
	
	//version of the used sync4j SDK
	osync_version_set_softwareversion(version, "6.0.8");
	
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	osync_trace(TRACE_EXIT, "%s", __func__);
	LOG.debug("End smc_discover()");
	
	return TRUE;
}

/**
* Initialize the plugin environment. 
* 
*/
void* smc_initialize(OSyncPlugin* plugin, OSyncPluginInfo* info, OSyncError **error)
{
	Log(0, LOGDIR, LOGFILE);
	LOG.reset(LOGFILE);
	LOG.setLevel(LOGLEVEL);
	
	
	LOG.debug("Processing smc_initialize()");
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	
	try {
		Sync4jClient* client = new Sync4jClient(osync_plugin_info_get_configdir(info));
				
		OSyncFormatEnv* formatenv = osync_plugin_info_get_format_env(info);
		
		vector<SmcSyncSource>& sources = client->getSources();
		vector<SmcSyncSource>::iterator iter;
		for(iter = sources.begin(); iter != sources.end(); iter++) {
			OSyncObjFormat* objformat = osync_format_env_find_objformat(formatenv, iter->getObjectFormat());
			if(!objformat) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Configured storage " \
						"format %s for object type %s is unknown. Is the format " \
						"plugin missing?", iter->getObjectFormat(), iter->getName());
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
				delete client;
				return NULL;
			}
			else {
				OSyncObjTypeSink* sink = osync_objtype_sink_new(iter->getName(), error);
				if(!sink) {
					osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
					delete client;
					return NULL;
				}
				else {
					osync_objtype_sink_add_objformat(sink, iter->getObjectFormat());
					
					OSyncObjTypeSinkFunctions functions;
					memset(&functions, 0, sizeof(functions));
					
					functions.connect = smc_os_connect;
					functions.disconnect = smc_disconnect;
					functions.get_changes = smc_get_changes;
					functions.commit = smc_commit_change;
					functions.committed_all = smc_committed_all;
					functions.sync_done = smc_sync_done;
					
					/* the third parameter is for userdata, see datasource.cpp
					 * in kdepim plugin 
					 */ 
					osync_objtype_sink_set_functions(sink, functions, NULL);
					osync_plugin_info_add_objtype(info, sink);

					iter->setOsyncEnvironment(sink, objformat);
					
					// transfering ownership to SmcSyncSource
					osync_objtype_sink_unref(sink);
				}
			}
		}	
		
				
		osync_trace(TRACE_EXIT, "%s", __func__);
		LOG.debug("End smc_initialize()");
		return( (void*) client );
	}
	catch(Sync4jClientException sce)
	{
		LOG.error(sce.getMsg());
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		LOG.debug("End smc_initialize()");
		return NULL;
	}
}



/**
* NOOP, since the client and the sources are already created, connection is
* setup just before the sync.
*/
void smc_os_connect(void* data, OSyncPluginInfo* info, OSyncContext* ctx)
{
    osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
    
    const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
    LOG.debug("Processing smc_os_connect() for source '%s'", currentSink);
    osync_context_report_success(ctx);
    LOG.debug("End smc_os_connect() for source '%s'", currentSink);
    
    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
* Do the sync to get changes FROM the server. 
*/
void smc_get_changes(void* data, OSyncPluginInfo* info, OSyncContext* ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	Sync4jClient* client = (Sync4jClient*) data;
	const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
	LOG.debug("Processing smc_get_changeinfo() for source '%s'", currentSink);
	
    try {               
        client->prepareFromServerSync();
        client->sync();
        client->endFromServerSync(ctx, currentSink);
                
        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch(Sync4jClientException sce) {
    	LOG.error("%s", sce.getMsg());
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, sce.getMsg());
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sce.getMsg());
    }
        
    LOG.debug("End smc_get_changeinfo() for source '%s'", currentSink);
}

/**
* Accept and store a change, but don't send it to remote side.
*/
void smc_commit_change(void* data, OSyncPluginInfo* info, OSyncContext* ctx, OSyncChange* change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
	
	Sync4jClient* client = (Sync4jClient*) data;
	const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
	LOG.debug("Processing smc_commit_change() for source '%s'", currentSink);
	
    try {
        client->addChange(change, currentSink);
               
        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch(SyncSourceException sse) {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, sse.getMsg());
        LOG.error("Error in commiting change to sync4j: %s", sse.getMsg());
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sse.getMsg() );
    }
	
	LOG.debug("End smc_commit_change() for source '%s'", currentSink);
}

/**
* Send all stored changes to remote side. This is another, complete
* SyncML session.
*/
void smc_committed_all(void* data, OSyncPluginInfo* info, OSyncContext* ctx)
{
    osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
    
    Sync4jClient* client = (Sync4jClient*) data;
    const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
    LOG.debug("Processing smc_committed_all() for source '%s'", currentSink);
    
    try { 
        client->prepareToServerSync(currentSink);
        client->sync();
                
        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch( Sync4jClientException sce ) {
        osync_context_report_error( ctx, OSYNC_ERROR_GENERIC, sce.getMsg() );
        LOG.error( "%s", sce.getMsg() );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sce.getMsg() );
    }
    
    LOG.debug("End smc_committed_all() for source '%s'", currentSink);
}



/**
* Update opensync anchors for each object type.
*/
void smc_sync_done(void* data, OSyncPluginInfo* info, OSyncContext* ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	
	Sync4jClient* client = (Sync4jClient*) data;
	const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
	LOG.debug("Processing smc_sync_done() for source '%s'", currentSink);
    	
    client->updateOSyncAnchor(currentSink);
    
    osync_context_report_success(ctx);
    LOG.debug("End smc_sync_done() for source '%s'", currentSink);
    
    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
* This is a noop.
*/
void smc_disconnect(void* data, OSyncPluginInfo* info, OSyncContext* ctx)
{
    osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
    
    const char* currentSink = osync_objtype_sink_get_name(osync_plugin_info_get_sink(info));
    LOG.debug("Processing smc_disconnect() for source '%s'", currentSink);
    osync_context_report_success(ctx);
    LOG.debug("End smc_disconnect() for source '%s'", currentSink);
    
    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
* Delete all memory allocated by smc_initialize() in  the plugin env and save
* the configuration.
*/
void smc_finalize(void* data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	LOG.debug("Processing smc_finalize()");
	
	Sync4jClient* client = (Sync4jClient*) data;
	client->saveConfig();
    delete client;
    
    LOG.debug("End smc_finalize()");
    osync_trace(TRACE_EXIT, "%s", __func__);
}

int get_version(void)
{
	return 1;
}
