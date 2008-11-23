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

#ifndef _OPENSYNC_UPDATER_PRIVATE_H
#define _OPENSYNC_UPDATER_PRIVATE_H 

#define OSYNC_UPDATER_SUFFIX ".xsl"

typedef enum {
	OSYNC_UPDATER_UNKOWN,
	OSYNC_UPDATER_PROCESSING_MEMBER_CONFIG,
	OSYNC_UPDATER_PROCESSING_MEMBER_DATABASE,
	OSYNC_UPDATER_PROCESSING_GROUP_CONFIG,
	OSYNC_UPDATER_PROCESSING_GROUP_DATABASE,
	OSYNC_UPDATER_NUM
} OSyncUpdaterEvent; 

struct OSyncUpdater {
	/* reference counter for OSyncUpdater */
	int ref_count;

	/* Error stack for errors during update process */
	OSyncError *error;

	/* The OSyncGroup which are handled */
	OSyncGroup *group;

	/* Updates Directory (by default: OPENSYNC_UPDATESDIR) */
	char *updatesdir;

	/* OSyncUpdater status callback */
	osync_updater_cb status_callback;

	GCond *updating;
	GMutex *updating_mutex;

	OSyncThread *thread;
	GMainContext *context;

	int member_version;
	int group_version;
	int plugin_version;

};

struct OSyncUpdaterStatus {
	/** The type of the status update */
	OSyncUpdaterEvent type;
	/** The member for which the status update is, on NULL it's about the group */
	OSyncMember *member;
	/** If the status was a error, this error will be set */
	OSyncError *error;
};

#endif /*  _OPENSYNC_UPDATER_PRIVATE_H */

