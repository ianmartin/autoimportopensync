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
#include "file_sync.h"

static OSyncConvCmpResult compare_file(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *leftfile = (fs_fileinfo *)osync_change_get_data(leftchange);
	fs_fileinfo *rightfile = (fs_fileinfo *)osync_change_get_data(rightchange);
	
	osync_bool data_same = FALSE;
	osync_bool path_same = FALSE;
	
	if (!strcmp(osync_change_get_uid(leftchange), osync_change_get_uid(rightchange)))
		path_same = TRUE;
	
	if (leftfile->size == rightfile->size) {
		if (leftfile->data == rightfile->data) {
			data_same = TRUE;
		} else {
			if (!memcmp(leftfile->data, rightfile->data, leftfile->size))
				data_same = TRUE;
		}
	}
	
	if (data_same && path_same)
		return CONV_DATA_SAME;
	if (path_same)
		return CONV_DATA_SIMILAR;
	
	return CONV_DATA_MISMATCH;
}

#ifdef STRESS_TEST
static void create_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file_info = g_malloc0(sizeof(fs_fileinfo));
	int file_size = g_random_int_range(0, 1000);
	osync_change_set_data(change, (char *)file_info, sizeof(fs_fileinfo), TRUE);
	
	file_info->data = g_malloc0(file_size * 105 * sizeof(char));
	file_info->size = file_size * 100 * sizeof(char);
	
	char *datap  = file_info->data;
	FILE *fd = fopen("/dev/urandom", "r");
	if (fd) {
		for (; file_size > 5; file_size--) {
			fread(datap, 100, 1, fd);
			datap += 100 * sizeof(char);
		}
	}
	fclose(fd);
	osync_change_set_uid(change, osync_rand_str(6));
}
#endif

static osync_bool conv_file_to_plain(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file = (fs_fileinfo *)input;
	
	*free_input = FALSE;
	*output = file->data;
	*outpsize = file->size;
	return TRUE;
}

static osync_bool conv_plain_to_file(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file = g_malloc0(sizeof(fs_fileinfo));

	file->data = input;
	file->size = inpsize;
	
	*free_input = FALSE;
	*output = (char *)file;
	*outpsize = sizeof(file);
	return TRUE;
}

static void destroy_file(char *input, size_t inpsize)
{
	fs_fileinfo *file = (fs_fileinfo *)input;
	if (inpsize != sizeof(fs_fileinfo)) {
		osync_debug("FILE", 0, "destroy_file: Wrong data size: %d, but it should be %u", inpsize, sizeof(fs_fileinfo));
		return;
	}
	g_free(file->data);
	g_free(file);
}

static void duplicate_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	char *newuid = g_strdup_printf ("%s-dupe", osync_change_get_uid(change));
	osync_change_set_uid(change, newuid);
	g_free(newuid);
}

static char *print_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file = (fs_fileinfo *)osync_change_get_data(change);

	char *printable = g_strdup_printf ("File: %s\nSize: %i", osync_change_get_uid(change), file->size);
	return printable;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "data");
	osync_env_register_objformat(env, "data", "file");
	osync_env_format_set_compare_func(env, "file", compare_file);
	osync_env_format_set_duplicate_func(env, "file", duplicate_file);
	osync_env_format_set_destroy_func(env, "file", destroy_file);
	osync_env_format_set_print_func(env, "file", print_file);
	
#ifdef STRESS_TEST
	osync_env_format_set_create_func(env, "file", create_file);
#endif
	osync_env_register_converter(env, CONVERTER_DECAP, "file", "plain", conv_file_to_plain);
	osync_env_register_converter(env, CONVERTER_ENCAP, "plain", "file", conv_plain_to_file);
}
