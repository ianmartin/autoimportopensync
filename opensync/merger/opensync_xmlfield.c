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
 * @defgroup OSyncXMLFieldPrivateAPI OpenSync XMLField Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLField
 * 
 */
/*@{*/

/** internal given node must be already inserted at the end of the xmlDoc root element */
OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node)
{
	OSyncXMLField *xmlfield = g_malloc0(sizeof(OSyncXMLField));
	
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
	
	return xmlfield;
}

void _osync_xmlfield_free(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	g_free(xmlfield);
}

void _osync_xmlfield_unlink(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	xmlUnlinkNode(xmlfield->node);
	if(xmlfield->prev)
		xmlfield->prev->next = xmlfield->next;
	if(xmlfield->next)
		xmlfield->next->prev = xmlfield->prev;
	xmlfield->next = NULL;
	xmlfield->prev = NULL;
	((OSyncXMLFormat *)xmlfield->node->doc->_private)->child_count--;
}

void _osync_xmlfield_link_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
{
	g_assert(xmlfield);
	g_assert(to_link);

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

void _osync_xmlfield_link_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
{
	g_assert(xmlfield);
	g_assert(to_link);

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

int _osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2)
{
	return strcmp(osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield1), osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield2));
}


xmlChar *_osync_xmlfield_node_get_content(xmlNodePtr node)
{
	if(node->children && node->children->content)
		return node->children->content;
		
	return (xmlChar *)"";
}

xmlChar *_osync_xmlfield_attr_get_content(xmlAttrPtr node)
{
	if(node->children && node->children->content)
		return node->children->content;
		
	return (xmlChar *)"";
}

/*@}*/

/**
 * @defgroup OSyncXMLFieldAPI OpenSync XMLField
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLField
 * 
 */
/*@{*/

/*! @brief Creates a new xmlfield of a xmlfield.
 * The returned OSyncXMLField will be freed with the given OSyncXMLFormat object.
 * @param xmlfield The xmlfield to free
 * 
 */
OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name)
{
	g_assert(xmlformat);
	g_assert(name);
	
	xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(xmlformat->doc), NULL, BAD_CAST name, NULL);
	return _osync_xmlfield_new(xmlformat, node);
}

const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	return (const char *) xmlfield->node->name;
}

OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	return xmlfield->next;
}

const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr)
{
	g_assert(xmlfield);
	g_assert(attr);
	
    xmlAttrPtr prop;
    prop = xmlHasProp(xmlfield->node, BAD_CAST attr);
    if(prop == NULL)
    	return NULL;
	return (const char *)_osync_xmlfield_attr_get_content(prop);
//	return (const char *)prop->children->content;
//	return (const char *)xmlGetProp(xmlfield->node, BAD_CAST attr);
}

void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value)
{
	g_assert(xmlfield);
	g_assert(attr);
	g_assert(value);
	
	xmlSetProp(xmlfield->node, BAD_CAST attr, BAD_CAST value);
}

int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++) {
		attr = attr->next;
	}
	return count;
}

const char *osync_xmlfield_get_nth_attr_name(OSyncXMLField *xmlfield, int nth)
{
	g_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attribute != NULL; count++) {
		if(count == nth)
			return (const char *)attr->name;
		attr = attr->next;
	}
	return NULL;
}

const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth)
{
	g_assert(xmlfield);
	
	int count;
	xmlAttrPtr attr = xmlfield->node->properties;
	
	for(count=0; attr != NULL; count++) {
		if(count == nth)
			return (const char *)_osync_xmlfield_attr_get_content(attr);
//			return (const char *)attr->children->content;
//			return (const char *)xmlNodeGetContent((xmlNodePtr)attr);
		attr = attr->next;
	}
	return NULL;
}

const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key)
{
	g_assert(xmlfield);
	g_assert(key);
	
	xmlNodePtr cur = xmlfield->node->children;
	for(; cur != NULL; cur = cur->next) {
		if(!xmlStrcmp(cur->name, BAD_CAST key))
				return (const char *)_osync_xmlfield_node_get_content(cur);
//				return (const char *)cur->children->content;
//				return (const char *)xmlNodeGetContent(cur);
	}
	return NULL;
}

void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	g_assert(xmlfield);
	g_assert(key);
	g_assert(value);

	xmlNodePtr cur = xmlfield->node->children;
	for(; cur != NULL; cur = cur->next) {
		if(!xmlStrcmp(cur->name, BAD_CAST key)) {
			xmlNodeSetContent(xmlfield->node, BAD_CAST value);
			break;
		}
	}
	if(cur == NULL)
		xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);
}

void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	g_assert(xmlfield);
	g_assert(key);
	g_assert(value);

	xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);
}

int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	int count;
	xmlNodePtr child = xmlfield->node->children;
	
	for(count=0; child != NULL; count++) {
		child = child->next;
	}
	return count;
}

const char *osync_xmlfield_get_nth_key_name(OSyncXMLField *xmlfield, int nth)
{
	g_assert(xmlfield);
	g_assert(nth);
	
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
	g_assert(xmlfield);
	
	int count;
	xmlNodePtr child = xmlfield->node->children;
	
	for(count=0; child != NULL; count++) {
		if(count == nth)
			return (const char *)_osync_xmlfield_node_get_content(child);
//			return (const char *)child->children->content;
//			return (const char *)xmlNodeGetContent(child);
		child = child->next;
	}
	return NULL;
}

void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value)
{
	g_assert(xmlfield);
	g_assert(value);
	
	int count;
	xmlNodePtr cur = xmlfield->node->children;
	
	for(count = 0; cur != NULL ; count++) {
		if(count == nth)
			xmlNodeSetContent(cur, BAD_CAST value);		
		cur = cur->next;
	}
}

osync_bool osync_xmlfield_compare(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2)
{
	g_assert(xmlfield1);
	g_assert(xmlfield2);
	
	int i;	
	osync_bool same;
	if(xmlStrcmp(xmlfield1->node->name, xmlfield2->node->name))
			return FALSE;

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
				same = FALSE;
				break;
			}
						
			GSList *cur_list1;
			GSList *cur_list2;
			
			do {
				cur_list1 = keylist1;
				cur_list2 = keylist2;
	
				do {
					if(!xmlStrcmp(_osync_xmlfield_node_get_content(cur_list1->data), _osync_xmlfield_node_get_content(cur_list2->data)))
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
	return same;
}

osync_bool osync_xmlfield_compare_similar(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2, char* keys[])
{
	g_assert(xmlfield1);
	g_assert(xmlfield2);
	
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
			
			while(!xmlStrcmp(_osync_xmlfield_node_get_content((xmlNodePtr)cur_list1->data),
							 _osync_xmlfield_node_get_content((xmlNodePtr)cur_list2->data))) {		
//			while(strcmp((const char *)xmlNodeGetContent(cur_list1->data),
//						 (const char *)xmlNodeGetContent(cur_list2->data)) != 0) {
				cur_list2 = g_slist_next(cur_list2);
				if(cur_list2 == NULL) {
					res = FALSE;	
					break;
				}
			};
			
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
	return res;
}

//OSyncXMLField *osync_xmlfield_insert_copy_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_copy)
//{
//	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlfield);
//	g_assert(xmlfield);
//	g_assert(to_copy);
//	
//	OSyncXMLField *newxmlfield = g_malloc0(sizeof(OSyncXMLField));
//	xmlNodePtr node = xmlCopyNode(to_copy->node, 1);
//
//	node = xmlAddPrevSibling(xmlfield->node, node);
//	
//	newxmlfield->next = xmlfield;
//	newxmlfield->node = node;
//	newxmlfield->prev = xmlfield->prev;
//	node->_private = newxmlfield;
//
//	if(xmlfield->prev)
//		xmlfield->prev->next = newxmlfield;
//	else
//		((OSyncXMLFormat *)xmlfield->node->doc->_private)->first_child = newxmlfield;
//	xmlfield->prev = newxmlfield;
//	
//	osync_trace(TRACE_EXIT, "%s");
//	return newxmlfield;
//}

/*@}*/
