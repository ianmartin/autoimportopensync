/*
 * xmlformat - registration of xml object formats 
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

#ifndef XMLFORMAT_H_
#define XMLFORMAT_H_

#include <stdio.h>
#include <string.h>

#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <opensync/opensync-xmlformat.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>

#define HANDLE_IGNORE (void *)1

/**
 * @brief Holds all information which will be needed to compaire two xmlfields. 
 * @ingroup OSyncXMLFormatAPI
 */
typedef struct OSyncXMLPoints {
	/** The name of a xmlfield */
	char *fieldname;
	/** The points for this xmlfield */
	int points;
	/** The keys of a xmlfield which have to be the same for equality. This array must end with NULL. */
	char** keys;
} OSyncXMLPoints;

void destroy_xmlformat(char *input, unsigned int inpsize);
osync_bool copy_xmlformat(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);
char *print_xmlformat(const char *data, unsigned int size);

osync_bool marshal_xmlformat(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error);
osync_bool demarshal_xmlformat(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error);

int xmlformat_get_points(OSyncXMLPoints points[], int* cur_pos, int basic_points, const char* fieldname);

OSyncConvCmpResult xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints points[], int basic_points, int treshold);

#endif /* XMLFORMAT_H_ */

