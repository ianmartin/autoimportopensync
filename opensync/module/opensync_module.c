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
#include "opensync_module_internals.h"

typedef osync_bool (* fkt_b_plugenv_error)(OSyncPluginEnv *env, OSyncError **error);
typedef osync_bool (* fkt_b_fmtenv_error)(OSyncFormatEnv *env, OSyncError **error);

OSyncModule *osync_module_new(OSyncError **error)
{
  OSyncModule *module = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
  module = osync_try_malloc0(sizeof(OSyncModule), error);
  if (!module) {
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return NULL;
  }
	
  osync_trace(TRACE_EXIT, "%s: %p", __func__, module);
  return module;
}

/*! @brief Used to free a module
 * 
 * Frees a module
 * 
 * @param module Pointer to the module
 * 
 */
void osync_module_free(OSyncModule *module)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, module);
  if (module->module)
    osync_module_unload(module);
		
  if (module->path)
    g_free(module->path);
	
  g_free(module);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Used to look up a symbol on the plugin
 * 
 * Looks up and returns a function
 * 
 * @param plugin Pointer to the plugin
 * @param name The name of the function to look up
 * @param error Pointer to a error struct
 * @return Pointer to the function
 * 
 */
void *osync_module_get_function(OSyncModule *module, const char *name, OSyncError **error)
{
  void *function = NULL;
  osync_assert(module);
  osync_assert(name);
	
  if (!module->module) {
    osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You need to load the module before getting a function");
    return NULL;
  }
	
  if (!g_module_symbol(module->module, name, &function)) {
    osync_error_set(error, OSYNC_ERROR_PARAMETER, "Unable to locate symbol %s: %s", __NULLSTR(name), g_module_error());
    return NULL;
  }
	
  return function;
}

osync_bool osync_module_get_sync_info(OSyncModule *module, OSyncPluginEnv *env, OSyncError **error)
{
  fkt_b_plugenv_error fct_info = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, module, env, error);
	
  /* Load the get_sync_info symbol */
  fct_info = (fkt_b_plugenv_error) osync_module_get_function(module, "get_sync_info", error);
  if (!fct_info) {
    osync_trace(TRACE_EXIT_ERROR, "%s: Not get_sync_info function", __func__);
    return FALSE;
  }
	
  /* Call the get_info function */
  if (!fct_info(env, error))
    goto error;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

osync_bool osync_module_get_format_info(OSyncModule *module, OSyncFormatEnv *env, OSyncError **error)
{
  fkt_b_fmtenv_error fct_info = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, module, env, error);
	
  /* Load the get_info symbol */
  fct_info = (fkt_b_fmtenv_error) osync_module_get_function(module, "get_format_info", NULL);
  if (!fct_info) {
    osync_trace(TRACE_EXIT, "%s: Not get_format_info function", __func__);
    return FALSE;
  }
	
  /* Call the get_info function */
  if (!fct_info(env, error))
    goto error;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

osync_bool osync_module_get_conversion_info(OSyncModule *module, OSyncFormatEnv *env, OSyncError **error)
{
  fkt_b_fmtenv_error fct_info = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, module, env, error);
	
  /* Load the get_info symbol */
  fct_info = (fkt_b_fmtenv_error) osync_module_get_function(module, "get_conversion_info", NULL);
  if (!fct_info) {
    osync_trace(TRACE_EXIT, "%s: Not get_format_info function", __func__);
    return FALSE;
  }
	
  /* Call the get_info function */
  if (!fct_info(env, error))
    goto error;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

int osync_module_get_version(OSyncModule *module)
{
  void *function = NULL;
  int (* fct_version)(void) = NULL;
  int version = 0;

  osync_assert(module);
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, module);
	
  /* Load the get_info symbol */
  if (!g_module_symbol(module->module, "get_version", &function) || !function) {
    osync_trace(TRACE_EXIT, "%s: get_version not found. Not a library?", __func__);
    return 0;
  }
	
  fct_version = (int (* )(void)) function;
  /* Call the get_info function */
  version = fct_version();
	
  osync_trace(TRACE_EXIT, "%s: %i", __func__, version);
  return version;
}

osync_bool osync_module_check(OSyncModule *module, OSyncError **error)
{
  int version = 0;
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, module, error);
	
  version = osync_module_get_version(module);
  if (!version) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not load plugin \"%s\". Not a opensync library?", __NULLSTR(module->path));
    osync_trace(TRACE_EXIT_ERROR, "%s: No version", __func__);
    return FALSE;
  }
	
  if (version != OPENSYNC_PLUGINVERSION) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Plugin API version mismatch. Is: %i. Should %i", version, OPENSYNC_PLUGINVERSION);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return FALSE;
  }
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

/*! @brief dlopen()s a format plugin
 * 
 * The get_info() function on the format plugin gets called
 * 
 * @param env The environment in which to open the plugin
 * @param path Where to find this plugin
 * @param error Pointer to a error struct
 * @return Pointer to the plugin on success, NULL otherwise
 * 
 */
osync_bool osync_module_load(OSyncModule *module, const char *path, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, module, path, error);
  osync_assert(module);
  osync_assert(!module->module);
	
  if (!g_module_supported()) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
    goto error;
  }

  /* Try to open the module or fail if an error occurs */
  module->module = g_module_open(path, 0);
  if (!module->module) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open module %s: %s", path, g_module_error());
    goto error;
  }
	
  module->path = g_strdup(path);
	
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

 error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

/*! @brief Closes a module
 * 
 * @param env The environment from which to remove the module
 * @param module The module to unload
 * 
 */
void osync_module_unload(OSyncModule *module)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, module);
  osync_assert(module);
  osync_assert(module->module);
	
  if (!osync_module_get_function(module, "dont_free", NULL))
#ifndef OPENSYNC_DEBUG_MODULES
    g_module_close(module->module);
#else	
  osync_trace(TRACE_INTERNAL, "Unloading modules got disabled in this build. debug_modules=1");
#endif	
  module->module = NULL;
	
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/*@}*/
