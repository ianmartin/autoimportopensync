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
#include "opensync-data.h"
#include "opensync-format.h"
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
	OSyncError *error = NULL;
	va_list args;
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s)", __func__, context, type, format);
	osync_assert(context);
	
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
	
	osync_assert_msg(osync_change_get_uid(change), "You forgot to set a uid on the change you reported!");
	osync_assert_msg(osync_change_get_data(change) || osync_change_get_changetype(change) == OSYNC_CHANGE_TYPE_DELETED, "You need to report some data unless you report CHANGE_DELETED");
	
	OSyncData *data = osync_change_get_data(change);
	osync_assert(data);
	
	osync_assert_msg((osync_data_get_objformat(data) != NULL) || osync_change_get_changetype(change) == OSYNC_CHANGE_TYPE_DELETED, "The reported change did not have a format set");
	osync_assert_msg((osync_data_get_objtype(data) != NULL) || osync_change_get_changetype(change) == OSYNC_CHANGE_TYPE_DELETED, "The reported change did not have a objtype set");
		
	osync_trace(TRACE_INTERNAL, "Reporting change with uid %s, changetype %i, data %p, objtype %s and format %s", osync_change_get_uid(change), osync_change_get_changetype(change), osync_change_get_data(change), osync_data_get_objtype(data), osync_data_get_objformat(data) ? osync_objformat_get_name(osync_data_get_objformat(data)) : "None");
	
	osync_assert_msg(context->changes_function, "The engine must set a callback to receive changes");
	
	context->changes_function(change, context->callback_data);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}
