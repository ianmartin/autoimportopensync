#ifndef _SYNCML_PLUGIN_DS_SERVER_H
#define _SYNCML_PLUGIN_DS_SERVER_H

#include "syncml_common.h"

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

#endif //_SYNCML_PLUGIN_DS_SERVER_H
