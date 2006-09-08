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
#include <glib.h>
#include <opensync/opensync_support.h>
#include <string.h>
#include <stdio.h>

static OSyncConvCmpResult compare_vcard(OSyncChange *leftchange, OSyncChange *rightchange)
{
	int leftinpsize = osync_change_get_datasize(leftchange);
	char *leftinput = osync_change_get_data(leftchange);
	int rightinpsize = osync_change_get_datasize(rightchange);
	char *rightinput = osync_change_get_data(rightchange);
	
	if (leftinpsize == rightinpsize) {
		if (!memcmp(leftinput, rightinput, leftinpsize))
			return CONV_DATA_SAME;
	}
	
	//Get the name of the contact and compare
	//If the same, return SIMILAR
	
	return CONV_DATA_MISMATCH;
}

static osync_bool detect_plain_as_vcard21(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCARD21", 3, "start: %s", __func__);
	
	if (!data)
		return FALSE;
		
	return osync_pattern_match("*BEGIN:VCARD*VERSION:2.1*", data, size);
}

static osync_bool detect_plain_as_vcard30(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCARD30", 3, "start: %s", __func__);
	
	if (!data)
		return FALSE;

	return osync_pattern_match("*BEGIN:VCARD*VERSION:3.0*", data, size);
}

static void create_vcard21(OSyncChange *change)
{
	char *vcard = g_strdup_printf("BEGIN:VCARD\r\nVERSION:2.1\r\nN:%s;%s;;;\r\nEND:VCARD\r\n", osync_rand_str(10), osync_rand_str(10));
	osync_change_set_data(change, vcard, strlen(vcard) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

static void create_vcard30(OSyncChange *change)
{
	char *vcard = g_strdup_printf("BEGIN:VCARD\r\nVERSION:3.0\r\nN:%s;%s;;;\r\nEND:VCARD\r\n", osync_rand_str(10), osync_rand_str(10));
	osync_change_set_data(change, vcard, strlen(vcard) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

static OSyncFilterAction vcard_categories_filter(OSyncChange *change, char *config)
{
	//Check what categories are supported here.
	return OSYNC_FILTER_IGNORE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	
	osync_env_register_objformat(env, "contact", "vcard21");
	osync_env_format_set_compare_func(env, "vcard21", compare_vcard);
	osync_env_format_set_create_func(env, "vcard21", create_vcard21);
	osync_env_register_detector(env, "plain", "vcard21", detect_plain_as_vcard21);
	osync_env_register_filter_function(env, "vcard_categories_filter", "contact", "vcard21", vcard_categories_filter);

	osync_env_register_objformat(env, "contact", "vcard30");
	osync_env_format_set_compare_func(env, "vcard30", compare_vcard);
	osync_env_format_set_create_func(env, "vcard30", create_vcard30);
	osync_env_register_detector(env, "plain", "vcard30", detect_plain_as_vcard30);
	osync_env_register_filter_function(env, "vcard_categories_filter", "contact", "vcard30", vcard_categories_filter);
}
