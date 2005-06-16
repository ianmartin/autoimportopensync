#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

typedef struct ITMessage ITMessage;
typedef struct ITMQueue ITMQueue;

#define segfault_me char **blablabla = NULL; *blablabla = "test";

typedef void (* OSyncFlagTriggerFunc) (gpointer user_data1, gpointer user_data2);

typedef struct OSyncFlag OSyncFlag;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMappingView OSyncMappingView;
typedef struct OSyncMappingEntry OSyncMappingEntry;
typedef struct timeout_info timeout_info;


#include "osengine_deciders_internals.h"
#include "osengine_message_internals.h"
#include "osengine_queue_internals.h"
#include "osengine_debug.h"
#include "osengine_flags_internals.h"
#include "osengine_engine_internals.h"
#include "osengine_mapping_internals.h"
#include "osengine_mapcmds_internals.h"
#include "osengine_client_internals.h"
#include "osengine_debug_internals.h"
