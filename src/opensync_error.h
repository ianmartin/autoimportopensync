void osync_error_free (OSyncError **error);
osync_bool osync_error_is_set (OSyncError *error);
void osync_error_set (OSyncError **error, OSyncErrorType type, const char *format, ...);
const char *osync_error_name_from_type(OSyncErrorType type);
const char *osync_error_get_name(OSyncError *error);
