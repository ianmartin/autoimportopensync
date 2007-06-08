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
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Connection failed");
		free_gnokiienv(env);
		return;
	}

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        gnokii_sinkenv *sinkenv = osync_objtype_sink_get_userdata(sink);

	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	sinkenv->hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), &error);
	g_free(tablepath);
	
	if (!sinkenv->hashtable)
		goto error;

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

//	gnokii_environment *env = (gnokii_environment *) data;
	
	// forget reported changes
	// osync_hashtable_forget(env->hashtable);
	
	// answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
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
		free_gnokiienv(env);
                return;
	}
	
	// close the hashtable
	osync_hashtable_free(sinkenv->hashtable);

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
		OSyncObjTypeSink *sink = s->data;
		osync_objtype_sink_set_available(sink, TRUE);
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
	
	// create gnokii_environment which stores config and statemachine for libgnokii
	gnokii_environment *env = malloc(sizeof(gnokii_environment));
	g_assert(env != NULL);
	memset(env, 0, sizeof(gnokii_environment));

	env->sinks = NULL;

	env->state = (struct gn_statemachine *) malloc(sizeof(struct gn_statemachine));
	g_assert(env->state != NULL);
	memset(env->state, 0, sizeof(struct gn_statemachine));

	if (!gnokii_config_parse(env->state, osync_plugin_info_get_config(info), error)) {
		free_gnokiienv(env);
		return NULL;
	}
	
	// init the contact sink
	OSyncObjTypeSink *contact_sink = NULL; 
	contact_sink = osync_objtype_sink_new("contact", error);
	osync_objtype_sink_add_objformat(contact_sink, "gnokii-contact");

	OSyncObjTypeSinkFunctions contact_functions;
	memset(&contact_functions, 0, sizeof(contact_functions));
	contact_functions.connect = connect;
	contact_functions.disconnect = disconnect;
	contact_functions.get_changes = gnokii_contact_get_changes;
	contact_functions.commit = gnokii_contact_commit_change;
//	contact_functions.read = osync_filesync_read;
//	contact_functions.write = osync_filesync_write;
	contact_functions.sync_done = sync_done;

	osync_objtype_sink_set_functions(contact_sink, contact_functions, env);
	osync_plugin_info_add_objtype(info, contact_sink);

	env->sinks = g_list_append(env->sinks, contact_sink);


	// init the event sink
	OSyncObjTypeSink *event_sink = osync_objtype_sink_new("event", error);
	osync_objtype_sink_add_objformat(event_sink, "gnokii-event");

	OSyncObjTypeSinkFunctions event_functions;
	memset(&event_functions, 0, sizeof(event_functions));
	event_functions.connect = connect;
	event_functions.disconnect = disconnect;
	event_functions.get_changes = gnokii_calendar_get_changes;
	event_functions.commit = gnokii_calendar_commit_change;
//	event_functions.read = osync_filesync_read;
//	event_functions.write = osync_filesync_write;
	event_functions.sync_done = sync_done;

	osync_objtype_sink_set_functions(event_sink, event_functions, env);
	osync_plugin_info_add_objtype(info, event_sink);

	env->sinks = g_list_append(env->sinks, event_sink);

	
	//Process the config data here and set the options on your environment
	if (configdata)
		g_free(configdata);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);

	return (void *)env;
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

