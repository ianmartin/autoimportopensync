/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * Copyright (C) 2007  Graham R. Cobb <g+opensync@cobb.uk.net>
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

#ifndef GPE_SYNC_H
#define GPE_SYNC_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <glib.h>
#include <glib-object.h>
#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

// this is needed for gpe_xml.c:
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "gpesync_client.h"

typedef struct sink_environment {
        OSyncObjTypeSink *sink;
	OSyncObjFormat *objformat;
        OSyncHashTable *hashtable;
	char *format;
  	struct gpe_environment *gpe_env;
} sink_environment;

typedef struct gpe_environment {
  //	OSyncMember *member;

	struct sink_environment contact_sink;
	struct sink_environment todo_sink;
	struct sink_environment calendar_sink;
	struct sink_environment main_sink;

	gpesync_client *client;
	
	// configuration
	char *device_addr; // the ip of the handheld;
	char *username; // The user on the handheld
	char *command; // Command to run
	int device_port;
	int use_ssh;
  	int use_local;
	int use_remote;
	char *calendar; // Name of GPE calendar to use or NULL
	
	int debuglevel;
} gpe_environment;

#include "utils.h"
#include "contacts.h"
#include "calendar.h"
#include "todo.h"
#include "gpe_xml.h"

#define GPE_CONNECT_ERROR 1
#define GPE_SQL_EXEC_ERROR 2
#define GPE_HASH_LOAD_ERROR 3

/*
 * Minimum protocol version number supported
 *
 * Major number must be equal: increment this if we cannot even parse
 * the protocol correctly any more
 *
 * Minor number must be less than or equal to the other end: increment 
 * this if a new command is vital for correct operation
 *
 * Edit number ignored but reported in logs
 */
#define MIN_PROTOCOL_MAJOR 1
#define MIN_PROTOCOL_MINOR 0

#endif
