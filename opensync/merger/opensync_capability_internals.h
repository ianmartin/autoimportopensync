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
 
#ifndef OPENSYNC_CAPABILITY_INTERNALS_H_
#define OPENSYNC_CAPABILITY_INTERNALS_H_

/**
 * @brief Represent a Capabilitiy object
 * @ingroup OSyncCapabilityPrivateAPI
 */
struct OSyncCapability {
	/** The pointer to the next capability */
	OSyncCapability *next;
	/** The pointer to the previous capability */
	OSyncCapability *prev;
	/** The wrapped xml node */
	xmlNodePtr node;
};

OSyncCapability *_osync_capability_new(OSyncCapabilitiesObjType *objtype, xmlNodePtr node, OSyncError **error);
void _osync_capability_free(OSyncCapability *capability);
int _osync_capability_compare_stdlib(const void *capability1, const void *capability2);

#endif /*OPENSYNC_CAPABILITY_INTERNALS_H_*/
