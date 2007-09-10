/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * Copyright (C) 2007  Graham R. Cobb <g+opensync@cobb.uk.net>
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

#include "gpe_sync.h"

/*! \brief Connects to the databases of GPE
 *
 * \param ctx		The context of the plugin
 */
static void gpe_connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);

	// We need to get the context to load all our stuff.
	gpe_environment *env = ((sink_environment *)userdata)->gpe_env;
	
	// Do nothing if already connected
	if (env->client) {
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: already connected", __func__);
	} else {
	  char *client_err;
	  if (env->use_local) {
	    env->client = gpesync_client_open_local(env->command, &client_err);
	  }
	  else if (env->use_ssh)
	    {
	      gchar *path = g_strdup_printf ("%s@%s", env->username, env->device_addr);
	      env->client = gpesync_client_open_ssh (path, env->command, &client_err);
	    }
	  else
	    env->client = gpesync_client_open (env->device_addr, env->device_port, &client_err);

	  if (env->client == NULL) {
	    osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, client_err);
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: connect failed: %s", __func__, client_err);
	    return;
	  }
	}
	
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

/*! \brief This is called once all objects have been sent to the plugin
 *
 * \param ctx		The context of the plugin
 */
static void sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);
	gpe_environment *env = ((sink_environment *)userdata)->gpe_env;

        //If we use anchors we have to update it now.
	
	//Answer the call
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

/*! \brief Closes the connection to the databases
 *
 * \brief ctx		The context of the plugin
 */
static void gpe_disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);
	gpe_environment *env = ((sink_environment *)userdata)->gpe_env;
	
	if (env->client) {
		gpesync_client_close (env->client);
		env->client = NULL;
	}

	//Answer the call
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

static void free_sink(sink_environment *sinkenv)
{
  if (sinkenv->sink) osync_objtype_sink_unref(sinkenv->sink);
  if (sinkenv->hashtable) osync_hashtable_free(sinkenv->hashtable);
}


/*! \brief The counterpart to initialize
 * 
 * \param data		The data of the plugin (configuration, etc.)
 */
static void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p)", __func__, data);
	gpe_environment *env = (gpe_environment *)data;
	
	//Free all stuff that you have allocated here.
	g_free(env->username);
	g_free(env->device_addr);

	if (env->client)
	  gpesync_client_close (env->client);
	
	free_sink(&env->contact_sink);
	free_sink(&env->todo_sink);
	free_sink(&env->calendar_sink);

	g_free(env);
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

/*! \brief Initializes the plugin (needed for opensync)
 *
 * \param member	The member of the sync pair
 * \param error		If an error occurs it will be stored here
 */
static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	const char *configdata = NULL;

	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, plugin, info, error);

        gpe_environment *env = osync_try_malloc0(sizeof(gpe_environment), error);
        if (!env)
                goto error;

        //now you can get the config file for this plugin
        configdata = osync_plugin_info_get_config(info);
        if (!configdata) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get config data.");
                goto error_free_env;
        }

	osync_trace(TRACE_INTERNAL, "GPE-SYNC configdata: %s", configdata);
	
	if (!gpe_parse_settings(env, configdata)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for gpe plugin");
		goto error_free_env;
	}

	// Here we set some default values
	//env->member = member;
	env->client = NULL;

	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = gpe_connect;
	functions.disconnect = gpe_disconnect;
	functions.sync_done = sync_done;

	// Set up contact sink
	if (!gpe_contacts_setup(&env->contact_sink, functions, env, info, error))
	  goto error_free_env;
	
	// Set up calendar sink
	if (!gpe_calendar_setup(&env->calendar_sink, functions, env, info, error))
	  goto error_free_env;
	
	// Set up todo sink
	if (!gpe_todo_setup(&env->todo_sink, functions, env, info, error))
	  goto error_free_env;

	// All done
	osync_trace(TRACE_EXIT, "GPE-SYNC %s: %p", __func__, env);
	return (void *)env;

error_free_env:
        finalize(env);
error:
        osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: %s", __func__, osync_error_print(error));
        return NULL;
}

/* Here we actually tell opensync which sinks are available. */
static osync_bool discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, error);

	gpe_environment *env = (gpe_environment *)userdata;

        // Report avaliable sinks...
	// FIXME: one day this should connect to the device first and check its capabilities
	if (env->contact_sink.sink) osync_objtype_sink_set_available(env->contact_sink.sink, TRUE);
	if (env->todo_sink.sink) osync_objtype_sink_set_available(env->todo_sink.sink, TRUE);
	if (env->calendar_sink.sink) osync_objtype_sink_set_available(env->calendar_sink.sink, TRUE);

	// FIXME: should get version info from the device
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "gpe-sync");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

        osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
        return TRUE;
}

/*! \brief This function has to be in every opensync plugin
 *
 * \brief env		The environment of the plugin containing basic
 * 			information about the plugin.
 */
osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p)", __func__, env, error);
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
	  goto error;
	
	//Tell opensync something about your plugin
	//Tell opensync something about your plugin
	osync_plugin_set_name(plugin, "gpe-sync");
	osync_plugin_set_longname(plugin, "Provides synchronisation with handhelds using GPE.");
	osync_plugin_set_description(plugin, "See http://gpe.handhelds.org for more information");

	//Now set the function we made earlier
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	//If you like, you can overwrite the default timeouts of your plugin
	//The default is set to 60 sec. Note that this MUST NOT be used to
	//wait for expected timeouts (Lets say while waiting for a webserver).
	//you should wait for the normal timeout and return a error.
	//info->timeouts.connect_timeout = 5;
	
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
        return TRUE;
 error:
	osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: Unable to register: %s", __func__, osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
  return 1;
}
