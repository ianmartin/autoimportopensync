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

#include "opensync-module.h"
#include "opensync-data.h"
#include "opensync-format.h"
#include "opensync_format_env_internals.h"

/*! @brief Loads the modules from a given directory
 * 
 * Loads all modules from a directory into a osync environment
 * 
 * @param env Pointer to a OSyncFormatEnv environment
 * @param path The path where to look for plugins, NULL for the default sync module directory
 * @param must_exist If set to TRUE, this function will return an error if the directory does not exist
 * @param error Pointer to a error struct to return an error
 * @returns TRUE on success, FALSE otherwise
 * 
 */
static osync_bool _osync_format_env_load_modules(OSyncFormatEnv *env, const char *path, osync_bool must_exist, OSyncError **error)
{
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	OSyncModule *module = NULL;
	const gchar *de = NULL;
	GList *m = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i, %p)", __func__, env, path, must_exist, error);
	osync_assert(env);
	osync_assert(path);
	
	//Load all available shared libraries (plugins)
	if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
		if (must_exist) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Path is not loadable");
			goto error;
		} else {
			osync_trace(TRACE_EXIT, "%s: Directory does not exist (non-fatal)", __func__);
			return TRUE;
		}
	}
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open directory %s: %s", path, gerror->message);
		g_error_free(gerror);
		goto error;
	}
	
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.so", filename)) {
			g_free(filename);
			continue;
		}
		
		module = osync_module_new(error);
		if (!module)
			goto error_free_filename;
		
		if (!osync_module_load(module, filename, error)) {
			osync_trace(TRACE_INTERNAL, "Unable to load module %s: %s", filename, osync_error_print(error));
			osync_error_unref(error);
			osync_module_free(module);
			g_free(filename);
			continue;
		}
		
		if (!osync_module_check(module, error)) {
			if (osync_error_is_set(error)) {
				osync_trace(TRACE_INTERNAL, "Module check error for %s: %s", filename, osync_error_print(error));
				osync_error_unref(error);
			}
			osync_module_free(module);
			g_free(filename);
			continue;
		}
	
		if (!osync_module_get_format_info(module, env, error)) {
			if (osync_error_is_set(error)) {
				osync_trace(TRACE_INTERNAL, "Module get format error for %s: %s", filename, osync_error_print(error));
				osync_error_unref(error);
			}
			osync_module_free(module);
			g_free(filename);
			continue;
		}
		
		env->modules = g_list_append(env->modules, module);
		
		g_free(filename);
	}
	
	g_dir_close(dir);
	
	/* Load the converters, filters, etc */
	for (m = env->modules; m; m = m->next) {
		module = m->data;
		if (!osync_module_get_conversion_info(module, env, error)) {
			osync_trace(TRACE_INTERNAL, "Module get conversion error %s", osync_error_print(error));
			osync_error_unref(error);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_filename:
	g_free(filename);
	g_dir_close(dir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** Compare the distance of two vertices
 *
 * First, try to minimize the losses. Then,
 * try to minimize the conversions between
 * different objtypes. Then, try to minimize
 * the total number of conversions.
 */
static int _compare_vertice_distance(const void *a, const void *b)
{
	const vertice *va = a;
	const vertice *vb = b;
	if (va->losses < vb->losses)
		return -1;
	else if (va->losses > vb->losses)
		return 1;
	else if (va->objtype_changes < vb->objtype_changes)
		return -1;
	else if (va->objtype_changes > vb->objtype_changes)
		return 1;
	else if (va->conversions < vb->conversions)
		return -1;
	else if (va->conversions > vb->conversions)
		return 1;
	else
		return 0;
}

/** Function used on a path search for a format name array
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtnames()
 */
static osync_bool _target_fn_formats(const void *data, OSyncObjFormat *fmt)
{
	OSyncObjFormat **list = (OSyncObjFormat **)data;
	OSyncObjFormat **i;
	for (i = list; *i; i++) {
		if (osync_objformat_is_equal(fmt, *i))
			/* Found */
			return TRUE;
	}

	/* Not found */
	return FALSE;
}

/** Function used on path searchs for a single format
 *
 * @see osync_conv_find_path_fn(), osync_change_convert()
 */
static osync_bool _target_fn_simple(const void *data, OSyncObjFormat *fmt)
{
	OSyncObjFormat *target = (OSyncObjFormat *)data;
	return osync_objformat_is_equal(target, fmt);
}

/** Increment a vertice reference count */
/*static void ref_vertice(vertice *v)
{
	v->references++;
}*/

/** Dereference a vertice
 */
static void _free_vertice(vertice *vertice)
{
	g_list_free(vertice->path);

	g_free(vertice);
}

/** Returns a neighbour of the vertice ve
 *
 * Returns a new reference to the vertice. The reference
 * should be dropped using deref_vertice(), later.
 */
static vertice *_get_next_vertice_neighbour(OSyncFormatEnv *env, conv_tree *tree, vertice *ve, OSyncError **error)
{
	GList *c = NULL;
	OSyncFormatConverter *converter = NULL;
	OSyncObjFormat *fmt_target = NULL;
	vertice *neigh = NULL;
	const char *source_objtype = NULL;
	const char *target_objtype = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, tree, ve);
	
	/* Ok. we need to get the next valid neighbour to our input vertice.
	 * Valid neighbours are the once that are reachable by a conversion. So
	 * we now go through all converters and check if they are valid */
	for (c = tree->unused; c; c = c->next) {
		converter = c->data;
		fmt_target = osync_converter_get_targetformat(converter);
		
		/* Check only valid converters, from the right format */
		if (!osync_objformat_is_equal(osync_converter_get_sourceformat(converter), ve->format))
			continue;

		/* Remove the converter from the unused list */
		tree->unused = g_list_remove(tree->unused, converter);

		/* Allocate the new neighbour */
		neigh = osync_try_malloc0(sizeof(vertice), error);
		if (!neigh)
			goto error;
		
		neigh->format = fmt_target;
		neigh->path = g_list_copy(ve->path);
		neigh->path = g_list_append(neigh->path, converter);
		
		/* Distance calculation */
		neigh->conversions = ve->conversions + 1;
		
		neigh->losses = ve->losses;
		if (osync_converter_get_type(converter) == OSYNC_CONVERTER_DECAP)
			neigh->losses++;
		
		neigh->objtype_changes = ve->objtype_changes;
		
		OSyncObjFormat *sourceformat = osync_converter_get_sourceformat(converter);
		OSyncObjFormat *targetformat = osync_converter_get_targetformat(converter);
		
		source_objtype = osync_objformat_get_objtype(sourceformat);
		target_objtype = osync_objformat_get_objtype(targetformat);
		if (strcmp(source_objtype, target_objtype))
			neigh->objtype_changes++;

		osync_trace(TRACE_EXIT, "%s: %p (converter from %s to %s)", __func__, neigh, osync_objformat_get_name(sourceformat), osync_objformat_get_name(targetformat));
		return neigh;
	}
	
	osync_trace(TRACE_EXIT, "%s: None found", __func__);
	return NULL;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void _free_tree(conv_tree *tree)
{
	/* Remove the remaining references on the search queue */
	g_list_foreach(tree->search, (GFunc)_free_vertice, NULL);
	
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
}

/** Search for the shortest path of conversions to one or more formats
 *
 * This function search for the shortest path of conversions
 * that can be made to a change, considering possible detections
 * that may be necessary. The target is given by a function
 * that check if a given format is a 'target vertice' or not. The
 * function is used to allow the search path code to be used
 * to search for 'objtype detection', search for the path
 * for an available sink for a format, and maybe other uses.
 *
 * There is a limitation however. To prevent
 * 
 * The list returned on path_edges should be freed by the caller.
 *
 * Note: NEVER use the detection/conversion functions on
 *       CHANGE_DELETED changes. Converting and detecting data
 *       on changes that have no data doesn't make sense
 *
 * @param env Pointer to a OSyncFormatEnv environment
 * @see osync_conv_convert_fn(), osync_change_convert(),
 *      osync_conv_convert_fmtlist(), osync_change_convert_member_sink()
 *
 * @see target_fn_fmtlist(), target_fn_fmtnames(),
 *      target_fn_simple(), target_fn_fmtname(),
 *      target_fn_membersink(), target_fn_no_any()
 */
static OSyncFormatConverterPath *_osync_format_env_find_path_fn(OSyncFormatEnv *env, OSyncObjFormat *source_format, OSyncPathTargetFn target_fn, const void *fndata, OSyncError **error)
{
	vertice *result = NULL;
	vertice *neighbour = NULL;
	OSyncFormatConverterPath *path = NULL;
	conv_tree *tree = NULL;
	vertice *begin = NULL;
	GList *e;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, env, source_format, target_fn, fndata, error);
	osync_assert(env);
	osync_assert(source_format);
	osync_assert(target_fn);
	
	//Vertice = Spitze = Format
	//edge = Kante = Converter

	/* Optimization: check if the format is already valid */
	if (target_fn(fndata, source_format)) {
		path = osync_converter_path_new(error);
		if (!path)
			goto error;
		
		osync_trace(TRACE_EXIT, "%s: Target already valid", __func__);
		return path;
	}

	/* Make a new search tree */
	tree = osync_try_malloc0(sizeof(conv_tree), error);
	if (!tree)
		goto error;
	tree->unused = g_list_copy(env->converters);
	
	/* We make our starting point (which is the current format of the 
	 * change of course */
	begin = osync_try_malloc0(sizeof(vertice), error);
	if (!begin)
		goto error_free_tree;
	
	begin->format = source_format;
	begin->path = NULL;
	
	tree->search = g_list_append(NULL, begin);
	
	/* While there are still vertices in our
	 * search queue */
	while (g_list_length(tree->search)) {
		/* Get the first vertice from the search queue
		 * and remove it from the queue */
		vertice *current = tree->search->data;
		tree->search = g_list_remove(tree->search, current);
		
		/* Check if we have reached a target format */
		if (target_fn(fndata, current->format)) {
			/* Done. return the result */
			result = current;
			break;
		}
		
		/* If we dont have reached a target, we look at our neighbours */
		while ((neighbour = _get_next_vertice_neighbour(env, tree, current, error))) {
			/* We found a neighbour and insert it sorted in our search queue */
			tree->search = g_list_insert_sorted(tree->search, neighbour, _compare_vertice_distance);
		}
		if (osync_error_is_set(error))
			goto error_free_tree;
		
		/* Done, drop the reference to the vertice */
		_free_vertice(current);
	}
			
	if (!result) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find conversion path");
		goto error_free_tree;
	}
	
	/* Found it. Create a path object */
	path = osync_converter_path_new(error);
	if (!path)
		goto error;
	
	for (e = result->path; e; e = e->next) {
		OSyncFormatConverter *edge = e->data;
		osync_converter_path_add_edge(path, edge);
	}
	
	/* Drop the reference to the result vertice */
	_free_vertice(result);
	
	/* Free the tree */
	_free_tree(tree);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;

error_free_tree:
	_free_tree(tree);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @defgroup PublicAPI Public APIs
 * @brief Available public APIs
 * 
 */

/**
 * @defgroup OSyncPublic OpenSync Public API
 * @ingroup PublicAPI
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */

/**
 * @defgroup OSyncFormatEnvAPI OpenSync Environment
 * @ingroup OSyncPublic
 * @brief The public API of the opensync environment
 * 
 */
/*@{*/


/*! @brief This will create a new opensync environment
 * 
 * The environment will hold all information about plugins, groups etc
 * 
 * @returns A pointer to a newly allocated environment. NULL on error.
 * 
 */
OSyncFormatEnv *osync_format_env_new(OSyncError **error)
{
	OSyncFormatEnv *env = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	env = osync_try_malloc0(sizeof(OSyncFormatEnv), error);
	if (!env) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return env;
}

/*! @brief Frees a osync environment
 * 
 * Frees a osync environment and all resources.
 * 
 * @param env Pointer to the environment to free
 * 
 */
void osync_format_env_free(OSyncFormatEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	osync_assert(env);
	
	/* Free the formats */
	while (env->objformats) {
		osync_objformat_unref(env->objformats->data);
		env->objformats = g_list_remove(env->objformats, env->objformats->data);
	}
	
	/* Free the converters */
	while (env->converters) {
		osync_converter_unref(env->converters->data);
		env->converters = g_list_remove(env->converters, env->converters->data);
	}
	
	/* Free the filters */
	while (env->custom_filters) {
		osync_custom_filter_unref(env->custom_filters->data);
		env->custom_filters = g_list_remove(env->custom_filters, env->custom_filters->data);
	}
	
	/* Free the modules */
	while (env->modules) {
		osync_module_free(env->modules->data);
		env->modules = g_list_remove(env->modules, env->modules->data);
	}
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Loads all format and conversion plugins
 * 
 * This command will load all plugins for the conversion system.
 * If you dont change the path before it will load the plugins
 * from the default location
 * 
 * @param env The format environment
 * @param path The path to load from or NULL if to load from default path
 * @param error The location to return a error to
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_format_env_load_plugins(OSyncFormatEnv *env, const char *path, OSyncError **error)
{
	osync_bool must_exist = TRUE;
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, path, error);
	
	if (!path) {
		path = OPENSYNC_FORMATSDIR;
		must_exist = FALSE;
	}
	
	if (!_osync_format_env_load_modules(env, path, must_exist, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void osync_format_env_register_objformat(OSyncFormatEnv *env, OSyncObjFormat *format)
{
	osync_assert(env);
	osync_assert(format);
	
	env->objformats = g_list_append(env->objformats, format);
	osync_objformat_ref(format);
}

/*! @brief Finds the object format with the given name
 * 
 * @param env Pointer to the environment
 * @param name Name of the format type to find
 * @returns The object format, or NULL if not found
 * 
 */
OSyncObjFormat *osync_format_env_find_objformat(OSyncFormatEnv *env, const char *name)
{
	GList *element = NULL;
	
	osync_assert(env);
	osync_assert(name);
	
	for (element = env->objformats; element; element = element->next) {
		OSyncObjFormat *format = element->data;
		if (!strcmp(osync_objformat_get_name(format), name))
			return format;
	}
	return NULL;
}

/*! @brief Returns the number of available object formats
 * 
 * @param env The format environment
 * @returns The number of object formats
 * 
 */
int osync_format_env_num_objformats(OSyncFormatEnv *env)
{
	osync_assert(env);
	return g_list_length(env->objformats);
}

/*! @brief Gets the nth object format
 * 
 * @param env The format environment
 * @param nth The position of the object format to retrieve
 * @returns The object format
 * 
 */
OSyncObjFormat *osync_format_env_nth_objformat(OSyncFormatEnv *env, int nth)
{
	osync_assert(env);
	return g_list_nth_data(env->objformats, nth);
}

void osync_format_env_register_converter(OSyncFormatEnv *env, OSyncFormatConverter *converter)
{
	osync_assert(env);
	osync_assert(converter);
	
	/* Register the inverse converter if its a detector. The inverse
	 * of a detector can always be used */
	if (osync_converter_get_type(converter) == OSYNC_CONVERTER_DETECTOR) {
		
		OSyncFormatConverter *conv = osync_converter_new_detector(osync_converter_get_targetformat(converter), osync_converter_get_sourceformat(converter), NULL, NULL);
		if (!conv)
			return;
	
		env->converters = g_list_append(env->converters, conv);
	}
	
	env->converters = g_list_append(env->converters, converter);
	osync_converter_ref(converter);
}

/*! @brief Finds the converter with the given source and target format
 * 
 * @param env Pointer to the environment
 * @param sourceformat The source format
 * @param targetformat The target format
 * @returns The converter, or NULL if not found
 * 
 */
OSyncFormatConverter *osync_format_env_find_converter(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat)
{
	GList *c = NULL;
	
	osync_assert(env);
	osync_assert(sourceformat);
	osync_assert(targetformat);
	
	for (c = env->converters; c; c = c->next) {
		OSyncFormatConverter *converter = c->data;
		if (!osync_objformat_is_equal(sourceformat, osync_converter_get_sourceformat(converter)))
			continue;
			
		if (!osync_objformat_is_equal(targetformat, osync_converter_get_targetformat(converter)))
			continue;
		
		return converter;
	}
	
	return NULL;
}

/*! @brief Returns the number of available converters
 * 
 * @param env The format environment
 * @returns The number of converters
 * 
 */
int osync_format_env_num_converters(OSyncFormatEnv *env)
{
	osync_assert(env);
	return g_list_length(env->converters);
}

/*! @brief Gets the nth format converter
 * 
 * @param env The format environment
 * @param nth The position of the format converter to retrieve
 * @returns The format converter
 * 
 */
OSyncFormatConverter *osync_format_env_nth_converter(OSyncFormatEnv *env, int nth)
{
	osync_assert(env);
	return g_list_nth_data(env->converters, nth);
}

void osync_format_env_register_filter(OSyncFormatEnv *env, OSyncCustomFilter *filter)
{
	osync_assert(env);
	osync_assert(filter);
	
	env->custom_filters = g_list_append(env->custom_filters, filter);
	osync_custom_filter_ref(filter);
}

/*! @brief Returns the number of available filters
 * 
 * @param env The format environment
 * @returns The number of filters
 * 
 */
int osync_format_env_num_filters(OSyncFormatEnv *env)
{
	osync_assert(env);
	return g_list_length(env->custom_filters);
}

/*! @brief Gets the nth filter
 * 
 * @param env The format environment
 * @param nth The position of the filter to retrieve
 * @returns The filter
 * 
 */
OSyncCustomFilter *osync_format_env_nth_filter(OSyncFormatEnv *env, int nth)
{
	osync_assert(env);
	return g_list_nth_data(env->custom_filters, nth);
}

/*! @brief Tries to detect the format of the given data object
 * 
 * This will try to detect the format of the specified data object
 * and return it, but not set it.
 * 
 * @param env The conversion environment to use
 * @param data The data object to detect
 * @returns The format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_format_env_detect_objformat(OSyncFormatEnv *env, OSyncData *data)
{
	GList *d = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, data);
	
	/* Run all datadetectors for our source type */
	for (d = env->converters; d; d = d->next) {
		OSyncFormatConverter *converter = d->data;
		/* We check if the converter might be able to converter the change */
		if (osync_converter_get_type(converter) == OSYNC_CONVERTER_DETECTOR && osync_converter_matches(converter, data)) {
			osync_trace(TRACE_INTERNAL, "running detector %s for format %s", osync_objformat_get_name(osync_converter_get_targetformat(converter)), osync_objformat_get_name(osync_data_get_objformat(data)));
			if (osync_converter_detect(converter, data))  {
				OSyncObjFormat *detected_format = osync_converter_get_targetformat(converter);
				osync_trace(TRACE_EXIT, "%s: %p", __func__, detected_format);
				return detected_format;
			}
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: No detector triggered", __func__);
	return NULL;
}

/*! @brief Tries to detect the encapsulated format of the given data object
 * 
 * This will try to detect the encapsulated format of the specified data object
 * and return it, but not set it. It will try to detect the format of the data object,
 * deencapsulate it, detect again etc until it cannot deencapsulate further.
 * 
 * @param env The conversion environment to use
 * @param input The data object to detect
 * @param error The error-return location
 * @returns The encapsulated format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_format_env_detect_objformat_full(OSyncFormatEnv *env, OSyncData *input, OSyncError **error)
{
	OSyncObjFormat *detected_format = NULL;
	OSyncData *new_data = NULL;
	GList *d = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, input, error);
	
	/* Make a copy of the data */
	new_data = osync_data_clone(input, error);
	if (!new_data)
		goto error;
	
	while (TRUE) {
		if ((detected_format = osync_format_env_detect_objformat(env, new_data))) {
			/* We detected the format. So we replace the original format. */
			osync_data_set_objformat(new_data, detected_format);
		} else
			detected_format = osync_data_get_objformat(new_data);
		
		OSyncFormatConverter *converter = NULL;
		/* Try to decap the change as far as possible */
		for (d = env->converters; d; d = d->next) {
			converter = d->data;
			if (osync_converter_matches(converter, new_data) && osync_converter_get_type(converter) == OSYNC_CONVERTER_DECAP) {
				/* Run the decap */
				if (!osync_converter_invoke(converter, new_data, NULL, error)) {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to decap the change");
					goto error_free_data;
				}
				
				break;
			} else
				converter = NULL;
		}
		
		/* We couldnt find a decap, so we quit. */
		if (!converter)
			break;
	}
	osync_data_unref(new_data);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, detected_format);
	return detected_format;

error_free_data:
	osync_data_unref(new_data);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*! @brief Convert a data object using a specific conversion path
 * 
 * This will convert the specified data object using the specified format
 * conversion path if possible.
 * 
 * @param env The conversion environment to use
 * @param path The conversion path to follow
 * @param data The data object to convert
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_format_env_convert(OSyncFormatEnv *env, OSyncFormatConverterPath *path, OSyncData *data, OSyncError **error)
{
	OSyncObjFormat *source = NULL;
	int length = 0;
	char *buffer = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, path, data, error);
	osync_assert(data);
	osync_assert(env);
	osync_assert(path);
	
	source = osync_data_get_objformat(data);
	osync_assert(source);
	
	length = osync_converter_path_num_edges(path);
	
	if (length == 0) {
		osync_trace(TRACE_EXIT, "%s: Path has 0 length");
		return TRUE;
	}
	
	osync_data_get_data(data, &buffer, NULL);
	
	if (!buffer) {
		/* Data without any data on them can be converted between any formats. Therefore
		 * we just take the format of the last converter and set it on
		 * the data */
		OSyncFormatConverter *converter = osync_converter_path_nth_edge(path, length - 1);
		osync_data_set_objformat(data, osync_converter_get_targetformat(converter));
		osync_data_set_objtype(data, osync_objformat_get_objtype(osync_converter_get_targetformat(converter)));
	} else {
		/* Otherwise we go through the conversion path
		 * and call all converters along the way */
		int i;
		for (i = 0; i < length; i++) {
			OSyncFormatConverter *converter = osync_converter_path_nth_edge(path, i);
			
			if (!osync_converter_invoke(converter, data, osync_converter_path_get_config(path), error)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
				return FALSE;
			}
		}
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*! @brief Find a conversion path between two formats
 * 
 * This will find a conversion path between two object formats
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param sourceformat The source format to be converted from
 * @param targetformat The target format to be converted to
 * @param error The error-return location
 * @returns The appropriate conversion path, or NULL if an error occurred.
 * 
 */
OSyncFormatConverterPath *osync_format_env_find_path(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, OSyncError **error)
{
	OSyncFormatConverterPath *path = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %p)", __func__, env, sourceformat, targetformat, targetformat ? osync_objformat_get_name(targetformat) : "NONE", error);
	
	path = _osync_format_env_find_path_fn(env, sourceformat, _target_fn_simple, targetformat, error);
	if (!path) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;
}

/*! @brief Find a conversion path from one format to one of a list of formats
 * 
 * This will convert the change with its data to one of the specified formats
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param sourceformat The source format to be converted from
 * @param targets NULL-Terminated array of possible formats to convert to
 * @param error The error-return location
 * @returns The appropriate conversion path, or NULL if an error occurred.
 * 
 */
OSyncFormatConverterPath *osync_format_env_find_path_formats(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat **targets, OSyncError **error)
{
	OSyncFormatConverterPath *path = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, sourceformat, targets, error);
	
	path = _osync_format_env_find_path_fn(env, sourceformat, _target_fn_formats, targets, error);
	if (!path) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;
}

/*@}*/
