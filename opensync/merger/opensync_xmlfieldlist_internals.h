#ifndef OPENSYNC_XMLFIELDLIST_INTERNALS_H_
#define OPENSYNC_XMLFIELDLIST_INTERNALS_H_

/**
 * @brief Represent a XMLFieldList object
 * @ingroup OSyncXMLFieldListPrivateAPI
 */
struct OSyncXMLFieldList {
	/** The array holds the OSyncXMLField pointers */
	GPtrArray *array;
};

OSyncXMLFieldList *_osync_xmlfieldlist_new(OSyncError **error);
void _osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield);
void _osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index);

#endif /*OPENSYNC_XMLFIELDLIST_INTERNALS_H_*/
