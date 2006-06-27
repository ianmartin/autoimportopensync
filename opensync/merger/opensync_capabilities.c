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
 * @defgroup OSyncCapabilitiesPrivateAPI OpenSync Capabilities Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncCapabilities
 * 
 */
/*@{*/

OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_new(OSyncCapabilities *capabilities, xmlNodePtr node)
{
	g_assert(capabilities);
	g_assert(node);
	
	OSyncCapabilitiesObjType *objtype = g_malloc0(sizeof(OSyncCapabilitiesObjType));
	
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

OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_get(OSyncCapabilities *capabilities, const char *objtype)
{
	OSyncCapabilitiesObjType *tmp = capabilities->first_objtype;
	while(tmp != NULL)
	{
		if( strcmp((const char *)tmp->node->name, objtype) == 0)
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

OSyncCapabilities *osync_capabilities_new(void)
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);
	
	OSyncCapabilities *capabilities = g_malloc0(sizeof(OSyncCapabilities));
	capabilities->refcount = 1;
	capabilities->doc = xmlNewDoc(BAD_CAST "1.0");
	capabilities->doc->children = xmlNewDocNode(capabilities->doc, NULL, (xmlChar *)"capabilities", NULL);
	capabilities->first_objtype = NULL;
	capabilities->last_objtype = NULL;
		
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

OSyncCapabilities *osync_capabilities_parse(const char *buffer, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, buffer);
	
	g_assert(buffer);
	
	OSyncCapabilities *capabilities = g_malloc0(sizeof(OSyncCapabilities));
	capabilities->refcount = 1;
	capabilities->doc = xmlReadMemory(buffer, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if(capabilities->doc == NULL)
	{
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse XML.");
		g_free(capabilities);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
	}
	capabilities->refcount = 1;
	capabilities->first_objtype = NULL;
	capabilities->last_objtype = NULL;
	capabilities->doc->_private = capabilities;
	
	xmlNodePtr cur = xmlDocGetRootElement(capabilities->doc);
	cur = cur->children;
	while (cur != NULL) {
		OSyncCapabilitiesObjType *objtype = _osync_capabilitiesobjtype_new(capabilities, cur);
		xmlNodePtr tmp = cur->children;
		while(tmp != NULL)
		{
			_osync_capability_new(objtype, tmp);
			tmp = tmp->next;
		}
		cur = cur->next;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

void osync_capabilities_ref(OSyncCapabilities *capabilities)
{
	g_assert(capabilities);
	capabilities->refcount++;
}

void osync_capabilities_unref(OSyncCapabilities *capabilities)
{
	g_assert(capabilities);
	capabilities->refcount--;
	if(capabilities->refcount <= 0) {
		OSyncCapabilitiesObjType *objtype, *tmp;
		objtype = capabilities->first_objtype;
		while(objtype)
		{
			OSyncCapability *capability, *tmp2;
			capability = objtype->first_child;
			while(capability)
			{
				tmp2 = osync_capability_get_next(capability);
				osync_capability_free(capability);
				capability = tmp2;
			}				
			
			tmp = objtype;
			g_free(objtype);
			objtype = tmp;
		}
		xmlFreeDoc(capabilities->doc);
		g_free(capabilities);
	}
}

OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities, const char *objtype)
{
	g_assert(capabilities);
	g_assert(objtype);
	
	OSyncCapability *res = NULL;
	OSyncCapabilitiesObjType *tmp = _osync_capabilitiesobjtype_get(capabilities, objtype);
	if(tmp)
		res = tmp->first_child;
	return res;
}	

osync_bool osync_capabilities_assemble(OSyncCapabilities *capabilities, char **buffer, int *size)
{
	g_assert(capabilities);
	g_assert(buffer);
	g_assert(size);
	xmlDocDumpFormatMemoryEnc(capabilities->doc, (xmlChar **) buffer, size, NULL, 1);
	return TRUE;	
}

void osync_capabilities_sort(OSyncCapabilities *capabilities)
{
	int index;
	OSyncCapabilitiesObjType *objtype;
	OSyncCapability *cur;
		
	objtype = capabilities->first_objtype;
	while(objtype != NULL)
	{
		if(objtype->child_count <= 1)
			return;
	
		void **list = malloc(sizeof(xmlNodePtr) * objtype->child_count);
	
		index = 0;
		cur = objtype->first_child;
		while(cur != NULL)
		{
			list[index] = cur;
			index++;
			xmlUnlinkNode(cur->node);
			cur = osync_capability_get_next(cur);
		}
	
		qsort(list, objtype->child_count, sizeof(OSyncCapability *), osync_capability_compare_stdlib);
	
		/** bring the capabilities and xmldoc in a consistent state */
		objtype->first_child = ((OSyncCapability *)list[0])->node->_private;
		objtype->last_child = ((OSyncCapability *)list[objtype->child_count - 1])->node->_private;

		index = 0;
		while(index < objtype->child_count)
		{
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
		
			index++;
		}
		
		free(list);
		
		objtype = objtype->next;
	}
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
