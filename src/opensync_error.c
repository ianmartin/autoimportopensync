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

const char *osync_error_name_from_type(OSyncErrorType type)
{
	switch (type) {
		case OSYNC_NO_ERROR:
			return "NoError";
		case OSYNC_ERROR_GENERIC:
			return "UnknownError";
		case OSYNC_ERROR_IO_ERROR:
			return "IOError";
		case OSYNC_ERROR_NOT_SUPPORTED:
			return "NotSupported";
		case OSYNC_ERROR_TIMEOUT:
			return "Timeout";
		case OSYNC_ERROR_DISCONNECTED:
			return "Disconnected";
		case OSYNC_ERROR_FILE_NOT_FOUND:
			return "FileNotFound";
		default:
			return "UnspecifiedError";
	}
}

const char *osync_error_get_name(OSyncError *error)
{
	return osync_error_name_from_type(error->type);
}

void osync_error_free (OSyncError **error)
{
	if (*error == NULL)
		return;


  if ((*error)->message)
    g_free ((*error)->message);

  g_free(*error);
  *error = NULL;
}

osync_bool osync_error_is_set (OSyncError **error)
{
	if (error == NULL)
		return FALSE;
		
	if (*error == NULL)
		return FALSE;
	
	if ((*error)->type)
		return TRUE;
		
	return FALSE;
}

void osync_error_update_message(OSyncError **error, const char *format, ...)
{
	g_assert(error);
	g_assert(*error);
	va_list args;
	va_start(args, format);
	
	char *buffer;
	g_vasprintf(&buffer, format, args);
	
	g_free((*error)->message);
	(*error)->message = buffer;
	
	va_end (args);
}

void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args)
{
	if (error == NULL)
		return;
	g_assert(*error == NULL);
	g_assert(osync_error_is_set(error) == FALSE);
	
	char *buffer;
	*error = g_malloc0(sizeof(OSyncError));
	g_vasprintf(&buffer, format, args);
	
	(*error)->message = buffer;
	(*error)->type = type;
	return;
}

void osync_error_set(OSyncError **error, OSyncErrorType type, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	osync_error_set_vargs(error, type, format, args);
	va_end (args);
}
