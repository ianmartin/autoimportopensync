#ifndef OPENSYNC_XMLFORMAT_INTERNALS_H_
#define OPENSYNC_XMLFORMAT_INTERNALS_H_

/*! @brief Represent a Capabilities Object
 */
struct OSyncXMLFormat {
	/** The reference counter for this object */
	int refcount;
	/** */
	OSyncXMLField *first_child;
	/** */
	OSyncXMLField *last_child;
	/** */
	int child_count;	
	/** */
	xmlDocPtr doc;
};

#endif /*OPENSYNC_XMLFORMAT_INTERNAL_H_*/
