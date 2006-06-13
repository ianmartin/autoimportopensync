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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-format.h"
#include "opensync_objformat_internals.h"

OSyncObjFormat *osync_objformat_new(const char *name, const char *objtype_name, OSyncError **error)
{
	OSyncObjFormat *format = osync_try_malloc0(sizeof(OSyncObjFormat), error);
	if (!format)
		return FALSE;
	
	format->name = g_strdup(name);
	format->objtype_name = g_strdup(objtype_name);
	format->ref_count = 1;
	
	return format;
}

void osync_objformat_ref(OSyncObjFormat *format)
{
	osync_assert(format);
	
	g_atomic_int_inc(&(format->ref_count));
}

void osync_objformat_unref(OSyncObjFormat *format)
{
	osync_assert(format);
	
	if (g_atomic_int_dec_and_test(&(format->ref_count))) {
		if (format->name)
			g_free(format->name);
			
		if (format->objtype_name)
			g_free(format->objtype_name);
		
		g_free(format);
	}
}

const char *osync_objformat_get_name(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->name;
}

const char *osync_objformat_get_objtype(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->objtype_name;
}

osync_bool osync_objformat_is_equal(OSyncObjFormat *leftformat, OSyncObjFormat *rightformat)
{
	osync_assert(leftformat);
	osync_assert(rightformat);
	
	return (!strcmp(leftformat->name, rightformat->name)) ? TRUE : FALSE;
}

void osync_objformat_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func)
{
	osync_assert(format);
	format->cmp_func = cmp_func;
}

OSyncConvCmpResult osync_objformat_compare(OSyncObjFormat *format, const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_assert(format);
	return format->cmp_func(leftdata, leftsize, rightdata, rightsize);
}

void osync_objformat_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func)
{
	osync_assert(format);
	format->destroy_func = destroy_func;
}

void osync_objformat_destroy(OSyncObjFormat *format, char *data, unsigned int size)
{
	osync_assert(format);
	
	if (!format->destroy_func) {
		osync_trace(TRACE_INTERNAL, "Format %s don't have a destroy function. Possible memory leak", format->name);
		return;
	}
	
	format->destroy_func(data, size);
}

void osync_objformat_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func)
{
	osync_assert(format);
	format->copy_func = copy_func;
}

osync_bool osync_objformat_copy(OSyncObjFormat *format, const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error)
{
	osync_assert(format);
	osync_assert(indata);
	osync_assert(outdata);

	if (!format->copy_func) {
		osync_trace(TRACE_INTERNAL, "We cannot copy the change, falling back to memcpy");
		*outdata = osync_try_malloc0(sizeof(char) * insize, error);
		if (!*outdata)
			return FALSE;
			
		memcpy(*outdata, indata, insize);
		*outsize = insize;
	} else {
		if (!format->copy_func(indata, insize, outdata, outsize, error)) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Something went wrong during copying");
			return FALSE;
		}
	}
	return TRUE;
}

void osync_objformat_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func)
{
	osync_assert(format);
	format->duplicate_func = dupe_func;
}

void osync_objformat_duplicate(OSyncObjFormat *format, const char *uid, char **newuid)
{
	osync_assert(format);
	format->duplicate_func(uid, newuid);
}

void osync_objformat_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func)
{
	osync_assert(format);
	format->create_func = create_func;
}

void osync_objformat_create(OSyncObjFormat *format, char **data, unsigned int *size)
{
	osync_assert(format);
	format->create_func(data, size);
}

void osync_objformat_set_print_func(OSyncObjFormat *format, OSyncFormatPrintFunc print_func)
{
	osync_assert(format);
	format->print_func = print_func;
}

char *osync_objformat_print(OSyncObjFormat *format, const char *data, unsigned int size)
{
	osync_assert(format);
	return format->print_func(data, size);
}

void osync_objformat_set_revision_func(OSyncObjFormat *format, OSyncFormatRevisionFunc revision_func)
{
	osync_assert(format);
	format->revision_func = revision_func;
}

time_t osync_objformat_get_revision(OSyncObjFormat *format, const char *data, unsigned int size, OSyncError **error)
{
	osync_assert(format);
	return format->revision_func(data, size, error);
}

void osync_objformat_set_marshal_func(OSyncObjFormat *format, OSyncFormatMarshalFunc marshal_func)
{
	osync_assert(format);
	format->marshal_func = marshal_func;
}

osync_bool osync_objformat_must_marshal(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->marshal_func ? TRUE : FALSE;
}

osync_bool osync_objformat_marshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	osync_assert(format);
	osync_assert(format->marshal_func);
	return format->marshal_func(input, inpsize, output, outpsize, error);
}

void osync_objformat_set_demarshal_func(OSyncObjFormat *format, OSyncFormatDemarshalFunc demarshal_func)
{
	osync_assert(format);
	format->demarshal_func = demarshal_func;
}

osync_bool osync_objformat_demarshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	osync_assert(format);
	osync_assert(format->demarshal_func);
	return format->demarshal_func(input, inpsize, output, outpsize, error);
}
