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

#ifndef _OPENSYNC_CONVERTER_H_
#define _OPENSYNC_CONVERTER_H_

typedef enum {
	/** Simple converter */
	OSYNC_CONVERTER_CONV = 1,
	/** Encapsulator */
	OSYNC_CONVERTER_ENCAP = 2,
	/** Desencapsulator */
	OSYNC_CONVERTER_DECAP = 3,
	/** Detector */
	OSYNC_CONVERTER_DETECTOR = 4
} OSyncConverterType;

typedef osync_bool (* OSyncFormatDetectFunc) (const char *data, int size, void *userdata);
typedef osync_bool (* OSyncFormatConvertFunc) (char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error);
typedef void * (* OSyncFormatConverterInitalizeFunc) (OSyncError **error);
typedef void (* OSyncFormatConverterFinalizeFunc) (void *userdata);

OSYNC_EXPORT OSyncFormatConverter *osync_converter_new(OSyncConverterType type, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncFormatConvertFunc convert_func, OSyncError **error);
OSYNC_EXPORT OSyncFormatConverter *osync_converter_new_detector(OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncFormatDetectFunc detect_func, OSyncError **error);
OSYNC_EXPORT OSyncFormatConverter *osync_converter_ref(OSyncFormatConverter *converter);
OSYNC_EXPORT void osync_converter_unref(OSyncFormatConverter *converter);

OSYNC_EXPORT OSyncObjFormat *osync_converter_get_sourceformat(OSyncFormatConverter *converter);
OSYNC_EXPORT OSyncObjFormat *osync_converter_get_targetformat(OSyncFormatConverter *converter);
OSYNC_EXPORT OSyncConverterType osync_converter_get_type(OSyncFormatConverter *converter);

OSYNC_EXPORT OSyncObjFormat *osync_converter_detect(OSyncFormatConverter *converter, OSyncData *data);
OSYNC_EXPORT osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncData *data, const char *config, OSyncError **error);
OSYNC_EXPORT osync_bool osync_converter_matches(OSyncFormatConverter *converter, OSyncData *data);

OSYNC_EXPORT OSyncFormatConverterPath *osync_converter_path_new(OSyncError **error);
OSYNC_EXPORT OSyncFormatConverterPath *osync_converter_path_ref(OSyncFormatConverterPath *path);
OSYNC_EXPORT void osync_converter_path_unref(OSyncFormatConverterPath *path);

OSYNC_EXPORT void osync_converter_path_add_edge(OSyncFormatConverterPath *path, OSyncFormatConverter *edge);
OSYNC_EXPORT unsigned int osync_converter_path_num_edges(OSyncFormatConverterPath *path);
OSYNC_EXPORT OSyncFormatConverter *osync_converter_path_nth_edge(OSyncFormatConverterPath *path, unsigned int nth);
OSYNC_EXPORT const char *osync_converter_path_get_config(OSyncFormatConverterPath *path);
OSYNC_EXPORT void osync_converter_path_set_config(OSyncFormatConverterPath *path, const char *config);

OSYNC_EXPORT void osync_converter_set_initalize_func(OSyncFormatConverter *converter, OSyncFormatConverterInitalizeFunc initalize_func);
OSYNC_EXPORT void osync_converter_set_finalize_func(OSyncFormatConverter *converter, OSyncFormatConverterFinalizeFunc finalize_func);
OSYNC_EXPORT void osync_converter_initalize(OSyncFormatConverter *converter, OSyncError **error);
OSYNC_EXPORT void osync_converter_finalize(OSyncFormatConverter *converter);

#endif //_OPENSYNC_CONVERTER_H_
