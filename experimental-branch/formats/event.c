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

static OSyncConvCmpResult compare_vevent(OSyncChange *leftchange, OSyncChange *rightchange)
{
	/*FIXME: Implement me */
	return CONV_DATA_MISMATCH;
}

static osync_bool detect_plain_as_vevent10(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:1.0*BEGIN:VEVENT*", data);
}

static osync_bool detect_plain_as_vevent20(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	return g_pattern_match_simple("*BEGIN:VCALENDAR*VERSION:2.0*BEGIN:VEVENT*", data);
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "event");
	
	osync_env_register_objformat(env, "event", "vevent10");
	osync_env_format_set_compare_func(env, "vevent10", compare_vevent);
	osync_env_register_detector(env, "plain", "vevent10", detect_plain_as_vevent10);
	
	osync_env_register_objformat(env, "event", "vevent20");
	osync_env_format_set_compare_func(env, "vevent20", compare_vevent);
	osync_env_register_detector(env, "plain", "vevent20", detect_plain_as_vevent20);
}
