
/*! @brief Represent an error
 */
struct OSyncError {
	/** The type of the error that occured */
	OSyncErrorType type;
	/** The message */
	char *message;
};

void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args);
