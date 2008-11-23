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

#ifndef OPENSYNC_CAPABILITIES_PRIVATE_H_
#define OPENSYNC_CAPABILITIES_PRIVATE_H_

/**
 * @brief Represent a CapabilitiesObjType object
 * @ingroup OSyncCapabilitiesPrivateAPI
 */
struct OSyncCapabilitiesObjType {
	/** The pointer to the next objtype */
	OSyncCapabilitiesObjType *next;
	/** The pointer to the first capability */
	OSyncCapability *first_child;
	/** The pointer to the last capability */
	OSyncCapability *last_child;
	/** Counter which holds the number of capabilities for one objtype*/
	int child_count;
	/** The wrapped xml node */
	xmlNodePtr node;	
};

/**
 * @brief Represent a Capabilities object
 * @ingroup OSyncCapabilitiesPrivateAPI
 */
struct OSyncCapabilities {
	/** The reference counter for this object */
	int ref_count;
	/** The pointer to the first objtype */
	OSyncCapabilitiesObjType *first_objtype;
	/** The pointer to the last objtype */
	OSyncCapabilitiesObjType *last_objtype;
	/** The wrapped xml document */
	xmlDocPtr doc;
};

OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_new(OSyncCapabilities *capabilities, xmlNodePtr node, OSyncError **error);
OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_get(OSyncCapabilities *capabilities, const char *objtype);

#endif /*OPENSYNC_CAPABILITIES_PRIVATE_H_*/
