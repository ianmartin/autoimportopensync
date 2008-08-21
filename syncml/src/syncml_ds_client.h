#ifndef _SYNCML_PLUGIN_DS_CLIENT_H
#define _SYNCML_PLUGIN_DS_CLIENT_H

#include "syncml_common.h"

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

#endif //_SYNCML_PLUGIN_DS_CLIENT_H
