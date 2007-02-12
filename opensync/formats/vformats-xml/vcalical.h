/*
 * vcalical - An vcal/ical converter 
 * Copyright (C)  2006 Daniel Gollub <dgollub@suse.de>
 * Copyright (C)  2006 Christopher Stender <cstender@suse.de>
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
 
#include "xml-support.h"
#include "vformat.h"
#include "xml-vcal.h"
#include <glib.h>

char *conv_ical2vcal_rrule(const char *ical);
GList *conv_vcal2ical_rrule(const char *vcal);
GList *conv_vcal2ical_rrule(const char *vcal);

