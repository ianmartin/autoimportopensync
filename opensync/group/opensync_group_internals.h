/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2006  Armin Bauer <armin.bauer@desscon.com>
 * Copyright (C) 2008       Daniel Gollub <dgollub@suse.de>
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
 

#ifndef _OPENSYNC_GROUP_INTERNALS_H_
#define _OPENSYNC_GROUP_INTERNALS_H_

/*! @brief Represent a group of members that should be synchronized */
struct OSyncGroup {
	/** The name of the group */
	char *name;
	/** The members of the group */
	GList *members;
	/** The path, where the configuration resides */
	char *configdir;
	/** The last time this group was synchronized successfully */
	time_t last_sync;
	/** The lock file of the group */
	int lock_fd;
	/** The filters of this group */
	GList *filters;
	/** The defined resolution for this group */
	OSyncConflictResolution conflict_resolution;
	/** The winning side if the select resolution is choosen */
	int conflict_winner;
	/** The configured merger status of this group */
	osync_bool merger_enabled;
	/** The configured converter status of this group */
	osync_bool converter_enabled;
	
#ifdef OPENSYNC_UNITTESTS
	char *schemadir;
#endif /* OPENSYNC_UNITTESTS*/	
	int ref_count;
};

#ifdef OPENSYNC_UNITTESTS
OSYNC_TEST_EXPORT void osync_group_set_schemadir(OSyncGroup *group, const char *schemadir);
#endif /* OPENSYNC_UNITTESTS*/

#endif /* _OPENSYNC_GROUP_INTERNALS_H_ */
