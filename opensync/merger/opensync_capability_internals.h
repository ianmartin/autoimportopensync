#ifndef OPENSYNC_CAPABILITY_INTERNALS_H_
#define OPENSYNC_CAPABILITY_INTERNALS_H_

/*! @brief Represent a Capabilitiy Object
 */
struct OSyncCapability {
	/** */
	OSyncCapability *next;
	/** */
	OSyncCapability *prev;
	/** */
	xmlNodePtr node;
};

OSyncCapability *_osync_capability_new(OSyncCapabilitiesObjType *objtype, xmlNodePtr node);

#endif /*OPENSYNC_CAPABILITY_INTERNALS_H_*/
