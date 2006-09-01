#ifndef OPENSYNC_CAPABILITY_INTERNALS_H_
#define OPENSYNC_CAPABILITY_INTERNALS_H_

/**
 * @brief Represent a Capabilitiy object
 * @ingroup OSyncCapabilityPrivateAPI
 */
struct OSyncCapability {
	/** The pointer to the next capability */
	OSyncCapability *next;
	/** The pointer to the previous capability */
	OSyncCapability *prev;
	/** The wrapped xml node */
	xmlNodePtr node;
};

OSyncCapability *_osync_capability_new(OSyncCapabilitiesObjType *objtype, xmlNodePtr node, OSyncError **error);
void _osync_capability_free(OSyncCapability *capability);
int _osync_capability_compare_stdlib(const void *capability1, const void *capability2);

#endif /*OPENSYNC_CAPABILITY_INTERNALS_H_*/
