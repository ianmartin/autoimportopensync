/*

   Copyright 2005 Holger Hans Peter Freyther

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


#include "opie_sync.h"

static osync_bool opie_sync_is_available( OSyncError** error)
{
    return TRUE;
}

static void* opie_sync_initialize( OSyncMember* member, OSyncError** error)
{
    return NULL;
}

static void opie_sync_finalize( void* data )
{
}

static void opie_sync_connect( OSyncContext* ctx)
{
}

static void opie_sync_sync_done( OSyncContext* ctx)
{
}

static void opie_sync_disconnect( OSyncContext* ctx)
{
}

static void opie_sync_get_changeinfo( OSyncContext* ctx )
{
}


void get_info(OSyncEnv* env )
{
    OSyncPluginInfo* info = osync_plugin_new_info(env);

    /*
     * Initial Names
     */
    info->name          = "opie-sync";
    info->longname      = "Opie Synchronization Plugin";
    info->description   = "Synchronize with Opie/Qtopia based devices";
    info->version       = 1;
    info->is_threadsafe = TRUE;
    info->config_type   = NEEDS_CONFIGURATION;


    /*
     * Function pointers
     */
    info->functions.is_available   = opie_sync_is_available;
    info->functions.initialize     = opie_sync_initialize;
    info->functions.finalize       = opie_sync_finalize;
    info->functions.connect        = opie_sync_connect;
    info->functions.disconnect     = opie_sync_disconnect;
    info->functions.sync_done      = opie_sync_sync_done;
    info->functions.get_changeinfo = opie_sync_get_changeinfo;


    /*
     *
     */
}
