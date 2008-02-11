/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008 Daniel Gollub <dgollub@suse.de> 
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

#ifndef _OPENSYNC_UPDATER_H_
#define _OPENSYNC_UPDATER_H_

/*! @brief Updater
 * 
 * @ingroup OSyncUpdaterAPI
 **/

typedef struct OSyncUpdaterStatus OSyncUpdaterStatus; 
typedef struct OSyncUpdater OSyncUpdater;

typedef void (* osync_updater_cb) (OSyncUpdater *updater, OSyncUpdaterStatus *status);

OSYNC_EXPORT OSyncUpdater *osync_updater_new(OSyncGroup *group, OSyncError **error);
OSYNC_EXPORT void osync_updater_unref(OSyncUpdater *updater);
OSYNC_EXPORT OSyncUpdater *osync_updater_ref(OSyncUpdater *updater);

OSYNC_EXPORT void osync_updater_set_callback(OSyncUpdater *updater, osync_updater_cb callback);

OSYNC_EXPORT osync_bool osync_updater_action_required(OSyncUpdater *updater);

OSYNC_EXPORT osync_bool osync_updater_process(OSyncUpdater *updater, OSyncError **error);
OSYNC_EXPORT osync_bool osync_updater_process_and_block(OSyncUpdater *updater, OSyncError **error);

OSYNC_EXPORT char *osync_updater_get_version(OSyncUpdater *updater);
OSYNC_EXPORT int osync_updater_get_major_version(OSyncUpdater *updater);
OSYNC_EXPORT int osync_updater_get_minor_version(OSyncUpdater *updater);

#endif /* _OPENSYNC_UPDATER_H_ */
