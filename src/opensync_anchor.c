#include <opensync.h>
#include "opensync_internals.h"

DB *osync_anchor_load_file(char *file, OSyncGroup *group)
{
	DB *dbhandle = osync_db_open(file, "Anchor", DB_BTREE, group->dbenv);
	return dbhandle;
}

DB *osync_anchor_load(OSyncMember *member)
{
	g_assert(member);
	char *filename = g_strdup_printf ("%s/anchor.db", member->configdir);
	DB *dbhandle = osync_anchor_load_file(filename, member->group);
	g_free(filename);
	return dbhandle;
}

osync_bool osync_anchor_compare(OSyncMember *member, char *objtype, char *new_anchor)
{
	g_assert(member);
	DB *dbhandle = osync_anchor_load(member);
	
	osync_bool retval = FALSE;
	
	void *old_anchorp = NULL;
	
	if (osync_db_get(dbhandle, objtype, strlen(objtype) + 1, &old_anchorp)) {
		char *old_anchor = (char *)old_anchorp;
		if (!strcmp(old_anchor, new_anchor)) {
			retval = TRUE;
		} else {
			retval = FALSE;
		}
	} else {
		retval = FALSE;
	}
	
	osync_db_close(dbhandle);
	
	return retval;
}

void osync_anchor_update(OSyncMember *member, char *objtype, char *new_anchor)
{
	DB *dbhandle = osync_anchor_load(member);
	osync_db_put(dbhandle, objtype, strlen(objtype) + 1, new_anchor, strlen(new_anchor) + 1);
	osync_db_close(dbhandle);
}
