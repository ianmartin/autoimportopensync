#include <opensync/opensync.h>

#include "tools.h"

/* A single calendar in the config */
#define TYP_FILE 1
#define TYP_WEBDAV 2

typedef struct {
   int typ; /* one of CALENDAR_CONFIG_TYP_... */
   int isdefault; /* boolean */
   GString* filename;
   GString* username; /* can be NULL if not specified */
   GString* password; /* can be NULL if not specified */
} plugin_calendar_config;

/* Main connection struct */
typedef struct {
  OSyncMember* member;
  GList* config_calendars;
  GList* pending_changes;       // The recorded changes to the key file that are pending until sync_done
} plugin_environment;
