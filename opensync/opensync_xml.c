/*
 * xml - A plugin for xml objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

xmlNode *osxml_node_add_root(xmlDoc *doc, const char *name)
{
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)name, NULL);
	return doc->children;
}

xmlNode *osxml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error)
{
	xmlNode *cur = xmlDocGetRootElement(doc);
	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		return NULL;
	}
	
	if (xmlStrcmp((cur)->name, (const xmlChar *) name)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		return NULL;
	}
	
	cur = (cur)->xmlChildrenNode;
	return cur;
}

void osxml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding)
{
	if (name)
		xmlNodeSetName(node, (xmlChar*)name); //FIXME Free previous name?
		
	if (data)
		xmlNewTextChild(node, NULL, (xmlChar*)"Content", (xmlChar*)data);
}

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data)
{
	if (!data)
		return NULL;
	if (strlen(data) == 0)
		return NULL;
	xmlNode *node = xmlNewTextChild(parent, NULL, (xmlChar*)name, (xmlChar*)data);
	return node;
}

void osxml_node_add_property(xmlNode *parent, const char *name, const char *data)
{
	xmlNewProp(parent, (xmlChar*)name, (xmlChar*)data);
}

void osxml_node_mark_unknown(xmlNode *parent)
{
	if (!xmlHasProp(parent, (xmlChar*)"Type"))
		xmlNewProp(parent, (xmlChar*)"Type", (xmlChar*)"Unknown");
}

void osxml_node_remove_unknown_mark(xmlNode *node)
{
	xmlAttr *attr = xmlHasProp(node, (xmlChar*)"Type");
	if (!attr)
		return;
	xmlRemoveProp(attr);
}

xmlNode *osxml_get_node(xmlNode *parent, const char *name)
{
	xmlNode *cur = (parent)->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)name))
			return cur;
		cur = cur->next;
	}
	return NULL;
}

char *osxml_find_node(xmlNode *parent, const char *name)
{
	return (char*)xmlNodeGetContent(osxml_get_node(parent, name));
}

xmlXPathObject *osxml_get_nodeset(xmlDoc *doc, const char *expression)
{
	xmlXPathContext *xpathCtx = NULL;
	xmlXPathObject *xpathObj = NULL;
    
    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        fprintf(stderr,"Error: unable to create new XPath context\n");
        return NULL;
    }
    
    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression((xmlChar*)expression, xpathCtx);
    if(xpathObj == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", expression);
        xmlXPathFreeContext(xpathCtx); 
        return NULL;
    }

	xmlXPathFreeContext(xpathCtx);
	/* Cleanup of XPath data */
   // xmlXPathFreeObject(xpathObj);
   return xpathObj;
}

xmlXPathObject *osxml_get_unknown_nodes(xmlDoc *doc)
{
	return osxml_get_nodeset(doc, "/*/*[@Type='Unknown']");
}

void osxml_map_unknown_param(xmlNode *node, const char *paramname, const char *newname)
{
	xmlNode *cur = node->xmlChildrenNode;
	while (cur) {
		if (xmlStrcmp(cur->name, (const xmlChar *)"UnknownParam"))
			goto next;
		
		char *name = osxml_find_node(cur, "ParamName");
		char *content = osxml_find_node(cur, "Content");
		if (!strcmp(name, paramname)) {
			osxml_node_add(node, newname, content);
			osxml_node_remove_unknown_mark(node);
			
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			g_free(name);
			g_free(content);
			return;
		}
		
		g_free(name);
		g_free(content);
			
		next:;
		cur = cur->next;
	}
}

osync_bool osxml_has_property_full(xmlNode *parent, const char *name, const char *data)
{
	if (osxml_has_property(parent, name))
		return (strcmp((char*)xmlGetProp(parent, (xmlChar*)name), data) == 0);
	return FALSE;
}

char *osxml_find_property(xmlNode *parent, const char *name)
{
	return (char*)xmlGetProp(parent, (xmlChar*)name);
}

osync_bool osxml_has_property(xmlNode *parent, const char *name)
{
	return (xmlHasProp(parent, (xmlChar*)name) != NULL);
}

xmlChar *osxml_write_to_string(xmlDoc *doc)
{
	xmlKeepBlanksDefault(0);
	xmlChar *temp = NULL;
	int size = 0;
	xmlDocDumpFormatMemoryEnc(doc, &temp, &size, NULL, 1);
	return temp;
}

osync_bool osxml_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	xmlDoc *doc = (xmlDoc *)(input);
	xmlDoc *newdoc = xmlCopyDoc(doc, TRUE);
	*output = (char *)newdoc;
	*outpsize = sizeof(newdoc);
	return TRUE;
}

osync_bool osxml_marshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	xmlDoc *doc = (xmlDoc*)input;
	xmlChar *result;
	int size;
	xmlDocDumpMemory(doc, &result, &size);
	*output = (char*)result;
	*outpsize = size;
	return TRUE;
}

osync_bool osxml_demarshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	xmlDoc *doc = xmlParseMemory(input, inpsize);
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid XML data received");
		goto error;
	}

	*output = (char*)doc;
	*outpsize = sizeof(*doc);
	return TRUE;

error:
	return FALSE;
}


