#ifndef HAVE_OPENSYNC_XML_H
#define HAVE_OPENSYNC_XML_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>

#include <opensync/opensync.h>
#include <string.h>
#include <stdio.h>

typedef enum osxmlEncoding {
	OSXML_8BIT = 0,
	OSXML_QUOTED_PRINTABLE = 1,
	OSXML_BASE64 = 2
} osxmlEncoding;

typedef enum osxmlCharset {
	OSXML_ASCII = 0,
	OSXML_UTF8 = 1
} osxmlCharset;

typedef struct OSyncXMLEncoding OSyncXMLEncoding;
struct OSyncXMLEncoding {
	osxmlEncoding encoding;
	osxmlCharset charset;
};

typedef struct OSyncXMLScore {
	int value;
	const char *path;
} OSyncXMLScore;

xmlNode *osync_xml_node_add_root(xmlDoc *doc, const char *name);
xmlNode *osync_xml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error);
xmlNode *osync_xml_get_node(xmlNode *parent, const char *name);

xmlNode *osync_xml_node_add(xmlNode *parent, const char *name, const char *data);
//void osync_xml_format_dump(OSyncXML *xml, char **data, int *size);
xmlNode *osync_xml_format_parse(const char *input, int size, const char *rootname, OSyncError **error);
char *osync_xml_find_node(xmlNode *parent, const char *name);
void osync_xml_node_add_property(xmlNode *parent, const char *name, const char *data);
char *osync_xml_find_property(xmlNode *parent, const char *name);
osync_bool osync_xml_has_property(xmlNode *parent, const char *name);
osync_bool osync_xml_has_property_full(xmlNode *parent, const char *name, const char *data);

void osync_xml_node_mark_unknown(xmlNode *parent);
void osync_xml_node_remove_unknown_mark(xmlNode *node);
void osync_xml_map_unknown_param(xmlNode *node, const char *paramname, const char *newname);

void osync_xml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding);
xmlXPathObject *osync_xml_get_nodeset(xmlDoc *doc, const char *expression);
xmlXPathObject *osync_xml_get_unknown_nodes(xmlDoc *doc);
OSyncConvCmpResult osync_xml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold);
char *osync_xml_write_to_string(xmlDoc *doc);
osync_bool osync_xml_copy(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error);

osync_bool osync_xml_marshal(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error);
osync_bool osync_xml_demarshal(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error);

osync_bool osync_xml_validate_document(xmlDocPtr doc, char *schemafilepath);

xmlChar *osync_xml_node_get_content(xmlNodePtr node);
xmlChar *osync_xml_attr_get_content(xmlAttrPtr node);

osync_bool osync_xml_open_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error);

#endif
