#include "opensync.h"
#include "opensync_internals.h"

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
