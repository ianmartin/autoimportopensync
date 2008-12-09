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

#ifndef _OPENSYNC_OBJFORMAT_INTERNALS_H_
#define _OPENSYNC_OBJFORMAT_INTERNALS_H_

OSYNC_TEST_EXPORT OSyncConvCmpResult osync_objformat_compare(OSyncObjFormat *format, const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
OSYNC_TEST_EXPORT osync_bool osync_objformat_duplicate(OSyncObjFormat *format, const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error);
OSYNC_TEST_EXPORT void osync_objformat_create(OSyncObjFormat *format, char **data, unsigned int *size);


OSYNC_TEST_EXPORT void osync_objformat_destroy(OSyncObjFormat *format, char *data, unsigned int size);

OSYNC_TEST_EXPORT osync_bool osync_objformat_copy(OSyncObjFormat *format, const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error);

OSYNC_TEST_EXPORT osync_bool osync_objformat_is_equal(OSyncObjFormat *leftformat, OSyncObjFormat *rightformat);
OSYNC_TEST_EXPORT osync_bool osync_objformat_must_marshal(OSyncObjFormat *format);
OSYNC_TEST_EXPORT osync_bool osync_objformat_marshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error);
OSYNC_TEST_EXPORT osync_bool osync_objformat_demarshal(OSyncObjFormat *format, OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error);
osync_bool osync_objformat_validate(OSyncObjFormat *format, const char *data, unsigned int size, OSyncError **error);
osync_bool osync_objformat_must_validate(OSyncObjFormat *format);

#endif /* _OPENSYNC_OBJFORMAT_INTERNALS_H_ */

