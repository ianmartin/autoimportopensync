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
 
#include "evolution_sync.h"

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
	char *configdata = NULL;
	int configsize = 0;
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = g_malloc0(sizeof(evo_environment));
	if (!env)
		goto error_ret;
	if (!osync_member_get_config(member, &configdata, &configsize, error)) 
		goto error_free;
	if (!evo2_parse_settings(env, configdata, configsize)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for evo2 plugin");
		goto error_free_data;
	}
	env->member = member;
	OSyncGroup *group = osync_member_get_group(member);
	env->change_id = g_strdup(osync_group_get_name(group));
	printf("evo2 init %s\n", env->change_id);
	
	g_free(configdata);
	return (void *)env;
	
	error_free_data:
		g_free(configdata);
	error_free:
		g_free(env);
	error_ret:
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
			printf("Comparing %s with %s\n", e_source_get_uri(source), uri);
			if (!strcmp(e_source_get_uri(source), uri))
				return source;
		}
	}
	return NULL;
}

static void evo2_connect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	osync_bool open_any = FALSE;
	if (osync_member_objtype_enabled(env->member, "contact") &&  env->adressbook_path && strlen(env->adressbook_path)) {
		osync_debug("EVO2-SYNC", 4, "opening adressbook");
		if (evo2_addrbook_open(env)) {
			open_any = TRUE;
			osync_debug("EVO2-SYNC", 4, "ok");
			if (!osync_anchor_compare(env->member, "contact", env->adressbook_path)) {
				printf("++Requesting slow-sync\n");
				osync_member_set_slow_sync(env->member, "contact", TRUE);
			osync_debug("EVO2-SYNC", 4, "ok2");
			}
		} else {
			osync_context_send_log(ctx, "Unable to open addressbook");
		}
	}
	osync_debug("EVO2-SYNC", 4, "cont");
	if (osync_member_objtype_enabled(env->member, "event") &&  env->calendar_path && strlen(env->calendar_path)) {
		if (evo2_calendar_open(env)) {
			open_any = TRUE;
			if (!osync_anchor_compare(env->member, "event", env->calendar_path))
				osync_member_set_slow_sync(env->member, "event", TRUE);
		} else {
			osync_context_send_log(ctx, "Unable to open calendar");
		}
	}

	if (osync_member_objtype_enabled(env->member, "todo") && env->tasks_path && strlen(env->tasks_path)) {
		if (evo2_tasks_open(env)) {
			open_any = TRUE;
			if (!osync_anchor_compare(env->member, "todo", env->tasks_path))
				osync_member_set_slow_sync(env->member, "todo", TRUE);
		} else {
			osync_context_send_log(ctx, "Unable to open tasks");
		}
	}

	srand(time(NULL));
	if (!open_any) {
		osync_debug("EVO2-SYNC", 0, "Unable to open anything!");
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to open anything");
		return;
	}
	
	osync_debug("EVO2-SYNC", 4, "end: %s", __func__);
	osync_context_report_success(ctx);
}

static OSyncChangeType evo2_get_data(void *object, char *objtype, char **data, int *datasize, const char **uid)
{
	ECalChange *ecc = NULL;
	
	if (!strcmp(objtype, "event") || !strcmp(objtype, "todo")) {
		ecc = (ECalChange *)object;
		e_cal_component_commit_sequence (ecc->comp);
		e_cal_component_strip_errors(ecc->comp);
		*data = e_cal_component_get_as_string (ecc->comp);
		*datasize = strlen(*data) + 1;
		e_cal_component_get_uid(ecc->comp, uid);
		switch (ecc->type) {
			case E_CAL_CHANGE_ADDED:
				return CHANGE_ADDED;
			case E_CAL_CHANGE_MODIFIED:
				return CHANGE_MODIFIED;
			case E_CAL_CHANGE_DELETED:
				return CHANGE_DELETED;
		}
	}
	return CHANGE_UNKNOWN;
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

static void evo2_report_changes(GList *changes, OSyncContext *ctx, char *objtypestr, char *objformatstr)
{
	OSyncMember *member = osync_context_get_member(ctx);
	GList *l;
	for (l = changes; l; l = l->next) {
		const char *uid = NULL;
		char *data = NULL;
		int datasize = 0;
		OSyncChangeType type = evo2_get_data(l->data, objtypestr, &data, &datasize, &uid);
		
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
}

static void evo2_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	GList *changes = NULL;

	if (env->adressbook)
		evo2_addrbook_get_changes(ctx);
	
	if (env->calendar) {
		if (osync_member_get_slow_sync(env->member, "event")) {
			if (!e_cal_get_changes(env->calendar, env->change_id, &changes, NULL)) {
				osync_context_send_log(ctx, "Unable to open changed calendar entries");
			}
		} else {
			/* FIXME HOW?
			
			EBookQuery *query = e_book_query_from_string("*"); //FIXME
			if (!e_book_get_contacts(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open contacts");
			}
			*/
		}	
		evo2_report_changes(changes, ctx, "event", "vevent");
	}
	
	if (env->tasks) {
		if (osync_member_get_slow_sync(env->member, "todo")) {
			if (!e_cal_get_changes(env->tasks, env->change_id, &changes, NULL)) {
				osync_context_send_log(ctx, "Unable to open changed tasks");
			}
		} else {
			/* FIXME HOW?
			
			EBookQuery *query = e_book_query_from_string("*"); //FIXME
			if (!e_book_get_contacts(env->adressbook, env->change_id, &changes, NULL)) {
				osync_context_send_warning(ctx, "Unable to open contacts");
			}
			*/
		}	
		evo2_report_changes(changes, ctx, "todo", "vtodo");
	}
	
	osync_context_report_success(ctx);
}

static void evo2_sync_done(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	GList *changes;
	
	if (env->adressbook) {
		osync_anchor_update(env->member, "contact", env->adressbook_path);
		e_book_get_changes(env->adressbook, env->change_id, &changes, NULL);
	}
	if (env->calendar)
		e_cal_get_changes(env->calendar, env->change_id, &changes, NULL);
	if (env->tasks)
		e_cal_get_changes(env->tasks, env->change_id, &changes, NULL);
	
	osync_context_report_success(ctx);
}

static void evo2_disconnect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	if (env->adressbook)
		g_object_unref(env->adressbook);
	//FIXME!!
	
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
	info->is_threadsafe = FALSE;
	
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
