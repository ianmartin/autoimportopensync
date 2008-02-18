
#ifndef SYNCML_VFORMAT_H
#define SYNCML_VFORMAT_H

#include"syncml_common.h"

GHashTable* get_vcard_hash();
GHashTable* get_ical_hash();

SmlBool set_capabilties(SmlPluginEnv *env, SmlError **error);

#endif // SYNCML_VFORMAT_H
