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

#ifndef _OPENSYNC_FORMAT_ENV_H_
#define _OPENSYNC_FORMAT_ENV_H_

#include <opensync/opensync_list.h>

OSYNC_EXPORT OSyncFormatEnv *osync_format_env_new(OSyncError **error);
OSYNC_EXPORT void osync_format_env_free(OSyncFormatEnv *env);
OSYNC_EXPORT osync_bool osync_format_env_load_plugins(OSyncFormatEnv *env, const char *path, OSyncError **error);

OSYNC_EXPORT void osync_format_env_register_objformat(OSyncFormatEnv *env, OSyncObjFormat *format);
OSYNC_EXPORT OSyncObjFormat *osync_format_env_find_objformat(OSyncFormatEnv *env, const char *name);
OSYNC_EXPORT int osync_format_env_num_objformats(OSyncFormatEnv *env);
OSYNC_EXPORT OSyncObjFormat *osync_format_env_nth_objformat(OSyncFormatEnv *env, int nth);

OSYNC_EXPORT void osync_format_env_register_converter(OSyncFormatEnv *env, OSyncFormatConverter *converter);
OSYNC_EXPORT OSyncFormatConverter *osync_format_env_find_converter(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat);
OSYNC_EXPORT OSyncList *osync_format_env_find_converters(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat);
OSYNC_EXPORT int osync_format_env_num_converters(OSyncFormatEnv *env);
OSYNC_EXPORT OSyncFormatConverter *osync_format_env_nth_converter(OSyncFormatEnv *env, int nth);

OSYNC_EXPORT void osync_format_env_register_filter(OSyncFormatEnv *env, OSyncCustomFilter *filter);
OSYNC_EXPORT int osync_format_env_num_filters(OSyncFormatEnv *env);
OSYNC_EXPORT OSyncCustomFilter *osync_format_env_nth_filter(OSyncFormatEnv *env, int nth);

OSYNC_EXPORT OSyncObjFormat *osync_format_env_detect_objformat(OSyncFormatEnv *env, OSyncData *data);
OSYNC_EXPORT OSyncObjFormat *osync_format_env_detect_objformat_full(OSyncFormatEnv *env, OSyncData *input, OSyncError **error);

OSYNC_EXPORT osync_bool osync_format_env_convert(OSyncFormatEnv *env, OSyncFormatConverterPath *path, OSyncData *data, OSyncError **error);

OSYNC_EXPORT OSyncFormatConverterPath *osync_format_env_find_path(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncError **error);
OSYNC_EXPORT OSyncFormatConverterPath *osync_format_env_find_path_formats(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncList *targets, OSyncError **error);

OSYNC_EXPORT OSyncFormatConverterPath *osync_format_env_find_path_with_detectors(OSyncFormatEnv *env, OSyncData *sourcedata, OSyncObjFormat *targetformat, const char *preferred_format, OSyncError **error);
OSYNC_EXPORT OSyncFormatConverterPath *osync_format_env_find_path_formats_with_detectors(OSyncFormatEnv *env, OSyncData *sourcedata, OSyncList *targets, const char *preferred_format, OSyncError **error);


#endif /* _OPENSYNC_FORMAT_ENV_H_ */
