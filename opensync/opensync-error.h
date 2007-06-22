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

#ifndef OPENSYNC_ERROR_H_
#define OPENSYNC_ERROR_H_

#include <stdarg.h>

#include <glib/gmacros.h>
G_BEGIN_DECLS

/*! @ingroup OSyncErrorAPI
 * @brief Defines the possible error types
 */
typedef enum {
	OSYNC_NO_ERROR = 0,
	OSYNC_ERROR_GENERIC = 1,
	OSYNC_ERROR_IO_ERROR = 2,
	OSYNC_ERROR_NOT_SUPPORTED = 3,
	OSYNC_ERROR_TIMEOUT = 4,
	OSYNC_ERROR_DISCONNECTED = 5,
	OSYNC_ERROR_FILE_NOT_FOUND = 6,
	OSYNC_ERROR_EXISTS = 7,
	OSYNC_ERROR_CONVERT = 8,
	OSYNC_ERROR_MISCONFIGURATION = 9,
	OSYNC_ERROR_INITIALIZATION = 10,
	OSYNC_ERROR_PARAMETER = 11,
	OSYNC_ERROR_EXPECTED = 12,
	OSYNC_ERROR_NO_CONNECTION = 13,
	OSYNC_ERROR_TEMPORARY = 14,
	OSYNC_ERROR_LOCKED = 15,
	OSYNC_ERROR_PLUGIN_NOT_FOUND = 16
} OSyncErrorType;

OSYNC_EXPORT void osync_error_ref(OSyncError **error);
OSYNC_EXPORT void osync_error_unref(OSyncError **error);
OSYNC_EXPORT osync_bool osync_error_is_set (OSyncError **error);
OSYNC_EXPORT void osync_error_set(OSyncError **error, OSyncErrorType type, const char *format, ...);
OSYNC_EXPORT const char *osync_error_get_name(OSyncError **error);
OSYNC_EXPORT void osync_error_set_from_error(OSyncError **target, OSyncError **source);
OSYNC_EXPORT const char *osync_error_print(OSyncError **error);
OSYNC_EXPORT char *osync_error_print_stack(OSyncError **error);
OSYNC_EXPORT void osync_error_stack(OSyncError **parent, OSyncError **child);
OSYNC_EXPORT OSyncError *osync_error_get_child(OSyncError **parent);
OSYNC_EXPORT OSyncErrorType osync_error_get_type(OSyncError **error);
OSYNC_EXPORT void osync_error_set_type(OSyncError **error, OSyncErrorType type);
OSYNC_EXPORT void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args);

G_END_DECLS

#endif //OPENSYNC_ERROR_H_
