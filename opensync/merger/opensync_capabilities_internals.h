#ifndef OPENSYNC_CAPABILITIES_INTERNALS_H_
#define OPENSYNC_CAPABILITIES_INTERNALS_H_

/*! @brief Represent a Capabilities Object
 */
struct OSyncCapabilities {
	/** The reference counter for this object */
	int refcount;
	/** */
	OSyncCapability *first_child;
	/** */
	OSyncCapability *last_child;
	/** */
	xmlDocPtr doc;	
};

#endif /*OPENSYNC_CAPABILITIES_INTERNAL_H_*/
