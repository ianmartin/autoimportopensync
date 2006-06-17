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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-context.h"
#include "opensync_context_internals.h"

OSyncContext *osync_context_new(OSyncError **error)
{
	OSyncContext *ctx = osync_try_malloc0(sizeof(OSyncContext), error);
	if (!ctx)
		return NULL;
	
	ctx->ref_count = 1;
	
	return ctx;
}

void osync_context_ref(OSyncContext *context)
{
	osync_assert(context);
	
	g_atomic_int_inc(&(context->ref_count));
}

void osync_context_unref(OSyncContext *context)
{
	osync_assert(context);
	
	if (g_atomic_int_dec_and_test(&(context->ref_count))) {
		g_free(context);
	}
}

void osync_context_set_callback(OSyncContext *context, OSyncContextCallbackFn callback, void *userdata)
{
	osync_assert(context);
	context->callback_function = callback;
	context->callback_data = userdata;
}

void osync_context_set_changes_callback(OSyncContext *context, OSyncContextChangeFn changes)
{
	osync_assert(context);
	context->changes_function = changes;
}

void *osync_context_get_plugin_data(OSyncContext *context)
{
	osync_assert(context);
	return context->plugindata;
}

void osync_context_set_plugin_data(OSyncContext *context, void *data)
{
	osync_assert(context);
	context->plugindata = data;
}

void osync_context_report_osyncerror(OSyncContext *context, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p:(%s))", __func__, context, error, osync_error_print(&error));
	osync_assert(context);
	
	if (context->callback_function)
		(context->callback_function)(context->callback_data, error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_error(OSyncContext *context, OSyncErrorType type, const char *format, ...)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s)", __func__, context, type, format);
	osync_assert(context);
	
	OSyncError *error = NULL;
	va_list args;
	va_start(args, format);
	osync_error_set_vargs(&error, type, format, args);
	osync_trace(TRACE_INTERNAL, "ERROR is: %s", osync_error_print(&error));
	va_end (args);
	
	if (context->callback_function)
		(context->callback_function)(context->callback_data, error);
	
	osync_error_unref(&error);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_success(OSyncContext *context)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, context);
	osync_assert(context);
	
	if (context->callback_function)
		(context->callback_function)(context->callback_data, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_context_report_change(OSyncContext *context, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, context, change);
	osync_assert(context);
	osync_assert(change);
	
	/*OSyncMember *member = context->member;
	osync_assert(member);
	osync_change_set_member(change, member);
	
	osync_assert_msg(change->uid, "You forgot to set a uid on the change you reported!");
	osync_assert_msg(change->data || change->changetype == CHANGE_DELETED, "You need to report some data unless you report CHANGE_DELETED");
	osync_assert_msg((!(change->data) && change->size == 0) || (change->data && change->size != 0), "No data and datasize was not 0!");
	osync_assert_msg((!change->data && change->changetype == CHANGE_DELETED) || (change->data && change->changetype != CHANGE_DELETED), "You cannot report data if you report CHANGE_DELETED. Just report the uid");
	
	osync_assert_msg((osync_change_get_objformat(change) != NULL) || change->changetype == CHANGE_DELETED, "The reported change did not have a format set");
	osync_assert_msg((osync_change_get_objtype(change) != NULL) || change->changetype == CHANGE_DELETED, "The reported change did not have a objtype set");
	osync_assert_msg((osync_change_get_changetype(change) != CHANGE_UNKNOWN), "The reported change did not have a changetype set");
	
	
	if (change->changetype == CHANGE_DELETED)
		change->has_data = TRUE;
	
	change->initial_format = osync_change_get_objformat(change);
	
	osync_trace(TRACE_INTERNAL, "Reporting change with uid %s, changetype %i, data %p, objtype %s and format %s", osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_change_get_objtype(change) ? osync_objtype_get_name(osync_change_get_objtype(change)) : "None", osync_change_get_objformat(change) ? osync_objformat_get_name(osync_change_get_objformat(change)) : "None");
	
	osync_assert_msg(member->memberfunctions->rf_change, "The engine must set a callback to receive changes");
	member->memberfunctions->rf_change(member, change, context->calldata);*/
	
	
	if (context->changes_function)
		(context->changes_function)(context->callback_data, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}
