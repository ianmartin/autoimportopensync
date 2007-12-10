/*
 * xmlformat-recurrence - common code for recurrence implementation
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

#ifndef XMLFORMAT_RECURRENCE_H_
#define XMLFORMAT_RECURRENCE_H_

OSyncXMLField *convert_vcal_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *rulename, OSyncError **error);
VFormatAttribute *convert_xml_rrule_to_vcal(VFormat *vformat, OSyncXMLField *xmlfield, const char *rulename, const char *encoding);
OSyncXMLField *convert_ical_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, const char *rulename, OSyncError **error);
VFormatAttribute *convert_xml_rrule_to_ical(VFormat *vformat, OSyncXMLField *xmlfield, const char *rulename, const char *encoding);

#endif // XMLFORMAT_RECURRENCE_H_
