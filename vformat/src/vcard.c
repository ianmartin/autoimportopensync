/*
 * contact - A plugin for contact objects for the opensync framework
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

static OSyncConvCmpResult compare_vcard(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
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

static osync_bool detect_plain_as_vcard21(const char *data, int size)
{
	if (!data)
		return FALSE;
		
	return g_pattern_match_simple("*BEGIN:VCARD*VERSION:2.1*", data);
}

static osync_bool detect_plain_as_vcard30(const char *data, int size)
{
	if (!data)
		return FALSE;

	return g_pattern_match_simple("*BEGIN:VCARD*VERSION:3.0*", data);
}

static void create_vcard21(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCARD\r\nVERSION:2.1\r\nN:%s;%s;;;\r\nEND:VCARD\r\n", osync_rand_str(10), osync_rand_str(10));
	*size = strlen(*data);
}

static void create_vcard30(char **data, unsigned int *size)
{
	*data = g_strdup_printf("BEGIN:VCARD\r\nVERSION:3.0\r\nN:%s;%s;;;\r\nEND:VCARD\r\n", osync_rand_str(10), osync_rand_str(10));
	*size = strlen(*data);
}

static osync_bool vcard_categories_filter(OSyncData *data, const char *config)
{
	//Check what categories are supported here.
	return FALSE;
}

static void destroy_vcard(char *input, unsigned int inpsize)
{
	g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("vcard21", "contact", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vcard);
	osync_objformat_set_create_func(format, create_vcard21);
	osync_objformat_set_destroy_func(format, destroy_vcard);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
	format = osync_objformat_new("vcard30", "contact", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_compare_func(format, compare_vcard);
	osync_objformat_set_create_func(format, create_vcard30);
	osync_objformat_set_destroy_func(format, destroy_vcard);
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	
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
	
	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *plain = osync_format_env_find_objformat(env, "plain");
	if (!plain) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plain format");
		return FALSE;
	}
	
	OSyncObjFormat *vcard21 = osync_format_env_find_objformat(env, "vcard21");
	if (!vcard21) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vcard21 format");
		return FALSE;
	}
	
	OSyncObjFormat *vcard30 = osync_format_env_find_objformat(env, "vcard30");
	if (!vcard30) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vcard30 format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new_detector(plain, vcard21, detect_plain_as_vcard21, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new_detector(plain, vcard30, detect_plain_as_vcard30, error);
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
