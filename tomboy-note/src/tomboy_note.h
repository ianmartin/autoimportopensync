/*
 * tomboy_note - convert tomboy notes to xmlformat-note and backwards
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@gmail.com>
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

#ifndef TOMBOY_NOTE_H_
#define TOMBOY_NOTE_H_

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-xmlformat.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>

#include <glib.h>

#define TOMBOY_FORMAT_OPENSYNC_PLUGINVERSION 1

/*
 * An overview of the tomboy format is found at http://live.gnome.org/Tomboy/NoteXmlFormat
 */

typedef struct TomboyNoteSchema TomboyNoteSchema;

osync_bool tomboynote_validate(xmlDocPtr doc, xmlSchemaValidCtxtPtr context);
osync_bool conv_tomboynote_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error);
osync_bool conv_xmlformat_to_tomboynote(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error);
osync_bool detect_tomboynote(const char *data, int size, void *userdata);

void* tomboynote_initialize(OSyncError **error);
void tomboynote_finalize(void *userdata);

#endif /*TOMBOY_NOTE_H_*/
