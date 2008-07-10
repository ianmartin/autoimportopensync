/*
 * libopensync - A synchronization framework
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
 * Author: Bjoern Ricks <bjoern.ricks@gmail.com>
 * 
 */
 
#ifndef OPENSYNC_XMLFORMAT_SCHEMA_H_
#define OPENSYNC_XMLFORMAT_SCHEMA_H_

OSYNC_EXPORT OSyncXMLFormatSchema *osync_xmlformat_schema_get_instance(OSyncXMLFormat *xmlformat, OSyncError **error);
OSYNC_EXPORT void osync_xmlformat_schema_unref(OSyncXMLFormatSchema *schema);
OSYNC_EXPORT OSyncXMLFormatSchema *osync_xmlformat_schema_ref(OSyncXMLFormatSchema *osyncschema);
OSYNC_EXPORT osync_bool osync_xmlformat_schema_validate(OSyncXMLFormatSchema *schema, OSyncXMLFormat *xmlformat, OSyncError **error);

#endif /* OPENSYNC_XMLFORMAT_SCHEMA_H_ */
