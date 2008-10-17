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

#ifndef OPENSYNC_CONVERTER_INTERNALS_H_
#define OPENSYNC_CONVERTER_INTERNALS_H_

/*! @brief Represent a converter from one format to another
 */
struct OSyncFormatConverter {
	OSyncObjFormat *source_format;
	OSyncObjFormat *target_format;
	OSyncFormatConvertFunc convert_func;
	OSyncFormatDetectFunc detect_func;
	OSyncFormatConverterInitalizeFunc initalize_func;
	OSyncFormatConverterFinalizeFunc finalize_func;
	OSyncConverterType type;
	int ref_count;
	void *userdata;
};

struct OSyncFormatConverterPath {
	GList *converters;
	char *config;
	int ref_count;
};

#endif /*OPENSYNC_CONVERTER_INTERNALS_H_*/
