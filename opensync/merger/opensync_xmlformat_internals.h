#ifndef OPENSYNC_XMLFORMAT_INTERNALS_H_
#define OPENSYNC_XMLFORMAT_INTERNALS_H_

/** 
 * @brief Represent a XMLFormat object
 * @ingroup OSyncXMLFormatPrivateAPI
 */
struct OSyncXMLFormat {
	/** The reference counter for this object */
	int ref_count;
	/** The first xmlfield */
	OSyncXMLField *first_child;
	/** The last xmlfield */
	OSyncXMLField *last_child;
	/** counter which holds the number of xmlfields */
	int child_count;	
	/** The wrapped xml document */
	xmlDocPtr doc;
};

int _osync_xmlformat_get_points(OSyncXMLPoints points[], int* cur_pos, int basic_points, const char* fieldname);

#endif /*OPENSYNC_XMLFORMAT_INTERNAL_H_*/
