/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#include "opensync-plugin.h"
#include "opensync_plugin_localization_private.h"

OSyncPluginLocalization *osync_plugin_localization_new(OSyncError **error)
{
	OSyncPluginLocalization *local = osync_try_malloc0(sizeof(OSyncPluginLocalization), error);
	if (!local)
		return NULL;

	local->ref_count = 1;

	return local;
}

OSyncPluginLocalization *osync_plugin_localization_ref(OSyncPluginLocalization *local)
{
	osync_assert(local);
	
	g_atomic_int_inc(&(local->ref_count));

	return local;
}

void osync_plugin_localization_unref(OSyncPluginLocalization *local)
{
	osync_assert(local);

	if (g_atomic_int_dec_and_test(&(local->ref_count))) {
		if (local->encoding)
			g_free(local->encoding);

		if (local->language)
			g_free(local->language);
			
		if (local->timezone)
			g_free(local->timezone);
			
		g_free(local);
	}
}

osync_bool osync_plugin_localization_option_is_supported(OSyncPluginLocalization *local, OSyncPluginLocalizationOptionSupportedFlag flag)
{
	osync_assert(local);
	if (local->supported_options & flag)
		return TRUE;

	return FALSE;
}

void osync_plugin_localization_option_set_supported(OSyncPluginLocalization *local, OSyncPluginLocalizationOptionSupportedFlags flags)
{
	osync_assert(local);
	local->supported_options = flags;
}

const char *osync_plugin_localization_get_encoding(OSyncPluginLocalization *local)
{
	osync_assert(local);
	return local->encoding;
}

void osync_plugin_localization_set_encoding(OSyncPluginLocalization *local, const char *encoding)
{
	osync_assert(local);
	if (local->encoding)
		g_free(local->encoding);

	local->encoding = g_strdup(encoding);
}

const char *osync_plugin_localization_get_timezone(OSyncPluginLocalization *local)
{
	osync_assert(local);
	return local->timezone;
}

void osync_plugin_localization_set_timezone(OSyncPluginLocalization *local, const char *timezone)
{
	osync_assert(local);
	if (local->timezone)
		g_free(local->timezone);

	local->timezone = g_strdup(timezone);
}

const char *osync_plugin_localization_get_language(OSyncPluginLocalization *local)
{
	osync_assert(local);
	return local->language;
}

void osync_plugin_localization_set_language(OSyncPluginLocalization *local, const char *language)
{
	osync_assert(local);
	if (local->language)
		g_free(local->language);

	local->language = g_strdup(language);
}

