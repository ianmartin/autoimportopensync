#include <opensync.h>
#include "opensync_internals.h"

/*
struct OSyncContext {
	void (*callback_function)(OSyncMember *, void *);
	void *user_data;
	OSyncMember *member;
	OSyncError error;
	osync_bool success;
};
*/

OSyncContext *osync_context_new(OSyncMember *member)
{
	OSyncContext *ctx = g_malloc0(sizeof(OSyncContext));
	ctx->member = member;
	return ctx;
}

void osync_context_free(OSyncContext *context)
{
	g_assert(context);
	//FIXME Do we need to free the user_data?
	g_free(context);
}

void *osync_context_get_plugin_data(OSyncContext *context)
{
	g_assert(context);
	g_assert(context->member);
	return context->member->plugindata;
}

void osync_context_report_error(OSyncContext *context, OSyncErrorType type, const char *format, ...)
{
	g_assert(context);
	OSyncError *error = g_malloc0(sizeof(OSyncError));
	va_list args;
	va_start(args, format);
	osync_error_set_vargs(error, type, format, args);
	if (context->callback_function)
		(context->callback_function)(context->member, context->calldata, error);
	va_end (args);
	osync_context_free(context);
}

void osync_context_report_success(OSyncContext *context)
{
	g_assert(context);
	if (context->callback_function)
		(context->callback_function)(context->member, context->calldata, NULL);
	osync_context_free(context);
}

void osync_context_report_change(OSyncContext *context, OSyncChange *change)
{
	g_assert(context);
	OSyncMember *member = context->member;
	g_assert(member);
	
	osync_assert(change->uid, "You forgot to set a uid on the change you reported!");
	osync_assert(change->data || change->changetype == CHANGE_DELETED, "You need to report some data unless you report CHANGE_DELETED");
	osync_assert((!(change->data) && change->size == 0) || (change->data && change->size != 0), "No data and datasize was not 0!");
	osync_assert((!change->data && change->changetype == CHANGE_DELETED) || (change->data && change->changetype != CHANGE_DELETED), "You cannot report data if you report CHANGE_DELETED. Just report the uid");
	
	if (change->changetype == CHANGE_DELETED)
		change->has_data = TRUE;
	
	member->memberfunctions->rf_change(member, change);
}

void osync_context_send_log(OSyncContext *ctx, const char *message, ...)
{
	g_assert(ctx);
	OSyncMember *member = ctx->member;
	g_assert(member);
	
	va_list arglist;
	char *buffer;
	va_start(arglist, message);
	g_vasprintf(&buffer, message, arglist);
	
	osync_debug("OSYNC", 3, "Sending logmessage \"%s\"", buffer);
	if (member->memberfunctions->rf_log)
		member->memberfunctions->rf_log(member, buffer);
	
	g_free(buffer);
	va_end(arglist);
}

void osync_report_message(OSyncMember *member, const char *message, void *data)
{
	member->memberfunctions->rf_message(member, message, data, FALSE);
}

void *osync_report_message_sync(OSyncMember *member, const char *message, void *data)
{
	return member->memberfunctions->rf_message(member, message, data, TRUE);
}

OSyncMember *osync_context_get_member(OSyncContext *ctx)
{
	g_assert(ctx);
	return ctx->member;
}
