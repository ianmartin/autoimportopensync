/*********************************************************************** 
MultiSync Plugin for KDE 3.x
Copyright (C) 2004 Stewart Heitmann <sheitmann@users.sourceforge.net>

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

#include <opensync/opensync.h> 
#include <string.h>

#include "kaddrbook.h"

typedef struct 
{
    void *kaddrbook;
} kde_connection;

static kde_connection *conn_for_context(OSyncContext *ctx)
{
    return (kde_connection *)osync_context_get_plugin_data(ctx);
}

static void *kde_initialize(OSyncMember *member, char *datapath)
{
    kde_connection *conn;

    if (getenv ("MULTISYNC_DEBUG"))
        printf("kdepim_plugin: %s() datapath=%s\n",__FUNCTION__);

    /* Allocate and initialise a kde_connection object. */
    conn = (kde_connection *)malloc(sizeof(kde_connection));
    kaddrbook_init();

    /* Return kde_connection object to the sync engine */
    return (void*)conn;
}

static void kde_finalize(void *data)
{
    kde_connection *conn = (kde_connection*)data;
    kaddrbook_quit();
    free(conn);
}

static void kde_connect(OSyncContext *ctx)
{
    kde_connection *conn = conn_for_context(ctx);
    conn->kaddrbook = kaddrbook_connect();

    if (!(conn->kaddrbook))
        //FIXME: Check if OSYNC_ERROR_DISCONNECTED is the righ error code in this case
        osync_context_report_error(ctx, OSYNC_ERROR_DISCONNECTED, "Couldn't connect to KDE addressbook");
    else
        osync_context_report_success(ctx);
}


static void kde_disconnect(OSyncContext *ctx)
{
    kde_connection *conn = conn_for_context(ctx);
    if (getenv ("MULTISYNC_DEBUG"))
        printf("kdepim_plugin: %s()\n",__FUNCTION__);

    kaddrbook_disconnect(conn->kaddrbook);

    osync_context_report_success(ctx);
}

static void kde_get_changeinfo(OSyncContext *ctx)
{
    kde_connection *conn = conn_for_context(ctx);
    if (getenv ("MULTISYNC_DEBUG"))
        printf("kdepim_plugin: %s\n",__FUNCTION__);

    //FIXME: newdbs support
    int reset = 0;
    int err = kaddrbook_get_changes(conn->kaddrbook, ctx, conn->member, reset);
    if (err) {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't access KDE addressbook");
        return;
    }
    osync_context_report_success(ctx);
    return;
}


static void kde_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    kde_connection *conn = conn_for_context(ctx);
    int err;

    if (getenv ("MULTISYNC_DEBUG"))
        printf("kdepim_plugin: %s()\n",__FUNCTION__);

    err = kaddrbook_modify(conn->kaddrbook, change); 
    kaddrbook_sync_done(conn->kaddrbook, !err);
    if (err)
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't update KDE addressbook");
    else 
        osync_context_report_success(ctx);
}


void get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "kde-sync";
    info->description = _("Plugin for the KDE 3.x Addressbook");

    info->functions.initialize = kde_initialize;
    info->functions.connect = kde_connect;
    info->functions.disconnect = kde_disconnect;
    info->functions.finalize = kde_finalize;
    info->functions.get_changeinfo = kde_get_changeinfo;
    info->functions.commit_change = kde_commit_change;
}

/* sync_object_type object_types(void)
{return(SYNC_OBJECT_TYPE_PHONEBOOK);}
*/


/*
 * int plugin_API_version(void)
{ return(3); }
*/

