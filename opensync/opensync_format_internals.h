struct OSyncFormatEnv {
	GList *objtypes;
	GList *objformats;
	GList *converters;
	GList *filter_functions;
};

struct OSyncObjType {
	char *name;
	GList *formats;
	GList *converters;
	OSyncFormatEnv *env;
	osync_bool needs_slow_sync;
	OSyncObjFormat *common_format;
};

struct OSyncObjFormat {
	char *name;
	OSyncFormatEnv *env;
	OSyncObjType *objtype;
	GList *extensions;
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCopyFunc copy_func;
	OSyncFormatCreateFunc create_func;
	OSyncFormatDestroyFunc destroy_func;
	OSyncFormatPrintFunc print_func;
};

struct OSyncFormatConverter {
	OSyncObjFormat *source_format;
	OSyncObjFormat *target_format;
	OSyncFormatConvertFunc convert_func;
	OSyncFormatDetectDataFunc detect_func;
	ConverterType type;
};

typedef struct OSyncDataDetector {
	const char *sourceformat;
	const char *targetformat;
	OSyncFormatDetectDataFunc detect_func;
} OSyncDataDetector;

typedef struct OSyncObjFormatSink {
	OSyncObjFormat *format;
	OSyncFormatFunctions functions;
	struct OSyncObjTypeSink *objtype_sink;
} OSyncObjFormatSink;

typedef struct OSyncObjTypeSink {
	OSyncMember *member;
	OSyncObjType *objtype;
	osync_bool write;
	osync_bool read;
	osync_bool enabled;
	GList *formatsinks;
	OSyncObjFormatSink *selected_format;
	GList *properties;
} OSyncObjTypeSink;

typedef struct OSyncFormatExtension {
	OSyncObjFormat *format;
	OSyncFormatConvertFunc conv_to;
	OSyncFormatConvertFunc conv_from;
} OSyncFormatExtension;

typedef struct OSyncObjTypeTemplate {
	const char *name;
	GList *formats;
} OSyncObjTypeTemplate;

typedef struct OSyncObjFormatTemplate {
	const char *name;
	const char *objtype;
	osync_bool (* commit_change) (OSyncContext *, OSyncChange *);
	osync_bool (* access) (OSyncContext *, OSyncChange *);
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCopyFunc copy_func;
	OSyncFormatCreateFunc create_func;
	OSyncFormatDestroyFunc destroy_func;
	OSyncFormatPrintFunc print_func;
} OSyncObjFormatTemplate;

typedef struct OSyncConverterTemplate {
	const char *source_format;
	const char *target_format;
	OSyncFormatConvertFunc convert_func;
	ConverterType type;
} OSyncConverterTemplate;

typedef struct OSyncFormatExtensionTemplate {
	const char *formatname;
	OSyncFormatConvertFunc conv_to;
	OSyncFormatConvertFunc conv_from;
} OSyncFormatExtensionTemplate;

/** A target function for osync_conv_find_path_fn() */
typedef osync_bool (*OSyncPathTargetFn)(const void *data, OSyncObjFormat *fmt);

osync_bool osync_conv_find_path_fmtlist(OSyncFormatEnv *env, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist);

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata, OSyncError **error);
osync_bool osync_conv_convert_fmtlist(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets);
osync_bool osync_change_convert_member_sink(OSyncFormatEnv *env, OSyncChange *change, OSyncMember *memb, OSyncError **error);
OSyncDataDetector *osync_env_find_detector(OSyncEnv *env, const char *sourcename, const char *targetname);
osync_bool osync_conv_objtype_is_any(const char *objstr);
