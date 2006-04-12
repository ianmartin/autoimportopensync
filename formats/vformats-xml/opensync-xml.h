#ifndef HAVE_OPENSYNC_XML_H
#define HAVE_OPENSYNC_XML_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <opensync/opensync.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

xmlNode *osxml_node_add_root(xmlDoc *doc, const char *name);
xmlNode *osxml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error);
xmlNode *osxml_get_node(xmlNode *parent, const char *name);

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data);
//void osxml_format_dump(OSyncXML *xml, char **data, int *size);
xmlNode *osxml_format_parse(const char *input, int size, const char *rootname, OSyncError **error);
char *osxml_find_node(xmlNode *parent, const char *name);
void osxml_node_add_property(xmlNode *parent, const char *name, const char *data);
char *osxml_find_property(xmlNode *parent, const char *name);
osync_bool osxml_has_property(xmlNode *parent, const char *name);
osync_bool osxml_has_property_full(xmlNode *parent, const char *name, const char *data);

void osxml_node_mark_unknown(xmlNode *parent);
void osxml_node_remove_unknown_mark(xmlNode *node);
void osxml_map_unknown_param(xmlNode *node, const char *paramname, const char *newname);

void osxml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding);
xmlXPathObject *osxml_get_nodeset(xmlDoc *doc, const char *expression);
xmlXPathObject *osxml_get_unknown_nodes(xmlDoc *doc);
OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold);
char *osxml_write_to_string(xmlDoc *doc);
osync_bool osxml_copy(const char *input, int inpsize, char **output, int *outpsize);

osync_bool osxml_marshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error);
osync_bool osxml_demarshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error);

#ifdef __cplusplus
}
#endif

#endif
