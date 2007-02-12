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

#include "opensync-data.h"
#include "opensync-format.h"
#include "opensync_converter_internals.h"

OSyncFormatConverter *osync_converter_new(ConverterType type, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncFormatConvertFunc convert_func, OSyncError **error)
{
	OSyncFormatConverter *converter = NULL;
	osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %p, %p)", __func__, type, sourceformat, targetformat, convert_func, error);
	
	converter = osync_try_malloc0(sizeof(OSyncFormatConverter), error);
	if (!converter) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	converter->source_format = sourceformat;
	osync_objformat_ref(sourceformat);
	
	converter->target_format = targetformat;
	osync_objformat_ref(targetformat);
	
	converter->convert_func = convert_func;
	converter->type = type;
	converter->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, converter);
	return converter;
}

OSyncFormatConverter *osync_converter_new_detector(OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncFormatDetectFunc detect_func, OSyncError **error)
{
	OSyncFormatConverter *converter = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sourceformat, targetformat, detect_func, error);

	converter = osync_try_malloc0(sizeof(OSyncFormatConverter), error);
	if (!converter) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	converter->source_format = sourceformat;
	osync_objformat_ref(sourceformat);
	
	converter->target_format = targetformat;
	osync_objformat_ref(targetformat);
	
	converter->detect_func = detect_func;
	converter->type = OSYNC_CONVERTER_DETECTOR;
	converter->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, converter);
	return converter;
}

void osync_converter_ref(OSyncFormatConverter *converter)
{
	osync_assert(converter);
	
	g_atomic_int_inc(&(converter->ref_count));
}

void osync_converter_unref(OSyncFormatConverter *converter)
{
	osync_assert(converter);
	
	if (g_atomic_int_dec_and_test(&(converter->ref_count))) {
		if (converter->source_format)
			osync_objformat_unref(converter->source_format);
			
		if (converter->target_format)
			osync_objformat_unref(converter->target_format);
		
		g_free(converter);
	}
}

OSyncObjFormat *osync_converter_get_sourceformat(OSyncFormatConverter *converter)
{
	osync_assert(converter);
	return converter->source_format;
}

OSyncObjFormat *osync_converter_get_targetformat(OSyncFormatConverter *converter)
{
	osync_assert(converter);
	return converter->target_format;
}

ConverterType osync_converter_get_type(OSyncFormatConverter *converter)
{
	osync_assert(converter);
	return converter->type;
}

OSyncObjFormat *osync_converter_detect(OSyncFormatConverter *converter, OSyncData *data)
{
	OSyncObjFormat *sourceformat = NULL;
	char *buffer = NULL;
	unsigned int size = 0;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, converter, data);
	osync_assert(converter);
	osync_assert(data);
	
	if (converter->type != OSYNC_CONVERTER_DETECTOR) {
		osync_trace(TRACE_EXIT, "%s: Not a detector", __func__);
		return NULL;
	}
	
	sourceformat = osync_data_get_objformat(data);
	
	/* First, we check if this is a "inverse" detection. So we check if our targetformat
	 * is the sourceformat of the data. There must always be a converter capable of converting
	 * this case */
	if (osync_objformat_is_equal(converter->target_format, sourceformat)) {
		osync_trace(TRACE_EXIT, "%s: %p", __func__, converter->source_format);
		return converter->source_format;
	}
	
	if (!osync_objformat_is_equal(converter->source_format, sourceformat)) {
		osync_trace(TRACE_EXIT, "%s: Format does not match", __func__);
		return NULL;
	}
	
	osync_data_get_data(data, &buffer, &size);
	if (!converter->detect_func || converter->detect_func(buffer, size)) {
		/* Successfully detected the data */
		osync_trace(TRACE_EXIT, "%s: %p", __func__, converter->target_format);
		return converter->target_format;
	}
	
	osync_trace(TRACE_EXIT, "%s: Not detected", __func__);
	return NULL;
}

/** Function does not replace the objtype */
osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncData *data, const char *config, OSyncError **error)
{
	char *input_data = NULL;
	unsigned int input_size = 0;
	char *output_data = NULL;
	unsigned int output_size = 0;
	osync_bool free_input = FALSE;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, converter, data, config, error);
	osync_trace(TRACE_INTERNAL, "Converter of type %i, from %p(%s) to %p(%s)", converter->type, converter->source_format, osync_objformat_get_name(converter->source_format), converter->target_format, osync_objformat_get_name(converter->target_format));
	
	if (converter->type != OSYNC_CONVERTER_DETECTOR) {
		
		osync_data_steal_data(data, &input_data, &input_size);
		if (input_data) {
			osync_assert(converter->convert_func);
		
			/* Invoke the converter */
			if (!converter->convert_func(input_data, input_size, &output_data, &output_size, &free_input, config, error))
				goto error;
			
			/* Good. We now have some new data. Now we have to see what to do with the old data */
			if (free_input) {
				osync_objformat_destroy(converter->source_format, input_data, input_size);
			}
			osync_data_set_data(data, output_data, output_size);
		}
	}
	
	osync_data_set_objformat(data, converter->target_format);
	osync_data_set_objtype(data, osync_objformat_get_objtype(converter->target_format));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_converter_matches(OSyncFormatConverter *converter, OSyncData *data)
{
	OSyncObjFormat *format = NULL;
	osync_assert(converter);
	osync_assert(data);
	
	format = osync_data_get_objformat(data);
	if (!strcmp(osync_objformat_get_name(converter->source_format), osync_objformat_get_name(format)))
		return TRUE;
	return FALSE;
}

OSyncFormatConverterPath *osync_converter_path_new(OSyncError **error)
{
	OSyncFormatConverterPath *path = osync_try_malloc0(sizeof(OSyncFormatConverterPath), error);
	if (!path)
		return NULL;
	
	path->ref_count = 1;
	
	return path;
}

void osync_converter_path_ref(OSyncFormatConverterPath *path)
{
	osync_assert(path);
	
	g_atomic_int_inc(&(path->ref_count));
}

void osync_converter_path_unref(OSyncFormatConverterPath *path)
{
	osync_assert(path);
	
	if (g_atomic_int_dec_and_test(&(path->ref_count))) {
		while (path->converters) {
			OSyncFormatConverter *converter = path->converters->data;
			osync_converter_unref(converter);
			path->converters = g_list_remove(path->converters, converter);
		}
		
		if (path->config)
			g_free(path->config);
		
		g_free(path);
	}
}

void osync_converter_path_add_edge(OSyncFormatConverterPath *path, OSyncFormatConverter *edge)
{
	osync_assert(path);
	path->converters = g_list_append(path->converters, edge);
	osync_converter_ref(edge);
}

int osync_converter_path_num_edges(OSyncFormatConverterPath *path)
{
	osync_assert(path);
	return g_list_length(path->converters);
}

OSyncFormatConverter *osync_converter_path_nth_edge(OSyncFormatConverterPath *path, unsigned int nth)
{
	osync_assert(path);
	return g_list_nth_data(path->converters, nth);
}

const char *osync_converter_path_get_config(OSyncFormatConverterPath *path)
{
	osync_assert(path);
	return path->config;
}

void osync_converter_path_set_config(OSyncFormatConverterPath *path, const char *config)
{
	osync_assert(path);
	if (path->config)
		g_free(path->config);
	path->config = g_strdup(config);
}

/*@}*/
