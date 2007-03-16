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

#include <opensync/opensync.h>

static void free_gnokiienv(gnokii_environment *env) { 
	osync_trace(TRACE_ENTRY, "%s()", __func__);

	if (env->config)
		g_free(env->config);

	if (env->state)
		g_free(env->state);

	g_free(env);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void *initialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);

	char *configdata = NULL;
	int configsize;
	
	// create gnokii_environment which stores config and statemachine for libgnokii
	gnokii_environment *env = malloc(sizeof(gnokii_environment));
	g_assert(env != NULL);
	memset(env, 0, sizeof(gnokii_environment));

	env->config = malloc(sizeof(gn_config));
	g_assert(env->config != NULL);
	memset(env->config, 0, sizeof(gn_config));

	env->state = (struct gn_statemachine *) malloc(sizeof(struct gn_statemachine));
	g_assert(env->state != NULL);
	memset(env->state, 0, sizeof(struct gn_statemachine));


	// now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		free_gnokiienv(env);
		return NULL;
	}
	
	if (!gnokii_config_parse(env->config, configdata, configsize, error)) {
		free_gnokiienv(env);
		return NULL;
	}
	
	// fill state structure with connection settings required by libgnokii
	gnokii_config_state(env->state, env->config);
	
	//Process the config data here and set the options on your environment
	if (configdata)
		g_free(configdata);
	env->member = member;
	
	// create the hashtable
	env->hashtable = osync_hashtable_new();
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);

	return (void *)env;
}

static void connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	
	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	// connect to cellphone
	if (!gnokii_comm_connect(env->state)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Connection failed");
		free_gnokiienv(env);
		return;
	}

	// load hashtable
	OSyncError *error = NULL;
	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}
	
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	osync_bool calendar_changes = TRUE; 
//	osync_bool todo_changes = TRUE;
	osync_bool contact_changes = TRUE;
	
#ifdef HAVE_EVENT	
	// get changes of events (calendar)
	if (osync_member_objtype_enabled(env->member, "event"))
		calendar_changes = gnokii_calendar_get_changeinfo(ctx);
#endif	

#ifdef HAVE_CONTACT	
	// get changes of contacts
	if (osync_member_objtype_enabled(env->member, "contact"))
		contact_changes = gnokii_contact_get_changeinfo(ctx);
#endif	
	
//	TODO todo
//	if (calendar_changes && todo_changes && contact_changes)
	if (calendar_changes && contact_changes)
		osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void sync_done(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);
	
	// forget reported changes
	osync_hashtable_forget(env->hashtable);
	
	// answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	
	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);
	
	// disconnect the connection with phone
	if (!gnokii_comm_disconnect(env->state)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "disconnect failed");
		free_gnokiienv(env);
                return;
	}
	
	// close the hashtable
	osync_hashtable_close(env->hashtable);

	// answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

	gnokii_environment *env = (gnokii_environment *)data;

	// close and free hashtable
	osync_hashtable_close(env->hashtable);
	osync_hashtable_free(env->hashtable);

	// free everything
	free_gnokiienv(env);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
	// create new plugin
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	//Tell opensync something about (y)our plugin
	info->name = "gnokii-sync";
	info->longname = "Nokia (gnokii) Mobile Device";
	info->description = "Sync with Nokia cellphones (FBUS)";
	//the version of the api we are using, (1 at the moment)
	info->version = 1;
//	info->is_threadsafe = TRUE;
	
	//Now set the function we made earlier
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	
	//If you like, you can overwrite the default timeouts of your plugin
	//The default is set to 60 sec. Note that this MUST NOT be used to
	//wait for expected timeouts (Lets say while waiting for a webserver).
	//you should wait for the normal timeout and return a error.
	info->timeouts.connect_timeout = 10;
	
	//Communicating via Gnokii can take lots of time, especially for
	//big calendars, so give it some time to complete. The rationale is
	//here that once we're connected we know that the communication is
	//working and therefore timeouts shouldn't be necessary in many cases.
	info->timeouts.sync_done_timeout = 10000;
	info->timeouts.disconnect_timeout = 10000;
	info->timeouts.get_changeinfo_timeout = 10000;
	info->timeouts.get_data_timeout = 10000;
	info->timeouts.commit_timeout = 10000;
	info->timeouts.read_change_timeout = 10000;

#ifdef HAVE_CONTACT	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "gnokii-contact", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "gnokii-contact", gnokii_contact_commit);
#endif	

#ifdef HAVE_EVENT	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "gnokii-event", NULL);
	osync_plugin_set_commit_objformat(info, "event", "gnokii-event", gnokii_calendar_commit);
#endif	

//	osync_plugin_accept_objtype(info, "todo");
//	osync_plugin_accept_objformat(info, "todo", "gnokii-todo", NULL);
//	osync_plugin_set_commit_objformat(info, "todo", "gnokii-todo", commit_change);

}
