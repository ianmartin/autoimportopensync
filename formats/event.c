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
 
#include <opensync.h>
#include <glib.h>
#include <string.h>

/** @defgroup event_vevent event/vevent data format
 *
 * The vevent data should be a malloc()ed block of data. See
 * osync_conv_format_set_malloced().
 *
 * It can be treated as a plain block of data. See
 * osync_conv_format_set_like().
 */

static OSyncConvCmpResult compare_vevent(OSyncChange *leftchange, OSyncChange *rightchange)
{
	/*FIXME: Implement me */
	return CONV_DATA_MISMATCH;
}


static const char *begin_vcalendar = "BEGIN:VCALENDAR";
static const char *begin_vevent = "\nBEGIN:VEVENT";

static osync_bool detect_plain_as_vevent(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	// first, check if it is a vcalendar
	if (size < strlen(begin_vcalendar) || strncmp(data, begin_vcalendar, strlen(begin_vcalendar)))
		return FALSE;

	// it is a vcalendar, search for BEGIN:VEVENT
	if (g_strstr_len(data, size, begin_vevent))
		return TRUE;

	return FALSE;
}


void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "event");
	OSyncObjFormat *vcal = osync_conv_register_objformat(env, "event", "vevent");
	osync_conv_format_set_compare_func(vcal, compare_vevent);

	osync_conv_register_data_detector(env, "plain", "vevent", detect_plain_as_vevent);
	osync_conv_format_set_like(vcal, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
}
