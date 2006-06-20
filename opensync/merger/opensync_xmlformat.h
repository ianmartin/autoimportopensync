#ifndef OPENSYNC_XMLFORMAT_H_
#define OPENSYNC_XMLFORMAT_H_

OSyncXMLFormat *osync_xmlformat_new(const char *objtype);
OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error);
void osync_xmlformat_ref(OSyncXMLFormat *xmlformat);
void osync_xmlformat_unref(OSyncXMLFormat *xmlformat);
const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat);
void osync_xmlformat_sort(OSyncXMLFormat *xmlformat);
OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat);
OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, ...);
osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, int *size);
osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat);
void osync_xmlformat_merging(OSyncXMLFormat *xmlfield, OSyncCapabilities *capabilities, OSyncXMLFormat *original);
//OSyncConvCmpResult osync_xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints *xmlscore);
//osync_bool osync_xmlformat_read_xml(OSyncXMLFormat *xmlformat, const char *path, OSyncError **error);

#endif /*OPENSYNC_XMLFORMAT_H_*/
