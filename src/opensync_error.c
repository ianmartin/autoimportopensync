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

/*
void osync_error_init (OSyncError *error)
{
	if (error == NULL)
		return;
		
	error->type = OSYNC_NO_ERROR;
	error->message = NULL;
}*/

void osync_error_free (OSyncError **error)
{
	if (*error == NULL)
		return;


  if ((*error)->message)
    g_free ((*error)->message);

  g_free(*error);
  *error = NULL;
}

osync_bool osync_error_is_set (OSyncError *error)
{
	if (error == NULL)
		return FALSE;
	
	if (error->type)
		return TRUE;
		
	return FALSE;
}

void osync_error_set_vargs(OSyncError **error, OSyncErrorType type, const char *format, va_list args)
{
	g_assert(*error == NULL);
	g_assert(osync_error_is_set(*error) == FALSE);
	
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
