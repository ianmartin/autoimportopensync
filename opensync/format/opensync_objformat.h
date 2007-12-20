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

#ifndef _OPENSYNC_OBJFORMAT_H_
#define _OPENSYNC_OBJFORMAT_H_

typedef OSyncConvCmpResult (* OSyncFormatCompareFunc) (const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
typedef osync_bool (* OSyncFormatCopyFunc) (const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);
typedef osync_bool (* OSyncFormatDuplicateFunc) (const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error);
typedef void (* OSyncFormatCreateFunc) (char **data, unsigned int *size);
typedef void (* OSyncFormatDestroyFunc) (char *data, unsigned int size);
typedef char *(* OSyncFormatPrintFunc) (const char *data, unsigned int size);
typedef time_t (* OSyncFormatRevisionFunc) (const char *data, unsigned int size, OSyncError **error);
typedef osync_bool (* OSyncFormatMarshalFunc) (const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error);
typedef osync_bool (* OSyncFormatDemarshalFunc) (OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error);

OSYNC_EXPORT OSyncObjFormat *osync_objformat_new(const char *name, const char *objtype_name, OSyncError **error);
OSYNC_EXPORT OSyncObjFormat *osync_objformat_ref(OSyncObjFormat *format);
OSYNC_EXPORT void osync_objformat_unref(OSyncObjFormat *format);

OSYNC_EXPORT const char *osync_objformat_get_name(OSyncObjFormat *format);
OSYNC_EXPORT const char *osync_objformat_get_objtype(OSyncObjFormat *format);
OSYNC_EXPORT const char *osync_objformat_get_config(OSyncObjFormat *format);
OSYNC_EXPORT void osync_objformat_set_config(OSyncObjFormat *format, const char *format_config);

OSYNC_EXPORT void osync_objformat_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func);
OSYNC_EXPORT void osync_objformat_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func);
OSYNC_EXPORT void osync_objformat_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func);
OSYNC_EXPORT void osync_objformat_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func);
OSYNC_EXPORT void osync_objformat_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func);
OSYNC_EXPORT void osync_objformat_set_print_func(OSyncObjFormat *format, OSyncFormatPrintFunc print_func);
OSYNC_EXPORT void osync_objformat_set_revision_func(OSyncObjFormat *format, OSyncFormatRevisionFunc revision_func);
OSYNC_EXPORT void osync_objformat_set_marshal_func(OSyncObjFormat *format, OSyncFormatMarshalFunc marshal_func);
OSYNC_EXPORT void osync_objformat_set_demarshal_func(OSyncObjFormat *format, OSyncFormatDemarshalFunc marshal_func);

OSYNC_EXPORT OSyncConvCmpResult osync_objformat_compare(OSyncObjFormat *format, const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
osync_bool osync_objformat_duplicate(OSyncObjFormat *format, const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error);
OSYNC_EXPORT void osync_objformat_create(OSyncObjFormat *format, char **data, unsigned int *size);
OSYNC_EXPORT char *osync_objformat_print(OSyncObjFormat *format, const char *data, unsigned int size);
OSYNC_EXPORT time_t osync_objformat_get_revision(OSyncObjFormat *format, const char *data, unsigned int size, OSyncError **error);
OSYNC_EXPORT void osync_objformat_destroy(OSyncObjFormat *format, char *data, unsigned int size);
OSYNC_EXPORT osync_bool osync_objformat_copy(OSyncObjFormat *format, const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error);
OSYNC_EXPORT osync_bool osync_objformat_is_equal(OSyncObjFormat *leftformat, OSyncObjFormat *rightformat);

OSYNC_EXPORT osync_bool osync_objformat_must_marshal(OSyncObjFormat *format);
OSYNC_EXPORT osync_bool osync_objformat_marshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error);
OSYNC_EXPORT osync_bool osync_objformat_demarshal(OSyncObjFormat *format, OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error);

#endif //_OPENSYNC_OBJFORMAT_H_
