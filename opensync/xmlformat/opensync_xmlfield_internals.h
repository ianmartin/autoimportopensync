/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */

#ifndef OPENSYNC_XMLFIELD_INTERNALS_H_
#define OPENSYNC_XMLFIELD_INTERNALS_H_

OSyncXMLField *osync_xmlfield_new_node(OSyncXMLFormat *xmlformat, xmlNodePtr node, OSyncError **error);
void osync_xmlfield_free(OSyncXMLField *xmlfield);

void osync_xmlfield_delete(OSyncXMLField *xmlfield);
void osync_xmlfield_adopt_xmlfield_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);
void osync_xmlfield_adopt_xmlfield_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);

osync_bool osync_xmlfield_compare(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2);
osync_bool osync_xmlfield_compare_similar(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2, char* keys[]);

void osync_xmlfield_sort(OSyncXMLField *xmlfield);

void osync_xmlfield_unlink(OSyncXMLField *xmlfield);

int osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2);
int osync_xmlfield_key_compare_stdlib(const void *key1, const void *key2);

#endif /*OPENSYNC_XMLFIELD_INTERNALS_H_*/
