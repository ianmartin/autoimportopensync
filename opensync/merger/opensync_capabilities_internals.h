#ifndef OPENSYNC_CAPABILITIES_INTERNALS_H_
#define OPENSYNC_CAPABILITIES_INTERNALS_H_

typedef struct OSyncCapabilitiesObjType OSyncCapabilitiesObjType;

/*! @brief Represent a CapabilitiesObjType
 */
struct OSyncCapabilitiesObjType {
	/** */
	OSyncCapabilitiesObjType *next;
	/** */
	OSyncCapability *first_child;
	/** */
	OSyncCapability *last_child;
	/** */
	int child_count;
	/** */
	xmlNodePtr node;	
};

/*! @brief Represent a Capabilities Object
 */
struct OSyncCapabilities {
	/** The reference counter for this object */
	int refcount;
	/** */
	OSyncCapabilitiesObjType *first_objtype;
	/** */
	OSyncCapabilitiesObjType *last_objtype;
	/** */
	xmlDocPtr doc;
};

OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_new(OSyncCapabilities *capabilities, xmlNodePtr node);
OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_get(OSyncCapabilities *capabilities, const char *objtype);

#endif /*OPENSYNC_CAPABILITIES_INTERNAL_H_*/
