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

#include "opensync_error_private.h"

/**
 * @defgroup OSyncErrorPrivateAPI OpenSync Error Internals
 * @ingroup OSyncPrivate
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

/*! @brief Translate a error type into something human readable
 * 
 * @param type The error type to look up
 * @returns The name of the error type
 * 
 */
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

/*! @brief Sets a error from a va_list
 * 
 * @param error A pointer to a error struct
 * @param type The type to set
 * @param format The message
 * @param args The arguments to the message
 * 
 */
void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args)
{
	osync_return_if_fail(error);
	osync_return_if_fail(osync_error_is_set(error) == FALSE);
	osync_return_if_fail(format);

	*error = g_malloc0(sizeof(OSyncError));
	(*error)->message = g_strdup_vprintf(format, args);
	(*error)->type = type;
	(*error)->ref_count = 1;

	osync_trace(TRACE_ERROR, "%s", (*error)->message);
	
	return;
}

/*@}*/

/**
 * @defgroup OSyncErrorAPI OpenSync Errors
 * @ingroup OSyncPublic
 * @brief OpenSync's error reporting facilities
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

/** @brief Increase the reference count of the error object 
 * 
 * @param error The error object 
 * @returns The referenced error pointer
 * 
 */
OSyncError **osync_error_ref(OSyncError **error)
{
	if (!osync_error_is_set(error))
		return error;
	
	g_atomic_int_inc(&(*error)->ref_count);

	return error;
}

/** @brief Decrease the reference count of the error object
 * 
 * @param error The error object 
 * 
 */
void osync_error_unref(OSyncError **error)
{
	if (!osync_error_is_set(error))
		return;
		
	if (g_atomic_int_dec_and_test(&(*error)->ref_count)) {
		if ((*error)->message)
			g_free ((*error)->message);
		
		if ((*error)->child)
			osync_error_unref(&((*error)->child));
		
		g_free(*error);
	}
	
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

/*! @brief Returns the type of the error
 * 
 * @param error The error
 * @returns The type of the error or OSYNC_NO_ERROR if no error
 * 
 */
OSyncErrorType osync_error_get_type(OSyncError **error)
{
	if (!osync_error_is_set(error))
		return OSYNC_NO_ERROR;
	return (*error)->type;
}

/*! @brief Returns the message of the error
 * 
 * @param error The error to print
 * @returns The message of the error or NULL if no error
 * 
 */
const char *osync_error_print(OSyncError **error)
{
	if (!osync_error_is_set(error))
		return NULL;
	return (*error)->message;
}


/*! @brief Returns the entired error stack as single string 
 * 
 * @param error The error stack to print
 * @returns The message of the error or NULL if no error
 * 
 */
char *osync_error_print_stack(OSyncError **error)
{
	if (!osync_error_is_set(error))
		return NULL;
		
	char *submessage = NULL;
	if ((*error)->child)
		submessage = osync_error_print_stack(&((*error)->child));
	
	char *message = NULL;
	if (submessage) {
		message = g_strdup_printf("NEXT ERROR: \"%s\"; %s", (*error)->message, submessage);
		g_free(submessage);
	} else
		message = g_strdup_printf("ROOT CAUSE: \"%s\"", (*error)->message);
	
	return message;
}

/*! @brief Duplicates the error into the target
 * 
 * 
 * @param target The target error to update
 * @param source The source error which to duplicate
 * 
 */
void osync_error_set_from_error(OSyncError **target, OSyncError **source)
{
	if (!target || osync_error_is_set(target))
		return;
	
	if (!osync_error_is_set(source)) {
		*target = NULL;
		return;
	}
	
	*target = *source;
	osync_error_ref(target);
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

/*! @brief Stack error on another error object 
 * 
 * Use this function to stack all errors to  describe the root cause of an error 
 * 
 * @param parent A pointer to a error which gets the child stacked 
 * @param child A pointer to a error to which get stacked on parent error
 * 
 */
void osync_error_stack(OSyncError **parent, OSyncError **child)
{
	if (!parent || !*parent)
		return;
	
	if (!child || !*child)
		return;

	/* Avoid infinite recursion. */
	if (*parent == *child)
		return;
	
	if ((*parent)->child)
		osync_error_unref(&((*parent)->child));
	
	(*parent)->child = *child;
	osync_error_ref(child);
}

/*! @brief Get stacked child of an error object 
 * 
 * Use this function to read an error stack 
 * 
 * @param parent A pointer to a error stack 
 * 
 */
OSyncError *osync_error_get_child(OSyncError **parent)
{
	if (!parent || !*parent)
		return NULL;
	
	return (*parent)->child;
}

/*! @brief Sets the type of an error
 * 
 * @param error A pointer to a error struct to set
 * @param type The Error type to set
 * 
 */
void osync_error_set_type(OSyncError **error, OSyncErrorType type)
{
	if (!error)
		return;
	
	(*error)->type = type;
	return;
}

/*@}*/
