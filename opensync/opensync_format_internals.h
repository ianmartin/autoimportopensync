/**
 * @defgroup OSyncConvPrivate OpenSync Conversion Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

/*! @brief The environment used for conversions
 */
struct OSyncFormatEnv {
	/** A list of object types */
	GList *objtypes;
	/** A List of formats */
	GList *objformats;
	/** A list of available converters */
	GList *converters;
	/** A list of filter functions */
	GList *filter_functions;
	/** A list of extensions */
	GList *extensions;
};

/*! @brief Represent a abstract object type (like "contact")
 */
struct OSyncObjType {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	char *name;
	GList *formats;
	GList *converters;
	OSyncFormatEnv *env;
	osync_bool needs_slow_sync;
	OSyncObjFormat *common_format;
#endif
};

/*! @brief Represent a format for a object type
 */
struct OSyncObjFormat {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	char *name;
	OSyncFormatEnv *env;
	OSyncObjType *objtype;
	//GList *extensions;
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCopyFunc copy_func;
	OSyncFormatCreateFunc create_func;
	OSyncFormatDestroyFunc destroy_func;
	OSyncFormatPrintFunc print_func;
	OSyncFormatRevisionFunc revision_func;
	OSyncFormatMarshallFunc marshall_func;
	OSyncFormatDemarshallFunc demarshall_func;
#endif
};

/*! @brief Represent a converter from one format to another
 */
struct OSyncFormatConverter {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	OSyncObjFormat *source_format;
	OSyncObjFormat *target_format;
	OSyncFormatConvertFunc convert_func;
	OSyncFormatDetectDataFunc detect_func;
	OSyncFormatConverterInitFunc init_func;
	OSyncFormatConverterFinalizeFunc fin_func;
	ConverterType type;
#endif
};

/*! @brief Represent a detector for a given format
 */
typedef struct OSyncDataDetector {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	const char *sourceformat;
	const char *targetformat;
	OSyncFormatDetectDataFunc detect_func;
#endif
} OSyncDataDetector;

/*! @brief An extension to a format
 */
typedef struct OSyncFormatExtension {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	OSyncObjFormat *from_format;
	OSyncObjFormat *to_format;
	char *name;
	OSyncFormatConvertFunc conv_func;
	OSyncFormatExtInitFunc init_func;
#endif
} OSyncFormatExtension;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef struct OSyncObjFormatSink {
	OSyncObjFormat *format;
	OSyncFormatFunctions functions;
	char *extension_name;
	struct OSyncObjTypeSink *objtype_sink;
	GList *commit_changes;
	GList *commit_contexts;
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
	const char *name;
	GList *formats;
} OSyncObjTypeTemplate;

typedef struct OSyncObjFormatTemplate {
	const char *name;
	const char *objtype;
	char *extension_name;
	OSyncFormatCommitFn commit_change;
	OSyncFormatAccessFn access;
	OSyncFormatReadFn read;
	OSyncFormatCommittedAllFn committed_all;
	OSyncFormatBatchCommitFn batch_commit;
	OSyncFormatCompareFunc cmp_func;
	OSyncFormatMergeFunc merge_func;
	OSyncFormatDuplicateFunc duplicate_func;
	OSyncFormatCopyFunc copy_func;
	OSyncFormatCreateFunc create_func;
	OSyncFormatDestroyFunc destroy_func;
	OSyncFormatPrintFunc print_func;
	OSyncFormatRevisionFunc revision_func;
	OSyncFormatMarshallFunc marshall_func;
	OSyncFormatDemarshallFunc demarshall_func;
} OSyncObjFormatTemplate;

typedef struct OSyncConverterTemplate {
	const char *source_format;
	const char *target_format;
	OSyncFormatConvertFunc convert_func;
	ConverterType type;
	OSyncFormatConverterInitFunc init_func;
	OSyncFormatConverterFinalizeFunc fin_func;
} OSyncConverterTemplate;

typedef struct OSyncFormatExtensionTemplate {
	char *from_formatname;
	char *to_formatname;
	char *name;
	OSyncFormatExtInitFunc init_func;
} OSyncFormatExtensionTemplate;

/** A target function for osync_conv_find_path_fn() */
typedef osync_bool (*OSyncPathTargetFn)(const void *data, OSyncObjFormat *fmt);

osync_bool osync_conv_find_path_fmtlist(OSyncFormatEnv *env, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist);

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata, const char *extension_name, OSyncError **error);
osync_bool osync_conv_convert_fmtlist(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets);
OSyncDataDetector *osync_env_find_detector(OSyncEnv *env, const char *sourcename, const char *targetname);
osync_bool osync_conv_objtype_is_any(const char *objstr);
OSyncFormatExtensionTemplate *osync_env_find_extension_template(OSyncEnv *env, const char *formatname);
OSyncFormatExtension *osync_conv_find_extension(OSyncFormatEnv *env, OSyncObjFormat *from_format, OSyncObjFormat *to_format, const char *extension_name);
OSyncChange *osync_converter_invoke_decap(OSyncFormatConverter *converter, OSyncChange *change, osync_bool *free_output);

#endif

/*@}*/
