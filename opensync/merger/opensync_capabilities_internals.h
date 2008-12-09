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
 
#ifndef OPENSYNC_CAPABILITIES_INTERNALS_H_
#define OPENSYNC_CAPABILITIES_INTERNALS_H_

typedef struct OSyncCapabilitiesObjType OSyncCapabilitiesObjType;

OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities, const char *objtype);

OSYNC_TEST_EXPORT void osync_capabilities_sort(OSyncCapabilities *capabilities);

OSyncCapabilities *osync_capabilities_load(const char *file, OSyncError **error);
osync_bool osync_capabilities_member_has_capabilities(OSyncMember *member);
OSyncCapabilities* osync_capabilities_member_get_capabilities(OSyncMember *member, OSyncError** error);
osync_bool osync_capabilities_member_set_capabilities(OSyncMember *member, OSyncCapabilities* capabilities, OSyncError** error);

#endif /*OPENSYNC_CAPABILITIES_INTERNAL_H_*/
