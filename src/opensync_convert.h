OSyncFormatEnv *osync_conv_env_new(void);
OSyncObjType *osync_conv_find_objtype(OSyncFormatEnv *env, const char *name);
OSyncObjFormat *osync_conv_find_objformat(OSyncFormatEnv *env, const char *name);
OSyncObjFormat *osync_conv_register_objformat(OSyncFormatEnv *env, const char *type, const char *name);
OSyncObjType *osync_conv_register_objtype(OSyncFormatEnv *env, const char *name);
const char *osync_objtype_get_name(OSyncObjType *type);
const char *osync_objformat_get_name(OSyncObjFormat *format);
char *osync_conv_objtype_get_name(OSyncObjType *type);
OSyncFormatConverter *osync_conv_find_converter(OSyncFormatEnv *env, const char *sourcename, const char *targetname);
osync_bool osync_conv_register_converter(OSyncFormatEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func, ConverterFlags flags);
void osync_conv_objtype_add_format(OSyncObjType *type, OSyncObjFormat *format);
void osync_conv_format_set_detect_func(OSyncObjFormat *format, OSyncFormatDetectFunc detect_func);
osync_bool osync_conv_detect_change_format(OSyncFormatEnv *env, OSyncChange *change);
osync_bool osync_conv_duplicate_change(OSyncChange *change);
void osync_conv_env_load(OSyncFormatEnv *env);

void osync_conv_format_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func);
void osync_conv_format_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func);
void osync_conv_format_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func);
void osync_conv_format_set_malloced(OSyncObjFormat *format);
void osync_conv_format_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func);
void osync_conv_format_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func);

void osync_conv_format_set_like(OSyncObjFormat *format, const char *base_format, ConverterFlags to_flags, ConverterFlags from_flags);
//void osync_conv_format_set_functions(OSyncObjFormat *format, OSyncFormatFunctions functions);
OSyncObjFormat *osync_conv_nth_objformat(OSyncObjType *type, int nth);
osync_bool osync_conv_objtype_is_any(const char *objstr);
void osync_conv_register_data_detector(OSyncFormatEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func);
OSyncConvCmpResult osync_conv_compare_changes(OSyncChange *leftchange, OSyncChange *rightchange);
int osync_conv_num_objtypes(OSyncFormatEnv *env);
OSyncObjType *osync_conv_nth_objtype(OSyncFormatEnv *env, int nth);
int osync_conv_num_objformats(OSyncObjType *type);
void osync_conv_env_free(OSyncFormatEnv *env);
void osync_conv_set_common_format(OSyncFormatEnv *env, const char *objtypestr, const char *formatname);

/* Conversion path functions */
osync_bool osync_conv_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *fmtname);
osync_bool osync_conv_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **names);
void osync_conv_register_filter_function(OSyncFormatEnv *env, const char *name, const char *objtype, const char *format, OSyncFilterFunction hook);

