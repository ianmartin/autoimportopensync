/*
 * todo - A plugin for todo objects for the opensync framework
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
 
#include "opensync/opensync.h"
#include <glib.h>
#include <string.h>

static OSyncConvCmpResult compare_vtodo(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

static osync_bool detect_plain_as_vtodo10(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:1.0*BEGIN:VTODO*", data);
}

static osync_bool detect_plain_as_vtodo20(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:2.0*BEGIN:VTODO*", data);
}

static void create_todo10(OSyncChange *change)
{
	char *vtodo = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:1.0\r\nBEGIN:VTODO\r\nSUMMARY:%s\r\nEND:VTODO\r\nEND:VCALENDAR", osync_rand_str(20));
	
	osync_change_set_data(change, vtodo, strlen(vtodo) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

static void create_todo20(OSyncChange *change)
{
	char *vtodo = g_strdup_printf("BEGIN:VCALENDAR\r\nPRODID:-//OpenSync//NONSGML OpenSync TestGenerator//EN\r\nVERSION:2.0\r\nBEGIN:VTODO\r\nSUMMARY:%s\r\nEND:VTODO\r\nEND:VCALENDAR", osync_rand_str(20));
	
	osync_change_set_data(change, vtodo, strlen(vtodo) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "todo");
	
	osync_env_register_objformat(env, "todo", "vtodo10");
	osync_env_format_set_compare_func(env, "vtodo10", compare_vtodo);
	osync_env_format_set_create_func(env, "vtodo10", create_todo10);
	osync_env_register_detector(env, "plain", "vtodo10", detect_plain_as_vtodo10);
	
	osync_env_register_objformat(env, "todo", "vtodo20");
	osync_env_format_set_compare_func(env, "vtodo20", compare_vtodo);
	osync_env_format_set_create_func(env, "vtodo20", create_todo20);
	osync_env_register_detector(env, "plain", "vtodo20", detect_plain_as_vtodo20);
}
