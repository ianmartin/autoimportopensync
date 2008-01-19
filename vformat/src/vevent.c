/*
 * event - A plugin for event objects for the opensync framework
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
#include <opensync/opensync-support.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <glib.h>
#include <string.h>

/** @defgroup event_vevent event/vevent data format
 *
 * The vevent data should be a malloc()ed block of data. See
 * osync_env_format_set_malloced().
 *
 * It can be treated as a plain block of data. See
 * osync_env_format_set_like().
 */

static OSyncConvCmpResult compare_vevent(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
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

static osync_bool detect_plain_as_vevent10(const char *data, int size)
{
	osync_trace(TRACE_INTERNAL, "start: %s", __func__);

	if(!data)
		return FALSE;

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:1.0*BEGIN:VEVENT*", data);
}

static osync_bool detect_plain_as_vevent20(const char *data, int size)
{
	osync_trace(TRACE_INTERNAL, "start: %s", __func__);

	if(!data)
		return FALSE;

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:2.0*BEGIN:VEVENT*", data);
}

static void create_vevent10(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:1.0\r\nBEGIN:VEVENT\r\nDTSTART:20050307T124500Z\r\nDTEND:20050307T130000Z\r\nSEQUENCE:0\r\nSUMMARY:%s\r\nEND:VEVENT\r\nEND:VCALENDAR", osync_rand_str(20));
	*size = strlen(*data);
}

static void create_vevent20(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:2.0\r\nBEGIN:VEVENT\r\nDTSTART:20050307T124500Z\r\nDTEND:20050307T130000Z\r\nSEQUENCE:0\r\nSUMMARY:%s\r\nEND:VEVENT\r\nEND:VCALENDAR", osync_rand_str(20));
	*size = strlen(*data);
}

static void destroy_vevent(char *input, unsigned int inpsize)
{
	g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("vevent10", "event", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vevent);
	osync_objformat_set_create_func(format, create_vevent10);
	osync_objformat_set_destroy_func(format, destroy_vevent);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	format = osync_objformat_new("vevent20", "event", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vevent);
	osync_objformat_set_create_func(format, create_vevent20);
	osync_objformat_set_destroy_func(format, destroy_vevent);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	/* TODO
	OSyncCustomFilter *filter = osync_custom_filter_new("contact", "vcard21", "vcard_categories_filter", vcard_categories_filter, error);
	if (!filter)
		return FALSE;
	
	osync_format_env_register_filter(env, filter);
	osync_custom_filter_unref(filter);
	
	filter = osync_custom_filter_new("contact", "vcard30", "vcard_categories_filter", vcard_categories_filter, error);
	if (!filter)
		return FALSE;
	
	osync_format_env_register_filter(env, filter);
	osync_custom_filter_unref(filter);
	*/
	
	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *plain = osync_format_env_find_objformat(env, "plain");
	if (!plain) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plain format");
		return FALSE;
	}
	
	OSyncObjFormat *vevent10 = osync_format_env_find_objformat(env, "vevent10");
	if (!vevent10) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vevent10 format");
		return FALSE;
	}
	
	OSyncObjFormat *vevent20 = osync_format_env_find_objformat(env, "vevent20");
	if (!vevent20) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vevent20 format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new_detector(plain, vevent10, detect_plain_as_vevent10, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new_detector(plain, vevent20, detect_plain_as_vevent20, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	return TRUE;
}

int get_version(void)
{
	return 1;
}
