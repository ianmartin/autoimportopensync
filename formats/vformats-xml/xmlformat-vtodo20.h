/*
 * xmlformat-ical - A plugin for parsing vevent20 objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
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

#ifndef XMLFORMAT_VTODO20_H_
#define XMLFORMAT_VTODO20_H_
#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>

#include "vformat.h"
#include "xmlformat.h"

osync_bool conv_xmlformat_to_vtodo20(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
osync_bool conv_vtodo20_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
/*
typedef struct OSyncHookTables OSyncHookTables;


struct OSyncHookTables {
	GHashTable *table;
	GHashTable *tztable;
	GHashTable *comptable;
	GHashTable *compparamtable;
	GHashTable *alarmtable;

	GHashTable *parameters;
	GHashTable *attributes;
};

#define HANDLE_IGNORE (void *)1
*/
#endif //XMLFORMAT_VTODO_20_H_

