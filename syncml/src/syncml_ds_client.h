#ifndef _SYNCML_PLUGIN_DS_CLIENT_H
#define _SYNCML_PLUGIN_DS_CLIENT_H

#include "syncml_common.h"

void ds_client_register_sync_mode(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx);

void ds_client_init_sync_mode(SmlDatabase *database);

void ds_client_get_changeinfo(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx);

void ds_client_batch_commit(
         void *data,
         OSyncPluginInfo *info,
         OSyncContext *ctx,
         OSyncContext **contexts,
         OSyncChange **changes);

SmlBool _ds_client_recv_alert(
            SmlDsSession *dsession,
            SmlAlertType type,
            const char *last,
            const char *next,
            void *userdata);

#endif //_SYNCML_PLUGIN_DS_CLIENT_H
