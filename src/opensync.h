#ifndef HAVE_OPENSYNC_H
#define HAVE_OPENSYNC_H

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

/**************************************************************
 * Enumerations
 *************************************************************/
typedef enum  {
	CHANGE_UNKNOWN = 0,
	CHANGE_ADDED = 1,
	CHANGE_UNMODIFIED = 2,
	CHANGE_DELETED = 3,
	CHANGE_MODIFIED = 4
} OSyncChangeType;

typedef enum {
	OSYNC_NO_ERROR = 0,
	OSYNC_ERROR_GENERIC = 1,
	OSYNC_ERROR_IO_ERROR = 2,
	OSYNC_ERROR_NOT_SUPPORTED = 3,
	OSYNC_ERROR_TIMEOUT = 4,
	OSYNC_ERROR_DISCONNECTED = 5,
	OSYNC_ERROR_FILE_NOT_FOUND = 6,
	OSYNC_ERROR_EXISTS = 7,
	OSYNC_ERROR_CONVERT = 8,
	OSYNC_ERROR_MISCONFIGURATION = 9,
	OSYNC_ERROR_INITIALIZATION = 10,
	OSYNC_ERROR_PARAMETER = 11
} OSyncErrorType;

typedef struct OSyncEnv OSyncEnv;
typedef struct OSyncPlugin OSyncPlugin;
typedef struct OSyncGroup OSyncGroup;
typedef struct OSyncUserInfo OSyncUserInfo;
typedef struct OSyncMember OSyncMember;
typedef struct OSyncChange OSyncChange;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMapping OSyncMapping;
typedef struct OSyncMappingColumn OSyncMappingColumn;
typedef struct OSyncContext OSyncContext;
typedef struct OSyncHashTable OSyncHashTable;
typedef struct OSyncFormatEnv OSyncFormatEnv;
typedef struct OSyncObjType OSyncObjType;
typedef struct OSyncObjFormat OSyncObjFormat;
typedef struct OSyncFormatConverter OSyncFormatConverter;
typedef struct OSyncFormatProperty OSyncFormatProperty;
typedef struct OSyncFilter OSyncFilter;
typedef struct OSyncCustomFilter OSyncCustomFilter;
typedef int osync_bool;

typedef struct OSyncPluginFunctions {
	osync_bool (* get_config) (char *, char **, int *);
	osync_bool (* store_config) (char *, const char *, int);
	void * (* initialize) (OSyncMember *);
	void (* connect) (OSyncContext *);
	void (* sync_done) (OSyncContext *ctx);
	void (* disconnect) (OSyncContext *);
	void (* finalize) (void *);
	void (* get_changeinfo) (OSyncContext *);
	void (* get_data) (OSyncContext *, OSyncChange *);
} OSyncPluginFunctions;

typedef struct OSyncFormatFunctions {
	osync_bool (* commit_change) (OSyncContext *, OSyncChange *);
	osync_bool (* access) (OSyncContext *, OSyncChange *);
} OSyncFormatFunctions;

/*FIXME:
 * The plugin get_info() function just gets a OSyncPluginInfo
 * structure, and the accepted object types should be registered
 * to the PluginInfo structure, but it would be better if
 * osync_plugin_find_accepted_objtype() functions get a OSyncPlugin object
 * as parameter.
 */
typedef struct OSyncPluginInfo {
	int version;
	const char *name;
	const char *description;
	osync_bool is_threadsafe;
	OSyncPluginFunctions functions;
	OSyncPlugin *plugin;
} OSyncPluginInfo;

typedef enum {
	CONV_DATA_UNKNOWN = 0,
	CONV_DATA_MISMATCH = 1,
	CONV_DATA_SIMILAR = 2,
	CONV_DATA_SAME = 3
} OSyncConvCmpResult;

typedef enum {
	/** Not Lossy converter
	 *
	 * Set this flag if information is not lossy when
	 * converting through this converter.
	 */
	CONV_NOTLOSSY = 1<<0,

	/** Data take-over converter
	 *
	 * Set this flag if the converter takes the ownership and responsibility
	 * of deallocating the data passed as input.
	 */
	CONV_TAKEOVER = 1<<1,

	/** No-copy converter
	 *
	 * Set this flag if the pointer returned by the converter
	 * is a reference to some data inside the input data, and the
	 * reference remain valid only while the input data is not changed and/or
	 * destroyed.
	 *
	 * The returned data may need to be copied, if the original
	 * data is going to be destroyed (i.e. on osync_converter_invoke()),
	 * so a copy function should be provided by the format of the
	 * returned data.
	 */
	CONV_NOCOPY = 1<<2,

	/** Detect first
	 *
	 * Set this flag if the converter is expected to be called
	 * only if the data was detected to be of the target format
	 */
	CONV_DETECTFIRST = 1<<3,
} ConverterFlags;

typedef enum {
	/** Simple converter */
	CONVERTER_CONV = 1,
	/** Encapsulator */
	CONVERTER_ENCAP = 2,
	/** Desencapsulator */
	CONVERTER_DESENCAP = 3,
} ConverterType;

typedef enum OSyncFilterAction {
	OSYNC_FILTER_IGNORE = 0,
	OSYNC_FILTER_ALLOW = 1,
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

typedef OSyncConvCmpResult (* OSyncFormatCompareFunc) (OSyncChange *leftchange, OSyncChange *rightchange);
typedef osync_bool (* OSyncFormatConvertFunc) (const char *input, int inpsize, char **output, int *outpsize);
typedef osync_bool (* OSyncFormatCopyFunc) (const char *input, int inpsize, char **output, int *outpsize);
typedef osync_bool (* OSyncFormatDetectFunc) (OSyncFormatEnv *env, const char *data, int size, OSyncObjFormat **format);
typedef osync_bool (* OSyncFormatDetectDataFunc) (OSyncFormatEnv *env, const char *data, int size);
typedef void (* OSyncFormatDuplicateFunc) (OSyncChange *change);
typedef void (* OSyncFormatCreateFunc) (OSyncChange *change);
typedef void (* OSyncFormatMergeFunc) (OSyncChange *leftchange, OSyncChange *rightchange);
typedef void (* OSyncFormatDestroyFunc) (char *data, size_t size);

typedef OSyncFilterAction (* OSyncFilterFunction)(OSyncChange *, char *config);

/**************************************************************
 * Structs
 *************************************************************/

typedef struct OSyncError {
	OSyncErrorType type;
	char *message;
} OSyncError;

typedef void (* OSyncEngCallback)(OSyncMember *, void *, OSyncError **);

typedef struct OSyncMemberFunctions {
	void (* rf_change) (OSyncMember *, OSyncChange *);
	void *(* rf_message) (OSyncMember *, const char *, void *, osync_bool);
	void (* rf_sync_alert) (OSyncMember *);
	void (*rf_log) (OSyncMember *, char *);
} OSyncMemberFunctions;

/**************************************************************
 * Includes
 *************************************************************/

char *osync_rand_str(int maxlength);
void osync_debug(const char *subpart, int level, const char *message, ...);
void osync_print_binary(const unsigned char *data, int len);

/**************************************************************
 * Prototypes
 *************************************************************/

#include "opensync_env.h"
#include "opensync_plugin.h"
#include "opensync_group.h"
#include "opensync_member.h"
#include "opensync_mapping.h"
#include "opensync_error.h"
#include "opensync_hashtable.h"
#include "opensync_change.h"
#include "opensync_context.h"
#include "opensync_convert.h"
#include "opensync_anchor.h"
#include "opensync_filter.h"

#endif
