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


static void kde_finalize(void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, userdata);
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;
	delete impl_object;
	osync_trace(TRACE_EXIT, "%s(%p)", __func__, userdata);
}

static void kde_connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase*)userdata;
	impl_object->connect(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}


static void kde_disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;
	impl_object->disconnect(info, ctx);
}

static void kde_get_changeinfo(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase*)userdata;
//	osync_debug("kde", 3, "%s",__FUNCTION__);

	impl_object->get_changeinfo(info, ctx);
}

static void kde_sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	impl_object->sync_done(info, ctx);
}

static void kde_vcard_commit_change(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata; 

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	impl_object->vcard_commit_change(info, ctx, change);
}

static osync_bool kde_vcard_access(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;

//	osync_debug("kde", 3, "%s()",__FUNCTION__);

	impl_object->vcard_access(info, ctx, change);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);

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

	do {
		impl_object->contact_sink = osync_objtype_sink_new("contact", error);
		if (!impl_object->contact_sink)
			goto error;

		osync_objtype_sink_add_objformat(impl_object->contact_sink, "vcard30");

		/* Every sink can have different functions ... */
		OSyncObjTypeSinkFunctions functions;
		memset(&functions, 0, sizeof(functions));
		functions.connect = kde_connect;
		functions.disconnect = kde_disconnect;
		functions.get_changes = kde_get_changeinfo;
		functions.commit = kde_vcard_commit_change;
		functions.sync_done = kde_sync_done;

		/* We pass the OSyncFileDir object to the sink, so we dont have to look it up
		 * again once the functions are called */
		osync_objtype_sink_set_functions(impl_object->contact_sink, functions, impl_object);
		osync_plugin_info_add_objtype(info, impl_object->contact_sink);

	} while(0);


	/* Return the created object to the sync engine */
	return (void*)impl_object;
	osync_trace(TRACE_EXIT, "%s: %p", __func__, impl_object);

error:
	osync_trace(TRACE_EXIT_ERROR,  "%s: NULL", __func__);
	return NULL;
}

extern "C"
{

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * go through the list of directories and enable all, since all have been configured */
static osync_bool kde_discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, error);
	
	KdePluginImplementationBase *impl_object = (KdePluginImplementationBase *)userdata;

	osync_objtype_sink_set_available(impl_object->contact_sink, TRUE);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error) {

	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;

	osync_plugin_set_name(plugin, "kdepim-sync");
	osync_plugin_set_longname(plugin, "KDE Desktop");
	osync_plugin_set_description(plugin, "Plugin for the KDE 3.5 Desktop");

	osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_NO_CONFIGURATION);

	osync_plugin_set_initialize(plugin, kde_initialize);
	osync_plugin_set_finalize(plugin, kde_finalize);
	osync_plugin_set_discover(plugin, kde_discover);


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
