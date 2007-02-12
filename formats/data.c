/*
 * data - A plugin for data objects for the opensync framework
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
 
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <opensync/opensync-support.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <glib.h>

static OSyncConvCmpResult compare_plain(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	/* Consider empty block equal NULL pointers */
	if (!leftsize) leftdata = NULL;
	if (!rightsize) rightdata = NULL;
	
	if (!leftdata && !rightdata)
		return OSYNC_CONV_DATA_SAME;
		
	if (leftdata && rightdata && (leftsize == rightsize)) {
		if (!memcmp(leftdata, rightdata, leftsize))
			return OSYNC_CONV_DATA_SAME;
		else
			return OSYNC_CONV_DATA_MISMATCH;
	}
	
	return OSYNC_CONV_DATA_MISMATCH;
}

static osync_bool copy_plain(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	char *r = g_malloc0(inpsize);

	memcpy(r, input, inpsize);
	*output = r;
	*outpsize = inpsize;
	return TRUE;
}

static void destroy_plain(char *input, size_t inpsize)
{
	g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("plain", "data", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_plain);
	osync_objformat_set_copy_func(format, copy_plain);
	osync_objformat_set_destroy_func(format, destroy_plain);

	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	return TRUE;
}

int get_version(void)
{
	return 1;
}
