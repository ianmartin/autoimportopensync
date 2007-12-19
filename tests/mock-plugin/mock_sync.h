/*
 * file-sync - A plugin for the opensync framework
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

#ifndef _FILE_PLUGIN_H
#define _FILE_PLUGIN_H

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

#include "config.h"

#include "opensync/plugin/opensync_plugin_info_internals.h"

typedef struct mock_env {
	GList *directories;
	OSyncObjFormat *objformat;

	OSyncMember *member;
	osync_bool committed_all;

	int num_connect;
	int num_disconnect;
	int num_get_changes;
	int num_commit_changes;
	int num_sync_done;

	int main_connect;
	int main_disconnect;
	int main_get_changes;
	int main_sync_done;

	OSyncContext *ctx[10];
} mock_env;

typedef struct OSyncFileDir {
	char *objtype;
	char *path;
	GDir *dir;
	OSyncHashTable *hashtable;
	OSyncObjTypeSink *sink;
	osync_bool recursive;
	mock_env *env;
} OSyncFileDir;

#endif //_FILE_PLUGIN_H
