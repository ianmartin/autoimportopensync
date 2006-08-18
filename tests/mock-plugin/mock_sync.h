#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>

#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>

#include <sys/stat.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

typedef struct mock_dir {
	char *objtype;
	char *path;
	GDir *dir;
	OSyncHashTable *hashtable;
} mock_dir;

typedef struct mock_env {
	GList *directories;
	
	OSyncMember *member;
	osync_bool committed_all;
	
	int num_connect;
	int num_disconnect;
	int num_get_changes;
	int num_commit_changes;
	int num_sync_done;
	
	int main_connect;
	int main_disconnect;
	int main_get_changes;
	int main_sync_done;
	
	OSyncContext *ctx[10];
} mock_env;
