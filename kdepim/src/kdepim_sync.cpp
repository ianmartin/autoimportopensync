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

#include "osyncbase.h"

static KdePluginImplementationBase *impl_object_for_context(OSyncContext *ctx)
{
	return (KdePluginImplementationBase *)osync_context_get_plugin_data(ctx);
}

/** Load actual plugin implementation
 *
 * Loads kde_impl.so and create a new KdePluginImplementation object,
 * that is linked against the KDE libraries, and implements the plugin
 * functions
 *
 * @see KdePluginImplementationBase
 */
static void *kde_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	KdeImplInitFunc init_func;
	KdePluginImplementationBase *impl_object;
	void *module;

//	osync_debug("kde", 3, "%s", __FUNCTION__);

//	osync_debug("kde", 3, "Loading implementation module");
	module = dlopen(KDEPIM_LIBDIR"/kdepim_lib.so", RTLD_NOW);
	if (!module) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Can't load plugin implementation module from %s: %s",
		                KDEPIM_LIBDIR"/kdepim_lib.so", dlerror());
		goto error;
	}
//	osync_debug("kde", 3, "Getting initialization function");
	init_func = (KdeImplInitFunc)dlsym(module, "new_KdePluginImplementation");
	if (!init_func) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Invalid plugin implementation module");
		goto error;
	}

//	osync_debug("kde", 3, "Initializing implementation module");
	impl_object = init_func(plugin, info, error);
	if (!impl_object)
		goto error;

	/* Return the created object to the sync engine */
	return (void*)impl_object;
error:
	return NULL;
}

static void kde_finalize(void *data)
{
//	osync_debug("kde", 3, "%s()", __FUNCTION__);

	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)data;
	delete impl_object;
}

static void kde_connect(OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);
	impl_object->connect(ctx);
}


static void kde_disconnect(OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);
	impl_object->disconnect(ctx);
}

static void kde_get_changeinfo(OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);
//	osync_debug("kde", 3, "%s",__FUNCTION__);

	impl_object->get_changeinfo(ctx);
}

static void kde_sync_done(OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	impl_object->sync_done(ctx);
}

static osync_bool kde_vcard_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	return impl_object->vcard_commit_change(ctx, change);
}

static osync_bool kde_vcard_access(OSyncContext *ctx, OSyncChange *change)
{
	KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	return impl_object->vcard_access(ctx, change);
}

extern "C"
{

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * go through the list of directories and enable all, since all have been configured */
static osync_bool kde_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	OSyncKDEEnv *env = (OSyncKDEEnv *)data;

	osync_objtype_sink_set_available(env->contact_sink, TRUE);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncEnv *env, OSyncError **error) {

	//OSyncPluginInfo *info = osync_plugin_new_info(env);
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;

	//info->version = 1;
	//now in int get_version(void);

	//info->name = "kdepim-sync";
	osync_plugin_set_name(plugin, "kdepim-sync");
	//info->longname = "KDE Desktop";
	osync_plugin_set_longname(plugin, "KDE Desktop");
	//info->description = "Plugin for the KDE 3.5 Desktop";
	osync_plugin_set_description(plugin, "Plugin for the KDE 3.5 Desktop");

	//info->config_type = NO_CONFIGURATION;
	osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_NO_CONFIGURATION);

	//info->functions.initialize = kde_initialize;
	osync_plugin_set_initialize(plugin, kde_initialize);
	
	//info->functions.connect = kde_connect;
	//now in initialize
	//info->functions.disconnect = kde_disconnect;
	//now in finalize

	//info->functions.finalize = kde_finalize;
	osync_plugin_set_finalize(plugin, kde_finalize);
	osync_plugin_set_discover(plugin, kde_discover);
	
	info->functions.get_changeinfo = kde_get_changeinfo;
	info->functions.sync_done = kde_sync_done;

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

}// extern "C"
