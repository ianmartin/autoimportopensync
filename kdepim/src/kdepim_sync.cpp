/***********************************************************************
Actual implementation of the KDE PIM OpenSync plugin
Copyright (C) 2004 Conectiva S. A.
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation;
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
SOFTWARE IS DISCLAIMED.
*************************************************************************/
/**
 * @autor Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * edit Matthias Jahn <jahn.matthias@freenet.de>
 */

#include <dlfcn.h>
#include <string.h>

#include "osyncbase.h"

extern "C"
{

/** Load actual plugin implementation
 *
 * Loads kdepim_lib.so and create a new KdePluginImplementation object,
 * that is linked against the KDE libraries, and implements the plugin
 * functions
 *
 * @see KdePluginImplementationBase
 */
static void *
kde_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);

	KdeImplInitFunc init_func;
	KdePluginImplementationBase *impl_object;
	void *module;

	module = dlopen(KDEPIM_LIBDIR"/kdepim_lib.so", RTLD_NOW);
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Can't load plugin implementation module from %s: %s",
		                KDEPIM_LIBDIR"/kdepim_lib.so", dlerror());
		goto error;
	}
	
	init_func = (KdeImplInitFunc)dlsym(module, "new_KdePluginImplementation");
	if (!init_func) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Invalid plugin implementation module");
		goto error;
	}

	impl_object = init_func(plugin, info, error);
	if (!impl_object)
		goto error;

	/* Return the created object to the sync engine */
	osync_trace(TRACE_EXIT, "%s: %p", __func__, impl_object);
	return (void*)impl_object;

error:
	osync_trace(TRACE_EXIT_ERROR,  "%s: NULL", __func__);
	return NULL;
}

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * go through and enable all the sinks */
static osync_bool kde_discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, error);

	int n, num_objtypes;
	num_objtypes = osync_plugin_info_num_objtypes(info);
	for (n = 0; n < num_objtypes; n++)
		osync_objtype_sink_set_available(osync_plugin_info_nth_objtype(info, n), TRUE);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

static void kde_finalize(void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, userdata);
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;
	delete impl_object;
	osync_trace(TRACE_EXIT, "%s", __func__);
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;

	osync_plugin_set_name(plugin, "kdepim-sync");
	osync_plugin_set_longname(plugin, "KDE Desktop");
	osync_plugin_set_description(plugin, "Plugin for the KDE 3.5 Desktop");
	osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_NO_CONFIGURATION);
	osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_PROCESS); 

	osync_plugin_set_initialize(plugin, kde_initialize);
	osync_plugin_set_finalize(plugin, kde_finalize);
	osync_plugin_set_discover(plugin, kde_discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: Unable to register: %s", __func__, osync_error_print(error));
	return FALSE;
}

int get_version(void)
{
	return 1;
}

}// extern "C"
