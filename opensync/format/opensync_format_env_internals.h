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

#ifndef _OPENSYNC_FORMAT_ENV_INTERNALS_H_
#define _OPENSYNC_FORMAT_ENV_INTERNALS_H_

/** A target function for osync_conv_find_path_fn() */
typedef osync_bool (*OSyncPathTargetFn)(const void *data, OSyncObjFormat *fmt);

osync_bool osync_conv_find_path_fmtlist(OSyncFormatEnv *env, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist);

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata, const char *extension_name, OSyncError **error);
osync_bool osync_conv_convert_fmtlist(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets);

/*! @brief The environment used for conversions
 */
struct OSyncFormatEnv {
	/** A List of formats */
	GList *objformats;
	/** A list of available converters */
	GList *converters;
	/** A list of filter functions */
	GList *custom_filters;
	
	GList *modules;
	GModule *current_module;
};

/**
 * @ingroup OSyncConvPrivate
 */
/*@{*/

/*! @brief search tree for format converters
 */
typedef struct OSyncFormatConverterTree {
	/* The converters that weren't reached yet */
	GList *unused;
	/* The search queue for the Breadth-first search */
	GList *search;
} OSyncFormatConverterTree;

typedef struct OSyncFormatConverterPathVertice {
	/** The format associated with this OSyncFormatConverterPathVertice */
	OSyncObjFormat *format;
	OSyncData *data;

	/** The path of converters taken to this OSyncFormatConverterPathVertice. If this OSyncFormatConverterPathVertice is a target, we will
	 * return this list as the result */
	GList *path;

	unsigned losses;
	unsigned objtype_changes;
	unsigned conversions;
	guint id;
	guint neighbour_id;
	osync_bool preferred;

	int ref_count;

} OSyncFormatConverterPathVertice;

typedef osync_bool (*OSyncTargetLastConverterFn)(const void *data, OSyncFormatConverterTree *tree);
#endif //_OPENSYNC_FORMAT_ENV_INTERNALS_H_
