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
#include "kaddrbook.h"

static KdePluginImplementationBase *addrbook_for_context(OSyncContext *ctx)
{
    return (KdePluginImplementationBase *)osync_context_get_plugin_data(ctx);
}

static void *kde_initialize(OSyncMember *member)
{
    KdeImplInitFunc init_func;
    KdePluginImplementationBase *addrbook;
    void *module;

    osync_debug("kde", 3, "%s", __FUNCTION__);
    /* Allocate and initialise a kaddrbook object. */

    module = dlopen(KDEPIM_LIBDIR"/kdepim_impl.so", RTLD_NOW);
    if (!module)
        return NULL;
    init_func = (KdeImplInitFunc)dlsym(module, "new_implementation_object");
    if (!init_func)
        return NULL;

    addrbook = init_func(member);
    if (!addrbook)
        return NULL;

    /* Return kaddrbook object to the sync engine */
    return (void*)addrbook;
}

static void kde_finalize(void *data)
{
    osync_debug("kde", 3, "%s()", __FUNCTION__);

    KdePluginImplementationBase *addrbook = (KdePluginImplementationBase *)data;
    delete addrbook;
}

static void kde_connect(OSyncContext *ctx)
{
    KdePluginImplementationBase *addrbook = addrbook_for_context(ctx);
    addrbook->connect(ctx);
}


static void kde_disconnect(OSyncContext *ctx)
{
    KdePluginImplementationBase *addrbook = addrbook_for_context(ctx);
    addrbook->disconnect(ctx);
}

static void kde_get_changeinfo(OSyncContext *ctx)
{
    KdePluginImplementationBase *addrbook = addrbook_for_context(ctx);
    osync_debug("kde", 3, "%s",__FUNCTION__);

    addrbook->get_changes(ctx);
}


static osync_bool kde_vcard_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *addrbook = addrbook_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return addrbook->vcard_commit_change(ctx, change); 
}

static osync_bool kde_vcard_access(OSyncContext *ctx, OSyncChange *change)
{
    KdePluginImplementationBase *addrbook = addrbook_for_context(ctx);

    osync_debug("kde", 3, "%s()",__FUNCTION__);

    return addrbook->vcard_access(ctx, change); 
}

extern "C" {
void get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "kde-sync";
    /*FIXME: i18n */
    info->description = "Plugin for the KDE 3.x Addressbook";

    info->functions.initialize = kde_initialize;
    info->functions.connect = kde_connect;
    info->functions.disconnect = kde_disconnect;
    info->functions.finalize = kde_finalize;
    info->functions.get_changeinfo = kde_get_changeinfo;

    osync_plugin_accept_objtype(info, "contact");
    osync_plugin_accept_objformat(info, "contact", "vcard");
    osync_plugin_set_commit_objformat(info, "contact", "vcard", kde_vcard_commit_change);
    osync_plugin_set_access_objformat(info, "contact", "vcard", kde_vcard_access);

}

}// extern "C"
