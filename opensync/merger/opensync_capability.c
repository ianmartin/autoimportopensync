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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

#include "opensync_capabilities_private.h" /* FIXME: include forgein private-header */

/**
 * @defgroup OSyncCapabilityPrivateAPI OpenSync Capability Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncCapability
 * 
 */
/*@{*/

/**
 * @brief Creates a new capability object which will be added to the end of capabilities of the capabilities object.
 *  The returned object will be freed with the capabilities object.
 * @param capabilitiesobjtype The pointer to a capabilitiesobjtype object
 * @param node The node must be already inserted at the end of childs of the objtype xml node
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capability object or NULL in case of error
 */
OSyncCapability *_osync_capability_new(OSyncCapabilitiesObjType *capabilitiesobjtype, xmlNodePtr node, OSyncError **error)
{
	osync_assert(capabilitiesobjtype);
	osync_assert(node);
	
	OSyncCapability *capability = osync_try_malloc0(sizeof(OSyncCapability), error);
	if(!capability) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}	
	
	capability->next = NULL;
	capability->node = node;
	capability->prev = capabilitiesobjtype->last_child;
	node->_private = capability;
	
	if(!capabilitiesobjtype->first_child)
		capabilitiesobjtype->first_child = capability;
	if(capabilitiesobjtype->last_child)
		capabilitiesobjtype->last_child->next = capability;
	capabilitiesobjtype->last_child = capability;
	capabilitiesobjtype->child_count++;
	
	return capability;
}

/**
 * @brief Frees a capability object 
 * @param capability The pointer to a capability object
 */
void _osync_capability_free(OSyncCapability *capability)
{
	osync_assert(capability);
	
	g_free(capability);
}

/**
 * @brief Compare the names of two capabilities  
 * @param capability1 The pointer to a capability object
 * @param capability2 The pointer to a capability object
 */
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

/**
 * @brief Creates a new capability object which will be added to the end of capabilities of the capabilities object.
 *  The returned object will be freed with the capabilities object.
 * @param capabilities The pointer to a capabilities object
 * @param objtype The name of the objtype (e.g.: contact)
 * @param name The name of the capability
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capability object or NULL in case of error
 */
OSyncCapability *osync_capability_new(OSyncCapabilities *capabilities, const char *objtype, const char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p)", __func__, capabilities, objtype, name, error);
	osync_assert(capabilities);
	osync_assert(objtype);
	osync_assert(name);
	
	OSyncCapabilitiesObjType *capabilitiesobjtype = _osync_capabilitiesobjtype_get(capabilities, objtype);
	if(!capabilitiesobjtype) {
		xmlNodePtr node = xmlNewTextChild(xmlDocGetRootElement(capabilities->doc), NULL, BAD_CAST objtype, NULL);
		capabilitiesobjtype = _osync_capabilitiesobjtype_new(capabilities, node, error);
		if(!capabilitiesobjtype) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
	}
	
	xmlNodePtr node = xmlNewTextChild(capabilitiesobjtype->node, NULL, (xmlChar *)name, NULL);
	OSyncCapability *capability = _osync_capability_new(capabilitiesobjtype, node, error);
	if(!capability) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capability);
	return capability;
}

/**
 * @brief Get the name of the capability
 * @param capability The pointer to a capability object
 * @return The name of the capability
 */
const char *osync_capability_get_name(OSyncCapability *capability)
{
	osync_assert(capability);
	
	return (const char *) capability->node->name;
}

/**
 * @brief Get the next capability
 * @param capability The pointer to a capability object
 * @return The pointer to the next capability or NULL if there is no more capability
 */
OSyncCapability *osync_capability_get_next(OSyncCapability *capability)
{
	osync_assert(capability);
	
	return capability->next;
}

/**
 * @brief Check if the capability has a key
 * @param capability The pointer to a capability object
 * @return TRUE if the capability has a key otherwise FALSE
 */
osync_bool osync_capability_has_key(OSyncCapability *capability)
{
	osync_assert(capability);

	if(capability->node->children)
		return TRUE;
	else
		return FALSE;	
}

/**
 * @brief Get the count of keys of a capability
 * @param capability The pointer to a capability object
 * @return The count of keys of the capability
 */
int osync_capability_get_key_count(OSyncCapability *capability)
{
	osync_assert(capability);
	
	int count;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	for(count=0 ; child != NULL; child = child->next)
		count++;
	
	return count;
}

/**
 * @brief Get the name of the nth key of a capability
 * @param capability The pointer to a capability object
 * @param nth The number of the key
 * @return The name of the nth key of the capability or NULL in case of error
 */
const char *osync_capability_get_nth_key(OSyncCapability *capability, int nth)
{
	osync_assert(capability);
	
	int count = 0;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	for(count=0; child != NULL; child = child->next) {
		if(count == nth)
			return (const char *)child->name;
		count++;
	}
	
	return NULL;
}

/**
 * @brief Add the key to capability
 * @param capability The pointer to a capability object
 * @param name The name of the key
 */
void osync_capability_add_key(OSyncCapability *capability, const char *name)
{
	osync_assert(capability);
	osync_assert(name);
	
	xmlNewTextChild(capability->node, NULL, (xmlChar*)name, NULL);
}

/*@}*/
