#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#define segfault_me char **blablabla = NULL; *blablabla = "test";

typedef void (* OSyncFlagTriggerFunc) (gpointer user_data1, gpointer user_data2);

typedef struct OSyncFlag OSyncFlag;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMappingView OSyncMappingView;
typedef struct OSyncMappingEntry OSyncMappingEntry;

#include "opensync/opensync_message_internals.h"
#include "opensync/opensync_queue_internals.h"

#include "osengine_deciders_internals.h"
#include "osengine_debug.h"
#include "osengine_flags_internals.h"
#include "osengine_engine_internals.h"
#include "osengine_mapping_internals.h"
#include "osengine_mapcmds_internals.h"
#include "osengine_client_internals.h"
#include "osengine_debug_internals.h"
