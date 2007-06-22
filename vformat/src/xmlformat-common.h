/*
 * xmlformat-common - common code for all xmlformat converter 
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
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

#ifndef XMLFORMAT_COMMON_H_
#define XMLFORMAT_COMMON_H_

#include <stdio.h>
#include <string.h>

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>
#include <opensync/opensync_xml.h>

#include "vformat.h"

#define HANDLE_IGNORE (void *)1
#define osync_assert(x) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: Assertion \"" #x "\" failed\n", __FILE__, __LINE__, __func__); abort();}

typedef struct OSyncHookTables {
	GHashTable *attributes;
	GHashTable *parameters;
	GHashTable *tztable; // hashtable for VTIMEZONE handler
	GHashTable *alarmtable; // hashtable for VALARM handler
} OSyncHookTables;

/*** PARAMETER ***/
void handle_value_parameter(OSyncXMLField *xmlfield, VFormatParam *param);

/** VFormat Attributes **/
OSyncXMLField *handle_attribute_simple_content(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *name, OSyncError **error); 
OSyncXMLField *handle_categories_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_class_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);
OSyncXMLField *handle_uid_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 
OSyncXMLField *handle_url_attribute(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error); 

/** XML Attributes **/
VFormatAttribute *handle_xml_attribute_simple_content(VFormat *vformat, OSyncXMLField *xmlfield, const char *name, const char *encoding);
VFormatAttribute *handle_xml_categories_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_class_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_uid_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding);
VFormatAttribute *handle_xml_url_attribute(VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding);

osync_bool needs_encoding(const unsigned char *tmp, const char *encoding);
osync_bool needs_charset(const unsigned char *tmp);

void add_value(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *name, const char *encoding);
void add_values(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *encoding);
void add_values_from_nth_field_on(VFormatAttribute *attr, OSyncXMLField *xmlfield, const char *encoding, int nth);

/* VFormat Handler for Attributes and Parameters */
void handle_parameter(OSyncHookTables *hooks, OSyncXMLField *xmlfield, VFormatParam *param);
void handle_attribute(OSyncHookTables *hooks, OSyncXMLFormat *xmlformat, VFormatAttribute *attr, OSyncError **error);

/* XML Handler for Attributes and Parameters */
void xml_handle_parameter(OSyncHookTables *hooks, VFormatAttribute *attr, OSyncXMLField *xmlfield, int attr_nr);
void xml_handle_attribute(OSyncHookTables *hooks, VFormat *vcard, OSyncXMLField *xmlfield, const char *encoding);

#endif // XMLFORMAT_COMMON_H_

