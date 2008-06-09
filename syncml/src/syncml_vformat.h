
#ifndef SYNCML_VFORMAT_H
#define SYNCML_VFORMAT_H

#include"syncml_common.h"

GHashTable* get_vcard_hash();
GHashTable* get_ical_hash();

SmlBool set_capabilities(SmlPluginEnv *env, OSyncError **error);

#endif // SYNCML_VFORMAT_H
