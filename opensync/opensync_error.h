
/*! @ingroup OSyncErrorAPI
 * @brief Defines the possible error types
 */
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

void osync_error_free(OSyncError **error);
osync_bool osync_error_is_set (OSyncError **error);
void osync_error_set(OSyncError **error, OSyncErrorType type, const char *format, ...);
const char *osync_error_get_name(OSyncError **error);
void osync_error_update(OSyncError **error, const char *format, ...);
void osync_error_duplicate(OSyncError **target, OSyncError **source);
const char *osync_error_print(OSyncError **error);
OSyncErrorType osync_error_get_type(OSyncError **error);
