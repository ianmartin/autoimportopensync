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


static osync_bool evo2_parse_settings(evo_environment *env, xmlDocPtr doc, OSyncError **error);


GList *evo2_list_calendars(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	osync_bool first = FALSE;
	
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
			if (!first) {
				first = TRUE;
				path->uri = g_strdup("default");
			} else {
				path->uri = g_strdup(e_source_get_uri(source));
			}
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
	osync_bool first = FALSE;
	
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
			if (!first) {
				first = TRUE;
				path->uri = g_strdup("default");
			} else {
				path->uri = g_strdup(e_source_get_uri(source));
			}
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
	osync_bool first = FALSE;
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
			if (!first) {
				first = TRUE;
				path->uri = g_strdup("default");
			} else {
				path->uri = g_strdup(e_source_get_uri(source));
			}
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

static void *evo2_initialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "EVO2-SYNC %s(%p, %p)", __func__, member, error);
	xmlDocPtr doc;
	
	g_type_init();
	
	evo_environment *env = g_malloc0(sizeof(evo_environment));

	if (!(doc = osync_member_get_config (member, error)))
		goto error_free;
	if (!evo2_parse_settings(env, doc, error)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for evo2 plugin");
		goto error_free;
	}
	
	xmlFreeDoc (doc);
	env->member = member;
	OSyncGroup *group = osync_member_get_group(member);
	env->change_id = g_strdup(osync_group_get_name(group));
	
	osync_trace(TRACE_EXIT, "EVO2-SYNC %s: %p", __func__, env);
	return (void *)env;
	
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
		osync_trace(TRACE_EXIT_ERROR, "EVO2-SYNC: %s", __func__);
		return;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "EVO2-SYNC: %s", __func__);
}

void evo2_report_change(OSyncContext *ctx, char *objtypestr, char *objformatstr, char *data, int datasize, const char *uid, OSyncChangeType type)
{
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, uid);
	osync_change_set_objformat_string(change, objformatstr);
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

static osync_bool
evo2_parse_settings(evo_environment *env, xmlDocPtr doc, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	xmlNodePtr cur;

	//set defaults
	env->addressbook_path = NULL;
	env->calendar_path = NULL;
	env->tasks_path = NULL;

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (strcmp (cur->name, "opensync-plugin-config") != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = cur->children;

	while (cur != NULL) {
		if (!strcmp (cur->name, "option")) {
			char *name;
			
			if ((name = xmlGetProp (cur, "name"))) {
				if (!strcmp (name, "addressbook")) {
					env->addressbook_path = xmlNodeGetContent (cur);
				} else if (!strcmp (name, "calendar")) {
					env->calendar_path = xmlNodeGetContent (cur);
				} else if (!strcmp (name, "tasks")) {
					env->tasks_path = xmlNodeGetContent (cur);
				}
				xmlFree (name);
			}
		}
		cur = cur->next;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static xmlDocPtr
evo2_get_default_config (void)
{
	xmlNodePtr root, node;
	xmlDocPtr doc;
	
	doc = xmlNewDoc ("1.0");
	
	root = xmlNewDocNode (doc, NULL, "opensync-plugin-config", NULL);
	xmlSetProp (root, "plugin", "evo2");
	xmlSetProp (root, "version", "1.0");
	xmlDocSetRootElement (doc, root);
	
	node = xmlNewChild (root, NULL, "option", NULL);
	xmlSetProp (node, "type", "path");
	xmlSetProp (node, "mode", "directory");
	xmlSetProp (node, "name", "addressbook");
	xmlSetProp (node, "label", "Addressbook");
	
	node = xmlNewChild (root, NULL, "option", NULL);
	xmlSetProp (node, "type", "path");
	xmlSetProp (node, "mode", "directory");
	xmlSetProp (node, "name", "calendar");
	xmlSetProp (node, "label", "Calendar");
	
	node = xmlNewChild (root, NULL, "option", NULL);
	xmlSetProp (node, "type", "path");
	xmlSetProp (node, "mode", "directory");
	xmlSetProp (node, "name", "tasks");
	xmlSetProp (node, "label", "Tasks");
	
	return doc;
}

static xmlDocPtr
evo2_get_config (const char *configdir)
{
	xmlDocPtr doc;
	char *path;
	
	path = g_strdup_printf ("%s/evo2.xml", configdir);
	if (!(doc = xmlParseFile (path)))
		doc = evo2_get_default_config ();
	g_free (path);
	
	return doc;
}

static osync_bool
evo2_set_config (const char *configdir, xmlDocPtr doc)
{
	char *path;
	
	path = g_strdup_printf ("%s/evo2.xml", configdir);
	xmlSaveFile (path, doc);
	g_free (path);
	
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->name = "evo2-sync";
	info->version = 1;
	info->is_threadsafe = TRUE;
    info->config_type = OPTIONAL_CONFIGURATION;
    
    info->functions.get_config = evo2_get_config;
    info->functions.set_config = evo2_set_config;
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
