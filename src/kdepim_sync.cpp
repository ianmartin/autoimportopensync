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
static void *kde_initialize(OSyncMember *member, OSyncError **e)
{
    KdeImplInitFunc init_func;
    KdePluginImplementationBase *impl_object;
    void *module;

    osync_debug("kde", 3, "%s", __FUNCTION__);

    osync_debug("kde", 3, "Loading implementation module");
    module = dlopen(KDEPIM_LIBDIR"/kdepim_lib.so", RTLD_NOW);
    if (!module) {
        osync_error_set(e, OSYNC_ERROR_INITIALIZATION, "Can't load plugin implementation module from %s: %s",
                           KDEPIM_LIBDIR"/kdepim_lib.so", dlerror());
        goto error;
    }
    osync_debug("kde", 3, "Getting initialization function");
    init_func = (KdeImplInitFunc)dlsym(module, "new_KdePluginImplementation");
    if (!init_func) {
        osync_error_set(e, OSYNC_ERROR_INITIALIZATION, "Invalid plugin implementation module");
        goto error;
    }

    osync_debug("kde", 3, "Initializing implementation module");
    impl_object = init_func(member, e);
    if (!impl_object)
        goto error;

    /* Return the created object to the sync engine */
    return (void*)impl_object;
error:
	return NULL;
}

static void kde_finalize(void *data)
{
    osync_debug("kde", 3, "%s()", __FUNCTION__);

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
    osync_debug("kde", 3, "%s",__FUNCTION__);

    impl_object->get_changeinfo(ctx);
}

static void kde_get_data(OSyncContext *ctx, OSyncChange *chg)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);
    osync_debug("kde", 3, "%s",__FUNCTION__);

    impl_object->get_data(ctx, chg);
}

static osync_bool kde_vcard_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->vcard_commit_change(ctx, change); 
}

static osync_bool kde_vcard_access(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->vcard_access(ctx, change); 
}

static osync_bool kde_event_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->event_commit_change(ctx, change); 
}

static osync_bool kde_event_access(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->event_access(ctx, change); 
}

static osync_bool kde_todo_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->todo_commit_change(ctx, change); 
}

static osync_bool kde_todo_access(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->todo_access(ctx, change); 
}

static osync_bool kde_note_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->note_commit_change(ctx, change); 
}

static osync_bool kde_note_access(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *impl_object = impl_object_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return impl_object->note_access(ctx, change); 
}

extern "C" {
void get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "kdepim";
    /*FIXME: i18n */
    info->description = "Plugin for the KDEPIM on KDE 3.x";
    info->config_type = NO_CONFIGURATION;

    info->functions.initialize = kde_initialize;
    info->functions.connect = kde_connect;
    info->functions.disconnect = kde_disconnect;
    info->functions.finalize = kde_finalize;
    info->functions.get_changeinfo = kde_get_changeinfo;
    info->functions.get_data = kde_get_data;

    osync_plugin_accept_objtype(info, "contact");
    osync_plugin_accept_objformat(info, "contact", "vcard30", "kde");
    osync_plugin_set_commit_objformat(info, "contact", "vcard30", kde_vcard_commit_change);
    osync_plugin_set_access_objformat(info, "contact", "vcard30", kde_vcard_access);

    osync_plugin_accept_objtype(info, "event");
    osync_plugin_accept_objformat(info, "event", "vevent20", "kde");
    osync_plugin_set_commit_objformat(info, "event", "vevent20", kde_event_commit_change);
    osync_plugin_set_access_objformat(info, "event", "vevent20", kde_event_access);

    osync_plugin_accept_objtype(info, "todo");
    osync_plugin_accept_objformat(info, "todo", "vtodo20", "kde");
    osync_plugin_set_commit_objformat(info, "todo", "vtodo20", kde_todo_commit_change);
    osync_plugin_set_access_objformat(info, "todo", "vtodo20", kde_todo_access);

    osync_plugin_accept_objtype(info, "note");
    osync_plugin_accept_objformat(info, "note", "xml-note", NULL);
    osync_plugin_set_commit_objformat(info, "note", "xml-note", kde_note_commit_change);
    osync_plugin_set_access_objformat(info, "note", "xml-note", kde_note_access);
}

}// extern "C"
