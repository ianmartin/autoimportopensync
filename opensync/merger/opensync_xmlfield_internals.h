#ifndef OPENSYNC_XMLFIELD_INTERNALS_H_
#define OPENSYNC_XMLFIELD_INTERNALS_H_

/*! @brief Represent a Capabilitiy Object
 */
struct OSyncXMLField {
	/** The reference counter for this object */
	OSyncXMLField *next;
	/** The reference counter for this object */
	OSyncXMLField *prev; /** needed for insert befor function which is needed for merging without sorting*/
	/** */
	xmlNodePtr node;
};

OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node);

const char *_osync_xmlfield_get_sortname(void *node);

#endif /*OPENSYNC_XMLFIELD_INTERNALS_H_*/
