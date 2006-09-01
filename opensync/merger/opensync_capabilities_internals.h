#ifndef OPENSYNC_CAPABILITIES_INTERNALS_H_
#define OPENSYNC_CAPABILITIES_INTERNALS_H_

typedef struct OSyncCapabilitiesObjType OSyncCapabilitiesObjType;

/**
 * @brief Represent a CapabilitiesObjType object
 * @ingroup OSyncCapabilitiesPrivateAPI
 */
struct OSyncCapabilitiesObjType {
	/** The pointer to the next objtype */
	OSyncCapabilitiesObjType *next;
	/** The pointer to the first capability */
	OSyncCapability *first_child;
	/** The pointer to the last capability */
	OSyncCapability *last_child;
	/** Counter which holds the number of capabilities for one objtype*/
	int child_count;
	/** The wrapped xml node */
	xmlNodePtr node;	
};

/**
 * @brief Represent a Capabilities object
 * @ingroup OSyncCapabilitiesPrivateAPI
 */
struct OSyncCapabilities {
	/** The reference counter for this object */
	int ref_count;
	/** The pointer to the first objtype */
	OSyncCapabilitiesObjType *first_objtype;
	/** The pointer to the last objtype */
	OSyncCapabilitiesObjType *last_objtype;
	/** The wrapped xml document */
	xmlDocPtr doc;
};

OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_new(OSyncCapabilities *capabilities, xmlNodePtr node, OSyncError **error);
OSyncCapabilitiesObjType *_osync_capabilitiesobjtype_get(OSyncCapabilities *capabilities, const char *objtype);

#endif /*OPENSYNC_CAPABILITIES_INTERNAL_H_*/
