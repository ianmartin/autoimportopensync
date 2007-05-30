/*
 * note - A plugin for note objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
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
#include "opensync/opensync.h"
#include "opensync/opensync_internals.h"
#include "opensync/opensync-support.h"
#include "opensync/opensync-serializer.h"
#include "opensync/opensync-format.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

static OSyncConvCmpResult compare_vnote(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
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

static osync_bool detect_plain_as_vnote(const char *data, int size)
{
	osync_trace(TRACE_INTERNAL, "start:%s", __func__);
	
	if (!data)
		return FALSE;

	return g_pattern_match_simple("*BEGIN:VNOTE*VERSION:1.1*", data);
}

static void create_vnote(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VNOTE\r\nVERSION:1.1\r\nBODY:%s\r\nSUMMARY:%s\r\nEND:VNOTE", osync_rand_str(20), osync_rand_str(6));
	*size = strlen(*data);	
}

static void destroy_vnote(char *input, unsigned int inpsize)
{
	g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("vnote11", "note", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vnote);
	osync_objformat_set_create_func(format, create_vnote);
	osync_objformat_set_destroy_func(format, destroy_vnote);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *plain = osync_format_env_find_objformat(env, "plain");
	if (!plain) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plain format");
		return FALSE;
	}
	
	OSyncObjFormat *vnote = osync_format_env_find_objformat(env, "vnote11");
	if (!vnote) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vnote11 format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new_detector(plain, vnote, detect_plain_as_vnote, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	return TRUE;
}
	
int get_version (void)
{
	return 1;
}
