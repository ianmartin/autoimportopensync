#ifndef _OPENSYNC_CONVREG_H_
#define _OPENSYNC_CONVREG_H_

void osync_env_register_detector(OSyncEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func);
void osync_env_register_filter_function(OSyncEnv *env, const char *name, const char *objtype, const char *format, OSyncFilterFunction hook);
void osync_env_register_objformat(OSyncEnv *env, const char *objtypename, const char *name);
void osync_env_register_objtype(OSyncEnv *env, const char *name);
void osync_env_register_converter(OSyncEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func);
void osync_env_converter_set_init(OSyncEnv *env, const char *sourcename, const char *targetname, OSyncFormatConverterInitFunc init_func);
void osync_env_register_extension(OSyncEnv *env, const char *objformatname, const char *extension_name, OSyncFormatExtInitFunc init_to_func, OSyncFormatExtInitFunc init_from_func);
void osync_env_format_set_compare_func(OSyncEnv *env, const char *formatname, OSyncFormatCompareFunc cmp_func);
void osync_env_format_set_destroy_func(OSyncEnv *env, const char *formatname, OSyncFormatDestroyFunc destroy_func);
void osync_env_format_set_copy_func(OSyncEnv *env, const char *formatname, OSyncFormatCopyFunc copy_func);
void osync_env_format_set_duplicate_func(OSyncEnv *env, const char *formatname, OSyncFormatDuplicateFunc dupe_func);
void osync_env_format_set_create_func(OSyncEnv *env, const char *formatname, OSyncFormatCreateFunc create_func);
void osync_env_format_set_print_func(OSyncEnv *env, const char *formatname, OSyncFormatPrintFunc print_func);

#endif //_OPENSYNC_CONVREG_H_
