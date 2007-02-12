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
 
#ifndef OPENSYNC_CAPABILITY_H_
#define OPENSYNC_CAPABILITY_H_

OSYNC_EXPORT OSyncCapability *osync_capability_new(OSyncCapabilities *capabilities, const char *objtype, const char *name, OSyncError **error);

OSYNC_EXPORT const char *osync_capability_get_name(OSyncCapability *capability);
OSYNC_EXPORT OSyncCapability *osync_capability_get_next(OSyncCapability *capability);

OSYNC_EXPORT osync_bool osync_capability_has_key(OSyncCapability *capability);
OSYNC_EXPORT int osync_capability_get_key_count(OSyncCapability *capability);
OSYNC_EXPORT const char *osync_capability_get_nth_key(OSyncCapability *capability, int nth);
OSYNC_EXPORT void osync_capability_add_key(OSyncCapability *capabilitiy, const char *name);

#endif /*OPENSYNC_CAPABILITY_H_*/
