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
 
#include "xml.h"

xmlNode *osxml_node_add_root(xmlDoc *doc, const char *name)
{
	doc->children = xmlNewDocNode(doc, NULL, name, NULL);
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

/*char *osxml_convert_from(const char *string, OSyncXMLEncoding encoding)
{
	char *tmp = NULL;
	char *tmp2 = NULL;
	if (!string)
		return NULL;
	if (encoding.encoding == OSXML_QUOTED_PRINTABLE)
		tmp = decode_quoted_printable(string);
	else if (encoding.encoding == OSXML_BASE64)
		tmp = decode_base64(string);
	else
		tmp = g_strdup(string);
	
	if (encoding.charset == OSXML_ASCII)
		tmp2 = g_convert(tmp, strlen(tmp), "utf8", "ISO-8859-1", NULL, NULL, NULL);
	else
		tmp2 = g_strdup(tmp);
	g_free(tmp);
	
	return tmp2;
}*/

void osxml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding)
{
	if (name)
		xmlNodeSetName(node, name); //FIXME Free previous name?
		
	if (data)
		xmlNewTextChild(node, NULL, "Content", data);
}

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data, OSyncXMLEncoding encoding)
{
	xmlNode *node = xmlNewTextChild(parent, NULL, name, data);
	return node;
}

void osxml_node_add_property(xmlNode *parent, const char *name, const char *data)
{
	xmlNewProp(parent, name, data);
}

void osxml_node_mark_unknown(xmlNode *parent)
{
	if (!xmlHasProp(parent, "Type"))
		xmlNewProp(parent, "Type", "Unknown");
}

/*void osxml_format_dump(OSyncXML *xml, char **data, int *size)
{
	xmlDocDumpMemory(xml->doc, (xmlChar **)data, size);
}*/


//Parsing
/*xmlNode *osxml_format_parse(const char *input, int size, const char *rootname, OSyncError **error)
{
	xmlDoc *doc = xmlParseMemory(input, size);
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse xml document");
		return NULL;
	}
	
	xmlNode *cur = xmlDocGetRootElement(doc);
	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		xmlFreeDoc(doc);
		return NULL;
	}
	
	if (xmlStrcmp((cur)->name, (const xmlChar *) rootname)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		xmlFreeDoc(doc);
		return NULL;
	}
	
	cur = (cur)->xmlChildrenNode;
	return cur;
}*/

char *osxml_find_node(xmlNode *parent, const char *name)
{
	xmlNode *cur = (parent)->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)name))
			return xmlNodeGetContent(cur);
		cur = cur->next;
	}
	return NULL;
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
    xpathObj = xmlXPathEvalExpression(expression, xpathCtx);
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

osync_bool osxml_has_property_full(xmlNode *parent, const char *name, const char *data)
{
	if (osxml_has_property(parent, name))
		return (strcmp(xmlGetProp(parent, name), data) == 0);
	return FALSE;
}

char *osxml_find_property(xmlNode *parent, const char *name)
{
	return xmlGetProp(parent, name);
}

osync_bool osxml_has_property(xmlNode *parent, const char *name)
{
	return (xmlHasProp(parent, name) != NULL);
}

/*
OSyncXMLNode *osxml_node_get_root(OSyncXML *xml)
{
	OSyncXMLNode *node = g_malloc0(sizeof(OSyncXMLNode));
	node->node = xmlDocGetRootElement(xml->doc);
	return node;
}

OSyncXMLNode *osxml_node_get_child(OSyncXMLNode *)
{
	OSyncXMLNode *node = g_malloc0(sizeof(OSyncXMLNode));
	node->node = xmlDocGetRootElement(xml->doc);
	return node;

get_next_node
compare_name
find_child*/



static OSyncConvCmpResult compare_xml(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *mxml = osync_conv_register_objformat(env, "contact", "x-opensync-xml");
	osync_conv_format_set_compare_func(mxml, compare_xml);
	osync_conv_format_set_destroy_func(mxml, destroy_xml);
}
