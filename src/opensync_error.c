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

/**
 * @defgroup OSyncErrorPrivateAPI OpenSync Error Internals
 * @ingroup OSyncPrivate
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

static const char *osync_error_name_from_type(OSyncErrorType type)
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

void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args)
{
	if (!error)
		return;
	
	osync_return_if_fail(osync_error_is_set(error) == FALSE);
	
	char *buffer;
	*error = g_malloc0(sizeof(OSyncError));
	g_vasprintf(&buffer, format, args);
	
	(*error)->message = buffer;
	(*error)->type = type;
	return;
}

/*@}*/

/**
 * @defgroup OSyncErrorAPI OpenSync Errors
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/


/*! @brief This will return a string describing the type of the error
 * 
 * @param error A pointer to a error struct
 * @returns The description, NULL on error
 * 
 */
const char *osync_error_get_name(OSyncError **error)
{
	osync_return_val_if_fail(error != NULL, NULL);
	if (!*error)
		return osync_error_name_from_type(OSYNC_NO_ERROR);
	return osync_error_name_from_type((*error)->type);
}

/*! @brief Frees the error so it can be reused
 * 
 * @param error A pointer to a error struct to free
 * 
 */
void osync_error_free(OSyncError **error)
{
	osync_return_if_fail(error != NULL);
	if (*error == NULL)
		return;

	if ((*error)->message)
		g_free ((*error)->message);
		
	g_free(*error);
	*error = NULL;
}

/*! @brief Checks if the error is set
 * 
 * @param error A pointer to a error struct to check
 * @returns TRUE if the error is set, FALSE otherwise
 * 
 */
osync_bool osync_error_is_set (OSyncError **error)
{
	if (!error)
		return FALSE;
		
	if (*error == NULL)
		return FALSE;
	
	if ((*error)->type)
		return TRUE;
		
	return FALSE;
}

/*! @brief Updates the error message
 * 
 * You can use this function to update the error message on
 * a error. You can use the old error->message as a parameter
 * for this function.
 * 
 * @param error A pointer to a error struct to update
 * @param format The new message
 * 
 */
void osync_error_update_message(OSyncError **error, const char *format, ...)
{
	osync_return_if_fail(error != NULL);
	osync_return_if_fail(*error != NULL);

	va_list args;
	va_start(args, format);
	
	char *buffer = NULL;
	g_vasprintf(&buffer, format, args);
	
	g_free((*error)->message);
	(*error)->message = buffer;
	
	va_end (args);
}

/*! @brief Sets the error
 * 
 * You can use this function to set the error to the given type and message
 * 
 * @param error A pointer to a error struct to set
 * @param type The Error type to set
 * @param format The message
 * 
 */
void osync_error_set(OSyncError **error, OSyncErrorType type, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	osync_error_set_vargs(error, type, format, args);
	va_end (args);
}

/*@}*/
