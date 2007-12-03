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

int convert_vcal_rrule_frequency(OSyncXMLField *xmlfield, const char *rule);
char *convert_vcal_rrule_freqmod(OSyncXMLField *xmlfield, gchar **rule, int size, int freqstate);
void convert_vcal_rrule_countuntil(OSyncXMLField *xmlfield, const char *duration_block);
void convert_vcal_rrule_to_xml(OSyncXMLField *xmlfield, const char *rule);
OSyncXMLField *convert_ical_rrule_to_xml(OSyncXMLFormat *xmlformat, VFormatAttribute *attr, char *rulename, OSyncError **error);

#endif // XMLFORMAT_RECURRENCE_H_
