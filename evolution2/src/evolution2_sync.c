/*
 * evolution2_sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "evolution2_sync.h"

GList *evo2_list_calendars(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_EVENT, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list calendars. Unable to get sources");
		return NULL;
	}

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

GList *evo2_list_tasks(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list tasks. Unable to get sources");
		return NULL;
	}

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

GList *evo2_list_addressbooks(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_book_get_addressbooks(&sources, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list addressbooks. Unable to get sources");
		return NULL;
    }

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

static void *evo2_initialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "EVO2-SYNC %s(%p, %p)", __func__, member, error);
	char *configdata = NULL;
	int configsize = 0;
	
	g_type_init();
	
	evo_environment *env = g_malloc0(sizeof(evo_environment));

	if (!osync_member_get_config(member, &configdata, &configsize, error))
		goto error_free;
	if (!evo2_parse_settings(env, configdata, configsize)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for evo2 plugin");
		goto error_free_data;
	}
	env->member = member;
	OSyncGroup *group = osync_member_get_group(member);
	env->change_id = g_strdup(osync_group_get_name(group));
	
	g_free(configdata);
	osync_trace(TRACE_EXIT, "EVO2-SYNC %s: %p", __func__, env);
	return (void *)env;
	
	error_free_data:
		g_free(configdata);
	error_free:
		g_free(env);
		osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC %s: %s", __func__, osync_error_print(error));
		return NULL;
}

ESource *evo2_find_source(ESourceList *list, char *uri)
{
	GSList *g;
	for (g = e_source_list_peek_groups (list); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);
			if (!strcmp(e_source_get_uri(source), uri))
				return source;
		}
	}
	return NULL;
}

static void evo2_connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "EVO2-SYNC: %s(%p)", __func__, ctx);
	OSyncError *error = NULL;
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	osync_bool open_any = FALSE;
	
	if (osync_member_objtype_enabled(env->member, "contact") &&  env->addressbook_path && strlen(env->addressbook_path)) {
		if (evo2_addrbook_open(env, &error))
			open_any = TRUE;
		else {
			osync_trace(TRACE_INTERNAL, "EVO2-SYNC: Error opening addressbook: %s", osync_error_print(&error));
			osync_context_send_log(ctx, "Unable to open addressbook");
			osync_error_free(&error);
		}
	}
	
	if (osync_member_objtype_enabled(env->member, "event") &&  env->calendar_path && strlen(env->calendar_path)) {
		if (evo2_calendar_open(env, &error))
			open_any = TRUE;
		else {
			osync_trace(TRACE_INTERNAL, "Error opening calendar: %s", osync_error_print(&error));
			osync_context_send_log(ctx, "Unable to open calendar");
			osync_error_free(&error);
		}
	}

	if (osync_member_objtype_enabled(env->member, "todo") &&  env->tasks_path && strlen(env->tasks_path)) {
		if (evo2_todo_open(env, &error))
			open_any = TRUE;
		else {
			osync_trace(TRACE_INTERNAL, "Error opening todo: %s", osync_error_print(&error));
			osync_context_send_log(ctx, "Unable to open todo");
			osync_error_free(&error);
		}
	}

	srand(time(NULL));
	if (!open_any) {
		osync_debug("EVO2-SYNC", 0, "Unable to open anything!");
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to open anything");
		osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s: %s", __func__);
		return;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "EVO2-SYNC: %s", __func__);
}

void evo2_report_change(OSyncContext *ctx, char *objtypestr, char *objformatstr, char *data, int datasize, const char *uid, OSyncChangeType type)
{
	OSyncMember *member = osync_context_get_member(ctx);

	OSyncFormatEnv *env = osync_member_get_format_env(member);
	OSyncObjType *objtype = osync_conv_find_objtype(env, objtypestr);
	OSyncObjFormat *objformat = osync_conv_find_objformat(env, objformatstr);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, uid);
	osync_change_set_objtype(change, objtype);
	osync_change_set_objformat(change, objformat);
	osync_change_set_changetype(change, type);
	osync_change_set_data(change, data, datasize, TRUE);
	osync_context_report_change(ctx, change);
}

static void evo2_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	if (env->addressbook)
		evo2_addrbook_get_changes(ctx);
	
	if (env->calendar)
		evo2_calendar_get_changes(ctx);
	
	if (env->tasks)
		evo2_todo_get_changes(ctx);
	
	osync_context_report_success(ctx);
}

static void evo2_sync_done(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	GList *changes;
	
	if (env->addressbook) {
		osync_anchor_update(env->member, "contact", env->addressbook_path);
		e_book_get_changes(env->addressbook, env->change_id, &changes, NULL);
	}
	
	if (env->calendar) {
		osync_anchor_update(env->member, "event", env->calendar_path);
		e_cal_get_changes(env->calendar, env->change_id, &changes, NULL);
	}
	
	if (env->tasks) {
		osync_anchor_update(env->member, "todo", env->tasks_path);
		e_cal_get_changes(env->tasks, env->change_id, &changes, NULL);
	}
	
	osync_context_report_success(ctx);
}

static void evo2_disconnect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	if (env->addressbook) {
		g_object_unref(env->addressbook);
		env->addressbook = NULL;
	}
	
	if (env->tasks) {
		g_object_unref(env->tasks);
		env->tasks = NULL;
	}
	
	if (env->calendar) {
		g_object_unref(env->calendar);
		env->calendar = NULL;
	}
	
	osync_context_report_success(ctx);
}

static void evo2_finalize(void *data)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)data;
	
	g_free(env->change_id);
	g_free(env);
}

void get_info(OSyncPluginInfo *info) {
	info->name = "evo2-sync";
	info->version = 1;
	info->is_threadsafe = TRUE;
	
	info->functions.initialize = evo2_initialize;
	info->functions.connect = evo2_connect;
	info->functions.get_changeinfo = evo2_get_changeinfo;
	info->functions.sync_done = evo2_sync_done;
	info->functions.disconnect = evo2_disconnect;
	info->functions.finalize = evo2_finalize;

	evo2_addrbook_setup(info);
	evo2_calendar_setup(info);
	evo2_tasks_setup(info);
}
