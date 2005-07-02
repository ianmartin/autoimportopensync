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
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, member, objtype, new_anchor);
	g_assert(member);
	
	OSyncError *error = NULL;
	OSyncDB *db = NULL;
	if (!(db = osync_db_open_anchor(member, &error)))
		goto error;
	
	osync_bool retval = FALSE;
	
	char *old_anchor = NULL;
	osync_db_get_anchor(db, objtype, &old_anchor);
	
	if (old_anchor) {
		if (!strcmp(old_anchor, new_anchor)) {
			retval = TRUE;
		} else {
			osync_trace(TRACE_INTERNAL, "Anchor mismatch");
			retval = FALSE;
		}
	} else {
		osync_trace(TRACE_INTERNAL, "No previous anchor");
		retval = FALSE;
	}
	
	osync_db_close_anchor(db);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, retval);
	return retval;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
	return FALSE;
}

void osync_anchor_update(OSyncMember *member, const char *objtype, const char *new_anchor)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s)", __func__, member, objtype, new_anchor);
	g_assert(member);
	
	OSyncError *error = NULL;
	OSyncDB *db = NULL;
	if (!(db = osync_db_open_anchor(member, &error)))
		goto error;
	
	osync_db_put_anchor(db, objtype, new_anchor);
	osync_db_close_anchor(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
	return;
}

char *osync_anchor_retrieve(OSyncMember *member, const char *objtype)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, member, objtype);
	g_assert(member);
	
	OSyncError *error = NULL;
	OSyncDB *db = NULL;
	if (!(db = osync_db_open_anchor(member, &error)))
		goto error;
	
	char *anchor = NULL;
	osync_db_get_anchor(db, objtype, &anchor);
	osync_db_close_anchor(db);
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, anchor);
	return anchor;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
	return NULL;
}
