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

/**
 * @defgroup OSyncXMLFieldListPrivateAPI OpenSync XMLFieldList Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFieldList
 * 
 */
/*@{*/

/**
 * @brief Creates a new xmlfieldlist object
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlfieldlist object or NULL in case of error
 */
OSyncXMLFieldList *_osync_xmlfieldlist_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncXMLFieldList *xmlfieldlist = osync_try_malloc0(sizeof(OSyncXMLFieldList), error);
	if(!xmlfieldlist) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	xmlfieldlist->array = g_ptr_array_new();
	
	osync_trace(TRACE_EXIT, "%s(%p)", __func__, xmlfieldlist);
	return  xmlfieldlist;
}

/**
 * @brief Adds a xmlfield pointer to the end of the xmlfieldlist
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @param xmlfield The xmlfield pointer to add to the xmlfieldlist 
 */
void _osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield)
{
	osync_assert(xmlfieldlist);
	osync_assert(xmlfield);
	
	g_ptr_array_add(xmlfieldlist->array, xmlfield);
}

/**
 * @brief Removes the xmlfield pointer at the given index from the xmlfieldlist.
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @param index The index of the xmlfield pointer to remove 
 */
void _osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index)
{
	osync_assert(xmlfieldlist);
	
	if(index >= xmlfieldlist->array->len)
		return;
	g_ptr_array_remove_index(xmlfieldlist->array, index);
}

/*@}*/

/**
 * @defgroup OSyncXMLFieldListAPI OpenSync XMLFieldList
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFieldList
 * 
 */
/*@{*/

/**
 * @brief Frees all memory which was allocated for the xmlfieldlist
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 */
void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist)
{
	osync_assert(xmlfieldlist);
	g_ptr_array_free(xmlfieldlist->array, FALSE);
	g_free(xmlfieldlist);
}

/**
 * @brief Get the count of xmlfield pointers of a xmlfieldlist 
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @return The count of xmlfield pointers of the xmlfieldlist 
 */
int osync_xmlfieldlist_get_length(OSyncXMLFieldList *xmlfieldlist)
{
	osync_assert(xmlfieldlist);
	
	return xmlfieldlist->array->len;
}

/**
 * @brief Get the xmlfield pointer of the given index of a xmlfieldlist 
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @param index The index of the xmlfield pointer to return
 * @return The xmlfield of the given index of the xmlfieldlist or NULL in case of error 
 */
OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index)
{
	osync_assert(xmlfieldlist);
	
	if(index >= xmlfieldlist->array->len)
		return NULL;
	return g_ptr_array_index(xmlfieldlist->array, index);
}

/*@}*/
