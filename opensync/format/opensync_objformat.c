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

#include "opensync-format.h"
#include "opensync_objformat_internals.h"

/**
 * @defgroup OSyncObjectFormatAPI OpenSync Object Format
 * @ingroup OSyncPublic
 * @brief Functions for handling object formats
 * 
 */
/*@{*/

/**
 * @brief Creates a new object format
 * @param name the name of the object format
 * @param objtype_name the name of the object type
 * @param error Pointer to an error struct
 * @return The pointer to the newly allocated object format or NULL in case of error
 */
OSyncObjFormat *osync_objformat_new(const char *name, const char *objtype_name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %s, %p)", __func__, name, objtype_name, error);
	
	OSyncObjFormat *format = osync_try_malloc0(sizeof(OSyncObjFormat), error);
	if (!format)
		return FALSE;
	
	format->name = g_strdup(name);
	format->objtype_name = g_strdup(objtype_name);
	format->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, format);
	return format;
}

/*! @brief Increase the reference count on an object format
 * 
 * @param format Pointer to the object format
 * 
 */
OSyncObjFormat *osync_objformat_ref(OSyncObjFormat *format)
{
	osync_assert(format);
	
	g_atomic_int_inc(&(format->ref_count));

	return format;
}

/*! @brief Decrease the reference count on an object format
 * 
 * @param format Pointer to the object format
 * 
 */
void osync_objformat_unref(OSyncObjFormat *format)
{
	osync_assert(format);
	
	if (g_atomic_int_dec_and_test(&(format->ref_count))) {
		if (format->name)
			g_free(format->name);
			
		if (format->objtype_name)
			g_free(format->objtype_name);
		
		g_free(format);
	}
}

/**
 * @brief Returns the name of an object format
 * @param format Pointer to the object format
 * @return The name of the specified object format
 */
const char *osync_objformat_get_name(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->name;
}

/**
 * @brief Returns the object type of an object format
 * @param format Pointer to the object format
 * @return The name of the specified object format's object type
 */
const char *osync_objformat_get_objtype(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->objtype_name;
}

/**
 * @brief Compares the names of two object formats
 * @param leftformat Pointer to the object format to compare
 * @param rightformat Pointer to the other object format to compare
 * @return TRUE if the two object format names are equal, false otherwise
 */
osync_bool osync_objformat_is_equal(OSyncObjFormat *leftformat, OSyncObjFormat *rightformat)
{
	osync_assert(leftformat);
	osync_assert(rightformat);
	
	return (!strcmp(leftformat->name, rightformat->name)) ? TRUE : FALSE;
}

/**
 * @brief Sets the optional compare function for an object format
 *
 * The compare function can be used to compare two objects in your object 
 * format. This is optional - if you prefer, you can instead provide a 
 * conversion to and from the xml format and let all the comparison be done 
 * there.
 *
 * @param format Pointer to the object format
 * @param cmp_func The compare function to use
 */
void osync_objformat_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func)
{
	osync_assert(format);
	format->cmp_func = cmp_func;
}

/**
 * @brief Compares two objects of the same object format
 *
 * Compares two objects of the same object format using the format's compare function
 *
 * @param format Pointer to the object format
 * @param leftdata Pointer to the object to compare
 * @param leftsize the size in bytes of the object specified by the leftdata parameter
 * @param rightdata Pointer to the other object to compare
 * @param rightsize the size in bytes of the object specified by the rightdata parameter
 * @returns the comparison result
 */
OSyncConvCmpResult osync_objformat_compare(OSyncObjFormat *format, const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_assert(format);
	osync_assert(format->cmp_func);
	return format->cmp_func(leftdata, leftsize, rightdata, rightsize);
}

/**
 * @brief Sets the destroy function for an object format
 *
 * The destroy function is used to free data structures allocated by your format.
 *
 * @param format Pointer to the object format
 * @param destroy_func The destroy function to use
 */
void osync_objformat_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func)
{
	osync_assert(format);
	format->destroy_func = destroy_func;
}

/**
 * @brief Destroy an object of the specified format
 * @param format Pointer to the object format
 * @param data Pointer to the object to destroy
 * @param size Size in bytes of the object specified by the data parameter
 */
void osync_objformat_destroy(OSyncObjFormat *format, char *data, unsigned int size)
{
	osync_assert(format);
	
	if (!format->destroy_func) {
		osync_trace(TRACE_INTERNAL, "Format %s don't have a destroy function. Possible memory leak", format->name);
		return;
	}
	
	format->destroy_func(data, size);
}

void osync_objformat_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func)
{
	osync_assert(format);
	format->copy_func = copy_func;
}

osync_bool osync_objformat_copy(OSyncObjFormat *format, const char *indata, unsigned int insize, char **outdata, unsigned int *outsize, OSyncError **error)
{
	osync_assert(format);
	osync_assert(indata);
	osync_assert(outdata);

	if (!format->copy_func) {
		osync_trace(TRACE_INTERNAL, "We cannot copy the change, falling back to memcpy");
		*outdata = osync_try_malloc0(sizeof(char) * insize, error);
		if (!*outdata)
			return FALSE;
			
		memcpy(*outdata, indata, insize);
		*outsize = insize;
	} else {
		if (!format->copy_func(indata, insize, outdata, outsize, error)) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Something went wrong during copying");
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief Sets the duplicate function for an object format
 *
 * The duplicate function can be used to duplicate an object in your format.
 * Duplication does not mean to make two objects out of one, but to change 
 * the uid of the object in such a way that it differs from the original uid.
 *
 * Most formats will never need this.
 *
 * @param format Pointer to the object format
 * @param dupe_func The duplicate function to use
 */
void osync_objformat_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func)
{
	osync_assert(format);
	format->duplicate_func = dupe_func;
}

/**
 * @brief Duplicate an object of the specified format
 *
 * Duplication does not mean to make two objects out of one, but to change 
 * the uid of the object in such a way that it differs from the original uid.
 *
 * @param format Pointer to the object format
 * @param uid The uid of the object
 * @param input Pointer to the object to duplicate
 * @param insize Size in bytes of the object specified by the input parameter
 * @param newuid The new uid for the duplicate object
 * @param output Pointer to a pointer to be set to the duplicate object
 * @param outsize Pointer to a variable to be set to the size of the duplicate object
 * @param error Pointer to an error struct
 * @return TRUE if the duplication succeeded, FALSE otherwise.
 */
osync_bool osync_objformat_duplicate(OSyncObjFormat *format, const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error)
{
	osync_assert(format);

	if (!format->duplicate_func) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No duplicate function set");
		return FALSE;
	}

	return format->duplicate_func(uid, input, insize, newuid, output, outsize, dirty, error);
}

void osync_objformat_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func)
{
	osync_assert(format);
	format->create_func = create_func;
}

void osync_objformat_create(OSyncObjFormat *format, char **data, unsigned int *size)
{
	osync_assert(format);
	osync_assert(format->create_func);

	format->create_func(data, size);
}

/**
 * @brief Sets the print function for an object format
 *
 * If your format is not in a human readable format already, you should set
 * the print function to a function that returns a human readable string 
 * describing the object as closely as possible. This information will be 
 * used by the user to decide which object to pick when there is a conflict.
 *
 * @param format Pointer to the object format
 * @param print_func The print function to use
 */
void osync_objformat_set_print_func(OSyncObjFormat *format, OSyncFormatPrintFunc print_func)
{
	osync_assert(format);
	format->print_func = print_func;
}

/**
 * @brief Prints the specified object
 *
 * Uses the object format's print function if set, otherwise the object's
 * data will be returned as a string.
 *
 * @param format Pointer to the object format
 * @param data Pointer to the object to destroy
 * @param size Size in bytes of the object specified by the data parameter
 */
char *osync_objformat_print(OSyncObjFormat *format, const char *data, unsigned int size)
{
	osync_assert(format);
	
	if (!format->print_func)
		return g_strndup(data, size);
	
	return format->print_func(data, size);
}

void osync_objformat_set_revision_func(OSyncObjFormat *format, OSyncFormatRevisionFunc revision_func)
{
	osync_assert(format);
	format->revision_func = revision_func;
}

time_t osync_objformat_get_revision(OSyncObjFormat *format, const char *data, unsigned int size, OSyncError **error)
{
	osync_assert(format);
	osync_assert(data);
	
	if (!format->revision_func) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No revision function set");
		return -1;
	}
	
	return format->revision_func(data, size, error);
}

void osync_objformat_set_marshal_func(OSyncObjFormat *format, OSyncFormatMarshalFunc marshal_func)
{
	osync_assert(format);
	format->marshal_func = marshal_func;
}

osync_bool osync_objformat_must_marshal(OSyncObjFormat *format)
{
	osync_assert(format);
	return format->marshal_func ? TRUE : FALSE;
}

osync_bool osync_objformat_marshal(OSyncObjFormat *format, const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
	osync_assert(format);
	osync_assert(format->marshal_func);
	return format->marshal_func(input, inpsize, message, error);
}

void osync_objformat_set_demarshal_func(OSyncObjFormat *format, OSyncFormatDemarshalFunc demarshal_func)
{
	osync_assert(format);
	format->demarshal_func = demarshal_func;
}

osync_bool osync_objformat_demarshal(OSyncObjFormat *format, OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error)
{
	osync_assert(format);
	osync_assert(format->demarshal_func);
	return format->demarshal_func(message, output, outpsize, error);
}

/*@}*/
