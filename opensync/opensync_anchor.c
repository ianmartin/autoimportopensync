/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include <opensync.h>
#include "opensync_internals.h"

osync_bool osync_anchor_compare(OSyncMember *member, const char *objtype, const char *new_anchor)
{
	g_assert(member);
	OSyncError *error = NULL;
	OSyncDB *db = NULL;
	if (!(db = osync_db_open_anchor(member, &error))) {
		osync_debug("ANCH", 0, "Unable to open anchor table: %s", error->message);
		osync_error_free(&error);
		return FALSE;
	}
	
	osync_bool retval = FALSE;
	
	char *old_anchor = NULL;
	osync_db_get_anchor(db, objtype, &old_anchor);
	
	if (old_anchor) {
		if (!strcmp(old_anchor, new_anchor)) {
			retval = TRUE;
		} else {
			osync_debug("ANCH", 1, "Anchor mismatch. Requesting slow sync");
			retval = FALSE;
		}
	} else {
		osync_debug("ANCH", 2, "No previous anchor. Requesting slow sync");
		retval = FALSE;
	}
	
	osync_db_close_anchor(db);
	return retval;
}

void osync_anchor_update(OSyncMember *member, const char *objtype, const char *new_anchor)
{
	g_assert(member);
	OSyncError *error = NULL;
	OSyncDB *db = NULL;
	if (!(db = osync_db_open_anchor(member, &error))) {
		osync_debug("ANCH", 0, "Unable to open anchor table: %s", error->message);
		osync_error_free(&error);
		return;
	}
	osync_db_put_anchor(db, objtype, new_anchor);
	osync_db_close_anchor(db);
}
