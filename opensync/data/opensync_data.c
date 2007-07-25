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
#include "opensync-data.h"
#include "opensync_data_internals.h"

/**
 * @defgroup OSyncDataAPI OpenSync Data
 * @ingroup OSyncPublic
 * @brief Handles data within changes
 * 
 */
/*@{*/

/*! @brief Create a new data object
 * 
 * @param buffer Character buffer containing the data
 * @param size The size of the data contained in the buffer
 * @param format The object format of the data
 * @param error An error struct
 * @returns The new data object
 * 
 */
OSyncData *osync_data_new(char *buffer, unsigned int size, OSyncObjFormat *format, OSyncError **error)
{
	OSyncData *data = osync_try_malloc0(sizeof(OSyncData), error);
	if (!data)
		return NULL;
	
	if (buffer && size) {
		data->data = buffer;
		data->size = size;
	}
	data->ref_count = 1;
	data->objformat = format;
	osync_objformat_ref(format);
	
	return data;
}

/*! @brief Increase the reference count on a data object
 * 
 * @param data The data object
 * 
 */
void osync_data_ref(OSyncData *data)
{
	osync_assert(data);
	
	g_atomic_int_inc(&(data->ref_count));
}

/*! @brief Decrease the reference count on a data object
 * 
 * @param data The data object
 * 
 */
void osync_data_unref(OSyncData *data)
{
	osync_assert(data);
	
	if (g_atomic_int_dec_and_test(&(data->ref_count))) {
		if (data->data)
			osync_objformat_destroy(data->objformat, data->data, data->size);
			
		if (data->objformat)
			osync_objformat_unref(data->objformat);
			
		if (data->objtype)
			g_free(data->objtype);
		
		g_free(data);
	}
}

/*! @brief Get the object format from a data object
 * 
 * @param data The data object
 * @returns the object format of the data object
 * 
 */
OSyncObjFormat *osync_data_get_objformat(OSyncData *data)
{
	osync_assert(data);
	return data->objformat;
}

/*! @brief Set the object format on a data object
 * 
 * @param data The data object
 * @param objformat The object format to set
 * 
 */
void osync_data_set_objformat(OSyncData *data, OSyncObjFormat *objformat)
{
	osync_assert(data);
	if (data->objformat)
		osync_objformat_unref(data->objformat);
	data->objformat = objformat;
	osync_objformat_ref(objformat);
}

/*! @brief Get the object type from a data object
 * 
 * @param data The data object
 * @returns the name of the object type of the data object
 * 
 */
const char *osync_data_get_objtype(OSyncData *data)
{
	osync_assert(data);
	if (data->objtype)
		return data->objtype;
	
	/* If no object type is explicitly set, we will just
	 * return the default objtype for this format */
	OSyncObjFormat *format = data->objformat;
	if (format)
		return osync_objformat_get_objtype(format);
	
	return NULL;
}

/*! @brief Set the object type of a data object
 * 
 * @param data The data object
 * @param objtype The name of the object type to set
 * 
 */
void osync_data_set_objtype(OSyncData *data, const char *objtype)
{
	osync_assert(data);
	if (data->objtype)
		g_free(data->objtype);
	data->objtype = g_strdup(objtype);
}

/*! @brief Get the data from a data object
 * 
 * @param data The data object
 * @param buffer Pointer to a char * that will be set to point to the data if specified. Do not free this buffer.
 * @param size Pointer to an integer variable that will be set to the size of the data if specified
 * 
 */
void osync_data_get_data(OSyncData *data, char **buffer, unsigned int *size)
{
	osync_assert(data);
	if (buffer)
		*buffer = data->data;
	
	if (size)
		*size = data->size;
}

/*! @brief Get a pointer to the data from a data object
 * 
 * @param data The data object
 * @returns a pointer to the data. Do not free this.
 * 
 */
void *osync_data_get_data_ptr(OSyncData *data)
{
	osync_assert(data);
	return data->data;
}

/*! @brief Get the data from a data object and then clear the data object's pointers to it
 * 
 * @param data The data object
 * @param buffer Pointer to a char * that will be set to point to the data. The caller is responsible for freeing this after calling.
 * @param size Pointer to an integer variable that will be set to the size of the data
 * 
 */
void osync_data_steal_data(OSyncData *data, char **buffer, unsigned int *size)
{
	osync_assert(data);
	osync_assert(buffer);
	osync_assert(size);
	
	*buffer = data->data;
	*size = data->size;
	
	data->data = NULL;
	data->size = 0;
}

/*! @brief Set the data of a data object
 * 
 * @param data The data object
 * @param buffer The data as a character array. Freeing this buffer will be handled by the data object.
 * @param size The size of the data contained in the buffer
 * 
 */
void osync_data_set_data(OSyncData *data, char *buffer, unsigned int size)
{
	osync_assert(data);
	if (data->data) {
		osync_objformat_destroy(data->objformat, data->data, data->size);
	}
	data->data = buffer;
	data->size = size;
}

/*! @brief Check if the data object has data stored
 * 
 * @param data The data object
 * @returns TRUE if the data object has data, FALSE otherwise
 * 
 */
osync_bool osync_data_has_data(OSyncData *data)
{
	osync_assert(data);
	return data->data ? TRUE : FALSE;
}

/*! @brief Clone a data object
 * 
 * @param source The data object to clone
 * @param error An error struct
 * @returns a copy of the specified data object, or NULL if an error occurred
 * 
 */
OSyncData *osync_data_clone(OSyncData *source, OSyncError **error)
{
	OSyncData *data = NULL;
	char *buffer = NULL;
	unsigned int size = 0;
	
	osync_assert(source);
	
	data = osync_data_new(NULL, 0, source->objformat, error);
	if (!data)
		return NULL;
	
	data->objtype = g_strdup(source->objtype);
	
	if (source->data) {
		if (!osync_objformat_copy(source->objformat, source->data, source->size, &buffer, &size, error)) {
			osync_data_unref(data);
			return NULL;
		}
		
		osync_data_set_data(data, buffer, size);
	}
	
	return data;
}

/*! @brief Compares two data objects
 * 
 * Compares the two given data objects and returns:
 * CONV_DATA_MISMATCH if they are not the same
 * CONV_DATA_SIMILAR if the are not the same but look similar
 * CONV_DATA_SAME if they are exactly the same
 * 
 * @param leftdata The left data to compare
 * @param rightdata The right data to compare
 * @returns The result of the comparison
 * 
 */
OSyncConvCmpResult osync_data_compare(OSyncData *leftdata, OSyncData *rightdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftdata, rightdata);
	osync_assert(leftdata);
	osync_assert(rightdata);

	if (leftdata == rightdata) {
		osync_trace(TRACE_EXIT, "%s: SAME: OK. data is the same", __func__);
		return OSYNC_CONV_DATA_SAME;
	}
	
	if (leftdata->data == rightdata->data && leftdata->size == rightdata->size) {
		osync_trace(TRACE_EXIT, "%s: SAME: OK. data point to same memory", __func__);
		return OSYNC_CONV_DATA_SAME;
	}
	
	if (!leftdata->objformat || !rightdata->objformat || strcmp(osync_objformat_get_name(leftdata->objformat), osync_objformat_get_name(rightdata->objformat))) {
		osync_trace(TRACE_EXIT, "%s: MISMATCH: Objformats do not match", __func__);
		return OSYNC_CONV_DATA_MISMATCH;
	}
		
	if (!rightdata->data || !leftdata->data) {
		osync_trace(TRACE_EXIT, "%s: MISMATCH: One change has no data", __func__);
		return OSYNC_CONV_DATA_MISMATCH;
	}
	
	OSyncConvCmpResult ret = osync_objformat_compare(leftdata->objformat, leftdata->data, leftdata->size, rightdata->data, rightdata->size);
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

/*! @brief Returns a string describing a data object
 * 
 * Some formats cannot be printed directly. To be able to print these
 * objects they should specify a print function.
 * 
 * @param data The data to get printable
 * @returns A string describing the object
 * 
 */
char *osync_data_get_printable(OSyncData *data)
{
	osync_assert(data);
		
	OSyncObjFormat *format = data->objformat;
	osync_assert(format);
	
	return osync_objformat_print(format, data->data, data->size);
}

/*! @brief Returns the revision of the object
 * 
 * @param data The change to get the revision from
 * @param error An error struct
 * @returns The revision of the object in seconds since the epoch in zulu time
 * 
 */
time_t osync_data_get_revision(OSyncData *data, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, error);
	osync_assert(data);
	
	OSyncObjFormat *format = data->objformat;
	osync_assert(format);
	
	time_t time = osync_objformat_get_revision(format, data->data, data->size, error);
	if (time == -1) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
		return -1;
	}
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, time);
	return time;
}

/*@}*/
