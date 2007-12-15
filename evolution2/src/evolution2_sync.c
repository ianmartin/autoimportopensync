/*
 * evolution2_sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007 Daniel Friedrich <daniel.friedrich@opensync.org>
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

static void free_env(OSyncEvoEnv *env)
{
	if (env->addressbook_path)
		g_free(env->addressbook_path);
		
	if (env->calendar_path)
		g_free(env->calendar_path);
		
	if (env->tasks_path)
		g_free(env->tasks_path);

	if (env->contact_sink)
		osync_objtype_sink_unref(env->contact_sink);
		
	if (env->calendar_sink)
		osync_objtype_sink_unref(env->calendar_sink);
		
	if (env->memos_sink)
		osync_objtype_sink_unref(env->memos_sink);
	
	if (env->tasks_sink)
		osync_objtype_sink_unref(env->tasks_sink);
	
	if (env->change_id)
		g_free(env->change_id);

	g_free(env);
}

GList *evo2_list_calendars(OSyncEvoEnv *env, void *data, OSyncError **error)
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

GList *evo2_list_memos(OSyncEvoEnv *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	osync_bool first = FALSE;
	
	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_JOURNAL, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list memos. Unable to get sources");
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

GList *evo2_list_tasks(OSyncEvoEnv *env, void *data, OSyncError **error)
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

GList *evo2_list_addressbooks(OSyncEvoEnv *env, void *data, OSyncError **error)
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

ESource *evo2_find_source(ESourceList *list, char *uri)
{
	GSList *g;
	for (g = e_source_list_peek_groups (list); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);
			osync_trace(TRACE_INTERNAL, "Comparing source uri %s and %s", e_source_get_uri(source), uri);
			if (!strcmp(e_source_get_uri(source), uri))
				return source;
			osync_trace(TRACE_INTERNAL, "Comparing source name %s and %s", e_source_peek_name(source), uri);
			if (!strcmp(e_source_peek_name(source), uri))
				return source;
		}
	}
	return NULL;
}

/*Load the state from a xml file and return it in the conn struct*/
static osync_bool evo2_parse_settings(OSyncEvoEnv *env, const char *data, OSyncError **error)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	//set defaults
	env->addressbook_path = NULL;
	env->calendar_path = NULL;
	env->memos_path = NULL;
	env->tasks_path = NULL;

	doc = xmlParseMemory(data, strlen(data));

	if (!doc) 
		return FALSE;
	

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		xmlFreeDoc(doc);
		return FALSE;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		xmlFreeDoc(doc);
		return FALSE;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"address_path")) {
				env->addressbook_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_path")) {
				env->calendar_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"memos_path")) {
				env->memos_path = g_strdup(str);	
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"tasks_path")) {
				env->tasks_path = g_strdup(str);	
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}

/* In initialize, we get the config for the plugin. Here we also must register
 * all _possible_ objtype sinks. */
static void *evo2_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	OSyncEvoEnv *env = osync_try_malloc0(sizeof(OSyncEvoEnv), error);
	if (!env)
		goto error;
		
	osync_trace(TRACE_INTERNAL, "Setting change id: %s", osync_plugin_info_get_groupname(info));
	
	env->change_id = g_strdup(osync_plugin_info_get_groupname(info));
	
	osync_trace(TRACE_INTERNAL, "The config: %s", osync_plugin_info_get_config(info));
	
	if (!evo2_parse_settings(env, osync_plugin_info_get_config(info), error))
		goto error_free_env;
	
	if (!evo2_ebook_initialize(env, info, error))
		goto error_free_env;
	
	if (!evo2_ecal_initialize(env, info, error))
		goto error_free_env;

	if (!evo2_memo_initialize(env, info, error))
		goto error_free_env;
	
	if (!evo2_etodo_initialize(env, info, error))
		goto error_free_env;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	free_env(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void evo2_finalize(void *data)
{
        osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	OSyncEvoEnv *env = data;

	if (env->contact_sink)
		osync_objtype_sink_unref(env->contact_sink);

	if (env->calendar_sink)
		osync_objtype_sink_unref(env->calendar_sink);

	if (env->memos_sink)
		osync_objtype_sink_unref(env->memos_sink);

	if (env->tasks_sink)
		osync_objtype_sink_unref(env->tasks_sink);

	free_env(env);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * go through the list of directories and enable all, since all have been configured */
static osync_bool evo2_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	GError* gerror = NULL;
	GList* fields = NULL;
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;
	
	if (env->addressbook_path)
		osync_objtype_sink_set_available(env->contact_sink, TRUE);
	
	if (env->calendar_path)
		osync_objtype_sink_set_available(env->calendar_sink, TRUE);
	
	if (env->memos_path)
		osync_objtype_sink_set_available(env->memos_sink, TRUE);
	
	if (env->tasks_path)
		osync_objtype_sink_set_available(env->tasks_sink, TRUE);
	
	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "Evolution");
	osync_version_set_modelversion(version, "2");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

/* generate wrong capabilities format : those are evolution2 capabilities format not xmlformat capabilities
 * FIXME : either create a mapping between evolution2 capabilities and xmlformat one or ship an evolution2 xmlformat type capabilities file and load it there
	EBook* ebook;
	if (!(ebook = e_book_new_default_addressbook(&gerror))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
		goto error;
	}
	if (!e_book_open(ebook, TRUE, &gerror)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to open addressbook: %s", gerror ? gerror->message : "None");
	  	goto error_free_book;
	}
	if (!(e_book_get_supported_fields (ebook, &fields, &gerror))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to get supported fields: %s", gerror ? gerror->message : "None");
		goto error_free_book;
	}
	
	OSyncCapabilities* capabilities = osync_capabilities_new(error);
	if(capabilities == NULL)
		goto error;
	
	for(; fields; fields = g_list_next(fields))
	{
		osync_capability_new(capabilities, "contact", fields->data, error);
		g_free(fields->data);	
	}
	g_list_free(fields);
	
	g_object_unref(ebook);
	
	osync_plugin_info_set_capabilities(info, capabilities);
	osync_capabilities_unref(capabilities);
*/
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_book:
		g_object_unref(env->addressbook);
error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "evo2-sync");
	osync_plugin_set_longname(plugin, "Evolution 2.x");
	osync_plugin_set_description(plugin, "Address book, calendar and task list of Evolution 2");
	osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_OPTIONAL_CONFIGURATION);
	/**
	 * Bug 477227 – libebook isn't designed to be loaded and unloaded
	 * see: http://bugzilla.gnome.org/show_bug.cgi?id=477227
	 * see: http://mail.gnome.org/archives/evolution-hackers/2007-September/msg00027.html
	 * also the other EDS client libraries; so we start the plugin in a own process.
	 */
	osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_PROCESS);
	
	osync_plugin_set_initialize(plugin, evo2_initialize);
	osync_plugin_set_finalize(plugin, evo2_finalize);
	osync_plugin_set_discover(plugin, evo2_discover);
	
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
