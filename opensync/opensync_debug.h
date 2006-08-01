#ifndef _OPENSYNC_DEBUG_H_
#define _OPENSYNC_DEBUG_H_

/*! @ingroup OSyncDebugAPI
 * @brief The type of the trace */
typedef enum {
	/** Used when entering a function. This will indent the callgraph */
	TRACE_ENTRY,
	/** Used when exiting a function. This will unindent the callgraph */
	TRACE_EXIT,
	/** Used for traces inside a function. Does not indent. */
	TRACE_INTERNAL,
	/** Used for traces with sensitive content inside a function. Does not indent. */
	TRACE_SENSITIVE,
	/** Used when exiting a function with a error. This will unindent the callgraph */
	TRACE_EXIT_ERROR,
	TRACE_ERROR
} OSyncTraceType;

char *osync_rand_str(int maxlength);
void osync_debug(const char *subpart, int level, const char *message, ...);
char *osync_print_binary(const unsigned char *data, int len);
void osync_trace_reset_indent(void);
void osync_trace(OSyncTraceType type, const char *message, ...);

#endif //_OPENSYNC_DEBUG_H_
