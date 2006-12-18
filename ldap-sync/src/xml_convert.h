#ifndef _XML_CONVERT_H
#define _XML_CONVERT_H

#include "ldap_plugin.h"

#include <libxml/tree.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

char **get_map_attribute_list();

xmlDoc* convert_ldap2xml (ldap_entry *ldapdata);

ldap_entry *convert_xml2ldap (xmlDoc *xmldata);

#endif /* _XML_CONVERT_H */
