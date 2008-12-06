/*
 * libopensync - A synchronization framework
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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-module.h"
#include "opensync-plugin.h"
#include "opensync_plugin_env_internals.h"
#include "opensync_plugin_env_private.h"

OSyncPluginEnv *osync_plugin_env_new(OSyncError **error)
{
	OSyncPluginEnv *env = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	env = osync_try_malloc0(sizeof(OSyncPluginEnv), error);
	if (!env) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return env;
}

void osync_plugin_env_free(OSyncPluginEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	osync_assert(env);
	
	/* Free the plugins */
	while (env->plugins) {
		osync_plugin_unref(env->plugins->data);
		env->plugins = g_list_remove(env->plugins, env->plugins->data);
	}
	
	/* Unload all modules */
	while (env->modules) {
		osync_module_unload(env->modules->data);
		osync_module_free(env->modules->data);
		env->modules = g_list_remove(env->modules, env->modules->data);
	}
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool osync_plugin_env_load(OSyncPluginEnv *env, const char *path, OSyncError **error)
{
	osync_bool must_exist = TRUE;
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	const gchar *de = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, __NULLSTR(path), error);
	
	if (!path) {
		path = OPENSYNC_PLUGINDIR;
		must_exist = FALSE;
	}
	
	//Load all available shared libraries (plugins)
	if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
		if (must_exist) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Path is not loadable");
			goto error;
		} else {
			osync_trace(TRACE_EXIT, "%s: Directory %s does not exist (non-fatal)", __func__, path);
			return TRUE;
		}
	}
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open directory %s: %s", path, gerror->message);
		g_error_free(gerror);
		goto error;
	}
	
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s%c%s", path, G_DIR_SEPARATOR, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || !g_pattern_match_simple("*."G_MODULE_SUFFIX, filename)) {
			g_free(filename);
			continue;
		}
		
		if (!osync_plugin_env_load_module(env, filename, error)) {
			osync_trace(TRACE_ERROR, "Unable to load module: %s", osync_error_print(error));
		}
		
		g_free(filename);
	}
	
	g_dir_close(dir);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void osync_plugin_env_register_plugin(OSyncPluginEnv *env, OSyncPlugin *plugin)
{
	osync_assert(env);
	osync_assert(plugin);
	
	env->plugins = g_list_append(env->plugins, plugin);
	osync_plugin_ref(plugin);
}

osync_bool osync_plugin_env_load_module(OSyncPluginEnv *env, const char *filename, OSyncError **error)
{
	OSyncModule *module = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, filename, error);
	osync_assert(env);
	osync_assert(filename);
	
	module = osync_module_new(error);
	if (!module)
		goto error;
	
	if (!osync_module_load(module, filename, error)) {
		osync_trace(TRACE_INTERNAL, "Unable to load module %s: %s", filename, osync_error_print(error));
		osync_module_free(module);
	} else {
		if (!osync_module_check(module, error)) {
			if (osync_error_is_set(error)) {
				osync_trace(TRACE_INTERNAL, "Module check error for %s: %s", filename, osync_error_print(error));
			}
			osync_module_unload(module);
			osync_module_free(module);
			osync_trace(TRACE_EXIT, "%s: Unable to load module", __func__);
			return FALSE;
		}
		
		if (!osync_module_get_sync_info(module, env, error)) {
			if (osync_error_is_set(error))
				goto error_free_module;
			
			osync_module_unload(module);
			osync_module_free(module);
			osync_trace(TRACE_EXIT, "%s: No get_sync_info function", __func__);
			return FALSE;
		}
		env->modules = g_list_append(env->modules, module);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_module:
	osync_module_unload(module);
	osync_module_free(module);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

OSyncPlugin *osync_plugin_env_find_plugin(OSyncPluginEnv *env, const char *name)
{
	GList *p;
	osync_assert(env);
	for (p = env->plugins; p; p = p->next) {
		OSyncPlugin *plugin = p->data;
		if (g_ascii_strcasecmp(osync_plugin_get_name(plugin), name) == 0)
			return plugin;
	}
	return NULL;
}

int osync_plugin_env_num_plugins(OSyncPluginEnv *env)
{
	return g_list_length(env->plugins);
}

OSyncPlugin *osync_plugin_env_nth_plugin(OSyncPluginEnv *env, int nth)
{
	return (OSyncPlugin *)g_list_nth_data(env->plugins, nth);
}

osync_bool osync_plugin_env_plugin_is_usable(OSyncPluginEnv *env, const char *pluginname, OSyncError **error)
{
	/* TODO: implement USABLE functions */
	return TRUE;
}

