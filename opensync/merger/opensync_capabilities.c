/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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

#include "opensync-group.h"

#define OSYNC_CAPABILITIES_DIRECTORY "/usr/share/opensync/capabilities"

/**
 * @defgroup OSyncCapabilitiesPrivateAPI OpenSync Capabilities Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncCapabilities
 * 
 */
/*@{*/

/**
 * @brief Creates a new capabilitiesobjtype object which will be added to the end of capabilitiesobjtype of the capabilities object.
 *  The returned object will be freed with the capabilities object. 
 * @param capabilities The pointer to a capabilities object
 * @param node The node must be already inserted at the end of childs of the xmlDoc root element
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capabilitiesobjtype object or NULL in case of error
 */
OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_new(OSyncCapabilities *capabilities, xmlNodePtr node, OSyncError **error)
{
	osync_assert(capabilities);
	osync_assert(node);
	
	OSyncCapabilitiesObjType *objtype = osync_try_malloc0(sizeof(OSyncCapabilitiesObjType), error);
	if(!objtype) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	objtype->child_count = 0;
	objtype->first_child = NULL;
	objtype->last_child = NULL;
	objtype->next = NULL;
	objtype->node = node;
	
	if(!capabilities->first_objtype)
		capabilities->first_objtype = objtype;
	if(capabilities->last_objtype)
		capabilities->last_objtype->next = objtype;
	capabilities->last_objtype = objtype;

	return objtype;
}

/**
 * @brief Get the first capabilitiesobjtype for a given objtype from the capabilities
 * @param capabilities The pointer to a capabilities object
 * @param objtype The name of the objtype (e.g.: contact)
 * @return The capabilitiesobjtype for a given objtype from the capabilities
 */
OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_get(OSyncCapabilities *capabilities, const char *objtype)
{
	osync_assert(capabilities);
	osync_assert(objtype);
	
	OSyncCapabilitiesObjType *tmp = capabilities->first_objtype;
	for(; tmp != NULL; tmp = tmp->next) {
		if(!strcmp((const char *)tmp->node->name, objtype))
			break;
	}	
	return tmp;	
}

/*@}*/

/**
 * @defgroup OSyncCapabilitiesAPI OpenSync Capabilities
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncCapabilities
 * 
 */
/*@{*/

/**
 * @brief Creates a new capabilities object
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capabilities object or NULL in case of error
 */
OSyncCapabilities *osync_capabilities_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncCapabilities *capabilities = osync_try_malloc0(sizeof(OSyncCapabilities), error);
	if(!capabilities) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	capabilities->ref_count = 1;
	capabilities->first_objtype = NULL;
	capabilities->last_objtype = NULL;
	capabilities->doc = xmlNewDoc(BAD_CAST "1.0");
	capabilities->doc->children = xmlNewDocNode(capabilities->doc, NULL, (xmlChar *)"capabilities", NULL);
	capabilities->doc->_private = capabilities;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

/**
 * @brief Creates a new capabilities object from a xml document. 
 * @param buffer The pointer to the xml document
 * @param size The size of the xml document
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capabilities object or NULL in case of error
 */
OSyncCapabilities *osync_capabilities_parse(const char *buffer, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %u, %p)", __func__, buffer, size, error);
	osync_assert(buffer);
	
	OSyncCapabilities *capabilities = osync_try_malloc0(sizeof(OSyncCapabilities), error);
	if(!capabilities) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	capabilities->ref_count = 1;
	capabilities->first_objtype = NULL;
	capabilities->last_objtype = NULL;	
	capabilities->doc = xmlReadMemory(buffer, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if(capabilities->doc == NULL) {
		g_free(capabilities);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse XML.");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	capabilities->doc->_private = capabilities;
	
	xmlNodePtr cur = xmlDocGetRootElement(capabilities->doc);
	cur = cur->children;
	for(; cur != NULL; cur = cur->next) {
		OSyncCapabilitiesObjType *capabilitiesobjtype = _osync_capabilitiesobjtype_new(capabilities, cur, error);
		if(!capabilitiesobjtype) {
			osync_capabilities_unref(capabilities);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
		
		xmlNodePtr tmp = cur->children;
		for(; tmp != NULL; tmp = tmp->next) {
			OSyncCapability *capability = _osync_capability_new(capabilitiesobjtype, tmp, error);
			if(!capability) {
				osync_capabilities_unref(capabilities);
				osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
				return NULL;
			}
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

/**
 * @brief Increments the reference counter
 * @param capabilities The pointer to a capabilities object
 */
void osync_capabilities_ref(OSyncCapabilities *capabilities)
{
	osync_assert(capabilities);
	
	g_atomic_int_inc(&(capabilities->ref_count));
}

/**
 * @brief Decrement the reference counter. The xmlformat object will 
 *  be freed if there is no more reference to it.
 * @param capabilities The pointer to a capabilities object
 */
void osync_capabilities_unref(OSyncCapabilities *capabilities)
{
	osync_assert(capabilities);
			
	if (g_atomic_int_dec_and_test(&(capabilities->ref_count))) {
		OSyncCapabilitiesObjType *objtype, *tmp;
		objtype = capabilities->first_objtype;
		while(objtype)
		{
			OSyncCapability *capability, *tmp2;
			capability = objtype->first_child;
			while(capability)
			{
				tmp2 = osync_capability_get_next(capability);
				_osync_capability_free(capability);
				capability = tmp2;
			}				
			
			tmp = objtype->next;
			g_free(objtype);
			objtype = tmp;
		}
		xmlFreeDoc(capabilities->doc);
		g_free(capabilities);
	}
}

/**
 * @brief Get the first capability for a given objtype from the capabilities
 * @param capabilities The pointer to a capabilities object
 * @param objtype The name of the objtype (e.g.: contact)
 * @return The first capability for a given objtype from the capabilities
 */
OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities, const char *objtype)
{
	osync_assert(capabilities);
	osync_assert(objtype);
	
	OSyncCapability *res = NULL;
	OSyncCapabilitiesObjType *tmp = _osync_capabilitiesobjtype_get(capabilities, objtype);
	if(tmp)
		res = tmp->first_child;
	return res;
}	

/**
 * @brief Dump the capabilities into the memory.
 * @param capabilities The pointer to a capabilities object 
 * @param buffer The pointer to the buffer which will hold the xml document
 * @param size The pointer to the buffer which will hold the size of the xml document
 * @return The xml document and the size of it. It's up to the caller to free
 *  the buffer. Always it return TRUE.
 */
osync_bool osync_capabilities_assemble(OSyncCapabilities *capabilities, char **buffer, int *size)
{
	osync_assert(capabilities);
	osync_assert(buffer);
	osync_assert(size);
	
	xmlDocDumpFormatMemoryEnc(capabilities->doc, (xmlChar **) buffer, size, NULL, 1);
	return TRUE;
}

/**
 * @brief Sort all the capabilities of every objtype of the capabilities object. This function has to
 *  be called after a capability was added to the capabilities.
 * @param capabilities The pointer to a capabilities object
 */
void osync_capabilities_sort(OSyncCapabilities *capabilities)
{
	int index;
	OSyncCapabilitiesObjType *objtype;
	OSyncCapability *cur;
		
	objtype = capabilities->first_objtype;
	for(; objtype != NULL; objtype = objtype->next)
	{
		if(objtype->child_count <= 1)
			continue;
	
		void **list = g_malloc0(sizeof(OSyncCapability *) * objtype->child_count);
	
		index = 0;
		for(cur = objtype->first_child; cur != NULL; cur = osync_capability_get_next(cur)) {
			list[index] = cur;
			index++;
			xmlUnlinkNode(cur->node);
		}
	
		qsort(list, objtype->child_count, sizeof(OSyncCapability *), _osync_capability_compare_stdlib);
	
		/** bring the capabilities and xmldoc in a consistent state */
		objtype->first_child = ((OSyncCapability *)list[0])->node->_private;
		objtype->last_child = ((OSyncCapability *)list[objtype->child_count - 1])->node->_private;

		for(index = 0; index < objtype->child_count; index++) {
			cur = (OSyncCapability *)list[index];
			xmlAddChild(objtype->node, cur->node);
			
			if(index < objtype->child_count-1)
				cur->next = (OSyncCapability *)list[index+1];
			else
				cur->next = NULL;
			
			if(index)
				cur->prev = (OSyncCapability *)list[index-1];
			else
				cur->prev = NULL;
		}
		
		g_free(list);
	}
}

/**
 * @brief Load a capabilities object from a prepackaged file 
 * @param file The name of the file
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated capabilities object or NULL in case of error
 */
OSyncCapabilities *osync_capabilities_load(const char *file, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, file, error);
	osync_assert(file);
	
	unsigned int size;
	char *buffer, *filename;
	OSyncCapabilities *capabilities;
	
	filename = g_strdup_printf("%s%c%s", OSYNC_CAPABILITIES_DIRECTORY, G_DIR_SEPARATOR, file);
	
	osync_bool b = osync_file_read(filename, &buffer, &size, error);
	g_free(filename);
	if(!b) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	capabilities = osync_capabilities_parse(buffer, size, error);
	g_free(buffer);
	if(!capabilities) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

/**
 * @brief Checks if the capabilities are already cached 
 * @param member The member which should be tested for cached capabilities
 * @return TRUE if the capabilities for this member are cached otherwise FALSE
 */
osync_bool osync_capabilities_member_has_capabilities(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	osync_assert(member);
	
	char *filename = g_strdup_printf("%s%ccapabilities.xml", osync_member_get_configdir(member), G_DIR_SEPARATOR);
	gboolean res = g_file_test(filename, G_FILE_TEST_IS_REGULAR);
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, res);
	return res;
}

/**
 * @brief Get the cached capabilities of a member
 * @param member The pointer to a member object
 * @param error The error which will hold the info in case of an error
 * @return The objtype of the xmlformat
 */
OSyncCapabilities* osync_capabilities_member_get_capabilities(OSyncMember *member, OSyncError** error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	osync_assert(member);
	
	unsigned int size;
	char* buffer, *filename;
	OSyncCapabilities *capabilities;
	
	filename = g_strdup_printf("%s%ccapabilities.xml", osync_member_get_configdir(member), G_DIR_SEPARATOR);
	osync_bool res = osync_file_read(filename, &buffer, &size, error);
	g_free(filename);
	
	if(!res) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	capabilities = osync_capabilities_parse(buffer, size, error);
	g_free(buffer);
	if(!capabilities) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

/**
 * @brief Set the capabilities of a member
 * @param member The pointer to a member object
 * @param capabilities The pointer to a capabilities object
 * @param error The error which will hold the info in case of an error
 * @return TRUE on success otherwise FALSE
 */
osync_bool osync_capabilities_member_set_capabilities(OSyncMember *member, OSyncCapabilities* capabilities, OSyncError** error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, capabilities, error);
	osync_assert(member);
	osync_assert(capabilities);
	
	int size;
	char* buffer, *filename;
	osync_bool res;
	
	osync_capabilities_assemble(capabilities, &buffer, &size);
	filename = g_strdup_printf("%s%ccapabilities.xml", osync_member_get_configdir(member), G_DIR_SEPARATOR);
	res = osync_file_write(filename, buffer, size, 0600, error);
	g_free(filename);
	g_free(buffer);
	if(!res) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, res);
	return res;
}

//void osync_algorithm_quicksort(void * array[], int left, int right, const char *(*getString)(void *))
//{
//  register int i, j;
//  void *x, *temp;
//
//  i = left; j = right;
//  x = array[(left+right)/2];
//
//  do {
//    while((strcmp(getString(array[i]), getString(x)) < 0) && (i < right)) i++;
//    while((strcmp(getString(array[j]), getString(x)) > 0) && (j > left)) j--;
//    if(i <= j) {
//      temp = array[i];
//      array[i] = array[j];
//      array[j] = temp;
//      i++; j--;
//   }
//  } while(i <= j);
//
//  if(left < j) osync_algorithm_quicksort(array, left, j, getString);
//  if(i < right) osync_algorithm_quicksort(array, i, right, getString);
//}

/*@}*/
