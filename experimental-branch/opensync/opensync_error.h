void osync_error_free(OSyncError **error);
osync_bool osync_error_is_set (OSyncError **error);
void osync_error_set(OSyncError **error, OSyncErrorType type, const char *format, ...);
const char *osync_error_get_name(OSyncError **error);
void osync_error_update(OSyncError **error, const char *format, ...);
void osync_error_duplicate(OSyncError **target, OSyncError **source);
const char *osync_error_print(OSyncError **error);
OSyncErrorType osync_error_get_type(OSyncError **error);
