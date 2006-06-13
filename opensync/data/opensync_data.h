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

OSyncData *osync_data_new(char *data, unsigned int size, OSyncObjFormat *format, OSyncError **error);
void osync_data_ref(OSyncData *data);
void osync_data_unref(OSyncData *data);

OSyncObjFormat *osync_data_get_objformat(OSyncData *data);
void osync_data_set_objformat(OSyncData *data, OSyncObjFormat *objformat);
const char *osync_data_get_objtype(OSyncData *data);
void osync_data_set_objtype(OSyncData *data, const char *objtype);

void osync_data_get_data(OSyncData *data, char **buffer, unsigned int *size);
void osync_data_steal_data(OSyncData *data, char **buffer, unsigned int *size);
void osync_data_set_data(OSyncData *data, char *buffer, unsigned int size);
osync_bool osync_data_has_data(OSyncData *data);

OSyncData *osync_data_clone(OSyncData *data, OSyncError **error);

#endif //_OPENSYNC_DATA_H_
