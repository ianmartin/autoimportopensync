#ifndef _OPENSYNC_CHANGE_INTERNALS_H_
#define _OPENSYNC_CHANGE_INTERNALS_H_

/*! @ingroup OSyncChangePrivate 
 * @brief A change object */
struct OSyncChange {
	/** The uid of this change */
	char *uid;
	/** The hash of this change*/
	char *hash; //Hash value to identify changes
	/** The data reported from the plugin */
	char *data; //The data of the object
	/** The size of the data from the plugin */
	int size;
	/** Is the set data already the "real" data */
	osync_bool has_data;
	
	/** The object type of the change */
	OSyncObjType *objtype;
	/** The name of the object type */
	char *objtype_name;
	/** The format of the change */
	OSyncObjFormat *format;
	/** The name of the format */
	char *format_name;
	
	/** The format that was initialy reported */
	OSyncObjFormat *initial_format;
	/** The name of the initial format */
	char *initial_format_name;
	
	/** The conversion environment of this change */
	OSyncFormatEnv *conv_env;
	
	/** The parent of this change */
	OSyncMember *member;
	/** The change type */
	OSyncChangeType changetype;
	/** The data associated by the engine with this change */
	void *engine_data;
	/** The unique id */
	long long int id;
	/** The reference counter for this object */
	int refcount;
	/** The id of the mapping for this change */
	long long int mappingid;
	/** The database where this change is stored */
	OSyncDB *changes_db;
	
	//For the filters
	/** The name of the destination object type for the filter */
	char *destobjtype;
	/** the name of the source object type for the filter */
	char *sourceobjtype;
	/** the member where this change originated */
	OSyncMember *sourcemember;
};

OSyncObjFormat *osync_change_get_initial_objformat(OSyncChange *change);

#endif //_OPENSYNC_CHANGE_INTERNALS_H_
