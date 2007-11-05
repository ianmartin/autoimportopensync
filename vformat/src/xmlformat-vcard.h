/*
 * xmlformat-vcard - convert vcard* to xmlformat-contact and backwards
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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

#ifndef XMLFORMAT_VCARD_H_
#define XMLFORMAT_VCARD_H_

#include "xmlformat-common.h"

/*
typedef struct OSyncHookTables {
	GHashTable *attributes;
	GHashTable *parameters;
} OSyncHookTables;

#define HANDLE_IGNORE (void *)1
*/


osync_bool conv_vcard_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
osync_bool conv_xmlformat_to_vcard30(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);
osync_bool conv_xmlformat_to_vcard21(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error);




#endif /*XMLFORMAT_VCARD_H_*/
