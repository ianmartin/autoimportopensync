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

static OSyncConvCmpResult compare_file(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	
	osync_bool data_same = FALSE;
	osync_bool path_same = FALSE;
	
	if (!strcmp(osync_change_get_uid(leftchange), osync_change_get_uid(rightchange)))
		path_same = TRUE;
	
	if (osync_change_get_datasize(leftchange) == osync_change_get_datasize(rightchange)) {
		if (osync_change_get_data(leftchange) == osync_change_get_data(rightchange)) {
			data_same = TRUE;
		} else {
			if (!memcmp(osync_change_get_data(leftchange), osync_change_get_data(rightchange), osync_change_get_datasize(leftchange)))
				data_same = TRUE;
		}
	}
	
	if (data_same && path_same) {
		osync_trace(TRACE_EXIT, "%s: Same", __func__);
		return CONV_DATA_SAME;
	}
	if (path_same) {
		osync_trace(TRACE_EXIT, "%s: Similar", __func__);
		return CONV_DATA_SIMILAR;
	}
	osync_trace(TRACE_EXIT, "%s: Mismatch", __func__);
	return CONV_DATA_MISMATCH;
}

static osync_bool conv_file_to_plain(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	
	*free_input = FALSE;
	*output = input;
	*outpsize = inpsize;
	return TRUE;
}

static osync_bool conv_plain_to_file(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	
	*free_input = FALSE;
	*output = input;
	*outpsize = inpsize;
	return TRUE;
}

static void destroy_file(char *input, size_t inpsize)
{
	g_free(input);
}

static void duplicate_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	char *newuid = g_strdup_printf ("%s-dupe", osync_change_get_uid(change));
	osync_change_set_uid(change, newuid);
	g_free(newuid);
}

static osync_bool copy_file(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);

	char *new = NULL;
	
	if (inpsize) {
		new = g_malloc0(inpsize);
		memcpy(new, input, inpsize);
	}
	
	*output = new;
	*outpsize = inpsize;
	return TRUE;
}

static void create_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);

	char *data = osync_rand_str(g_random_int_range(1, 100));
	osync_change_set_data(change, data, strlen(data) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

static char *print_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);

	char *printable = g_strdup_printf ("File: %s\nSize: %i", osync_change_get_uid(change), osync_change_get_datasize(change));
	return printable;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "data");
	osync_env_register_objformat(env, "data", "mockformat");
	osync_env_format_set_compare_func(env, "mockformat", compare_file);
	osync_env_format_set_duplicate_func(env, "mockformat", duplicate_file);
	osync_env_format_set_destroy_func(env, "mockformat", destroy_file);
	osync_env_format_set_print_func(env, "mockformat", print_file);
	osync_env_format_set_copy_func(env, "mockformat", copy_file);
	osync_env_format_set_create_func(env, "mockformat", create_file);
	
	osync_env_format_set_create_func(env, "mockformat", create_file);
	osync_env_register_converter(env, CONVERTER_DECAP, "mockformat", "plain", conv_file_to_plain);
	osync_env_register_converter(env, CONVERTER_ENCAP, "plain", "mockformat", conv_plain_to_file);
}
