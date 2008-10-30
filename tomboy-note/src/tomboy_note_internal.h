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

#ifndef TOMBOY_NOTE_INTERNAL_H_
#define TOMBOY_NOTE_INTERNAL_H_

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <glib/gstring.h>

struct TomboyNoteSchema {
	/** The schema object */
	xmlSchemaPtr schema;
	/** The schema validation context */
	xmlSchemaValidCtxtPtr context;
};

void tomboynote_parse_content_node(xmlNodePtr node, GString * output);
void tomboynote_parse_content(xmlDocPtr doc, GString * output);

const char * tomboynote_parse_node(xmlDocPtr doc, const char * nodename);
GList * tomboynote_parse_tags(xmlDocPtr doc);

void tomboynote_validate_and_set_datetime(xmlNodePtr node);
char * tomboynote_create_datetime_now();
osync_bool tomboynote_validate_datetime(const char *datetime);

#endif /*TOMBOY_NOTE_INTERNAL_H_*/
