#ifndef _OPENSYNC_CHANGECMDS_H_
#define _OPENSYNC_CHANGECMDS_H_

osync_bool osync_change_duplicate(OSyncChange *change);
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

osync_bool osync_change_copy_data(OSyncChange *source, OSyncChange *target, OSyncError **error);
OSyncChange *osync_change_copy(OSyncChange *source, OSyncError **error);

#endif //_OPENSYNC_CHANGECMDS_H_
