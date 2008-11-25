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
 
#ifndef OPENSYNC_XMLFORMAT_H_
#define OPENSYNC_XMLFORMAT_H_

OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_new(const char *objtype, OSyncError **error);
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error);
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_ref(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT void osync_xmlformat_unref(OSyncXMLFormat *xmlformat);

OSYNC_EXPORT OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error, ...);

OSYNC_EXPORT osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, unsigned int *size);

OSYNC_EXPORT unsigned int osync_xmlformat_size();

OSYNC_EXPORT void osync_xmlformat_sort(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT osync_bool osync_xmlformat_is_sorted(OSyncXMLFormat *xmlformat);

#endif /*OPENSYNC_XMLFORMAT_H_*/
