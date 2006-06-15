#ifndef OPENSYNC_XMLFIELDLIST_INTERNALS_H_
#define OPENSYNC_XMLFIELDLIST_INTERNALS_H_

/*! @brief Represent a XMLFieldList Object
 */
struct OSyncXMLFieldList {
	/** The holding array of the OSyncXMLField pointers */
	GPtrArray *array;
};

OSyncXMLFieldList *osync_xmlfieldlist_new(void);
void osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield);
void osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index);

#endif /*OPENSYNC_XMLFIELDLIST_INTERNALS_H_*/
