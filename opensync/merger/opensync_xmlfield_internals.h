#ifndef OPENSYNC_XMLFIELD_INTERNALS_H_
#define OPENSYNC_XMLFIELD_INTERNALS_H_

/**
 * @brief Represent a XMLField object
 * @ingroup OSyncXMLFieldPrivateAPI
 */
struct OSyncXMLField {
	/** The pointer to the next xmlfield */
	OSyncXMLField *next;
	/** The pointer to the previous xmlfield */
	OSyncXMLField *prev; 
	/** The wrapped xml node */
	xmlNodePtr node;
};

OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node, OSyncError **error);
void _osync_xmlfield_free(OSyncXMLField *xmlfield);

void _osync_xmlfield_unlink(OSyncXMLField *xmlfield);
void _osync_xmlfield_link_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);
void _osync_xmlfield_link_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);

int _osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2);

xmlChar *_osync_xmlfield_node_get_content(xmlNodePtr node);
xmlChar *_osync_xmlfield_attr_get_content(xmlAttrPtr node);

//OSyncXMLField *osync_xmlfield_insert_copy_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_copy);

#endif /*OPENSYNC_XMLFIELD_INTERNALS_H_*/
