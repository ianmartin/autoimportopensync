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

/*! \brief Commits changes to the gpe client
 *
 * \param ctx		The current context of the sync
 * \param change	The change that has to be committed
 *
 */
void gpe_calendar_commit_change (void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p, %p)", __func__, userdata, info, ctx, change);

	gpe_environment *env = (gpe_environment *)userdata;
	sink_environment *sinkenv = (sink_environment *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	gchar *response = NULL;
	char *result = NULL;
	char *modified = NULL;
	char *error = NULL;
	char *data_ptr;
	unsigned int data_size;
		
	switch (osync_change_get_changetype (change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			osync_trace(TRACE_INTERNAL, "calendar: delete item %d", get_type_uid (osync_change_get_uid (change)));
			gpesync_client_exec_printf (env->client, "del vevent %d", client_callback_string, &response, NULL, get_type_uid (osync_change_get_uid (change)));
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
		  	osync_data_get_data(osync_change_get_data (change), &data_ptr, &data_size);
			osync_trace(TRACE_INTERNAL, "calendar: adding item: %s", data_ptr);
			gpesync_client_exec_printf (env->client, "add vevent %s", client_callback_string, &response, NULL, data_ptr);
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
		  	osync_data_get_data(osync_change_get_data (change), &data_ptr, &data_size);
			osync_trace(TRACE_INTERNAL, "calendar: modifying item %d: %s", get_type_uid (osync_change_get_uid (change)), data_ptr);
			gpesync_client_exec_printf (env->client, "modify vevent %d %s", client_callback_string, &response, NULL, get_type_uid (osync_change_get_uid (change)), data_ptr);
			break;
		default:
			osync_trace(TRACE_ERROR, "GPE-SYNC Unknown change type");
	}

	osync_trace(TRACE_INTERNAL, "commit result: %s", response);

	if (parse_value_modified (response, &result, &modified))
	{
		if (!strcasecmp (result, "OK"))
		{
			if (osync_change_get_changetype (change) == OSYNC_CHANGE_TYPE_ADDED)
			{
				/* We need to return the uid of the item
				 * in the gpe databases also, so that the
				 * mapping in opensync is correct.
				 * Fortunately the command "add" returns
				 * on success:
				 * OK:MODIFIED:UID
				 * so we can split value again */
				char *uid = NULL;
				char buf[25];
				parse_value_modified (modified, &modified, &uid);
				sprintf (buf, "gpe-event-%s", uid);
				osync_change_set_uid (change, g_strdup (buf));
			}
			osync_change_set_hash (change, modified);
			osync_hashtable_update_hash (sinkenv->hashtable, osync_change_get_changetype(change),
						     osync_change_get_uid(change), modified);
			osync_context_report_success(ctx);
		}
		else {
			/* result was split up into result (the part before the
			 * ':') and modified (the part after ':'); */
		        error = modified;
			osync_trace(TRACE_ERROR, "Couldn't commit event: %s", error);
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't commit event %s", error);
		}
	} else {
	  	osync_trace(TRACE_ERROR, "Couldn't process answer from gpesyncd: %s", result);
		osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't process answer from gpesyncd: %s", result);
	}
	
	if (response)
		g_free (response);

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}

/*! \brief Reports all available items to opensync
 *
 * \param ctx		Context of the plugin
 *
 */
void gpe_calendar_get_changes(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p)", __func__, userdata, info, ctx);

	gpe_environment *env = (gpe_environment *)userdata;
	sink_environment *sinkenv = (sink_environment *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));

	if (osync_objtype_sink_get_slowsync(sinkenv->sink)) {
		osync_trace(TRACE_INTERNAL, "Slow sync requested");
		osync_hashtable_reset(sinkenv->hashtable);
	}

	gchar *errmsg = NULL;
	GSList *uid_list = NULL, *iter;
	osync_trace(TRACE_INTERNAL, "Getting uidlists for vevents:");
	gpesync_client_exec (env->client, "uidlist vevent", client_callback_list, &uid_list, &errmsg);


	if ((uid_list) && (!strncasecmp ((gchar *)uid_list->data, "ERROR", 5)))
	{
	  errmsg = (gchar *) uid_list->data;
	}
	
	if (errmsg)
	{
		if (strncasecmp (errmsg, "Error: No item found", 20))
		{
		  	osync_trace(TRACE_ERROR, "Error getting event uidlist: %s", errmsg);
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Error getting event uidlist: %s", errmsg);
		} else {
		  	/* We haven't found any items, so go on. */
			osync_trace(TRACE_INTERNAL, "Found no items");
			uid_list = NULL;
		}
	
		g_slist_free (uid_list);
		uid_list = NULL;

		g_free (errmsg);
	}

	GString *vevent_data = g_string_new("");

	for (iter = uid_list; iter; iter = iter->next)
	{
		/* The list we got has the format:
		 * uid:modified */
	  	osync_trace(TRACE_INTERNAL, "Read: \"%s\"", (gchar *) iter->data);

	  	gchar *modified = NULL;
		gchar *uid = NULL;

		if (parse_value_modified ((gchar *)iter->data, &uid, &modified) == FALSE)
		{
			osync_context_report_error (ctx, OSYNC_ERROR_CONVERT, "Wrong uidlist item: %s");
			g_slist_free (uid_list);
		  	osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: Wrong uidlist item: %s", __func__, uid);
			return;
		}

		g_string_assign (vevent_data, "");
		osync_trace(TRACE_INTERNAL, "Getting vevent %s", uid);
		gpesync_client_exec_printf (env->client, "get vevent %s", client_callback_gstring, &vevent_data, NULL, uid);
		osync_trace(TRACE_SENSITIVE, "vevent output:\n%s", vevent_data->str);

		report_change (sinkenv, ctx, "event", uid, modified, vevent_data->str);
		
		g_free (iter->data);

		/* We don't need to free modified and uid, because they
		 * are only pointers to iter->data */
		modified = NULL;
		uid = NULL;
	}
	g_string_free (vevent_data, TRUE);

	if (uid_list)
		g_slist_free (uid_list);

	report_deleted(sinkenv, ctx);

        //Now we need to answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);

	return;
}


/*! \brief Tells the plugin to accept events
 *
 * \param info	The plugin info on which to operate
 */
osync_bool gpe_calendar_setup(sink_environment *sinkenv, OSyncObjTypeSinkFunctions functions, gpe_environment *env, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, functions, %p, %p, %p)", __func__, sinkenv, env, info, error);

	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	// Note: caller responsible for freeing hashtable on any error
	sinkenv->hashtable = osync_hashtable_new(tablepath, "event", error);
	g_free(tablepath);
	if (!sinkenv->hashtable) goto error;

	// Note: caller responsible for freeing sink  on any error
	sinkenv->sink = osync_objtype_sink_new("event", error);
	if (!sinkenv->sink) goto error;

	sinkenv->format = "vevent20";
	osync_objtype_sink_add_objformat(sinkenv->sink, sinkenv->format);

	functions.get_changes = gpe_calendar_get_changes;
	functions.commit = gpe_calendar_commit_change;

	osync_objtype_sink_set_functions(sinkenv->sink, functions, sinkenv);

	osync_plugin_info_add_objtype(info, sinkenv->sink);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	sinkenv->objformat = osync_format_env_find_objformat(formatenv, sinkenv->format);
	if (!sinkenv->objformat) {
	  osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Engine does not support format %s", sinkenv->format);
	  osync_trace(TRACE_ERROR, "GPE-SYNC %s: engine does not support format %s", __func__, sinkenv->format);
	  goto error;
	}

	sinkenv->gpe_env = env;

	osync_trace(TRACE_EXIT, "GPE-SYNC %s: TRUE", __func__);
	return TRUE;
 error:
	osync_trace(TRACE_EXIT, "GPE-SYNC %s: FALSE", __func__);
	return FALSE;
}
