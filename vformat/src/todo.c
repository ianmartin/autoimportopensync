/*
 * todo - A plugin for todo objects for the opensync framework
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
#include <string.h>
/** @defgroup todo_vtodo todo/vtodo data format */

static OSyncConvCmpResult compare_vtodo(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
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

static osync_bool detect_plain_as_vtodo10(const char *data, int size)
{
	osync_trace(TRACE_INTERNAL, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:1.0*BEGIN:VTODO*", data);
}

static osync_bool detect_plain_as_vtodo20(const char *data, int size)
{
	osync_trace(TRACE_INTERNAL, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:2.0*BEGIN:VTODO*", data);
}

static void create_vtodo10(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:1.0\r\nBEGIN:VTODO\r\nSUMMARY:%s\r\nEND:VTODO\r\nEND:VCALENDAR", osync_rand_str(20));
	*size = strlen(*data);	
}

static void create_vtodo20(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:2.0\r\nBEGIN:VTODO\r\nSUMMARY:%s\r\nEND:VTODO\r\nEND:VCALENDAR", osync_rand_str(20));
	*size = strlen(*data);	
}

static void destroy_vtodo(char *input, unsigned int inpsize)
{
	g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("vtodo10", "todo", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vtodo);
	osync_objformat_set_create_func(format, create_vtodo10);
	osync_objformat_set_destroy_func(format, destroy_vtodo);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	format = osync_objformat_new("vtodo20", "todo", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vtodo);
	osync_objformat_set_create_func(format, create_vtodo20);
	osync_objformat_set_destroy_func(format, destroy_vtodo);
	
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
	
	OSyncObjFormat *vtodo10 = osync_format_env_find_objformat(env, "vtodo10");
	if (!vtodo10) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vtodo10 format");
		return FALSE;
	}
	
	OSyncObjFormat *vtodo20 = osync_format_env_find_objformat(env, "vtodo20");
	if (!vtodo20) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vtodo20 format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new_detector(plain, vtodo10, detect_plain_as_vtodo10, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new_detector(plain, vtodo20, detect_plain_as_vtodo20, error);
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
