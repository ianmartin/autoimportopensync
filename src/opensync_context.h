OSyncContext *osync_context_new(OSyncMember *member);
void osync_context_free(OSyncContext *context);
void osync_context_report_error(OSyncContext *context, OSyncErrorType type, const char *format, ...);
void osync_context_report_success(OSyncContext *context);
void osync_context_report_change(OSyncContext *context, OSyncChange *change);
void osync_report_message(OSyncMember *member, const char *message, void *data);
void *osync_report_message_sync(OSyncMember *member, const char *message, void *data);
void *osync_context_get_plugin_data(OSyncContext *context);
