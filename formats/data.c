/*
 * data - A plugin for data objects for the opensync framework
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
#include <string.h>
#include <stdlib.h>

/** @defgroup data_plain data/plain format
 *
 * Definition: pointer to a malloc()ed block of data, or a NULL
 * pointer.
 */

/** data/plain comparison function
 *
 * The comparison function is a memcpy() on the data.
 *
 * @ingroup data_plain
 */
static OSyncConvCmpResult compare_plain(OSyncChange *a, OSyncChange *b)
{
	const char *d1 = osync_change_get_data(a);
	const char *d2 = osync_change_get_data(b);
	size_t s1 = osync_change_get_datasize(a);
	size_t s2 = osync_change_get_datasize(b);

	/* Consider empty block equal NULL pointers */
	if (!s1) d1 = NULL;
	if (!s2) d2 = NULL;

	if (d1 && d2) {
		int r = memcmp(d1, d2, s1 < s2 ? s1 : s2);
		if (!r && s1 == s2)
			return CONV_DATA_SAME;
		else
			return CONV_DATA_MISMATCH;
	} else if (!d1 && !d2)
		return CONV_DATA_SAME;
	else
		return CONV_DATA_MISMATCH;
}

static osync_bool copy_plain(const char *input, int inpsize, char **output, int *outpsize)
{
	char *r = malloc(inpsize);
	if (!r)
		return FALSE;
	memcpy(r, input, inpsize);
	*output = r;
	*outpsize = inpsize;
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "data");
	osync_env_register_objformat(env, "data", "plain");
	osync_env_format_set_compare_func(env, "plain", compare_plain);
	osync_env_format_set_copy_func(env, "plain", copy_plain);
}
