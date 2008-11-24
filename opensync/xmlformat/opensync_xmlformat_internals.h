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
 
#ifndef OPENSYNC_XMLFORMAT_INTERNALS_H_
#define OPENSYNC_XMLFORMAT_INTERNALS_H_

const char *osync_xmlformat_root_name(OSyncXMLFormat *xmlformat);
const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat);

osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat, OSyncError **error);

void osync_xmlformat_sort(OSyncXMLFormat *xmlformat);
osync_bool osync_xmlformat_is_sorted(OSyncXMLFormat *xmlformat);

osync_bool osync_xmlformat_copy(OSyncXMLFormat *source, OSyncXMLFormat **destination, OSyncError **error);

#endif /* OPENSYNC_XMLFORMAT_INTERNAL_H_ */

