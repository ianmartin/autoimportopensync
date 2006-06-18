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

void osync_data_ref(OSyncData *data)
{
	osync_assert(data);
	
	g_atomic_int_inc(&(data->ref_count));
}

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

OSyncObjFormat *osync_data_get_objformat(OSyncData *data)
{
	osync_assert(data);
	return data->objformat;
}

void osync_data_set_objformat(OSyncData *data, OSyncObjFormat *objformat)
{
	osync_assert(data);
	if (data->objformat)
		osync_objformat_unref(data->objformat);
	data->objformat = objformat;
	osync_objformat_ref(objformat);
}

const char *osync_data_get_objtype(OSyncData *data)
{
	osync_assert(data);
	return data->objtype;
}

void osync_data_set_objtype(OSyncData *data, const char *objtype)
{
	osync_assert(data);
	if (data->objtype)
		g_free(data->objtype);
	data->objtype = g_strdup(objtype);
}

void osync_data_get_data(OSyncData *data, char **buffer, unsigned int *size)
{
	osync_assert(data);
	if (buffer)
		*buffer = data->data;
	
	if (size)
		*size = data->size;
}

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

void osync_data_set_data(OSyncData *data, char *buffer, unsigned int size)
{
	osync_assert(data);
	if (data->data) {
		osync_objformat_destroy(data->objformat, data->data, data->size);
	}
	data->data = buffer;
	data->size = size;
}

osync_bool osync_data_has_data(OSyncData *data)
{
	osync_assert(data);
	return data->data ? TRUE : FALSE;
}

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


#if 0
/**
 * @defgroup OSyncChangeCmds OpenSync Change Commands
 * @ingroup OSyncPublic
 * @brief High level functions to manipulate changes
 */
/*@{*/

/*! @brief Returns a string describing a change object
 * 
 * Some formats cannot be printed directly. To be able to print these
 * objects they should specify a print function.
 * 
 * @param change The change to get printable
 * @returns A string describing the object
 * 
 */
char *osync_change_get_printable(OSyncChange *change)
{
	g_assert(change);
	if (!change->has_data)
		return NULL;
		
	OSyncObjFormat *format = osync_change_get_objformat(change);
	g_assert(format);
	
	if (!format->print_func)
		return g_strndup(change->data, change->size);
		
	return format->print_func(change);
}

/*! @brief Returns the revision of the object
 * 
 * @param change The change to get the revision from
 * @param error A error struct
 * @returns The revision of the object in seconds since the epoch in zulu time
 * 
 */
time_t osync_change_get_revision(OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, change, error);
	
	g_assert(change);
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No data set when asking for the timestamp");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}
	
	OSyncObjFormat *format = osync_change_get_objformat(change);
	g_assert(format);
	
	if (!format->revision_func) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No revision function set");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return -1;
	}
		
	time_t time = format->revision_func(change, error);
	
	osync_trace(osync_error_is_set(error) ? TRACE_EXIT_ERROR : TRACE_EXIT, "%s: %s, %i", __func__, osync_error_print(error), time);
	return time;
}

/*! @brief Compares the data of 2 changes
 * 
 * Compares the two given changes and returns:
 * CONV_DATA_MISMATCH if they are not the same
 * CONV_DATA_SIMILAR if the are not the same but look similar
 * CONV_DATA_SAME if they are exactly the same
 * 
 * @param leftchange The left change to compare
 * @param rightchange The right change to compare
 * @returns The result of the comparison
 * 
 */
OSyncConvCmpResult osync_change_compare_data(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "osync_change_compare_data(%p, %p)", leftchange, rightchange);
	
	g_assert(rightchange);
	g_assert(leftchange);

	OSyncError *error = NULL;
	if (!osync_change_convert_to_common(leftchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare_data: %s", osync_error_print(&error));
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}
	if (!osync_change_convert_to_common(rightchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare_data: %s", osync_error_print(&error));
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}

	if (!(rightchange->data == leftchange->data)) {
		if (!(osync_change_get_objtype(leftchange) == osync_change_get_objtype(rightchange))) {
			osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: Objtypes do not match");
			return CONV_DATA_MISMATCH;
		}
		if (leftchange->format != rightchange->format) {
			osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: Objformats do not match");
			return CONV_DATA_MISMATCH;
		}
		if (!rightchange->data || !leftchange->data) {
			osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: One change has no data");
			return CONV_DATA_MISMATCH;
		}
		OSyncObjFormat *format = osync_change_get_objformat(leftchange);
		g_assert(format);
		
		OSyncConvCmpResult ret = format->cmp_func(leftchange, rightchange);
		osync_trace(TRACE_EXIT, "osync_change_compare_data: %i", ret);
		return ret;
	} else {
		osync_trace(TRACE_EXIT, "osync_change_compare_data: SAME: OK. data point to same memory");
		return CONV_DATA_SAME;
	}
}

/*! @brief Compares 2 changes
 * 
 * Compares the two given changes and returns:
 * CONV_DATA_MISMATCH if they are not the same
 * CONV_DATA_SIMILAR if the are not the same but look similar
 * CONV_DATA_SAME if they are exactly the same
 * This function does also compare changetypes etc unlike
 * osync_change_compare_data()
 * 
 * @param leftchange The left change to compare
 * @param rightchange The right change to compare
 * @returns The result of the comparison
 * 
 */
OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "osync_change_compare(%p, %p)", leftchange, rightchange);
	
	g_assert(rightchange);
	g_assert(leftchange);

	OSyncError *error = NULL;
	if (!osync_change_convert_to_common(leftchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare: %s", osync_error_print(&error));
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}
	if (!osync_change_convert_to_common(rightchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare: %s", osync_error_print(&error));
		osync_error_unref(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}

	if (rightchange->changetype == leftchange->changetype) {
		OSyncConvCmpResult ret = osync_change_compare_data(leftchange, rightchange);
		osync_trace(TRACE_EXIT, "osync_change_compare: Compare data: %i", ret);
		return ret;
	} else {
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Change types do not match");
		return CONV_DATA_MISMATCH;
	}
}

/*! @brief Copies the data from one change to another change
 * 
 * @param source The change to copy from
 * @param target The change to copy to
 * @param error A error struct
 * @returns TRUE if the copy was successful
 * 
 */
osync_bool osync_change_copy_data(OSyncChange *source, OSyncChange *target, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_copy_data(%p, %p, %p)", source, target, error);
	
	OSyncObjFormat *format = NULL;
	format = source->format;
	if (!format)
		format = target->format;
	
	if (target->data)
		osync_change_free_data(target);
	
	if (!source->data) {
		target->data = NULL;
		target->size = 0;
		osync_trace(TRACE_EXIT, "%s: Source had not data", __func__);
		return TRUE;
	}
	
	if (!format || !format->copy_func) {
		osync_trace(TRACE_INTERNAL, "We cannot copy the change, falling back to memcpy");
		target->data = g_malloc0(sizeof(char) * source->size);
		memcpy(target->data, source->data, source->size);
		target->size = source->size;
	} else {
		if (!format->copy_func(source->data, source->size, &(target->data), &(target->size))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Something went wrong during copying");
			osync_trace(TRACE_EXIT_ERROR, "osync_change_copy_data: %s", osync_error_print(error));
			return FALSE;
		}
	}
	
	osync_trace(TRACE_EXIT, "osync_change_copy_data");
	return TRUE;
}

/*! @brief Makes a exact copy of change
 * 
 * @param source The change to copy from
 * @param error A error struct
 * @returns A new change that is the copy, NULL on error
 * 
 */
OSyncChange *osync_change_copy(OSyncChange *source, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_copy(%p, %p)", source, error);
	g_assert(source);
	
	OSyncChange *newchange = osync_change_new();
	newchange->uid = g_strdup(source->uid);
	newchange->hash = g_strdup(source->hash);
	
	newchange->has_data = source->has_data;
	newchange->changetype = source->changetype;
	newchange->format = osync_change_get_objformat(source);
	newchange->objtype = osync_change_get_objtype(source);
	newchange->sourceobjtype = g_strdup(osync_change_get_objtype(source)->name);
	newchange->changes_db = source->changes_db;
	newchange->member = source->member;
	
	if (!osync_change_copy_data(source, newchange, error)) {
		osync_change_free(newchange);
		osync_trace(TRACE_EXIT_ERROR, "osync_change_copy: %s", osync_error_print(error));
		return NULL;
	}

	osync_trace(TRACE_EXIT, "osync_change_copy: %p", newchange);
	return newchange;
}
#endif
/*@}*/
