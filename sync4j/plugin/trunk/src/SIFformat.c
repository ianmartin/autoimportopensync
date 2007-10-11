/*
 * Copyright (C) 2007 Michael Unterkalmsteiner, <michael.unterkalmsteiner@stud-inf.unibz.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SIFformat.h"
#include "SIFNformat.h"
#include "SIFTformat.h"

osync_bool get_format_info(OSyncFormatEnv* env, OSyncError** error) {
	return get_specific_format_info(env, error, SIFN, OBJ_TYPE_NOTE) &&
			get_specific_format_info(env, error, SIFT, OBJ_TYPE_TODO);
}

osync_bool get_conversion_info(OSyncFormatEnv* env, OSyncError** error) {
	return get_sifn_conversion_info(env, error) &&
			get_sift_conversion_info(env, error);
}

char* print_sif(const char* data, unsigned int size) {
	return data;
}

void destroy_sif(char* input, size_t inpsize) {
	osync_trace(TRACE_ENTRY, "%s(%p,%d)", __func__, input, inpsize);

	g_free(input);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

int get_version(void) {
	return 1;
}

static osync_bool get_specific_format_info(OSyncFormatEnv* env, OSyncError** error,
	const char* formatname, const char* objtype) {

	OSyncObjFormat* format = osync_objformat_new(formatname, objtype, error);
	if (!format)
		return FALSE;

	osync_objformat_set_destroy_func(format, destroy_sif);
	osync_objformat_set_print_func(format, print_sif);

	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}









