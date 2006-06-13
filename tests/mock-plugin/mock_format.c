/*
 * file - A plugin for file objects for the opensync framework
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
 
#include <opensync/opensync.h>
#include <glib.h>
#include "mock_sync.h"

static OSyncConvCmpResult compare_file(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %i)", __func__, leftdata, leftsize, rightdata, rightsize);
	
	if (rightsize == leftsize) {
		if (!memcmp(leftdata, rightdata, leftsize)) {
			osync_trace(TRACE_EXIT, "%s: Same", __func__);
			return OSYNC_CONV_DATA_SAME;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: Mismatch", __func__);
	return OSYNC_CONV_DATA_MISMATCH;
}

static osync_bool conv_file_to_plain(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	*free_input = FALSE;
	*output = input;
	*outpsize = inpsize;
	return TRUE;
}

static osync_bool conv_plain_to_file(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	*free_input = FALSE;
	*output = input;
	*outpsize = inpsize;
	return TRUE;
}

static void destroy_file(char *input, size_t inpsize)
{
	g_free(input);
}

static void duplicate_file(const char *uid, char **newuid)
{
	*newuid = g_strdup_printf ("%s-dupe", uid);
}

static osync_bool copy_file(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
	char *new = NULL;
	
	if (inpsize) {
		new = g_malloc0(inpsize);
		memcpy(new, input, inpsize);
	}
	
	*output = new;
	*outpsize = inpsize;
	return TRUE;
}

static void create_file(char **buffer, unsigned int *size)
{
	*buffer = osync_rand_str(g_random_int_range(1, 100));
	*size = strlen(*buffer) + 1;
}

static char *print_file(const char *data, unsigned int size)
{
	char *printable = g_strdup_printf ("Filesize: %i", size);
	return printable;
}

void get_format_info(OSyncFormatEnv *env)
{
	OSyncError *error = NULL;
	
	OSyncObjFormat *format = osync_objformat_new("mockformat1", "data", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format mockformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_objformat_set_compare_func(format, compare_file);
	osync_objformat_set_destroy_func(format, destroy_file);
	osync_objformat_set_duplicate_func(format, duplicate_file);
	osync_objformat_set_print_func(format, print_file);
	osync_objformat_set_copy_func(format, copy_file);
	osync_objformat_set_create_func(format, create_file);
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	format = osync_objformat_new("mockformat2", "data", &error);
	if (!format) {
		osync_trace(TRACE_ERROR, "Unable to register format mockformat: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
}

void get_conversion_info(OSyncFormatEnv *env)
{
	
	OSyncError *error = NULL;
	
	OSyncObjFormat *mockformat1 = osync_format_env_find_objformat(env, "mockformat1");
	OSyncObjFormat *mockformat2 = osync_format_env_find_objformat(env, "mockformat2");
	
	OSyncFormatConverter *conv = osync_converter_new(OSYNC_CONVERTER_DECAP, mockformat1, mockformat2, conv_file_to_plain, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_ENCAP, mockformat2, mockformat1, conv_plain_to_file, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}
