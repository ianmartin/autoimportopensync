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

#ifndef OPENSYNC_CAPABILITIES_H_
#define OPENSYNC_CAPABILITIES_H_


OSYNC_EXPORT OSyncCapabilities *osync_capabilities_new(OSyncError **error);
OSYNC_EXPORT OSyncCapabilities *osync_capabilities_parse(const char *buffer, unsigned int size, OSyncError **error);
OSYNC_EXPORT OSyncCapabilities *osync_capabilities_ref(OSyncCapabilities *capabilities);
OSYNC_EXPORT void osync_capabilities_unref(OSyncCapabilities *capabilities);

OSYNC_EXPORT osync_bool osync_capabilities_assemble(OSyncCapabilities *capabilities, char **buffer, int *size);

#endif /*OPENSYNC_CAPABILITIES_H_*/
