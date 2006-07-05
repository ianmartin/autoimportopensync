#ifndef OPENSYNC_XMLFIELDLIST_H_
#define OPENSYNC_XMLFIELDLIST_H_

OSYNC_EXPORT void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist);
OSYNC_EXPORT int osync_xmlfieldlist_get_length(OSyncXMLFieldList *xmlfieldlist);
OSYNC_EXPORT OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index);

#endif /*OPENSYNC_XMLFIELDLIST_H_*/
