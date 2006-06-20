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
 * @defgroup OSyncXMLFieldListPrivateAPI OpenSync XMLFieldList Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFieldList
 * 
 */
/*@{*/

OSyncXMLFieldList *osync_xmlfieldlist_new(void)
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	OSyncXMLFieldList *xmlfieldlist = g_malloc0(sizeof(OSyncXMLFieldList));
	xmlfieldlist->array = g_ptr_array_new();
	
	osync_trace(TRACE_EXIT, "%s(%p)", __func__, xmlfieldlist);
	return  xmlfieldlist;
}

void osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield)
{
	g_ptr_array_add(xmlfieldlist->array, xmlfield);
}

void osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index)
{
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

void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist)
{
	g_ptr_array_free(xmlfieldlist->array, FALSE);
	g_free(xmlfieldlist);
}

int osync_xmlfieldlist_getLength(OSyncXMLFieldList *xmlfieldlist)
{
	return xmlfieldlist->array->len;
}

OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index)
{
	return g_ptr_array_index(xmlfieldlist->array, index);
}

/*@}*/
