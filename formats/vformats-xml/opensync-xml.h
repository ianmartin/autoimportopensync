#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <opensync.h>
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

xmlNode *osxml_node_add_root(xmlDoc *doc, const char *name);
xmlNode *osxml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error);

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data);
//void osxml_format_dump(OSyncXML *xml, char **data, int *size);
xmlNode *osxml_format_parse(const char *input, int size, const char *rootname, OSyncError **error);
char *osxml_find_node(xmlNode *parent, const char *name);
void osxml_node_add_property(xmlNode *parent, const char *name, const char *data);
char *osxml_find_property(xmlNode *parent, const char *name);
osync_bool osxml_has_property(xmlNode *parent, const char *name);
osync_bool osxml_has_property_full(xmlNode *parent, const char *name, const char *data);
void osxml_node_mark_unknown(xmlNode *parent);
void osxml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding);
xmlXPathObject *osxml_get_nodeset(xmlDoc *doc, const char *expression);
xmlXPathObject *osxml_get_unknown_nodes(xmlDoc *doc);
OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores);
