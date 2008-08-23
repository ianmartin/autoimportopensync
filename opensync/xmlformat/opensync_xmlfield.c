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
 OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node, OSyncError **error)
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
void _osync_xmlfield_free(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	xmlFreeNode(xmlfield->node);
	g_free(xmlfield);
}

/**
 * @brief Unlink a xmlfield object  
 * @param xmlfield The pointer to a xmlfield object
 */
void _osync_xmlfield_unlink(OSyncXMLField *xmlfield)
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
int _osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2)
{
	return strcmp(osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield1), osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield2));
}

/**
 * @brief Compare the key names of two xmlfield key nodes
 * @param key1 The pointer to a xmlNodePtr
 * @param key2 The pointer to a xmlNodePtr
 * @returns same as strcmp(), 0 is equal
 */
int _osync_xmlfield_key_compare_stdlib(const void *key1, const void *key2)
{
	return strcmp((const char*) (*(xmlNodePtr *) key1)->name, (const char*) (*(xmlNodePtr *) key2)->name);
}

/*@}*/

/**
 * @defgroup OSyncXMLFieldAPI OpenSync XMLField
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLField
 * 
 */
/*@{*/

 /**
 * @brief Creates a new xmlfield object which will be added to the end of xmlfields of the xmlformat object.
 *  The returned object will be freed with the xmlformat object.
 * @param xmlformat The pointer to a xmlformat object
 * @param name The name of the xmlfield
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, xmlformat, name, error);
	osync_assert(xmlformat);
	osync_assert(name);
	
	xmlNodePtr node = xmlNewTextChild(xmlDocGetRootElement(xmlformat->doc), NULL, BAD_CAST name, NULL);
	
	OSyncXMLField *xmlfield = _osync_xmlfield_new(xmlformat, node, error);
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

/** 
 * @brief Unlink a xmlfield from its context and frees it
 * @param xmlfield The pointer to a xmlfield object
 */
void osync_xmlfield_delete(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	_osync_xmlfield_unlink(xmlfield);
	_osync_xmlfield_free(xmlfield);	
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

	_osync_xmlfield_unlink(to_link);

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
	
	_osync_xmlfield_unlink(to_link);
	
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

/**
 * @brief Get the name of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @return The name of the xmlfield
 */
const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	return (const char *)xmlfield->node->name;
}

/**
 * @brief Set the name of a xmlfield
 * @param name The name to set
 * @param xmlfield The pointer to a xmlfield object
 */
void osync_xmlfield_set_name(OSyncXMLField *xmlfield, const char *name)
{
	osync_assert(xmlfield);
	osync_assert(name);

	xmlNodeSetName(xmlfield->node, BAD_CAST name);	
}

/**
 * @brief Get the next xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @return The pointer to the next xmlfield or NULL if there is no more xmlfield
 */
OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	return xmlfield->next;
}

/**
 * @brief Get the value of a attribute of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param attr The name of the attribute
 * @return The value of the attribute of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Set the attribute name and the attribute value of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param attr The name of the attribute
 * @param value The value of the attribute
 */
void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(attr);
	osync_assert(value);
	
	xmlSetProp(xmlfield->node, BAD_CAST attr, BAD_CAST value);
}

/**
 * @brief Get the count of attributes of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @return The count of attributes of the xmlfield
 */
int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield)
{
	osync_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++)
		attr = attr->next;
	return count;
}

/**
 * @brief Get the name of the nth attribute of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param nth The number of the attribute
 * @return The name of the nth attribute of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Get the value of the nth attribute of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param nth The number of the attribute
 * @return The value of the nth attribute of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Get the value of a key/value pair of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param key The key of the key/value pair
 * @return The value of the key/value pair of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Set the key/value pair of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param key The key of the key/value pair
 * @param value The value of the key/value pair
 */
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

/**
 * @brief Add the key/value pair to a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param key The key of the key/value pair
 * @param value The value of the key/value pair
 */
void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	osync_assert(xmlfield);
	osync_assert(key);
	osync_assert(value);

	xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);

	xmlfield->sorted = FALSE;
}

/**
 * @brief Get the count of keys of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @return The count of keys of the xmlfield
 */
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

/**
 * @brief Get the name of the nth key of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param nth The number of the key
 * @return The name of the nth key of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Get the value of the nth key of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param nth The number of the key
 * @return The value of the nth key of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Set the value of the nth key of a xmlfield
 * @param xmlfield The pointer to a xmlfield object
 * @param nth The number of the key
 * @param value The value of the key/value pair
 * @return The value of the nth key of the xmlfield or NULL in case of error
 */
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

/**
 * @brief Compares two xmlfield objects with each other
 * @param xmlfield1 The pointer to a xmlformat object
 * @param xmlfield2 The pointer to a xmlformat object
 * @return TRUE if both xmlfield objects are the same otherwise FALSE
 */
osync_bool osync_xmlfield_compare(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, xmlfield1, xmlfield2);
	osync_assert(xmlfield1);
	osync_assert(xmlfield2);
	
	int i;	
	osync_bool same;
	
	if(xmlStrcmp(xmlfield1->node->name, xmlfield2->node->name)) {
		osync_trace(TRACE_EXIT, "%s: %i", __func__, FALSE);
		return FALSE;
	}

	same = TRUE;
	xmlNodePtr key1 = xmlfield1->node->children;
	xmlNodePtr key2 = xmlfield2->node->children;
	
	while(same)
	{
		if(key1 == NULL && key2 == NULL) {
			break;
		}
			
		if(key1 == NULL || key2 == NULL) {
			same = FALSE;
			break;	
		}
		
		GSList *keylist1;
		GSList *keylist2;
		keylist1 = NULL;
		keylist2 = NULL;
		
		const char *curkeyname = (const char *)key1->name;
		do {
			keylist1 = g_slist_prepend(keylist1, key1);
			key1 = key1->next;
			if(key1 == NULL)
				break;
			i = strcmp((const char *)key1->name, curkeyname);
		} while(i == 0);
		
		do {
			keylist2 = g_slist_prepend(keylist2, key2);
			key2 = key2->next;
			if(key2 == NULL)
				break;
			i = strcmp((const char *)key2->name, curkeyname);
		} while(i == 0);	
		
		do{
			/* both lists must have the same length */	
			if(g_slist_length(keylist1) != g_slist_length(keylist2)) {
				osync_trace(TRACE_INTERNAL, "It's not the same anymore...");
				same = FALSE;
				break;
			}
						
			GSList *cur_list1;
			GSList *cur_list2;
			
			do {
				cur_list1 = keylist1;
				cur_list2 = keylist2;
	
				do {
					if(!xmlStrcmp(osync_xml_node_get_content(cur_list1->data), osync_xml_node_get_content(cur_list2->data)))
						break;
					cur_list2 = g_slist_next(cur_list2);
					if(cur_list2 == NULL) {
						same = FALSE;	
						break;
					}
				}while(1);
				
				if(same) {
					keylist1 = g_slist_delete_link(keylist1, cur_list1);
					keylist2 = g_slist_delete_link(keylist2, cur_list2);
				}else
					break;
				
			}while(keylist1 != NULL);
		}while(0);	
		
		if(keylist1)
			g_slist_free(keylist1);
		if(keylist2)
			g_slist_free(keylist2);
	}
	
	/* now we check if the attributes are equal */
	do {
		if(!same)
			break;
		
		int i1 =osync_xmlfield_get_attr_count(xmlfield1);
		int i2 =osync_xmlfield_get_attr_count(xmlfield2);
		
		if(i1 != i2) {
			same = FALSE;
			break;
		}
		
		for(i=0; i < i1; i++) {
			const char *attrvalue = osync_xmlfield_get_attr(xmlfield2, osync_xmlfield_get_nth_attr_name(xmlfield1, i));
			if( attrvalue == NULL ||
				strcmp(attrvalue, osync_xmlfield_get_nth_attr_value(xmlfield1, i))) {
				same = FALSE;
				break;	
			}
		}
	} while(0);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, same);
	return same;
}

/**
 * @brief Compares two xmlfield objects with each other of similarity
 * @param xmlfield1 The pointer to a xmlformat object
 * @param xmlfield2 The pointer to a xmlformat object
 * @param keys OSyncXMLPoints::keys
 * @return TRUE if both xmlfield objects are the similar otherwise FALSE
 */
osync_bool osync_xmlfield_compare_similar(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2, char* keys[])
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, xmlfield1, xmlfield2, keys);
	osync_assert(xmlfield1);
	osync_assert(xmlfield2);
	
	osync_bool res = TRUE;
	if(strcmp((const char *) xmlfield1->node->name, (const char *) xmlfield2->node->name) != 0)
		res = FALSE;

	xmlNodePtr node1, node2;
	node1 = xmlfield1->node->children;
	node2 = xmlfield2->node->children;

	int i;
	for(i=0; keys[i]; i++) {
		GSList *list1;
		GSList *list2;
		list1 = NULL;
		list2 = NULL;
		
		while(node1 != NULL) {
			if(strcmp(keys[i], (const char *)node1->name) == 0)
				list1 = g_slist_prepend(list1, node1);
			node1 = node1->next;
		};
		
		while(node2 != NULL) {
			if(strcmp(keys[i], (const char *)node2->name) == 0)
				list2 = g_slist_prepend(list2, node2);
			node2 = node2->next;
		};
		
		GSList *cur_list1;
		GSList *cur_list2;
		
		while(list1 != NULL)
		{
			cur_list1 = list1;
			cur_list2 = list2;
			
			if(cur_list2 == NULL) {
				res = FALSE;
				break;	
			}
			
			while(xmlStrcmp(osync_xml_node_get_content((xmlNodePtr)cur_list1->data),
							 osync_xml_node_get_content((xmlNodePtr)cur_list2->data))) {		
//			while(strcmp((const char *)xmlNodeGetContent(cur_list1->data),
//						 (const char *)xmlNodeGetContent(cur_list2->data)) != 0) {
				cur_list2 = g_slist_next(cur_list2);
				if(cur_list2 == NULL) {
					res = FALSE;	
					break;
				}
			}
			
			if(res) {
				list1 = g_slist_delete_link(list1, cur_list1);
				list2 = g_slist_delete_link(list2, cur_list2);
			}else
				break;
		}
		if(list2 != NULL)
			res = FALSE;
			
		if(!res) {
			if(list1 != NULL)
				g_slist_free(list1);
			if(list2 != NULL)
				g_slist_free(list2);
		}
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, res);
	return res;
}

/**
 * @brief Sort all key nodes  of the xmlfield. This function have to be called
 *  if the xmlfield got build in a unsorted way. Sorting is not needed if the
 *  xmfield got build in a sorted way.
 * @param xmlfield The pointer to a xmlfield object
 */
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
	
	qsort(list, count, sizeof(xmlNodePtr), _osync_xmlfield_key_compare_stdlib);
	
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

/*@}*/
