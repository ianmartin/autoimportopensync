/*
 * xmlformat-ical - A plugin for parsing vevent20 objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
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

#ifndef XMLFORMAT_ICAL_H_
#define XMLFORMAT_ICAL_H_
#include "xmlformat-event.h"

osync_bool conv_xmlformat_to_ical(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
osync_bool conv_ical_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
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
#endif //XMLFORMAT_ICAL_H_

