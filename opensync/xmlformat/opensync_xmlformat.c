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
#include "opensync_xmlformat_private.h"

#include "opensync_xmlfield_private.h"		/* FIXME: direct access of private header */

/**
 * @defgroup OSyncXMLFormatPrivateAPI OpenSync XMLFormat Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFormat
 * 
 */
/*@{*/

/*@}*/

/**
 * @defgroup OSyncXMLFormatAPI OpenSync XMLFormat
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFormat
 * 
 */
/*@{*/

/**
 * @brief Creates a new xmlformat object
 * @param objtype The name of the objtype (e.g.: contact)
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSyncXMLFormat *osync_xmlformat_new(const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, objtype, error);
	osync_assert(objtype);
	
	OSyncXMLFormat *xmlformat = osync_try_malloc0(sizeof(OSyncXMLFormat), error);
	if(!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	xmlformat->doc = xmlNewDoc(BAD_CAST "1.0");
	xmlformat->doc->children = xmlNewDocNode(xmlformat->doc, NULL, BAD_CAST objtype, NULL);
	xmlformat->ref_count = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->sorted = FALSE;
	xmlformat->doc->_private = xmlformat;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

/**
 * @brief Creates a new xmlformat object from a xml document. 
 * @param buffer The pointer to the xml document
 * @param size The size of the xml document
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, buffer, size, error);
	osync_assert(buffer);

	OSyncXMLFormat *xmlformat = osync_try_malloc0(sizeof(OSyncXMLFormat), error);
	if(!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	xmlformat->doc = xmlReadMemory(buffer, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if(!xmlformat->doc) {
		g_free(xmlformat);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse XML.");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;	
	}

	xmlformat->ref_count = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->doc->_private = xmlformat;
		
	xmlNodePtr cur = xmlDocGetRootElement(xmlformat->doc);
	cur = cur->children;
	while (cur != NULL) {
		OSyncXMLField *xmlfield = osync_xmlfield_new_node(xmlformat, cur, error);
		if(!xmlfield) {
			osync_xmlformat_unref(xmlformat);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
		cur = cur->next;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

/**
 * @brief Increments the reference counter
 * @param xmlformat The pointer to a xmlformat object
 */
OSyncXMLFormat *osync_xmlformat_ref(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	g_atomic_int_inc(&(xmlformat->ref_count));

	return xmlformat;
}

/**
 * @brief Decrement the reference counter. The xmlformat object will 
 *  be freed if there is no more reference to it.
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_xmlformat_unref(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	if (g_atomic_int_dec_and_test(&(xmlformat->ref_count))) {
		OSyncXMLField *cur, *tmp;
		cur = xmlformat->first_child;
		while(cur != NULL)
		{
			tmp = osync_xmlfield_get_next(cur);
			osync_xmlfield_delete(cur);
			cur = tmp;
		}
		osync_xml_free_doc(xmlformat->doc);
		g_free(xmlformat);
	}
}

/**
 * @brief Get the name of the root node in a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The name of the root node of the xmlformat
 */
const char *osync_xmlformat_root_name(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	return (const char *)xmlDocGetRootElement(xmlformat->doc)->name;
}

/**
 * @brief Get the objtype of a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The objtype of the xmlformat
 */
const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	return osync_xmlformat_root_name(xmlformat);
}

/**
 * @brief Get the first field of a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The first field of the xmlformat
 */
OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	return xmlformat->first_child;
}

/**
 * @brief Serarch for xmlfields in the given xmlformat. It's up to the caller to
 *  free the returned list with OSyncXMLFieldList::osync_xmlfieldlist_free
 * @param xmlformat The pointer to a xmlformat object 
 * @param name The name of the xmlfields to search for
 * @param error The error which will hold the info in case of an error
 * @param ... If the xmlfield should have a attribute with spezial value,
 *  then it is possible to specify the attribute name and the attribute 
 *  value. But always there have to set both parametes! There can be more 
 *  than one attribute pair. The last parameter has always to be NULL.
 * @return The Pointer to the xmlfieldlist which hold all founded xmlfields
 *  or NULL in case of error
 */
OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error, ...)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, ...)", __func__, xmlformat, name, error);
	osync_assert(xmlformat);
	osync_assert(name);
	
	int index;
	void *ret;
	OSyncXMLField *cur, *key, *res;

	/* Searching breaks if the xmlformat is not sorted (bsearch!).
	   ASSERT in development builds (when NDEBUG is not defined) - see ticket #754. */
	osync_assert(xmlformat->sorted);
	if (!xmlformat->sorted) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "XMLFormat is unsorted. Search result would be not valid.");
		goto error;
	}

	OSyncXMLFieldList *xmlfieldlist = osync_xmlfieldlist_new(error);
	if (!xmlfieldlist)
		goto error;

	void **liste = osync_try_malloc0(sizeof(OSyncXMLField *) * xmlformat->child_count, error);
	if (!liste)
		goto error;

	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	for (; cur != NULL; cur = osync_xmlfield_get_next(cur)) {
		liste[index] = cur;
		index++;
	}

	key = osync_try_malloc0(sizeof(OSyncXMLField), error);
	if (!key) {
		g_free(liste);
		goto error;
	}

	key->node = xmlNewNode(NULL, BAD_CAST name);
	
	ret = bsearch(&key, liste, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compare_stdlib);

	/* no result - return empty xmlfieldlist */
	if (!ret)
		goto end;

	/* if ret is valid pointer (not NULL!) - reference it here. avoid segfaults */
	res = *(OSyncXMLField **) ret;

	/* we set the cur ptr to the first field from the fields with name name because -> bsearch -> more than one field with the same name*/
	for (cur = res; cur->prev != NULL && !strcmp(osync_xmlfield_get_name(cur->prev), name); cur = cur->prev) ;

	osync_bool all_attr_equal;
	for (; cur != NULL && !strcmp(osync_xmlfield_get_name(cur), name); cur = cur->next) {
		const char *attr, *value;
		all_attr_equal = TRUE;
		va_list args;
		va_start(args, error);
		do {
			attr = va_arg(args, char *);
			value = va_arg(args, char *);
			if (attr == NULL || value == NULL)
				break;

			if (strcmp(value, osync_xmlfield_get_attr(cur, attr)) != 0)
				all_attr_equal = FALSE;
		} while (1);
		va_end(args);

		if(all_attr_equal)
			osync_xmlfieldlist_add(xmlfieldlist, cur);
	}

end:	
	/* free lists here (later) - bsearch result is still pointing in liste array */
	xmlFreeNode(key->node);
	g_free(key);
	g_free(liste);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlfieldlist);
	return xmlfieldlist;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Dump the xmlformat into the memory.
 * @param xmlformat The pointer to a xmlformat object 
 * @param buffer The pointer to the buffer which will hold the xml document
 * @param size The pointer to the buffer which will hold the size of the xml document
 * @return The xml document and the size of it. It's up to the caller to free
 *  the buffer. Always it return TRUE.
 */
osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, unsigned int *size)
{
	osync_assert(xmlformat);
	osync_assert(buffer);
	osync_assert(size);
	
	xmlDocDumpFormatMemoryEnc(xmlformat->doc, (xmlChar **)buffer, (int *)size, NULL, 1);
	return TRUE;	
}

/**
 * @brief Validate the xmlformat against its schema
 * @param xmlformat The pointer to a xmlformat object 
 * @param error The error which will hold the info in case of an error
 * @return TRUE if xmlformat valid else FALSE
 */
osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat, OSyncError **error)
{
	osync_assert(xmlformat);
	
	OSyncXMLFormatSchema * schema = osync_xmlformat_schema_get_instance(xmlformat, error);
	return osync_xmlformat_schema_validate(schema, xmlformat, error);
}

/**
 * @brief Sort all xmlfields of the xmlformat. This function has to
 *  be called after a xmlfield was added to the xmlformat.
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_xmlformat_sort(OSyncXMLFormat *xmlformat)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlformat);
	osync_assert(xmlformat);
	
	int index;
	OSyncXMLField *cur;
	
	if(xmlformat->child_count <= 1) {
		osync_trace(TRACE_INTERNAL, "child_count <= 1 - no need to sort");
		goto end;
	}
	
	void **list = g_malloc0(sizeof(OSyncXMLField *) * xmlformat->child_count);
	
	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	for(; cur != NULL; cur = osync_xmlfield_get_next(cur)) {
		list[index] = cur;
		index++;
		xmlUnlinkNode(cur->node);
	}
	
	qsort(list, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compare_stdlib);
	
	/** bring the xmlformat and the xmldoc in a consistent state */
	xmlformat->first_child = ((OSyncXMLField *)list[0])->node->_private;
	xmlformat->last_child = ((OSyncXMLField *)list[xmlformat->child_count-1])->node->_private;

	for(index = 0; index < xmlformat->child_count; index++) {
		cur = (OSyncXMLField *)list[index];
		xmlAddChild(xmlDocGetRootElement(xmlformat->doc), cur->node);
			
		if(index < xmlformat->child_count-1)
			cur->next = (OSyncXMLField *)list[index+1];
		else
			cur->next = NULL;
		
		if(index)
			cur->prev = (OSyncXMLField *)list[index-1];
		else
			cur->prev = NULL;
	}
	g_free(list);

end:	
	xmlformat->sorted = TRUE;
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * @brief Check if all xmlfields of the xmlformat are sorted.
 * @param xmlformat The pointer to a xmlformat object
 * @returns TRUE if sorted, FALSE otherwise
 */
osync_bool osync_xmlformat_is_sorted(OSyncXMLFormat *xmlformat)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlformat);
	osync_assert(xmlformat);
	
	OSyncXMLField *cur, *prev = NULL;
	
	/* No need to check if sorted when 1 or less xmlfields */
	if (xmlformat->child_count <= 1)
		return TRUE;
	
	cur = osync_xmlformat_get_first_field(xmlformat);
	for(; cur != NULL; cur = osync_xmlfield_get_next(cur)) {

		/* Equal when returns 0, like strcmp() */
		if (prev && osync_xmlfield_compare_stdlib(&prev, &cur) > 0)
			return FALSE;

		prev = cur;
	}

	return TRUE;
}


/**
 * @brief Returns true if the copy succedded, false otherwise
 *
 * @return Boolean status of the copy operation.
 */ 
osync_bool osync_xmlformat_copy(OSyncXMLFormat *source, OSyncXMLFormat **destination, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, source, destination);

        char *buffer = NULL;
        unsigned int size;

        osync_xmlformat_assemble(source, &buffer, &size);
        *destination = osync_xmlformat_parse(buffer, size, error);
        if (!(*destination)) {
                osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
                return FALSE;
        }

	if (source->sorted) (*destination)->sorted = TRUE;



        g_free(buffer);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

/**
 * @brief Returns the size of the OSyncXMLFormat struct.
 *
 * This is needed since the struct itself is private.
 *
 * @return The size of OSyncXMLFormat struct. 
 */
unsigned int osync_xmlformat_size()
{
	return sizeof(OSyncXMLFormat);
}

/*@}*/
