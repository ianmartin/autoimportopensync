#ifndef _OPENSYNC_CHANGE_INTERNALS_H_
#define _OPENSYNC_CHANGE_INTERNALS_H_

/*! @ingroup OSyncChangePrivate 
 * @brief A change object */
struct OSyncChange {
	/** The uid of this change */
	char *uid;
	/** The hash of this change*/
	char *hash; /*Hash value to identify changes*/
	/** The change type */
	OSyncChangeType changetype;
	/** The data reported from the plugin */
	OSyncData *data;
	int ref_count;
};

#endif /*_OPENSYNC_CHANGE_INTERNALS_H_*/
