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

/*! \brief Closes the connection to the databases
 *
 * \brief ctx		The context of the plugin
 */
static void gpe_disconnect_internal(gpe_environment *env)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p)", __func__, env);
	
	if (env->client) {
		gpesync_client_close (env->client);
		env->client = NULL;
	}

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}
static void gpe_disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);
	gpe_environment *env = ((sink_environment *)userdata)->gpe_env;
	
	gpe_disconnect_internal(env);

	//Answer the call
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

/*! \brief Connects to the databases of GPE
 *
 * \param ctx		The context of the plugin
 */
static void gpe_connect_internal(gpe_environment *env, char **client_err)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p)", __func__, env, client_err);

	// Do nothing if already connected
	if (env->client) {
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: already connected", __func__);
	} else {
	  if (env->use_local) {
	    env->client = gpesync_client_open_local(env->command, client_err);
	  }
	  else if (env->use_ssh)
	    {
	      gchar *path = g_strdup_printf ("%s@%s", env->username, env->device_addr);
	      env->client = gpesync_client_open_ssh (path, env->command, client_err);
	    }
	  else
	    env->client = gpesync_client_open (env->device_addr, env->device_port, client_err);
	}

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}
static void gpe_connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	char *client_err = NULL;

	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);

	// We need to get the context to load all our stuff.
	gpe_environment *env = ((sink_environment *)userdata)->gpe_env;
	
	gpe_connect_internal(env, &client_err);

	if (env->client == NULL) {
	  osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, client_err);
	  osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: connect failed: %s", __func__, client_err);
	  if (client_err) g_free(client_err);
	  return;
	}
	
	// Set calendar name -- note that we checked that the version of gpesyncd supports
	// setting the calendar name in the discovery phase
	if (env->calendar) {
	  gchar *response = NULL;
	  gpesync_client_exec_printf(env->client, "path vevent %s", client_callback_string, &response, NULL, env->calendar);
	  if (strncmp(response, "OK", 2) != 0) {
	    osync_context_report_error(ctx, OSYNC_ERROR_MISCONFIGURATION, "calendar %s not found", 
			  env->calendar);
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: calendar %s not found", 
		      __func__, env->calendar);
	    gpe_disconnect_internal(env);
	    g_free(response);
	    return;
	  }
	  g_free(response);
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
	//gpe_environment *env = ((sink_environment *)userdata)->gpe_env;

        //If we use anchors we have to update it now.
	
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
	free_sink(&env->main_sink);

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

	env->client = NULL;

	// Create the main sink (which handles connect & disconnect)
	env->main_sink.sink = osync_objtype_main_sink_new(error);
	if (!env->main_sink.sink) goto error_free_env;

	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = gpe_connect;
	functions.disconnect = gpe_disconnect;
	functions.sync_done = sync_done;
	osync_objtype_sink_set_functions(env->main_sink.sink, functions, &env->main_sink);

	osync_plugin_info_set_main_sink(info, env->main_sink.sink);
	env->main_sink.gpe_env = env;

	// Set up contact sink
	if (!gpe_contacts_setup(&env->contact_sink, env, info, error))
	  goto error_free_env;
	
	// Set up calendar sink
	if (!gpe_calendar_setup(&env->calendar_sink, env, info, error))
	  goto error_free_env;
	
	// Set up todo sink
	if (!gpe_todo_setup(&env->todo_sink, env, info, error))
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
	gchar *err_string = NULL;
	gchar *response = NULL;
	unsigned int v_major = 1, v_minor = 0, v_edit = 0; // Default version 1.0.0

        osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, error);

	gpe_environment *env = (gpe_environment *)userdata;

	// Try to connect
	gpe_connect_internal(env, &err_string);

	if (env->client == NULL) {
	  osync_error_set(error, OSYNC_ERROR_NO_CONNECTION, err_string);
	  osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: connect failed: %s", __func__, err_string);
	  if (err_string) g_free(err_string);
	  return FALSE;
	}

	// Get version info from the device
	gpesync_client_exec(env->client, "version", client_callback_string, &response, NULL);

	if (sscanf(response, "OK:%u:%u:%u", &v_major, &v_minor, &v_edit) != 3) {
	  osync_trace(TRACE_INTERNAL, "version command error: %s", response);
	}
	osync_trace(TRACE_INTERNAL, "gpesyncd version = %d.%d.%d", v_major, v_minor, v_edit);

	if (v_major != MIN_PROTOCOL_MAJOR || v_minor < MIN_PROTOCOL_MINOR) {
	  osync_error_set(error, OSYNC_ERROR_NOT_SUPPORTED, "gpesyncd version %d.%d not supported -- require %d.%d", 
			  v_major, v_minor, MIN_PROTOCOL_MAJOR, MIN_PROTOCOL_MINOR);
	  osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: gpesyncd version %d.%d not supported -- require %d.%d", 
		      __func__, v_major, v_minor, MIN_PROTOCOL_MAJOR, MIN_PROTOCOL_MINOR);
	  g_free(response);
	  return FALSE;
	}

	gchar *version_string = g_strdup_printf("%d.%d.%d", v_major, v_minor, v_edit);
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, "gpe-sync");
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        osync_version_set_softwareversion(version, version_string);
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);
	g_free(version_string);

	if (env->calendar) {
	  // Calendar support requires at least version 1.2
	  if (v_minor < 2) {
	    osync_error_set(error, OSYNC_ERROR_NOT_SUPPORTED, "gpesyncd version %d.%d does not support <calendar> -- require %d.%d", 
			  v_major, v_minor, MIN_PROTOCOL_MAJOR, 2);
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: gpesyncd version %d.%d does not support <calendar> -- require %d.%d", 
		      __func__, v_major, v_minor, MIN_PROTOCOL_MAJOR, 2);
	    gpe_disconnect_internal(env);
	    g_free(response);
	    return FALSE;
	  }

	  // Verify calendar name
	  gpesync_client_exec_printf(env->client, "path vevent %s", client_callback_string, &response, NULL, env->calendar);
	  if (strncmp(response, "OK", 2) != 0) {
	    osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "calendar %s not found", 
			  env->calendar);
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: calendar %s not found", 
		      __func__, env->calendar);
	    gpe_disconnect_internal(env);
	    g_free(response);
	  }
	}

        // Report available sinks...
	// GPE always supports contacts, todos and events
	if (env->contact_sink.sink) osync_objtype_sink_set_available(env->contact_sink.sink, TRUE);
	if (env->todo_sink.sink) osync_objtype_sink_set_available(env->todo_sink.sink, TRUE);
	if (env->calendar_sink.sink) osync_objtype_sink_set_available(env->calendar_sink.sink, TRUE);
	// One day we may add notes...

	gpe_disconnect_internal(env);

	g_free(response);
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
