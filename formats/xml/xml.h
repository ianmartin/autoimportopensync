#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <opensync.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>

typedef struct OSyncXML OSyncXML;
typedef struct OSyncXMLNode OSyncXMLNode;

OSyncXML *osxml_format_new(void);
void osxml_format_free(OSyncXML *xml);
OSyncXMLNode *osxml_node_new(void);
void osxml_node_free(OSyncXMLNode *node);
OSyncXMLNode *osxml_node_add_root(OSyncXML *xml, const char *name);
OSyncXMLNode *osxml_node_add(OSyncXMLNode *parent, const char *name, const char *data);
void osxml_format_dump(OSyncXML *xml, char **data, int *size);
xmlNode *osxml_format_parse(const char *input, int size, const char *rootname);
char *osxml_find_node(xmlNode *parent, const char *name);
void osxml_node_add_property(OSyncXMLNode *parent, const char *name, const char *data);
char *osxml_find_property(xmlNode *parent, const char *name);
osync_bool osxml_has_property(xmlNode *parent, const char *name);
