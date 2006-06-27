#ifndef OPENSYNC_XMLFIELD_INTERNALS_H_
#define OPENSYNC_XMLFIELD_INTERNALS_H_

/*! @brief Represent a XMLField Object
 */
struct OSyncXMLField {
	/** */
	OSyncXMLField *next;
	/**  */
	OSyncXMLField *prev; /** needed for insert befor function which is needed for merging without sorting*/
	/** */
	xmlNodePtr node;
};

OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node);

#endif /*OPENSYNC_XMLFIELD_INTERNALS_H_*/
