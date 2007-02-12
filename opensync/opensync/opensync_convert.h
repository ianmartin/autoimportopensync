
typedef enum {
	/** Simple converter */
	CONVERTER_CONV = 1,
	/** Encapsulator */
	CONVERTER_ENCAP = 2,
	/** Desencapsulator */
	CONVERTER_DECAP = 3,
	/** Detector */
	CONVERTER_DETECTOR = 4
} ConverterType;

/*! @ingroup OSyncChangeCmds
 * @brief The possible returns of a change comparison
 */
typedef enum {
	/** The result is unknown, there was a error */
	CONV_DATA_UNKNOWN = 0,
	/** The changes are not the same */
	CONV_DATA_MISMATCH = 1,
	/** The changs are not the same but look similar */
	CONV_DATA_SIMILAR = 2,
	/** The changes are exactly the same */
	CONV_DATA_SAME = 3
} OSyncConvCmpResult;

typedef OSyncConvCmpResult (* OSyncFormatCompareFunc) (OSyncChange *leftchange, OSyncChange *rightchange);
typedef osync_bool (* OSyncFormatConvertFunc) (void *init_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error);
typedef osync_bool (* OSyncFormatCopyFunc) (const char *input, int inpsize, char **output, int *outpsize);
typedef osync_bool (* OSyncFormatDetectDataFunc) (OSyncFormatEnv *env, const char *data, int size);
typedef void (* OSyncFormatDuplicateFunc) (OSyncChange *change);
typedef void (* OSyncFormatCreateFunc) (OSyncChange *change);
typedef void (* OSyncFormatMergeFunc) (OSyncChange *leftchange, OSyncChange *rightchange, OSyncError **);
typedef void (* OSyncFormatDestroyFunc) (char *data, size_t size);
typedef char *(* OSyncFormatPrintFunc) (OSyncChange *change);
typedef void *(* OSyncFormatConverterInitFunc) (void);
typedef void (* OSyncFormatConverterFinalizeFunc) (void *);
typedef osync_bool (* OSyncFormatExtInitFunc) (void *);
typedef time_t (* OSyncFormatRevisionFunc) (OSyncChange *change, OSyncError **error);
typedef osync_bool (* OSyncFormatMarshallFunc) (const char *nput, int inpsize, char **output, int *outpsize, OSyncError **);
typedef osync_bool (* OSyncFormatDemarshallFunc) (const char *nput, int inpsize, char **output, int *outpsize, OSyncError **);

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
