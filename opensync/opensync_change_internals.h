#ifndef _OPENSYNC_CHANGE_INTERNALS_H_
#define _OPENSYNC_CHANGE_INTERNALS_H_

/*! @ingroup OSyncChangePrivate 
 * @brief A change object */
struct OSyncChange {
	char *uid; //unique resource locater
	char *hash; //Hash value to identify changes
	char *data; //The data of the object
	int size;
	osync_bool has_data;
	/*FIXME: do we need this field, as OSyncObjFormat has
	 * a objtype field set?
	 */
	OSyncObjType *objtype;
	char *objtype_name;
	OSyncObjFormat *format;
	char *format_name;
	
	OSyncObjFormat *initial_format;
	char *initial_format_name;
	
	OSyncFormatEnv *conv_env;
		
	OSyncMember *member;
	OSyncChangeType changetype;
	void *engine_data;
	long long int id;
	int refcount;
	long long int mappingid;
	OSyncDB *changes_db;
	
	//For the filters
	char *destobjtype;
	char *sourceobjtype;
	OSyncMember *sourcemember;
	osync_bool is_detected;
};

OSyncObjFormat *osync_change_get_initial_objformat(OSyncChange *change);

#endif //_OPENSYNC_CHANGE_INTERNALS_H_
