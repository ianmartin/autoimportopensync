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

#include "gnokii_comm.h"
#include "gnokii_config.h"

#include "gnokii_calendar.h"
#include "gnokii_contact.h"

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

	//OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

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

	gnokii_environment *env = NULL;
	OSyncPluginConfig *config = NULL;
	gnokii_sinkenv *contact_sinkenv = NULL;
	gnokii_sinkenv *event_sinkenv = NULL;
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	if (!tablepath) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "No memory left for Plugin initialization");
		goto error;
	}
	
	/* create gnokii_environment which stores config and statemachine for libgnokii */
	env = osync_try_malloc0(sizeof(gnokii_environment), error);
	if (!env)
		goto error;

	env->state = osync_try_malloc0(sizeof(struct gn_statemachine), error);
	if (!env->state)
		goto error;

	config = osync_plugin_info_get_config(info);
	if (!config) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "No Plugin configuration");
		goto error;
	}

	if (!gnokii_config_parse(env, config, error))
		goto error;
	
	/* main sink (objtype neutral) handles only connect and disconnect! */
	env->mainsink = osync_objtype_main_sink_new(error);
	if (!env->mainsink)
		goto error;

	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));

	main_functions.connect = connect;
	main_functions.disconnect = disconnect;

	osync_objtype_sink_set_functions(env->mainsink, main_functions, NULL);
	osync_plugin_info_set_main_sink(info, env->mainsink);
	/* end of main sink */

	/* init the contact sink */
	contact_sinkenv = osync_try_malloc0(sizeof(gnokii_sinkenv), error);
	if (!contact_sinkenv)
		goto error;

	contact_sinkenv->sink = osync_plugin_info_find_objtype(info, "contact");
	if (contact_sinkenv->sink) {
		OSyncObjTypeSinkFunctions contact_functions;
		memset(&contact_functions, 0, sizeof(contact_functions));
		contact_functions.get_changes = gnokii_contact_get_changes;
		contact_functions.commit = gnokii_contact_commit_change;
		contact_functions.sync_done = sync_done;
		/*
		contact_functions.read = osync_filesync_read;
		contact_functions.write = osync_filesync_write;
		*/

		osync_objtype_sink_set_functions(contact_sinkenv->sink, contact_functions, contact_sinkenv);

		contact_sinkenv->objformat = osync_format_env_find_objformat(formatenv, "gnokii-contact");
		if (!contact_sinkenv->objformat) {
			osync_error_set(error, OSYNC_ERROR_PLUGIN_NOT_FOUND, "Format Plugin \"gnokii-contact\" not found.");
			goto error;
		}


		env->sinks = g_list_append(env->sinks, contact_sinkenv);

		contact_sinkenv->hashtable = osync_hashtable_new(tablepath, "contact", error);
		if (!contact_sinkenv->hashtable)
			goto error;

		if (!osync_hashtable_load(contact_sinkenv->hashtable, error))
			goto error;
	} else {
		osync_trace(TRACE_INTERNAL, "Contact sink is disable by configuration.");
	}
	/* end of contact sink */

	/* init the event sink */
	event_sinkenv = osync_try_malloc0(sizeof(gnokii_sinkenv), error);
	if (!event_sinkenv)
		goto error;

	event_sinkenv->sink = osync_plugin_info_find_objtype(info, "event");
	if (event_sinkenv->sink) {
		OSyncObjTypeSinkFunctions event_functions;
		memset(&event_functions, 0, sizeof(event_functions));
		event_functions.get_changes = gnokii_calendar_get_changes;
		event_functions.commit = gnokii_calendar_commit_change;
		event_functions.sync_done = sync_done;
		/*
		event_functions.read = osync_filesync_read;
		event_functions.write = osync_filesync_write;
		*/

		osync_objtype_sink_set_functions(event_sinkenv->sink, event_functions, event_sinkenv);

		event_sinkenv->objformat = osync_format_env_find_objformat(formatenv, "gnokii-event");
		if (!event_sinkenv->objformat) {
			osync_error_set(error, OSYNC_ERROR_PLUGIN_NOT_FOUND, "Format Plugin \"gnokii-event\" not found.");
			goto error;
		}

		event_sinkenv->hashtable = osync_hashtable_new(tablepath, "event", error);

		if (!event_sinkenv->hashtable)
			goto error;

		if (!osync_hashtable_load(event_sinkenv->hashtable, error))
			goto error;

		env->sinks = g_list_append(env->sinks, event_sinkenv);
	} else {
		osync_trace(TRACE_INTERNAL, "Event sink is disable by configuration.");
	}
	/* end of event sink */
	
	/* Free Hashtable path */
	g_free(tablepath);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error:
	if (env)
		free_gnokiienv(env);

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

