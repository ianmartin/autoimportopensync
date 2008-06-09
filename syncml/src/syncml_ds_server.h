#ifndef _SYNCML_PLUGIN_DS_SERVER_H
#define _SYNCML_PLUGIN_DS_SERVER_H

#include "syncml_common.h"

void ds_server_register_sync_mode(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx);

void ds_server_init_sync_mode(SmlDatabase *database);

void ds_server_get_changeinfo(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx);

void ds_server_batch_commit(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx,
         OSyncContext **contexts,
         OSyncChange **changes);

SmlBool _ds_server_recv_alert(
            SmlDsSession *dsession,
            SmlAlertType type,
            const char *last,
            const char *next,
            void *userdata);

SmlBool ds_server_init_databases(
            SmlPluginEnv *env,
            OSyncPluginInfo *info,
            OSyncError **error);

#endif //_SYNCML_PLUGIN_DS_SERVER_H
