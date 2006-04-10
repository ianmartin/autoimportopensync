/*
 *  The SyncML client plugin
 *
 * Copyright (C) 2006 Michael Kolmodin
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
#include "SmcChangeFactory.h"
#include "ItemFactory.h"
#include "SmcConfig.h"

/**
* Create an exception describing an error code from SyncManager.
*/
SyncManagerException::SyncManagerException( const int errorArg, const char* hint)
    : error( errorArg )
{
    switch( error ) {
        case ERR_PROTOCOL_ERROR :
            snprintf( msg, sizeof( msg ), "%s: Protocol error (%d)", hint, error );
            break;
        case  ERR_AUTH_NOT_AUTHORIZED :
            snprintf( msg, sizeof( msg ), "%s: Not authorized (%d)", hint, error );
            break;
        case  ERR_AUTH_EXPIRED :
            snprintf( msg, sizeof( msg ), "%s: Authorization expired (%d)", hint, error );
            break;
        case  ERR_NOT_FOUND:
            snprintf( msg, sizeof( msg ), "%s: Not found (%d)", hint, error );
            break;
        case ERR_AUTH_REQUIRED:
            snprintf( msg, sizeof( msg ), "%s: Authorization required (%d)", hint, error );
            break;
        case  ERR_SRV_FAULT:
            snprintf( msg, sizeof( msg ), "%s: Server failure (%d)", hint, error );
            break;
        default:
            snprintf( msg, sizeof( msg ), "%s: Unknown error (%d)", hint, error );
            break;
    }
}

char* SyncManagerException::getMsg()
{
    return msg;
}

/**
* Check SyncManager return value, throw exception if != 0.
*/
static void check_sync_code( int code, char* hint )
    throw( SyncManagerException )
{
    if( code == 0 ){
        LOG.setMessage( L"%s -- OK", hint );
        LOG.debug();
    }
    else
        throw SyncManagerException( code, hint );
}

static SyncManager* createSyncManager( OSyncContext* ctx, char* uri )
    throw( ConfigException )
{
    wchar_t* wcsUri;
    SyncManager* syncManager;

    wcsUri = new_mbsrtowcs( uri );
    SyncManagerFactory factory = SyncManagerFactory();

    syncManager = factory.getSyncManager( wcsUri );
    delete[] wcsUri;
 
    if (syncManager == 0)  
        throw ConfigException( "Cannot create source (bad gconf uri?)" );

    return syncManager;
}

/**
* Initialize the plugin environment, read config data.
*/
extern "C" static void* smc_initialize(OSyncMember *member, OSyncError **error)
{
    char *configData;
    int  configSize;
        
    syncml_env *env = (syncml_env*)  malloc(sizeof( syncml_env ));
    osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env );

    assert(env != NULL);
    memset(env, 0, sizeof( syncml_env ));

    env->member            = member;
    env->overallSyncMode   = (SyncMode)-1;
        
    try{
        if (!osync_member_get_config(member, &configData, &configSize, error)) 
            throw ConfigException( "Unable to get config data" );
        env->config = new SmcConfig( configData, configSize );

        osync_trace(TRACE_EXIT, "%s", __func__);
        return( (void*) env );    
    }
    catch( ConfigException& cfe ){
        osync_error_update(error, "Unable to parse config data: %s", cfe.why );
        free( env );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, cfe.why );
        return NULL;
    }
}

/**
* Set up SyncSource, SyncManager and sync mode, establish connection to server.
*/
extern "C"  void smc_os_connect(OSyncContext *ctx)
{
    int retval;
    syncml_env *env = (syncml_env*) osync_context_get_plugin_data(ctx);

    try{
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env );

        env->syncSource = createSyncSource( env->config->getSyncSourceName() );
        env->syncManager = createSyncManager( ctx, env->config->getUri() );

        retval = env->syncManager->prepareSync( *(env->syncSource) );
        check_sync_code( retval, "Sync server->client, mods, alert" );

        env->overallSyncMode = env->syncSource->getPreferredSyncMode(); 
        if( env->overallSyncMode == SYNC_SLOW ){
            // The two-way, slow sync is done as two separate sync's, one
            // now and one in batch_commit().
            env->syncSource->setSyncMode( SYNC_ONE_WAY_FROM_SERVER );
            osync_member_set_slow_sync(env->member, 
                                    env->config->getObjType(), 
                                    TRUE);
        }
        osync_context_report_success( ctx );
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch( ConfigException cfe ){
        osync_context_report_error(ctx,
                                   OSYNC_ERROR_MISCONFIGURATION,
                                   cfe.why );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, cfe.why );
    }
    catch( SyncManagerException sme ) {
        osync_context_report_error( ctx, OSYNC_ERROR_GENERIC, sme.getMsg());
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sme.getMsg()  );
        free( env );
    }
}

/**
*
*  Get changes from remote server. This completes the SyncML session
*  initiated in smc_connect().
*
*/
extern "C" void smc_get_changeinfo( OSyncContext *ctx )
{
    int retval;
 
    try{
        syncml_env* env = (syncml_env*) osync_context_get_plugin_data( ctx );
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
        retval = env->syncManager->sync( *(env->syncSource) );
        check_sync_code( retval, "Sync server->client, mods" );
        
        env->lastAnchor = env->syncSource->getLastAnchor( );
        LOG.setMessage( L"getChanges, lastAnchor: %s",  env->lastAnchor );
        LOG.debug();

        SmcChangeFactory* changeFactory = new SmcChangeFactory( env->member );

        env->syncSource->reportChanges( ctx, changeFactory );

        retval = env->syncManager->endSync( *(env->syncSource) );
        check_sync_code( retval, "Sync server->client, mapping" );

        // Update anchors for next sync, but dont store them now. We
        // will not commit the complete transaction until disconnect().
        env->syncSource->updateAnchors();

        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch( SyncManagerException sme ){
        osync_context_report_error( ctx, OSYNC_ERROR_GENERIC, sme.getMsg() );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sme.getMsg() );
    }
         
}

/**
* Accept and store a change, but don't send it to remote side.
*/
extern "C" osync_bool smc_commit_change( OSyncContext* ctx, OSyncChange* change)
{
    try{
        osync_trace(TRACE_ENTRY, "%s", __func__);
        syncml_env* env = (syncml_env*) osync_context_get_plugin_data( ctx );
        ItemFactory itemFactory( env->config->getMimeType() );

        env->syncSource->commitChange( &itemFactory, change );
        osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
    }
    catch( SyncSourceException sse ){
        osync_context_report_error( ctx, OSYNC_ERROR_GENERIC, sse.getMsg() );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sse.getMsg() );
        return FALSE;
    }        
}

/**
* Send all stored changes to remote side. This is another, complete
* SyncML session.
*/
extern "C" void smc_committed_all( OSyncContext* ctx)
{
    int retval; 
    try{ 
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
        syncml_env* env = (syncml_env*) osync_context_get_plugin_data( ctx );

        retval = env->syncManager->prepareSync( *(env->syncSource) );
        check_sync_code( retval, "Sync client->server, alert" );

        env->syncSource->prepareForSync();

        if( env->overallSyncMode == SYNC_SLOW )
            env->syncSource->setSyncMode( SYNC_ONE_WAY_FROM_CLIENT );

        retval = env->syncManager->sync( *(env->syncSource) );
        check_sync_code( retval, "Sync client ->server, mods" );

        retval = env->syncManager->endSync( *(env->syncSource) );
        check_sync_code( retval, "Sync client ->server, mapping" );

        osync_context_report_success( ctx );
        osync_trace(TRACE_EXIT, "%s", __func__);
    }
    catch( SyncManagerException sme ){
        osync_context_report_error( ctx, OSYNC_ERROR_GENERIC, sme.getMsg() );
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, sme.getMsg() );
    }
}

/**
* Commit updated anchors.
*/
extern "C"  void smc_sync_done(OSyncContext *ctx)
{
    syncml_env *env = (syncml_env*) osync_context_get_plugin_data(ctx);
    osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env );

    env->lastAnchor = env->syncSource->getNextAnchor() ;
    osync_anchor_update(env->member, "lanchor", env->lastAnchor );

    osync_context_report_success(ctx);
    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
* This is a noop.
*/
extern "C" void smc_disconnect(OSyncContext *ctx)
{
    syncml_env *env = (syncml_env*) osync_context_get_plugin_data( ctx );
    osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env );

    osync_context_report_success(ctx);
    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
*  Delete all memory allocated by smc_initialize() in  the plugin env.
*/
static void smc_finalize(void *data)
{
    syncml_env* env = (syncml_env*) data;
    osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env );
        
    delete env->syncSource;
    delete env->syncManager;
    delete[] env->lastAnchor;
    delete env->config;

    osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
* Register this plugin as a handler for a given object type.
*/
static void registerObjectType( OSyncPluginInfo*   info,
                                ObjectTypeConfig*  config )
{
    osync_plugin_accept_objtype(info, config->objType );
    osync_plugin_accept_objformat(info, 
                                  config->objType,
                                  config->objFormat, 
                                  NULL );
    osync_plugin_set_commit_objformat( info, 
                                       config->objType, 
                                       config->objFormat, 
                                       smc_commit_change );
    osync_plugin_set_committed_all_objformat (info, 
                                              config->objType,
                                              config->objFormat,
                                              smc_committed_all );
}

/**
* Return the overall description of this plugin to opensync engine.
*/
extern "C" void get_info(OSyncEnv *env )
{
    osync_trace(TRACE_ENTRY, "%s", __func__ );

    OSyncPluginInfo *info = osync_plugin_new_info(env);
        
    info->name        = "syncml-client";
    info->longname    = "SyncML client, syncs against SyncML servers";
    info->description = "This opensync plugin syncs data against syncml \
                         servers such as http://www.scheduleworld.com ";
    info->version = 1;
    info->is_threadsafe = TRUE;
    
    //Now set the function we made earlier
    info->functions.initialize     = smc_initialize;
    info->functions.connect        = smc_os_connect;
    info->functions.sync_done      = smc_sync_done;
    info->functions.disconnect     = smc_disconnect;
    info->functions.finalize       = smc_finalize;
    info->functions.get_changeinfo = smc_get_changeinfo;
    
    info->timeouts.connect_timeout = 300;

    ObjectTypeConfig config;
    for( config.readFirst(); config.hasNext(); config.readNext() )
        registerObjectType( info, &config );
    
    osync_trace(TRACE_EXIT, "%s", __func__);
}
