/*
 * libopensync - A synchronization framework
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

#ifndef _OPENSYNC_FILTER_H_
#define _OPENSYNC_FILTER_H_

/*! @brief The action that should be invoked
 * @ingroup OSyncFilterAPI
 **/
typedef enum OSyncFilterAction {
	/** This filter should be ignored */
	OSYNC_FILTER_IGNORE = 0,
	/** The change should be allowed to pass (overrides previous action) */
	OSYNC_FILTER_ALLOW = 1,
	/** The change should be prevented from passing (overrides previous action) */
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

typedef osync_bool (* OSyncFilterFunction) (OSyncData *data, const char *config);

OSYNC_EXPORT OSyncFilter *osync_filter_new(const char *objtype, OSyncFilterAction action, OSyncError **error);
OSYNC_EXPORT OSyncFilter *osync_filter_new_custom(OSyncCustomFilter *custom_filter, const char *config, OSyncFilterAction action, OSyncError **error);
OSYNC_EXPORT OSyncFilter *osync_filter_ref(OSyncFilter *filter);
OSYNC_EXPORT void osync_filter_unref(OSyncFilter *filter);
OSYNC_EXPORT void osync_filter_set_config(OSyncFilter *filter, const char *config);
OSYNC_EXPORT const char *osync_filter_get_config(OSyncFilter *filter);
OSYNC_EXPORT const char *osync_filter_get_objtype(OSyncFilter *filter);
OSYNC_EXPORT OSyncFilterAction osync_filter_invoke(OSyncFilter *filter, OSyncData *data);

OSYNC_EXPORT OSyncCustomFilter *osync_custom_filter_new(const char *objtype, const char *objformat, const char *name, OSyncFilterFunction hook, OSyncError **error);
OSYNC_EXPORT OSyncCustomFilter *osync_custom_filter_ref(OSyncCustomFilter *filter);
OSYNC_EXPORT void osync_custom_filter_unref(OSyncCustomFilter *filter);
OSYNC_EXPORT osync_bool osync_custom_filter_invoke(OSyncCustomFilter *filter, OSyncData *data, const char *config);

#endif //_OPENSYNC_FILTER_H_
