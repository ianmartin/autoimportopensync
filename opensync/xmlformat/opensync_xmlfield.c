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
#include "opensync_xmlformat_private.h"		/* FIXME: direct access of private header */

#include "opensync_xmlfield_private.h"
#include "opensync_xmlfield_internals.h"

/**
 * @defgroup OSyncXMLFieldPrivateAPI OpenSync XMLField Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLField
 * 
 */
/*@{*/

/**
 * @brief Creates a new xmlfield object which will be added to the end of xmlfields of the xmlformat object.
 *  The returned object will be freed with the xmlformat object.
 * @param xmlformat The pointer to a xmlformat object
 * @param node The node must be already inserted at the end of childs of the xmlDoc root element
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlfield object or NULL in case of error
 */
 OSyncXMLField *osync_xmlfield_new_node(OSyncXMLFormat *xmlformat, xmlNodePtr node, OSyncError **error)
{
	OSyncXMLField *xmlfield = osync_try_malloc0(sizeof(OSyncXMLField), error);
	if(!xmlfield) {
		osync_trace(TRACE_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	xmlfield->next = NULL;
	xmlfield->node = node;
	xmlfield->prev = xmlformat->last_child;
	node->_private = xmlfield;
	
	if(!xmlformat->first_child)
		xmlformat->first_child = xmlfield;
	if(xmlformat->last_child)
		xmlformat->last_child->next = xmlfield;
	xmlformat->last_child = xmlfield;
	xmlformat->child_count++;

	// We don't know if the parsed xmlformat got sorted xmlfield -> unsorted
	xmlfield->sorted = FALSE;
	
	return xmlfield;
}

/**
 * @brief Frees a already unlinked xmlfield object
 * @param xmlfield The pointer to a xmlfield object
 */
void osync_xmlfield_free(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	xmlFreeNode(xmlfield->node);
	g_free(xmlfield);
}

/**
 * @brief Unlink a xmlfield object  
 * @param xmlfield The pointer to a xmlfield object
 */
void osync_xmlfield_unlink(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	xmlUnlinkNode(xmlfield->node);
	if(!xmlfield->prev)
		((OSyncXMLFormat *)xmlfield->node->doc->_private)->first_child = xmlfield->next;
	if(xmlfield->prev)
		xmlfield->prev->next = xmlfield->next;
	if(xmlfield->next)
		xmlfield->next->prev = xmlfield->prev;
	xmlfield->next = NULL;
	xmlfield->prev = NULL;
	((OSyncXMLFormat *)xmlfield->node->doc->_private)->child_count--;
}

/**
 * @brief Compare the names of two xmlfields  
 * @param xmlfield1 The pointer to a xmlfield object
 * @param xmlfield2 The pointer to a xmlfield object
 * @returns same as strcmp(), 0 is equal
 */
int osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2)
{
	return strcmp(osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield1), osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield2));
}

/**
 * @brief Compare the key names of two xmlfield key nodes
 * @param key1 The pointer to a xmlNodePtr
 * @param key2 The pointer to a xmlNodePtr
 * @returns same as strcmp(), 0 is equal
 */
int osync_xmlfield_key_compare_stdlib(const void *key1, const void *key2)
{
	return strcmp((const char*) (*(xmlNodePtr *) key1)->name, (const char*) (*(xmlNodePtr *) key2)->name);
}

/** 
 * @brief Unlink a xmlfield from its context and frees it
 * @param xmlfield The pointer to a xmlfield object
 */
void osync_xmlfield_delete(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);

	osync_xmlfield_unlink(xmlfield);
	osync_xmlfield_free(xmlfield);  
}

/**
 * @brief Links a xmlfield object from a xmlformat object before a other xmlfield object of a other xmlformat object  
 * @param xmlfield The pointer to a xmlfield object
 * @param to_link The pointer to a xmlfield object
 */
void osync_xmlfield_adopt_xmlfield_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
{
	osync_assert(xmlfield);
	osync_assert(to_link);

	osync_xmlfield_unlink(to_link);

	xmlDOMWrapAdoptNode(NULL, to_link->node->doc, to_link->node, xmlfield->node->doc, xmlfield->node, 0);
	xmlAddPrevSibling(xmlfield->node, to_link->node);

	to_link->next = xmlfield;
	to_link->prev = xmlfield->prev;

	if(xmlfield->prev)
		xmlfield->prev->next = to_link;
	else
		((OSyncXMLFormat *)xmlfield->node->doc->_private)->first_child = to_link;
	xmlfield->prev = to_link;
	((OSyncXMLFormat *)xmlfield->node->doc->_private)->child_count++;
}

/**
 * @brief Links a xmlfield object from a xmlformat object after a other xmlfield object of a other xmlformat object
 * @param xmlfield The pointer to a xmlfield object
 * @param to_link The pointer to a xmlfield object
 */
void osync_xmlfield_adopt_xmlfield_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
{
	osync_assert(xmlfield);
	osync_assert(to_link);

	osync_xmlfield_unlink(to_link);

	xmlDOMWrapAdoptNode(NULL, to_link->node->doc, to_link->node, xmlfield->node->doc, xmlfield->node, 0);
	xmlAddNextSibling(xmlfield->node, to_link->node);

	to_link->next = xmlfield->next;
	to_link->prev = xmlfield;

	if(xmlfield->next)
		xmlfield->next->prev = to_link;
	else
		((OSyncXMLFormat *)xmlfield->node->doc->_private)->last_child = to_link;
	xmlfield->next = to_link;
	((OSyncXMLFormat *)xmlfield->node->doc->_private)->child_count++;
}

/*@}*/

OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, xmlformat, name, error);
	osync_assert(xmlformat);
	osync_assert(name);
	
	xmlNodePtr node = xmlNewTextChild(xmlDocGetRootElement(xmlformat->doc), NULL, BAD_CAST name, NULL);
	
	OSyncXMLField *xmlfield = osync_xmlfield_new_node(xmlformat, node, error);
	if(!xmlfield) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	// XMLFormat entry got added - not sure if it is still sorted
	xmlformat->sorted = FALSE;

	// This XMLField has no keys, so it's for sure it's sorted
	xmlfield->sorted = TRUE;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlfield);
	return xmlfield;
}

const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	return (const char *)xmlfield->node->name;
}

void osync_xmlfield_set_name(OSyncXMLField *xmlfield, const char *name)
{
	osync_assert(xmlfield);
	osync_assert(name);

	xmlNodeSetName(xmlfield->node, BAD_CAST name);	
}

OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	return xmlfield->next;
}

const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr)
{
	osync_assert(xmlfield);
	osync_assert(attr);
	
	xmlAttrPtr prop;
	prop = xmlHasProp(xmlfield->node, BAD_CAST attr);
	if(prop == NULL)
		return NULL;
	return (const char *)osync_xml_attr_get_content(prop);
//	return (const char *)prop->children->content;
//	return (const char *)xmlGetProp(xmlfield->node, BAD_CAST attr);
}

void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(attr);
	osync_assert(value);
	
	xmlSetProp(xmlfield->node, BAD_CAST attr, BAD_CAST value);
}

int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++)
		attr = attr->next;
	return count;
}

const char *osync_xmlfield_get_nth_attr_name(OSyncXMLField *xmlfield, int nth)
{
	osync_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++) {
		if(count == nth)
			return (const char *)attr->name;
		attr = attr->next;
	}
	return NULL;
}

const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth)
{
	osync_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++) {
		if(count == nth)
			return (const char *)osync_xml_attr_get_content(attr);
//			return (const char *)attr->children->content;
//			return (const char *)xmlNodeGetContent((xmlNodePtr)attr);
		attr = attr->next;
	}
	return NULL;
}

const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key)
{
	osync_assert(xmlfield);
	osync_assert(key);
	
	xmlNodePtr cur = xmlfield->node->children;
	for(; cur != NULL; cur = cur->next) {
		if(!xmlStrcmp(cur->name, BAD_CAST key))
				return (const char *)osync_xml_node_get_content(cur);
//				return (const char *)cur->children->content;
//				return (const char *)xmlNodeGetContent(cur);
	}
	return NULL;
}

void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(key);

	// If value is null or empty we don't add it to a xmlfield
	if (!value || strlen(value) == 0)
		return;

	xmlNodePtr cur = xmlfield->node->children;
	for(; cur != NULL; cur = cur->next) {
		if(!xmlStrcmp(cur->name, BAD_CAST key)) {
			xmlNodeSetContent(xmlfield->node, BAD_CAST value);
			break;
		}
	}
	if(cur == NULL)
		xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);

	xmlfield->sorted = FALSE;
}

void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(key);
	osync_assert(value);

	xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);

	xmlfield->sorted = FALSE;
}

int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	int count;
	xmlNodePtr child = xmlfield->node->children;
	
	for(count=0; child != NULL; count++) {
		child = child->next;
	}
	return count;
}

const char *osync_xmlfield_get_nth_key_name(OSyncXMLField *xmlfield, int nth)
{
	osync_assert(xmlfield);
	
	int count;
	xmlNodePtr child = xmlfield->node->children;
	
	for(count=0; child != NULL; count++) {
		if(count == nth)
			return (const char *)child->name;
		child = child->next;
	}
	return NULL;
}

const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth)
{
	osync_assert(xmlfield);
	
	int count;
	xmlNodePtr child = xmlfield->node->children;
	
	for(count=0; child != NULL; count++) {
		if(count == nth)
			return (const char *)osync_xml_node_get_content(child);
//			return (const char *)child->children->content;
//			return (const char *)xmlNodeGetContent(child);
		child = child->next;
	}
	return NULL;
}

void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(value);
	
	int count;
	xmlNodePtr cur = xmlfield->node->children;
	
	for(count = 0; cur != NULL ; count++) {
		if(count == nth)
			xmlNodeSetContent(cur, BAD_CAST value);		
		cur = cur->next;
	}
}

void osync_xmlfield_sort(OSyncXMLField *xmlfield)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlfield);
	osync_assert(xmlfield);
	
	int index, count;

	if (xmlfield->sorted) {
		osync_trace(TRACE_INTERNAL, "already sorted");
		goto end;
	}

	count = osync_xmlfield_get_key_count(xmlfield);
	if( count <= 1 ) {
		osync_trace(TRACE_INTERNAL, "attribute count <= 1 - no need to sort");
		goto end;
	}
	
	void **list = g_malloc0(sizeof(xmlNodePtr) * count);
	
	xmlNodePtr cur = xmlfield->node->children;
	for (index=0; cur != NULL; index++) {
		xmlNodePtr tmp = cur;
		list[index] = cur;
		cur = cur->next;
		xmlUnlinkNode(tmp);
	}
	
	qsort(list, count, sizeof(xmlNodePtr), osync_xmlfield_key_compare_stdlib);
	
	for(index = 0; index < count; index++) {
		cur = (xmlNodePtr)list[index];
		xmlAddChild(xmlfield->node, cur);
			
		if(index < count-1)
			cur->next = (xmlNodePtr)list[index+1];
		else
			cur->next = NULL;
		
		if(index)
			cur->prev = (xmlNodePtr)list[index-1];
		else
			cur->prev = NULL;
	}
	g_free(list);

end:	
	xmlfield->sorted = TRUE;
	osync_trace(TRACE_EXIT, "%s", __func__);
}

