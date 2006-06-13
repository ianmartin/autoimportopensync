#ifndef _OPENSYNC_CHANGE_INTERNALS_H_
#define _OPENSYNC_CHANGE_INTERNALS_H_

/*! @ingroup OSyncChangePrivate 
 * @brief A change object */
struct OSyncChange {
	/** The uid of this change */
	char *uid;
	/** The hash of this change*/
	char *hash; //Hash value to identify changes
	/** The change type */
	OSyncChangeType changetype;
	/** The data reported from the plugin */
	OSyncData *data;
	int ref_count;
};


#ifdef asdasd
	/** The format that was initialy reported */
	OSyncObjFormat *initial_format;
	/** The name of the initial format */
	char *initial_format_name;
	
	/** The conversion environment of this change */
	OSyncFormatEnv *conv_env;
	
	/** The parent of this change */
	OSyncMember *member;
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

OSyncObjFormat *osync_change_get_initial_objformat(OSyncChange *change);


OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);
OSyncConvCmpResult osync_change_compare_data(OSyncChange *leftchange, OSyncChange *rightchange);
time_t osync_change_get_revision(OSyncChange *change, OSyncError **error);

OSyncObjFormat *osync_change_detect_objformat(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjFormat *osync_change_detect_objformat_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);

osync_bool osync_change_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *fmt, OSyncError **error);
osync_bool osync_change_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *fmtname, OSyncError **error);
osync_bool osync_change_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **names, OSyncError **error);
osync_bool osync_change_convert_to_common(OSyncChange *change, OSyncError **error);
osync_bool osync_change_convert_extension(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat, const char *extension_name, OSyncError **error);
osync_bool osync_change_convert_member_sink(OSyncFormatEnv *env, OSyncChange *change, OSyncMember *memb, OSyncError **error);

osync_bool osync_change_copy_data(OSyncChange *source, OSyncChange *target, OSyncError **error);
OSyncChange *osync_change_copy(OSyncChange *source, OSyncError **error);

#endif

#endif //_OPENSYNC_CHANGE_INTERNALS_H_
