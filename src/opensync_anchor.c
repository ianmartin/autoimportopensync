#include <opensync.h>
#include "opensync_internals.h"

osync_bool osync_anchor_compare(OSyncMember *member, char *objtype, char *new_anchor)
{
	g_assert(member);
	OSyncDB *db = osync_db_open_anchor(member);
	
	osync_bool retval = FALSE;
	
	char *old_anchor = NULL;
	osync_db_get_anchor(db, objtype, &old_anchor);
	
	if (old_anchor) {
		if (!strcmp(old_anchor, new_anchor)) {
			retval = TRUE;
		} else {
			retval = FALSE;
		}
	} else {
		retval = FALSE;
	}
	
	osync_db_close_anchor(db);
	return retval;
}

void osync_anchor_update(OSyncMember *member, char *objtype, char *new_anchor)
{
	g_assert(member);
	OSyncDB *db = osync_db_open_anchor(member);
	osync_db_put_anchor(db, objtype, new_anchor);
	osync_db_close_anchor(db);
}
