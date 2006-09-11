#ifndef _OSENGINE__DEBUG_H
#define _OSENGINE__DEBUG_H

void osengine_print_all(OSyncEngine *engine);
void osengine_print_flags(OSyncEngine *engine);
void osync_client_print_flags(OSyncClient *client);
void osengine_mapping_print_flags(OSyncMapping *mapping);
void osengine_get_wasted(OSyncEngine *engine, int *all, int *wasted);

#endif
