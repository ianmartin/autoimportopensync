#ifndef OPENSYNC_CAPABILITY_INTERNALS_H_
#define OPENSYNC_CAPABILITY_INTERNALS_H_

/*! @brief Represent a Capabilitiy Object
 */
struct OSyncCapability {
	/** The reference counter for this object */
	OSyncCapability *next;
	/** */
	OSyncCapability *first_child;
	/** */
	OSyncCapability *last_child;
	/** */
	xmlNodePtr node;
};

OSyncCapability *_osync_capability_new(OSyncCapability *capability, xmlNodePtr node);
OSyncCapability *_osync_capability_new_content_type(OSyncCapabilities *capabilities, xmlNodePtr node);
const char *_osync_capability_get_sortname(void *node);

#endif /*OPENSYNC_CAPABILITY_INTERNALS_H_*/
