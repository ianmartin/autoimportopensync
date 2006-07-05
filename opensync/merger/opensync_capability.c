/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

/**
 * @defgroup OSyncCapabilityPrivateAPI OpenSync Capability Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncCapability
 * 
 */
/*@{*/

OSyncCapability *_osync_capability_new(OSyncCapabilitiesObjType *objtype, xmlNodePtr node)
{
	g_assert(objtype);
	g_assert(node);
	
	OSyncCapability *capability = g_malloc0(sizeof(OSyncCapability));
	
	capability->next = NULL;
	capability->node = node;
	capability->prev = objtype->last_child;
	node->_private = capability;
	
	if(!objtype->first_child)
		objtype->first_child = capability;
	if(objtype->last_child)
		objtype->last_child->next = capability;
	objtype->last_child = capability;
	objtype->child_count++;
	
	return capability;
}

void _osync_capability_free(OSyncCapability *capability)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, capability);
	g_assert(capability);
	
	g_free(capability);
	
	osync_trace(TRACE_EXIT, "%s");
}

int _osync_capability_compare_stdlib(const void *capability1, const void *capability2)
{
	return strcmp(osync_capability_get_name(*(OSyncCapability **)capability1), osync_capability_get_name(*(OSyncCapability **)capability2));
}

/*@}*/

/**
 * @defgroup OSyncCapabilityAPI OpenSync Capability
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncCapability
 * 
 */
/*@{*/

OSyncCapability *osync_capability_new(OSyncCapabilities *capabilities, const char *objtype, const char *name)
{
	g_assert(capabilities);
	g_assert(objtype);
	g_assert(name);
	
	OSyncCapabilitiesObjType *tmp = _osync_capabilitiesobjtype_get(capabilities, objtype);
	if(tmp == NULL) {
		xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(capabilities->doc), NULL, BAD_CAST objtype, NULL);
		tmp = _osync_capabilitiesobjtype_new(capabilities, node);
	}
	xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(capabilities->doc), NULL, (xmlChar *)name, NULL);
	return _osync_capability_new(tmp, node);
}

const char *osync_capability_get_name(OSyncCapability *capability)
{
	g_assert(capability);
	return (const char *) capability->node->name;
}

OSyncCapability *osync_capability_get_next(OSyncCapability *capability)
{
	g_assert(capability);
	return capability->next;
}

osync_bool osync_capability_has_key(OSyncCapability *capability)
{
	g_assert(capability);

	if(capability->node->children)
		return TRUE;
	else
		return FALSE;	
}

int osync_capability_get_key_count(OSyncCapability *capability)
{
	g_assert(capability);
	
	int count;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	for(count=0 ; child != NULL; child = child->next) {
		count++;
	}
	
	return count;
}

const char *osync_capability_get_nth_key(OSyncCapability *capability, int nth)
{
	g_assert(capability);
	
	int count = 0;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	for(count=0; child != NULL; child = child->next) {
		if(count == nth)
			return (const char *)child->name;
		count++;
	}
	
	return NULL;
}

void osync_capability_add_key(OSyncCapability *capabilitiy, const char *name)
{
	g_assert(capabilitiy);
	g_assert(name);
	
	xmlNewChild(capabilitiy->node, NULL, (xmlChar*)name, NULL);
}

/*@}*/
