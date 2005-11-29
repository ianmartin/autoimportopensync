/*
 * OpenSync SynCE plugin
 *
 * Copyright © 2005 by MirKuZ
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
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
 */

#ifndef	_OPENSYNC_SYNCE_PLUGIN_H_
#define	_OPENSYNC_SYNCE_PLUGIN_H_

#include <rra/syncmgr.h>
#include <rra/timezone.h>

typedef struct ids_list {
	uint32_t changed_count;
	uint32_t unchanged_count;
	uint32_t deleted_count;
	uint32_t *changed_ids;
	uint32_t *unchanged_ids;
	uint32_t *deleted_ids;
	RRA_SyncMgrType *type;
} ids_list;

typedef struct SyncePluginPtr {
	OSyncMember	*member;
	OSyncHashTable	*hashtable;	/* Need a hash for the file sync part. */

	RRA_SyncMgr*	syncmgr;	/* This is the connection to SynCE */
	RRA_Timezone	timezone;
	int		last_change_counter;
	int		change_counter;
	ids_list*	contact_ids;
	ids_list*	todo_ids;
	ids_list*	cal_ids;

	/* Configuration */
	osync_bool	config_contacts, config_todos, config_calendar;
	char		*config_file;
} SyncePluginPtr;

extern osync_bool synce_parse_settings(SyncePluginPtr *env, char *data, int size, OSyncError **error);

#endif
