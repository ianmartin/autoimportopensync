/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include <opensync.h>
#include "opensync_internals.h"

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

void osync_context_report_osyncerror(OSyncContext *context, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p:(%s))", __func__, context, error, osync_error_print(error));
	g_assert(context);
	if (context->callback_function)
		(context->callback_function)(context->member, context->calldata, error);
	osync_context_free(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_error(OSyncContext *context, OSyncErrorType type, const char *format, ...)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s)", __func__, context, type, format);
	g_assert(context);
	OSyncError *error = NULL;
	va_list args;
	va_start(args, format);
	osync_error_set_vargs(&error, type, format, args);
	osync_trace(TRACE_INTERNAL, "ERROR is: %s", osync_error_print(&error));
	if (context->callback_function)
		(context->callback_function)(context->member, context->calldata, &error);
	va_end (args);
	osync_context_free(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_success(OSyncContext *context)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, context);
	g_assert(context);
	if (context->callback_function)
		(context->callback_function)(context->member, context->calldata, NULL);
	osync_context_free(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_change(OSyncContext *context, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, context, change);
	g_assert(context);
	OSyncMember *member = context->member;
	g_assert(member);
	
	osync_assert(change->uid, "You forgot to set a uid on the change you reported!");
	osync_assert(change->data || change->changetype == CHANGE_DELETED, "You need to report some data unless you report CHANGE_DELETED");
	osync_assert((!(change->data) && change->size == 0) || (change->data && change->size != 0), "No data and datasize was not 0!");
	osync_assert((!change->data && change->changetype == CHANGE_DELETED) || (change->data && change->changetype != CHANGE_DELETED), "You cannot report data if you report CHANGE_DELETED. Just report the uid");
	
	osync_assert((osync_change_get_objformat(change) != NULL) || change->changetype == CHANGE_DELETED, "The reported change did not have a format set");
	osync_assert((osync_change_get_objtype(change) != NULL) || change->changetype == CHANGE_DELETED, "The reported change did not have a objtype set");
	
	
	if (change->changetype == CHANGE_DELETED)
		change->has_data = TRUE;
	
	change->initial_format = change->format;
	change->member = context->member;
	
	osync_trace(TRACE_INTERNAL, "Reporting change with uid %s, changetype %i, data %p, format %s and objtype %s", osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
	
	osync_assert(member->memberfunctions->rf_change, "The engine must set a callback to receive changes");
	member->memberfunctions->rf_change(member, change, context->calldata);
	osync_trace(TRACE_EXIT, "%s", __func__);
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
