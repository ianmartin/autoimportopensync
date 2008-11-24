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

#ifndef _OPENSYNC_DATA_H_
#define _OPENSYNC_DATA_H_

OSYNC_EXPORT OSyncData *osync_data_new(char *data, unsigned int size, OSyncObjFormat *format, OSyncError **error);
OSYNC_EXPORT OSyncData *osync_data_ref(OSyncData *data);
OSYNC_EXPORT void osync_data_unref(OSyncData *data);

OSYNC_EXPORT OSyncObjFormat *osync_data_get_objformat(OSyncData *data);
OSYNC_EXPORT void osync_data_set_objformat(OSyncData *data, OSyncObjFormat *objformat);

OSYNC_EXPORT const char *osync_data_get_objtype(OSyncData *data);
OSYNC_EXPORT void osync_data_set_objtype(OSyncData *data, const char *objtype);

OSYNC_EXPORT void osync_data_get_data(OSyncData *data, char **buffer, unsigned int *size);
OSYNC_EXPORT void osync_data_set_data(OSyncData *data, char *buffer, unsigned int size);

OSYNC_EXPORT osync_bool osync_data_has_data(OSyncData *data);

OSYNC_EXPORT char *osync_data_get_printable(OSyncData *data);


OSYNC_EXPORT OSyncData *osync_data_clone(OSyncData *data, OSyncError **error);
OSYNC_EXPORT time_t osync_data_get_revision(OSyncData *data, OSyncError **error);

#endif /* _OPENSYNC_DATA_H_ */

