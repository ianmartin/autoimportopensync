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


/** Function used on a path search for a format name array
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtnames()
 */
static osync_bool target_fn_fmtnames(const void *data, OSyncObjFormat *fmt)
{
	const char * const *list = data;
	const char * const *i;
	for (i = list; *i; i++) {
		if (!strcmp(fmt->name, *i))
			/* Found */
			return TRUE;
	}

	/* Not found */
	return FALSE;
}

/** Function used on path searchs for a single format
 *
 * @see osync_conv_find_path_fn(), osync_change_convert()
 */
static osync_bool target_fn_simple(const void *data, OSyncObjFormat *fmt)
{
	const OSyncObjFormat *target = data;
	return target == fmt;
}

/** Function used on path searchs for a single format name
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtname()
 */
static osync_bool target_fn_fmtname(const void *data, OSyncObjFormat *fmt)
{
	const char *name = data;
	return !strcmp(name, fmt->name);
}

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
		
	OSyncObjFormat *format = change->format;
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
	
	OSyncObjFormat *format = change->format;
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
		osync_error_free(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare_data: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}
	if (!osync_change_convert_to_common(rightchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare_data: %s", osync_error_print(&error));
		osync_error_free(&error);
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
		OSyncObjFormat *format = leftchange->format;
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
		osync_error_free(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}
	if (!osync_change_convert_to_common(rightchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare: %s", osync_error_print(&error));
		osync_error_free(&error);
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
 * @returns TRUE if the copy was successfull
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
	newchange->is_detected = source->is_detected;
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

/*! @brief Duplicates the uid of the change
 * 
 * This will call the duplicate function of a format.
 * This is used if a uid is not unique.
 * 
 * @param source The change to duplicate
 * @returns TRUE if the uid was duplicated successfull
 * 
 */
osync_bool osync_change_duplicate(OSyncChange *change)
{
	g_assert(change);
	OSyncObjFormat *format = change->format;
	osync_debug("OSCONV", 3, "Duplicating change %s with format %s\n", change->uid, format->name);
	if (!format || !format->duplicate_func)
		return FALSE;
	format->duplicate_func(change);
	return TRUE;
}

/*! @brief Convert a change to a specific format with a specific extension
 * 
 * This will convert the change with its data to the specified format
 * if possible. And this will also convert the data to the specified format
 * extension.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetformat To which format you want to convert to
 * @param extension_name The name of the extension
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_extension(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat, const char *extension_name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_convert(%p, %p, %p:%s, %s, %p)", env, change, targetformat, targetformat ? targetformat->name : "NONE", extension_name, error);
	if (osync_conv_convert_fn(env, change, target_fn_simple, targetformat, extension_name, error)) {
		osync_trace(TRACE_EXIT, "osync_change_convert: TRUE");
		return TRUE;
	} else {
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert: %s", osync_error_print(error));
		return FALSE;
	}
}

/*! @brief Convert a change to a specific format
 * 
 * This will convert the change with its data to the specified format
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetformat To which format you want to convert to
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat, OSyncError **error)
{
	return osync_change_convert_extension(env, change, targetformat, NULL, error);
}

/*! @brief Convert a change to the specified common format
 * 
 * @param change The change to convert
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_to_common(OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_convert_to_common(%p, %p)", change, error);
	
	if (!osync_change_get_objtype(change)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no objtype");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert_to_common: %s", osync_error_print(error));
		return FALSE;
	}
	OSyncFormatEnv *env = osync_change_get_objtype(change)->env;
	
	if (!osync_change_get_objtype(change)->common_format) {
		osync_trace(TRACE_EXIT, "osync_change_convert_to_common: No common format set");
		return TRUE;
	}
	
	osync_trace(TRACE_INTERNAL, "Converting from %s to %s", osync_change_get_objformat(change)->name, osync_change_get_objtype(change)->common_format->name);
	
	if (osync_change_convert(env, change, osync_change_get_objtype(change)->common_format, error)) {
		osync_trace(TRACE_EXIT, "osync_change_convert_to_common: TRUE");
		return TRUE;
	} else {
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert_to_common: %s", osync_error_print(error));
		return FALSE;
	}
}

/*! @brief Convert a change to a specific format with the given name
 * 
 * This will convert the change with its data to the specified format
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetname To which format you want to convert to
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *targetname, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_fmtname, targetname, NULL, error);
}

/*! @brief Convert a change to some formats
 * 
 * This will convert the change with its data to one of the specified formats
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetnames NULL-Terminated array of formatnames
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **targetnames, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_fmtnames, targetnames, NULL, error);
}

/*! @brief Tries to detect the object type of the given change
 * 
 * This will try to detect the object type of the data on the change
 * and return it, but not set it.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The objecttype on success, NULL otherwise
 * 
 */
OSyncObjType *osync_change_detect_objtype(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	OSyncObjFormat *format = NULL;
	if (!(format = osync_change_detect_objformat(env, change, error)))
		return NULL;
	return format->objtype;
}

/*! @brief Tries to detect the encapsulated object type of the given change
 * 
 * This will try to detect the encapsulated object type of the data on the change
 * and return it, but not set it. This will try to detect the change, deencapsulate
 * it, detect again etc until it cannot deencapsulate further.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The objecttype on success, NULL otherwise
 * 
 */
OSyncObjType *osync_change_detect_objtype_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	OSyncObjFormat *format = NULL;
	if (!(format = osync_change_detect_objformat_full(env, change, error)))
		return NULL;
	return format->objtype;
}

/*! @brief Tries to detect the format of the given change
 * 
 * This will try to detect the format of the data on the change
 * and return it, but not set it.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_change_detect_objformat(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_detect_objformat(%p, %p, %p)", env, change, error);
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no data");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
		return NULL;
	}
	
	//Run all datadetectors for our source type
	GList *d = NULL;
	for (d = env->converters; d; d = d->next) {
		OSyncFormatConverter *converter = d->data;
		if (!strcmp(converter->source_format->name, osync_change_get_objformat(change)->name)) {
			if (converter->detect_func && converter->detect_func(env, change->data, change->size)) {
				osync_trace(TRACE_EXIT, "osync_change_detect_objformat: %p:%s", converter->target_format, converter->target_format->name);
				return converter->target_format;
			}
		}
	}
	
	osync_error_set(error, OSYNC_ERROR_GENERIC, "None of the detectors was able to recognize this data");
	osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
	return NULL;
}

/*! @brief Tries to detect the encapsulated format of the given change
 * 
 * This will try to detect the encapsulated format of the data on the change
 * and return it, but not set it. This will try to detect the change, deencapsulate
 * it, detect again etc until it cannot deencapsulate further.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_change_detect_objformat_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_detect_objformat_full(%p, %p, %p)", env, change, error);
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no data");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
		return NULL;
	}
	OSyncChange *new_change = change;
	
	//Try to decap the change as far as possible
	while (1) {
		GList *d = NULL;
		for (d = env->converters; d; d = d->next) {
			OSyncFormatConverter *converter = d->data;
			if (!strcmp(converter->source_format->name, osync_change_get_objformat(change)->name) && converter->type == CONVERTER_DECAP) {
				osync_bool free_output = FALSE;
				if (!(new_change = osync_converter_invoke_decap(converter, new_change, &free_output))) {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to decap the change");
					osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat_full: %s", osync_error_print(error));
					return NULL;
				}
				continue;
			}
		}
		break;
	}

	OSyncObjFormat *ret = osync_change_detect_objformat(env, new_change, error);
	if (!ret)
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat_full: %s", osync_error_print(error));
	else
		osync_trace(TRACE_EXIT, "osync_change_detect_objformat_full: %p:%s", ret, ret->name);
	return ret;
}

/*@{*/
