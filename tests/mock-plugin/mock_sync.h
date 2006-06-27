#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>

#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>

#include <sys/stat.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <config.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

typedef struct mock_env {
        char *path;
        OSyncMember *member;
        GDir *dir;
        OSyncHashTable *hashtable;
        osync_bool committed_all;
} mock_env;
