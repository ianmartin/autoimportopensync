OSyncFormatEnv *osync_conv_env_new(void);
void osync_conv_env_free(OSyncFormatEnv *env);
void osync_conv_set_common_format(OSyncFormatEnv *env, const char *objtypestr, const char *formatname);
osync_bool osync_conv_env_load(OSyncFormatEnv *env, OSyncError **error);
void osync_conv_env_unload(OSyncFormatEnv *env);

OSyncObjType *osync_conv_find_objtype(OSyncFormatEnv *env, const char *name);
OSyncObjFormat *osync_conv_find_objformat(OSyncFormatEnv *env, const char *name);
OSyncFormatConverter *osync_conv_find_converter(OSyncFormatEnv *env, const char *sourcename, const char *targetname);
int osync_conv_num_objtypes(OSyncFormatEnv *env);
OSyncObjType *osync_conv_nth_objtype(OSyncFormatEnv *env, int nth);
int osync_conv_num_objformats(OSyncObjType *type);
OSyncObjFormat *osync_conv_nth_objformat(OSyncObjType *type, int nth);

void osync_conv_register_data_detector(OSyncFormatEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func);
OSyncObjFormat *osync_conv_register_objformat(OSyncFormatEnv *env, const char *type, const char *name);
OSyncObjType *osync_conv_register_objtype(OSyncFormatEnv *env, const char *name);
osync_bool osync_conv_register_converter(OSyncFormatEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func, ConverterFlags flags);
void osync_conv_register_filter_function(OSyncFormatEnv *env, const char *name, const char *objtype, const char *format, OSyncFilterFunction hook);

const char *osync_objtype_get_name(OSyncObjType *type);
const char *osync_objformat_get_name(OSyncObjFormat *format);
OSyncObjType *osync_objformat_get_objtype(OSyncObjFormat *format);

void osync_conv_objtype_add_format(OSyncObjType *type, OSyncObjFormat *format);
void osync_conv_format_set_detect_func(OSyncObjFormat *format, OSyncFormatDetectFunc detect_func);

osync_bool osync_change_duplicate(OSyncChange *change);
OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);
OSyncObjFormat *osync_change_detect_objformat(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjFormat *osync_change_detect_objformat_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);
OSyncObjType *osync_change_detect_objtype_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error);

osync_bool osync_change_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *fmt, OSyncError **error);
osync_bool osync_change_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *fmtname, OSyncError **error);
osync_bool osync_change_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **names, OSyncError **error);

void osync_conv_format_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func);
void osync_conv_format_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func);
void osync_conv_format_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func);
void osync_conv_format_set_malloced(OSyncObjFormat *format);
void osync_conv_format_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func);
void osync_conv_format_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func);
void osync_conv_format_set_like(OSyncObjFormat *format, const char *base_format, ConverterFlags to_flags, ConverterFlags from_flags);
