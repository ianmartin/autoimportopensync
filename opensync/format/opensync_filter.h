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
	/** The change should be allowed to pass (Overwrites previous action) */
	OSYNC_FILTER_ALLOW = 1,
	/** The change should be denied to pass (Overwrites previous action) */
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

typedef osync_bool (* OSyncFilterFunction) (OSyncData *data, const char *config);

OSyncFilter *osync_filter_new(const char *objtype, OSyncFilterAction action, OSyncError **error);
OSyncFilter *osync_filter_new_custom(OSyncCustomFilter *custom_filter, const char *config, OSyncFilterAction action, OSyncError **error);
void osync_filter_ref(OSyncFilter *filter);
void osync_filter_unref(OSyncFilter *filter);
void osync_filter_set_config(OSyncFilter *filter, const char *config);
const char *osync_filter_get_config(OSyncFilter *filter);
const char *osync_filter_get_objtype(OSyncFilter *filter);
OSyncFilterAction osync_filter_invoke(OSyncFilter *filter, OSyncData *data);

OSyncCustomFilter *osync_custom_filter_new(const char *objtype, const char *objformat, const char *name, OSyncFilterFunction hook, OSyncError **error);
void osync_custom_filter_ref(OSyncCustomFilter *filter);
void osync_custom_filter_unref(OSyncCustomFilter *filter);
osync_bool osync_custom_filter_invoke(OSyncCustomFilter *filter, OSyncData *data, const char *config);

#endif //_OPENSYNC_FILTER_H_
