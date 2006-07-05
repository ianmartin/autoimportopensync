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
void _osync_capability_free(OSyncCapability *capability);
int _osync_capability_compare_stdlib(const void *capability1, const void *capability2);

#endif /*OPENSYNC_CAPABILITY_INTERNALS_H_*/
