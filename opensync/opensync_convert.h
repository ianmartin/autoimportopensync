OSyncFormatEnv *osync_conv_env_new(OSyncEnv *env);
void osync_conv_env_free(OSyncFormatEnv *env);
osync_bool osync_conv_set_common_format(OSyncFormatEnv *env, const char *objtypestr, const char *formatname, OSyncError **error);

OSyncObjType *osync_conv_find_objtype(OSyncFormatEnv *env, const char *name);
OSyncObjFormat *osync_conv_find_objformat(OSyncFormatEnv *env, const char *name);
int osync_conv_num_objtypes(OSyncFormatEnv *env);
OSyncObjType *osync_conv_nth_objtype(OSyncFormatEnv *env, int nth);
int osync_conv_num_objformats(OSyncObjType *type);
OSyncObjFormat *osync_conv_nth_objformat(OSyncObjType *type, int nth);
OSyncFormatConverter *osync_conv_find_converter(OSyncFormatEnv *env, const char *sourcename, const char *targetname);

const char *osync_objtype_get_name(OSyncObjType *type);
const char *osync_objformat_get_name(OSyncObjFormat *format);
OSyncObjType *osync_objformat_get_objtype(OSyncObjFormat *format);

osync_bool osync_change_duplicate(OSyncChange *change);
OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);
OSyncObjFormat *osync_change_detect_objformat(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjFormat *osync_change_detect_objformat_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);

osync_bool osync_change_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *fmt, OSyncError **error);
osync_bool osync_change_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *fmtname, OSyncError **error);
osync_bool osync_change_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **names, OSyncError **error);
osync_bool osync_change_convert_to_common(OSyncChange *change, OSyncError **error);
