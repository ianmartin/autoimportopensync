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
 
#ifndef OPENSYNC_XMLFIELD_H_
#define OPENSYNC_XMLFIELD_H_

OSYNC_EXPORT OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error);


OSYNC_EXPORT const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield);
OSYNC_EXPORT void osync_xmlfield_set_name(OSyncXMLField *xmlfield, const char *name);
OSYNC_EXPORT OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield);

OSYNC_EXPORT const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr);
OSYNC_EXPORT void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value);
OSYNC_EXPORT int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_name(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth);

OSYNC_EXPORT const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key);
OSYNC_EXPORT void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);
OSYNC_EXPORT void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);
OSYNC_EXPORT int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_name(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value);

#endif /*OPENSYNC_XMLFIELD_H_*/
