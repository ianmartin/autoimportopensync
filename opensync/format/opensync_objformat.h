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

/*! @ingroup OSyncChangeCmds
 * @brief The possible returns of a change comparison
 */
typedef enum {
	/** The result is unknown, there was a error */
	OSYNC_CONV_DATA_UNKNOWN = 0,
	/** The changes are not the same */
	OSYNC_CONV_DATA_MISMATCH = 1,
	/** The changs are not the same but look similar */
	OSYNC_CONV_DATA_SIMILAR = 2,
	/** The changes are exactly the same */
	OSYNC_CONV_DATA_SAME = 3
} OSyncConvCmpResult;

typedef OSyncConvCmpResult (* OSyncFormatCompareFunc) (const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
typedef osync_bool (* OSyncFormatCopyFunc) (const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);
typedef void (* OSyncFormatDuplicateFunc) (const char *uid, char **newuid);
typedef void (* OSyncFormatCreateFunc) (char **data, unsigned int *size);
typedef void (* OSyncFormatDestroyFunc) (char *data, unsigned int size);
typedef char *(* OSyncFormatPrintFunc) (const char *data, unsigned int size);
typedef time_t (* OSyncFormatRevisionFunc) (const char *data, unsigned int size, OSyncError **error);
typedef osync_bool (* OSyncFormatMarshalFunc) (const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);
typedef osync_bool (* OSyncFormatDemarshalFunc) (const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);

OSyncObjFormat *osync_objformat_new(const char *name, const char *objtype_name, OSyncError **error);
void osync_objformat_ref(OSyncObjFormat *format);
void osync_objformat_unref(OSyncObjFormat *format);

const char *osync_objformat_get_name(OSyncObjFormat *format);
const char *osync_objformat_get_objtype(OSyncObjFormat *format);

void osync_objformat_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func);
void osync_objformat_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func);
void osync_objformat_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func);
void osync_objformat_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func);
void osync_objformat_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func);
void osync_objformat_set_print_func(OSyncObjFormat *format, OSyncFormatPrintFunc print_func);
void osync_objformat_set_revision_func(OSyncObjFormat *format, OSyncFormatRevisionFunc revision_func);
void osync_objformat_set_marshal_func(OSyncObjFormat *format, OSyncFormatMarshalFunc marshal_func);
void osync_objformat_set_demarshal_func(OSyncObjFormat *format, OSyncFormatDemarshalFunc marshal_func);

OSyncConvCmpResult osync_objformat_compare(OSyncObjFormat *format, const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
void osync_objformat_duplicate(OSyncObjFormat *format, const char *uid, char **newuid);
void osync_objformat_create(OSyncObjFormat *format, char **data, unsigned int *size);
char *osync_objformat_print(OSyncObjFormat *format, const char *data, unsigned int size);
time_t osync_objformat_get_revision(OSyncObjFormat *format, const char *data, unsigned int size, OSyncError **error);
void osync_objformat_destroy(OSyncObjFormat *format, char *data, unsigned int size);
osync_bool osync_objformat_copy(OSyncObjFormat *format, const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error);
osync_bool osync_objformat_is_equal(OSyncObjFormat *leftformat, OSyncObjFormat *rightformat);

osync_bool osync_objformat_must_marshal(OSyncObjFormat *format);
osync_bool osync_objformat_marshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);
osync_bool osync_objformat_demarshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);

#endif //_OPENSYNC_OBJFORMAT_H_
