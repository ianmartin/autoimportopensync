#ifndef HAVE_OPENSYNC_H
#define HAVE_OPENSYNC_H

#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************
 * Defines
 *************************************************************/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

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
	OSYNC_ERROR_PARAMETER = 11,
	OSYNC_ERROR_EXPECTED = 12,
	OSYNC_ERROR_NO_CONNECTION = 13,
	OSYNC_ERROR_TEMPORARY = 14,
	OSYNC_ERROR_LOCKED = 15
} OSyncErrorType;

typedef struct OSyncError OSyncError;
typedef struct OSyncEnv OSyncEnv;
typedef struct OSyncPlugin OSyncPlugin;
typedef struct OSyncGroup OSyncGroup;
typedef struct OSyncUserInfo OSyncUserInfo;
typedef struct OSyncMember OSyncMember;
typedef struct OSyncChange OSyncChange;
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
	void * (* initialize) (OSyncMember *, OSyncError **);
	void (* finalize) (void *);
	void (* connect) (OSyncContext *);
	void (* sync_done) (OSyncContext *ctx);
	void (* disconnect) (OSyncContext *);
	void (* get_changeinfo) (OSyncContext *);
	void (* get_data) (OSyncContext *, OSyncChange *);
} OSyncPluginFunctions;

typedef struct OSyncPluginTimeouts {
	unsigned int connect_timeout;
	unsigned int sync_done_timeout;
	unsigned int disconnect_timeout;
	unsigned int get_changeinfo_timeout;
	unsigned int get_data_timeout;
	unsigned int commit_timeout;
} OSyncPluginTimeouts;

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
	const char *longname;
	const char *description;
	osync_bool is_threadsafe;
	OSyncPluginFunctions functions;
	OSyncPluginTimeouts timeouts;
	OSyncPlugin *plugin;
} OSyncPluginInfo;

typedef enum {
	CONV_DATA_UNKNOWN = 0,
	CONV_DATA_MISMATCH = 1,
	CONV_DATA_SIMILAR = 2,
	CONV_DATA_SAME = 3
} OSyncConvCmpResult;

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

typedef enum OSyncFilterAction {
	OSYNC_FILTER_IGNORE = 0,
	OSYNC_FILTER_ALLOW = 1,
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

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

typedef OSyncFilterAction (* OSyncFilterFunction)(OSyncChange *, char *config);

/**************************************************************
 * Structs
 *************************************************************/

typedef void (* OSyncEngCallback)(OSyncMember *, void *, OSyncError **);

typedef struct OSyncMemberFunctions {
	void (* rf_change) (OSyncMember *, OSyncChange *, void *);
	void *(* rf_message) (OSyncMember *, const char *, void *, osync_bool);
	void (* rf_sync_alert) (OSyncMember *);
	void (*rf_log) (OSyncMember *, char *);
} OSyncMemberFunctions;

/**************************************************************
 * Includes
 *************************************************************/

typedef enum OSyncTraceType {
	TRACE_ENTRY,
	TRACE_EXIT,
	TRACE_INTERNAL,
	TRACE_EXIT_ERROR
} OSyncTraceType;

char *osync_rand_str(int maxlength);
void osync_debug(const char *subpart, int level, const char *message, ...);
void osync_print_binary(const unsigned char *data, int len);
void osync_trace(OSyncTraceType type, const char *message, ...);

/**************************************************************
 * Prototypes
 *************************************************************/

#include "opensync_env.h"
#include "opensync_plugin.h"
#include "opensync_group.h"
#include "opensync_member.h"
#include "opensync_error.h"
#include "opensync_hashtable.h"
#include "opensync_change.h"
#include "opensync_context.h"
#include "opensync_convert.h"
#include "opensync_convreg.h"
#include "opensync_anchor.h"
#include "opensync_filter.h"

#ifdef __cplusplus
}
#endif

#endif
