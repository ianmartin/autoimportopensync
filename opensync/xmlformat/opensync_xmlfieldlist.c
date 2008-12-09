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

#include "opensync-xmlformat.h"
#include "opensync-xmlformat_internals.h"

#include "opensync_xmlfieldlist_private.h"

OSyncXMLFieldList *osync_xmlfieldlist_new(OSyncError **error)
{
  OSyncXMLFieldList *xmlfieldlist = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
  xmlfieldlist = osync_try_malloc0(sizeof(OSyncXMLFieldList), error);
  if(!xmlfieldlist) {
    osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
    return NULL;
  }
  xmlfieldlist->array = g_ptr_array_new();
	
  osync_trace(TRACE_EXIT, "%s(%p)", __func__, xmlfieldlist);
  return  xmlfieldlist;
}

void osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield)
{
  osync_assert(xmlfieldlist);
  osync_assert(xmlfield);
	
  g_ptr_array_add(xmlfieldlist->array, xmlfield);
}

void osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index)
{
  osync_assert(xmlfieldlist);
	
  if(index >= xmlfieldlist->array->len)
    return;
  g_ptr_array_remove_index(xmlfieldlist->array, index);
}

void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist)
{
  osync_assert(xmlfieldlist);
  g_ptr_array_free(xmlfieldlist->array, TRUE);
  g_free(xmlfieldlist);
}

int osync_xmlfieldlist_get_length(OSyncXMLFieldList *xmlfieldlist)
{
  osync_assert(xmlfieldlist);
	
  return xmlfieldlist->array->len;
}

OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index)
{
  osync_assert(xmlfieldlist);
	
  if(index >= xmlfieldlist->array->len)
    return NULL;
  return g_ptr_array_index(xmlfieldlist->array, index);
}
