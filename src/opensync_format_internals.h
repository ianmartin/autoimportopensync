struct OSyncFormatEnv {
	GList *objtypes;
	GList *objformats;
	GList *converters;
	GList *data_detectors;
	GList *unresolved_converters;
	char *pluginpath;
};

struct OSyncObjType {
	char *name;
	GList *formats;
	GList *converters;
	OSyncFormatEnv *env;
	osync_bool needs_slow_sync;
	OSyncObjFormat *common_format;
};

typedef struct OSyncObjFormatSink {
	OSyncObjFormat *format;
	OSyncFormatFunctions functions;
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

typedef struct OSyncObjTypeTemplate {
	char *name;
	GList *formats;
} OSyncObjTypeTemplate;

typedef struct OSyncObjFormatTemplate {
	char *name;
	osync_bool (* commit_change) (OSyncContext *, OSyncChange *);
	osync_bool (* access) (OSyncContext *, OSyncChange *);
} OSyncObjFormatTemplate;

struct OSyncFormatProperty {
	char *name;
	void *add_func;
	void *remove_func;
	OSyncFormatDetectFunc detect_func;
};

struct OSyncObjFormat {
	char *name;
	OSyncFormatEnv *env;
	OSyncObjType *objtype;
	GList *properties;
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDetectFunc detect_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCopyFunc copy_func;
	OSyncFormatCreateFunc create_func;
	OSyncFormatDestroyFunc destroy_func;
};

struct OSyncFormatConverter {
	OSyncObjFormat *source_format;
	OSyncObjFormat *target_format;
	OSyncFormatConvertFunc convert_func;
	ConverterType type;
	ConverterFlags flags;
};

/** Unresolved converter
 * 
 * Used to keep the list of converters
 * to types that weren't registered yet.
 */
typedef struct OSyncUnresolvedConverter {
	const char *source_format;
	const char *target_format;
	OSyncFormatConvertFunc convert_func;
	ConverterType type;
	ConverterFlags flags;
} OSyncUnresolvedConverter;

/** Data detector for a format
 *
 * It takes the data on a given format and detects
 * if it can be converted to another format
 */
typedef struct OSyncDataDetector {
	const char *sourceformat;
	const char *targetformat;
	OSyncFormatDetectDataFunc detect_func;
} OSyncDataDetector;

osync_bool osync_conv_detect_objtype(OSyncFormatEnv *env, OSyncChange *change);
OSyncDataDetector *osync_conv_find_detector(OSyncFormatEnv *env, const char *origformat, const char *trgformat);

