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

const char *_osync_xmlfield_get_sortname(void *xmlfield)
{
	return (const char *)((OSyncXMLField *)xmlfield)->node->name;
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
 * 
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

/*! @brief Frees a xmlfield
 * 
 * @param xmlfield The xmlfield to free
 * 
 */
void osync_xmlfield_free(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	g_free(xmlfield);
}

OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	return xmlfield->next;
}

const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	return (const char *) xmlfield->node->name;
}

const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr)
{
	g_assert(xmlfield);
	g_assert(attr);
	
	return (const char *)xmlGetProp(xmlfield->node, BAD_CAST attr);
}

void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value)
{
	g_assert(xmlfield);
	g_assert(attr);
	g_assert(value);
	
	xmlNewProp(xmlfield->node, BAD_CAST attr, BAD_CAST value);
}

const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key)
{
	g_assert(xmlfield);
	g_assert(key);
	
	xmlNodePtr cur = xmlfield->node->children;
	while(cur != NULL)
	{
		if(!xmlStrcmp(cur->name, BAD_CAST key))
			return (const char *)xmlNodeGetContent(cur);
		cur = cur->next;
	}
	return NULL;
}

void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value)
{
	g_assert(xmlfield);
	g_assert(key);
	g_assert(value);

	xmlNodePtr cur = xmlfield->node->children;
	while(cur != NULL)
	{
		if(!xmlStrcmp(cur->name, BAD_CAST key))
		{
			xmlNodeSetContent(xmlfield->node, BAD_CAST value);
			break;			
		}
		cur = cur->next;
	}
	if(cur == NULL)
		xmlNewTextChild(xmlfield->node, NULL, BAD_CAST key, BAD_CAST value);
}

int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield)
{
	g_assert(xmlfield);
	
	int count = 0;
	xmlNodePtr child = xmlfield->node->children;
	while(child != NULL)
	{
		count++;
		child = child->next;
	}
	return count;
}

const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth)
{
	g_assert(xmlfield);
	
	int count = 0;
	xmlNodePtr child = xmlfield->node->children;
	
	while(child != NULL)
	{
		if(count == nth)
		{
			return (const char *)xmlNodeGetContent(child);
		}
		count++;
		child = child->next;
	}
	return NULL;
}

void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value)
{
	g_assert(xmlfield);
	g_assert(value);
	
	int i;
	xmlNodePtr cur = xmlfield->node->children;
	for(i = 0; i < nth; i++)
	{
		if(cur->next)
			cur = cur->next;
		else
			return;		
	}
	xmlNodeSetContent(cur, BAD_CAST value);
}

void osync_xmlfield_unlink(OSyncXMLField *xmlfield)
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

void osync_xmlfield_link_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
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

void osync_xmlfield_link_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link)
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

int osync_xmlfield_compaire(const void *xmlfield1, const void *xmlfield2)
{
	//g_assert(*(void **)xmlfield1);
	//g_assert(*(void **)xmlfield2);
	return strcmp(osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield1), osync_xmlfield_get_name(*(OSyncXMLField **)xmlfield2));
}

OSyncXMLField *osync_xmlfield_insert_copy_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_copy)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlfield);
	g_assert(xmlfield);
	g_assert(to_copy);
	
	OSyncXMLField *newxmlfield = g_malloc0(sizeof(OSyncXMLField));
	xmlNodePtr node = xmlCopyNode(to_copy->node, 1);

	node = xmlAddPrevSibling(xmlfield->node, node);
	
	newxmlfield->next = xmlfield;
	newxmlfield->node = node;
	newxmlfield->prev = xmlfield->prev;
	node->_private = newxmlfield;

	if(xmlfield->prev)
		xmlfield->prev->next = newxmlfield;
	else
		((OSyncXMLFormat *)xmlfield->node->doc->_private)->first_child = newxmlfield;
	xmlfield->prev = newxmlfield;
	
	osync_trace(TRACE_EXIT, "%s");
	return newxmlfield;
}

/*@}*/
