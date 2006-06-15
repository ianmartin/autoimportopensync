#ifndef OPENSYNC_XMLFIELDLIST_H_
#define OPENSYNC_XMLFIELDLIST_H_

void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist);
int osync_xmlfieldlist_getLength(OSyncXMLFieldList *xmlfieldlist);
OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index);

#endif /*OPENSYNC_XMLFIELDLIST_H_*/
