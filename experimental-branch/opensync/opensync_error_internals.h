
struct OSyncError {
	OSyncErrorType type;
	char *message;
};

void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args);
