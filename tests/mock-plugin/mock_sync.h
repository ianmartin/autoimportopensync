#include <opensync/opensync.h>
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
} mock_env;
