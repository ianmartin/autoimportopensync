#include "xml.h"

struct OSyncXML {
	xmlDoc *doc;
};

struct OSyncXMLNode {
	xmlNode *node;
};

//Creating
OSyncXML *osxml_format_new(void)
{
	OSyncXML *xml = g_malloc0(sizeof(OSyncXML));
	xml->doc = xmlNewDoc("1.0");
	return xml;
}

void osxml_format_free(OSyncXML *xml)
{
	if (xml->doc)
		xmlFreeDoc(xml->doc);
	g_free(xml);
}

OSyncXMLNode *osxml_node_new(void)
{
	OSyncXMLNode *node = g_malloc0(sizeof(OSyncXMLNode));
	return node;
}

void osxml_node_free(OSyncXMLNode *node)
{
	g_free(node);
}

OSyncXMLNode *osxml_node_add_root(OSyncXML *xml, const char *name)
{
	xml->doc->children = xmlNewDocNode(xml->doc, NULL, name, NULL);
	OSyncXMLNode *node = osxml_node_new();
	node->node = xml->doc->children;
	return node;
}

OSyncXMLNode *osxml_node_add(OSyncXMLNode *parent, const char *name, const char *data)
{
	OSyncXMLNode *node = osxml_node_new();
	node->node = xmlNewChild(parent->node, NULL, name, data);
	return node;
}

void osxml_node_add_property(OSyncXMLNode *parent, const char *name, const char *data)
{
	xmlNewProp(parent->node, name, data);
}

void osxml_format_dump(OSyncXML *xml, char **data, int *size)
{
	xmlDocDumpMemory(xml->doc, (xmlChar **)data, size);
}


//Parsing
xmlNode *osxml_format_parse(const char *input, int size, const char *rootname)
{
	xmlDoc *doc = xmlParseMemory(input, size);

	if (!doc)
		return NULL;
	
	xmlNode *cur = xmlDocGetRootElement(doc);
	
	if (!cur) {
		xmlFreeDoc(doc);
		return NULL;
	}
	
	if (xmlStrcmp((cur)->name, (const xmlChar *) rootname)) {
		xmlFreeDoc(doc);
		return NULL;
	}
	
	cur = (cur)->xmlChildrenNode;
	return cur;
}

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

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *mxml = osync_conv_register_objformat(env, "contact", "x-opensync-xml");
	osync_conv_format_set_compare_func(mxml, compare_xml);
}
