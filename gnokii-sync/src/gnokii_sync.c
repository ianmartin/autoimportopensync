/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"

static void free_gnokiienv(gnokii_environment *env) { 
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	while (env->sinks) {
		gnokii_sinkenv *sinkenv = env->sinks->data;

		// close the hashtable
		if (sinkenv->hashtable) {
			osync_hashtable_unref(sinkenv->hashtable);
		}

		osync_objtype_sink_unref(sinkenv->sink);
		g_free(sinkenv);

		env->sinks = g_list_remove(env->sinks, sinkenv);
	}

	if (env->sinks)
		g_free(env->sinks);

	if (env->mainsink)
		osync_objtype_sink_unref(env->mainsink);


	if (env->state)
		g_free(env->state);

	g_free(env);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncError *error = NULL;
	
	gnokii_environment *env = (gnokii_environment *) data;

	// connect to cellphone
	if (!gnokii_comm_connect(env->state)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Connection failed");
		goto error;
	}

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	OSyncError *error = NULL;

	//gnokii_environment *env = (gnokii_environment *) data;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        gnokii_sinkenv *sinkenv = osync_objtype_sink_get_userdata(sink);

	// commit changes to persistent hahstable 
	if (!osync_hashtable_save(sinkenv->hashtable, &error))
		goto error;
	
	// answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        gnokii_sinkenv *sinkenv = osync_objtype_sink_get_userdata(sink);

	gnokii_environment *env = (gnokii_environment *) data;
	
	// disconnect the connection with phone
	if (!gnokii_comm_disconnect(env->state)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "disconnect failed");
                return;
	}
	
	// answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

	gnokii_environment *env = (gnokii_environment *)data;

	// free everything
	free_gnokiienv(env);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	gnokii_environment *env = (gnokii_environment *)data;

	// Set available sinks
	// TODO: check configured driver and support of sinks:

	GList *s = NULL;
	for (s = env->sinks; s; s = s->next) {
		gnokii_sinkenv *sinkenv = s->data;
		osync_objtype_sink_set_available(sinkenv->sink, TRUE);
	}

	
	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "gnokii-sync");
	//osync_version_set_modelversion(version, "version");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}


static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);

	char *configdata = NULL;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info); 

	// create gnokii_environment which stores config and statemachine for libgnokii
	gnokii_environment *env = osync_try_malloc0(sizeof(gnokii_environment), error);
	if (!env)
		return NULL;

	env->sinks = NULL;

	env->state = osync_try_malloc0(sizeof(struct gn_statemachine), error);
	if (!env->state) {
		free_gnokiienv(env);
		return NULL;
	}

	// parse the member configuration
	if (!gnokii_config_parse(env, osync_plugin_info_get_config(info), error)) {
		free_gnokiienv(env);
		return NULL;
	}
	
	// main sink - objtype neutral, handles (only) connect and disconnect!
	env->mainsink = osync_objtype_main_sink_new(error);

	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));

	main_functions.connect = connect;
	main_functions.disconnect = disconnect;

	osync_objtype_sink_set_functions(env->mainsink, main_functions, NULL);
	osync_plugin_info_set_main_sink(info, env->mainsink);


	// init the contact sink
	gnokii_sinkenv *contact_sinkenv = osync_try_malloc0(sizeof(gnokii_sinkenv), error); 
	contact_sinkenv->sink = osync_objtype_sink_new("contact", error);
	osync_objtype_sink_add_objformat(contact_sinkenv->sink, "gnokii-contact");

	OSyncObjTypeSinkFunctions contact_functions;
	memset(&contact_functions, 0, sizeof(contact_functions));
	contact_functions.get_changes = gnokii_contact_get_changes;
	contact_functions.commit = gnokii_contact_commit_change;
//	contact_functions.read = osync_filesync_read;
//	contact_functions.write = osync_filesync_write;
	contact_functions.sync_done = sync_done;

	osync_objtype_sink_set_functions(contact_sinkenv->sink, contact_functions, contact_sinkenv);
	osync_plugin_info_add_objtype(info, contact_sinkenv->sink);

        contact_sinkenv->objformat = osync_format_env_find_objformat(formatenv, "gnokii-contact");
	osync_trace(TRACE_INTERNAL, "contact_sinkenv->objformat: %p", contact_sinkenv->objformat);

	env->sinks = g_list_append(env->sinks, contact_sinkenv);

	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	contact_sinkenv->hashtable = osync_hashtable_new(tablepath, "contact", error);
	
	if (!contact_sinkenv->hashtable)
		goto error;

	if (!osync_hashtable_load(contact_sinkenv->hashtable, error))
		goto error;

	// init the event sink
	gnokii_sinkenv *event_sinkenv = osync_try_malloc0(sizeof(gnokii_sinkenv), error); 
	event_sinkenv->sink = osync_objtype_sink_new("event", error);
	osync_objtype_sink_add_objformat(event_sinkenv->sink, "gnokii-event");

	OSyncObjTypeSinkFunctions event_functions;
	memset(&event_functions, 0, sizeof(event_functions));
	event_functions.get_changes = gnokii_calendar_get_changes;
	event_functions.commit = gnokii_calendar_commit_change;
//	event_functions.read = osync_filesync_read;
//	event_functions.write = osync_filesync_write;
	event_functions.sync_done = sync_done;

	osync_objtype_sink_set_functions(event_sinkenv->sink, event_functions, event_sinkenv);
	osync_plugin_info_add_objtype(info, event_sinkenv->sink);

        event_sinkenv->objformat = osync_format_env_find_objformat(formatenv, "gnokii-event");
	event_sinkenv->hashtable = osync_hashtable_new(tablepath, "event", error);

	if (!event_sinkenv->hashtable)
		goto error;

	if (!osync_hashtable_load(event_sinkenv->hashtable, error))
		goto error;

	env->sinks = g_list_append(env->sinks, event_sinkenv);
	
	g_free(tablepath);

	//Process the config data here and set the options on your environment
	if (configdata)
		g_free(configdata);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);

	return (void *)env;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}


osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "gnokii-sync");
	osync_plugin_set_longname(plugin, "Nokia (gnokii) Mobile Device");
	osync_plugin_set_description(plugin, "Synchronize with Nokia cellphones (FBUS)");
	
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);
	
	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);
	
	return TRUE;
	
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}

