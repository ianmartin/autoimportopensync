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

	/** This field is necessary only because
	 * the is_like() function can be called
	 * before the base_format is registered.
	 *
	 * This may be not necessary if we add
	 * dependency information to the plugins.
	 *
	 * @see OSyncUnresolvedConverter
	 */
	char *is_like;
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
 *
 *FIXME: We have similar problems with the
 * detectors and other functions that refers
 * to other formats. We have two possible solutions:
 *
 * - Add a OSyncUnresolvedXXX struct for every type,
 *   and do the same that is done for the converters
 * - Add dependency information to the plugins
 *
 * The second approach can be easier. Two possible
 * implementations:
 *
 * - make get_info() function return RUN_LATER, and
 *   loop on the list of plugins until all initialized
 *   sucessfully (or all returned error)
 * - Provide a osync_require_format_plugin() or something
 *   similar, that will load the required plugin, if
 *   it is not loaded yet
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
 * if it can be converted to another format.
 *
 * The format references are strings because
 * of the plugin loading order problem.
 *
 * @see OSyncUnresolvedConverter
 */
typedef struct OSyncDataDetector {
	const char *sourceformat;
	const char *targetformat;
	OSyncFormatDetectDataFunc detect_func;
} OSyncDataDetector;

OSyncDataDetector *osync_conv_find_detector(OSyncFormatEnv *env, const char *origformat, const char *trgformat);

/** A target function for osync_conv_find_path_fn() */
typedef osync_bool (*OSyncPathTargetFn)(const void *data, OSyncObjFormat *fmt);

osync_bool osync_conv_find_path_fmtlist(OSyncFormatEnv *env, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist);

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata);
osync_bool osync_conv_convert_simple(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *fmt);
osync_bool osync_conv_convert_fmtlist(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets);
osync_bool osync_conv_convert_member_sink(OSyncFormatEnv *env, OSyncChange *change, OSyncMember *memb);

OSyncObjType *osync_conv_detect_objtype(OSyncFormatEnv *env, OSyncChange *change);
