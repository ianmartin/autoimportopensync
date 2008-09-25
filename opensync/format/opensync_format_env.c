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
		filename = g_strdup_printf ("%s%c%s", path, G_DIR_SEPARATOR, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || !g_pattern_match_simple("*."G_MODULE_SUFFIX, filename)) {
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
	const OSyncFormatConverterPathVertice *va = a;
	const OSyncFormatConverterPathVertice *vb = b;
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
	else if (va->preferred)
		return -1;
	else if (vb->preferred)
		return 1;
  	else if (va->id < vb->id)
		return -1;
	else if (va->id > vb->id)
		return 1;
 	else if (va->neighbour_id < vb->neighbour_id)
		return -1;
	else if (va->neighbour_id > vb->neighbour_id)
		return 1;
	else
		return 0;
}

/** Function used on a path search OSyncFormatConverterPathVertice objformat to check if it matches an OSyncList of OSyncObjFormatSinks 
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtnames()
 */
static osync_bool _target_fn_format_sinks(const void *data, OSyncObjFormat *fmt)
{
	OSyncList *f, *formats = (OSyncList *) data;
	for (f = formats; f; f = f->next) {
		OSyncObjFormatSink *format_sink = f->data;
		const char *format = osync_objformat_sink_get_objformat(format_sink);
		/*if (osync_objformat_is_equal(fmt, format))*/
		if (!strcmp(format, osync_objformat_get_name(fmt)))
			/* Found */
			return TRUE;
	}

	/* Not found */
	return FALSE;
}

/** Function used on path search OSyncFormatConverterPathVertice objformat to check if it matches a single format
 *
 * @see osync_conv_find_path_fn(), osync_change_convert()
 */
static osync_bool _target_fn_simple(const void *data, OSyncObjFormat *fmt)
{
	OSyncObjFormat *target = (OSyncObjFormat *)data;
	return osync_objformat_is_equal(target, fmt);
}

/** Function used on a tree list of unused converters to check if there remains any that accomplish an OSyncList of OSyncObjFormatSinks 
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtnames()
 */
static osync_bool _target_fn_format_sinks_reached_lastconverter(const void *data, OSyncFormatConverterTree *tree)
{
	OSyncList *f, *formats = (OSyncList *) data;
	GList *c;
	OSyncFormatConverter *converter = NULL;
	for (c = tree->unused; c; c = c->next) {
		converter = c->data;
		for (f = formats; f; f = f->next) {
			OSyncObjFormatSink *format_sink = f->data;
			const char *format = osync_objformat_sink_get_objformat(format_sink);
			if (!strcmp(format, osync_objformat_get_name(osync_converter_get_targetformat(converter)))) return FALSE;
		}
	}
	return TRUE;
}

/** Function used on a tree list of unused converters to check if there remains one that match a single format
 *
 * @see osync_conv_find_path_fn(), osync_change_convert()
 */
static osync_bool _target_fn_simple_reached_lastconverter(const void *data, OSyncFormatConverterTree *tree)
{
	OSyncObjFormat *target = (OSyncObjFormat *)data;
	GList *c;
	OSyncFormatConverter *converter = NULL;
	for (c = tree->unused; c; c = c->next) {
		converter = c->data;
		if (!strcmp(osync_objformat_get_name(target), osync_objformat_get_name(osync_converter_get_targetformat(converter)))) return FALSE;
	}
	return TRUE;
}

static OSyncFormatConverterPathVertice *_vertice_new(OSyncError **error)
{
	OSyncFormatConverterPathVertice *v = osync_try_malloc0(sizeof(OSyncFormatConverterPathVertice), error);
	if (!v)
		return NULL;

	v->ref_count = 1;

	return v;
}

/** Increment a OSyncFormatConverterPathVertice reference count */
static OSyncFormatConverterPathVertice *_vertice_ref(OSyncFormatConverterPathVertice *OSyncFormatConverterPathVertice)
{
	osync_assert(OSyncFormatConverterPathVertice);
	
	g_atomic_int_inc(&(OSyncFormatConverterPathVertice->ref_count));

	return OSyncFormatConverterPathVertice;
}

/** Dereference a OSyncFormatConverterPathVertice
 */
static void _vertice_unref(OSyncFormatConverterPathVertice *OSyncFormatConverterPathVertice)
{
	osync_assert(OSyncFormatConverterPathVertice);
		
	if (g_atomic_int_dec_and_test(&(OSyncFormatConverterPathVertice->ref_count))) {

		g_list_free(OSyncFormatConverterPathVertice->path);

		if(OSyncFormatConverterPathVertice->data)
			osync_data_unref(OSyncFormatConverterPathVertice->data);

		g_free(OSyncFormatConverterPathVertice);
	}
}



/** Returns a boolean telling if the current OSyncFormatConverterPathVertice can be converter with the provided converter
 *
 * Concepts :
 * - "Detectors" are cheap converters in that all they do is merely check that the data conforms to a given detection function.
 * They are also automatically two ways in that they register their opposite when created.
 * They can also be used to prevent a conversion for a non detector converter which provides the same conversion.
 *
 * Internals :
 * We assume that there is at least a detector converter for this conversion. In theory this is not true  : there is at least
 * the provided converter which is likely not a detector and there could be no detector at all. Though in practice the implementation
 * behaves well as we seek all converters (including the provided one) for a "non detector" one. Then we give preference to this
 * non detector converter (though we always seek for a detector that provides the same conversion before telling this "non detector" 
 * converter is valid).
 */
static osync_bool _validate_path_with_detector(OSyncFormatConverterPathVertice *ve, OSyncFormatEnv *env, OSyncFormatConverter *converter) {

	OSyncList *cs = NULL;
	OSyncList *cd = NULL;
	osync_bool has_nondetector = FALSE;
	osync_trace(TRACE_INTERNAL, "Converter %s to %s type %i", osync_objformat_get_name(osync_converter_get_sourceformat(converter)), osync_objformat_get_name(osync_converter_get_targetformat(converter)), osync_converter_get_type(converter));

	/* We search the converters for the given conversion to see if there are other converters than "detector" for this conversion */
	OSyncList *converters_seeknondetectors = osync_format_env_find_converters(env, osync_converter_get_sourceformat(converter), osync_converter_get_targetformat(converter));
	for(cd = converters_seeknondetectors; cd ; cd = cd->next) {
		OSyncFormatConverter *converter_seeknondetectors = cd->data;
		if ( converter_seeknondetectors && (osync_converter_get_type(converter_seeknondetectors) != OSYNC_CONVERTER_DETECTOR) ) {
			osync_trace(TRACE_INTERNAL, "Found non detector converter. We will pair the detector later on with this 'non detector' converter if a detector is available.");
			has_nondetector = TRUE;
			break;
		}
	}

	if (has_nondetector) {
		/*  There were other converters than the detector (if there is a detector at all) for the given conversion. */

		/* Skip the detector : it will be handled when processing the non detector converter
		 Non detector are preferred in that we will validate them instead of the detector but still the detector
		 will be called before telling if the "non detector" converter is valid for the */
		if ( osync_converter_get_type(converter) == OSYNC_CONVERTER_DETECTOR )
			return FALSE;

		/* Looking after detector for non detector converter.
		   If there was a detector converter for the same conversion as the non detector converter and the detection fails force the failure
		   of the non detector converter */
		OSyncList *converters_sameformat = osync_format_env_find_converters(env, osync_converter_get_sourceformat(converter), osync_converter_get_targetformat(converter));
		for(cs = converters_sameformat; cs ; cs = cs->next) {
			OSyncFormatConverter *converter_sameformat = cs->data;
			if ( converter_sameformat && (osync_converter_get_type(converter_sameformat) == OSYNC_CONVERTER_DETECTOR) ) {
				osync_trace(TRACE_INTERNAL, "detector found");
				if(!osync_converter_detect(converter_sameformat, ve->data)) {
					osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: FALSE", osync_objformat_get_name(osync_converter_get_sourceformat(converter)), osync_objformat_get_name(osync_converter_get_targetformat(converter)));
					return FALSE;
				} else {
					osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: TRUE", osync_objformat_get_name(osync_converter_get_sourceformat(converter)), osync_objformat_get_name(osync_converter_get_targetformat(converter)));
				}
			}
		}
	} else {
		/*  The detector was the only converter for the given conversion. Check that the "conversion" (detection) is valid. */
		osync_trace(TRACE_INTERNAL, "alone detector found");
		if(!osync_converter_detect(converter, ve->data)) {
			osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: FALSE", osync_objformat_get_name(osync_converter_get_sourceformat(converter)), osync_objformat_get_name(osync_converter_get_targetformat(converter)));
			return FALSE;
		} else {
			osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: TRUE", osync_objformat_get_name(osync_converter_get_sourceformat(converter)), osync_objformat_get_name(osync_converter_get_targetformat(converter)));
		}
	}

	return TRUE;
}

/** Returns a neighbour of the OSyncFormatConverterPathVertice ve
 *
 * Returns a new reference to the OSyncFormatConverterPathVertice. The reference
 * should be dropped using deref_vertice(), later.
 */
static OSyncFormatConverterPathVertice *_get_next_vertice_neighbour(OSyncFormatEnv *env, OSyncFormatConverterTree *tree, OSyncFormatConverterPathVertice *ve, OSyncError **error)
{
	GList *c = NULL;
	OSyncFormatConverter *converter = NULL;
	OSyncObjFormat *fmt_target = NULL;
	OSyncFormatConverterPathVertice *neigh = NULL;
	const char *source_objtype = NULL;
	const char *target_objtype = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, tree, ve);
	
	/* Ok. we need to get the next valid neighbour to our input OSyncFormatConverterPathVertice.
	 * Valid neighbours are the once that are reachable by a conversion. So
	 * we now go through all converters and check if they are valid */
	for (c = tree->unused; c; c = c->next) {
		converter = c->data;
		fmt_target = osync_converter_get_targetformat(converter);
		
		/* Check only valid converters, from the right format */
		if (!osync_objformat_is_equal(osync_converter_get_sourceformat(converter), ve->format))
			continue;

		/* Only validate with the help of detectors, if data is
		   available to run a detector on it. 
		   Check if a detector validate this path */
		if (osync_data_has_data(ve->data) 
			&& !_validate_path_with_detector(ve, env, converter))
			continue;

		/* Remove the converter from the unused list */
		tree->unused = g_list_remove(tree->unused, converter);

		/* Allocate the new neighbour */
		neigh = _vertice_new(error);
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

		osync_trace(TRACE_EXIT, "%s: %p (converter from %s to %s) objtype changes : %i losses : %i, conversions : %i", __func__, neigh, osync_objformat_get_name(sourceformat), osync_objformat_get_name(targetformat), neigh->objtype_changes, neigh->losses, neigh->conversions);
		return neigh;
	}
	
	osync_trace(TRACE_EXIT, "%s: None found", __func__);
	return NULL;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void _free_tree(OSyncFormatConverterTree *tree)
{
	/* Remove the remaining references on the search queue */
	g_list_foreach(tree->search, (GFunc)_vertice_unref, NULL);
	
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
}

/** Search for the shortest path of conversions to one or more formats
 *
 * This function search for the shortest path of conversions
 * that can be made to a change, considering possible detections
 * that may be necessary. The target is given by a function
 * that check if a given format is a 'target OSyncFormatConverterPathVertice' or not. The
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
static OSyncFormatConverterPath *_osync_format_env_find_path_fn(OSyncFormatEnv *env, OSyncData *sourcedata, OSyncPathTargetFn target_fn, OSyncTargetLastConverterFn last_converter_fn, const void *fndata, const char * preferred_format, OSyncError **error)
{
	OSyncFormatConverterPathVertice *result = NULL;
	OSyncFormatConverterPathVertice *neighbour = NULL;
	OSyncFormatConverterPath *path = NULL;
	OSyncFormatConverterTree *tree = NULL;
	OSyncFormatConverterPathVertice *begin = NULL;
	GList *e, *v;
	guint vertice_id = 0;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, env, sourcedata, target_fn, fndata, error);
	osync_assert(env);
	osync_assert(sourcedata);
	osync_assert(target_fn);
	
	//Vertice = Spitze = Format
	//edge = Kante = Converter

	/* Optimization: check if the format is already valid */
	if (target_fn(fndata, osync_data_get_objformat(sourcedata))) {
		path = osync_converter_path_new(error);
		if (!path)
			goto error;
		
		osync_trace(TRACE_EXIT, "%s: Target already valid", __func__);
		return path;
	}

	/* Make a new search tree */
	tree = osync_try_malloc0(sizeof(OSyncFormatConverterTree), error);
	if (!tree)
		goto error;
	tree->unused = g_list_copy(env->converters);
	
	/* We make our starting point (which is the current format of the 
	 * change of course */
	begin = _vertice_new(error);
	if (!begin)
		goto error_free_tree;
	
	begin->format = osync_data_get_objformat(sourcedata);
	begin->path = NULL;
	begin->id = vertice_id;
	begin->neighbour_id = 0;
	
	tree->search = g_list_append(NULL, begin);
	
	/* While there are still vertices in our
	 * search queue */
	while (g_list_length(tree->search)) {
		/* log current tree search list */
		GString *string = g_string_new("");
		guint size = g_list_length(tree->search);
		guint count = 0;
		for (v = tree->search; v; v = v->next) {
			count ++;
			OSyncFormatConverterPathVertice *OSyncFormatConverterPathVertice = v->data;
			GString *string2 = g_string_new("");
			guint size2 = g_list_length(OSyncFormatConverterPathVertice->path);
			guint count2 = 0;
			for (e = OSyncFormatConverterPathVertice->path; e; e = e->next) {
				count2 ++;
				OSyncFormatConverter *edge = e->data;
				if (count2 == 1) {
					  g_string_append(string2, osync_objformat_get_name(osync_converter_get_sourceformat(edge)));
					  g_string_append(string2, " -> ");
				}
				g_string_append(string2, osync_objformat_get_name(osync_converter_get_targetformat(edge)));
				if (size2 > 1 && count2 < size2) g_string_append(string2, " -> ");
			}
			g_string_append(string, osync_objformat_get_name(OSyncFormatConverterPathVertice->format));
			g_string_append(string, " ( ");
			g_string_append(string, g_string_free(string2, FALSE));
			g_string_append(string, " ) ");
			if (size > 1 && count < size) g_string_append(string, " -> ");
		}
		osync_trace(TRACE_INTERNAL, "Tree : %s", g_string_free(string, FALSE));

		/* Get the first OSyncFormatConverterPathVertice from the search queue
		 * and remove it from the queue */
		OSyncFormatConverterPathVertice *current = tree->search->data;
		tree->search = g_list_remove(tree->search, current);
		
		/* log current OSyncFormatConverterPathVertice */
		string = g_string_new("");
		size = g_list_length(current->path);
		count = 0;
		for (e = current->path; e; e = e->next) {
			count ++;
			OSyncFormatConverter *edge = e->data;
			if (count == 1) {
				  g_string_append(string, osync_objformat_get_name(osync_converter_get_sourceformat(edge)));
				  g_string_append(string, " -> ");
			}
			g_string_append(string, osync_objformat_get_name(osync_converter_get_targetformat(edge)));
			if (size > 1 && count < size) g_string_append(string, " -> ");
		}
		osync_trace(TRACE_INTERNAL, "Next vertice : %s (%s).", osync_objformat_get_name(current->format), g_string_free(string, FALSE));
		guint neighbour_id = 0;
		current->neighbour_id = 0;
		vertice_id++; // current OSyncFormatConverterPathVertice id for its neighbours

		/* Check if we have reached a target format */
		if (target_fn(fndata, current->format)) {
			osync_trace(TRACE_INTERNAL, "Target %s found", osync_objformat_get_name(current->format));
			/* Done. return the result */
			result = current;
			break;
		}
		
		/*
		 * Optimizations : 
		 */
		if (last_converter_fn(fndata, tree)) {
			osync_trace(TRACE_INTERNAL, "Last converter for target format reached: %s.", (result)?osync_objformat_get_name(result->format):"null");
			break;
		}
		/* Check if saved result is equal to current regarding losses, objtype_changes
		 * and conversions. If yes, we can skip further searches and break here */
		if (result) {
			if (result->losses <= current->losses && result->objtype_changes <= current->objtype_changes && result->conversions <= current->conversions) {
				osync_trace(TRACE_INTERNAL, "Target %s found in queue", osync_objformat_get_name(result->format));
				tree->search = g_list_remove(tree->search, result);
				break;
			} else {
				result = NULL;
			}
		}


		/*
		 * If we dont have reached a target, we look at our neighbours 
		 */
		osync_trace(TRACE_INTERNAL, "Looking at %s's neighbours.", osync_objformat_get_name(current->format));

		/* Convert the "current" data to the last edge found in the "current" conversion path  */
		current->data = osync_data_clone(sourcedata, error);
		OSyncFormatConverterPath *path_tmp = osync_converter_path_new(error);
		if (!path_tmp)
			goto error;
		for (e = current->path; e; e = e->next) {
			OSyncFormatConverter *edge = e->data;
			osync_converter_path_add_edge(path_tmp, edge);
		}
		if (!(osync_format_env_convert(env, path_tmp, current->data, error))) {
			osync_trace(TRACE_INTERNAL, "osync format env convert on this path failed - skipping the conversion");
			continue;
		}
		osync_converter_path_unref(path_tmp);

		/* Find all the neighboors or "current" at its current conversion point */
		while ((neighbour = _get_next_vertice_neighbour(env, tree, current, error))) {
			neighbour->id = vertice_id;
			neighbour_id++;
			neighbour->neighbour_id = neighbour_id;

			neighbour->preferred = FALSE;
			if (current->preferred)	  /* preferred is inherited by the neighbours */
				neighbour->preferred = TRUE;
			if(preferred_format && !strcmp(preferred_format, osync_objformat_get_name(neighbour->format)))
				neighbour->preferred = TRUE;

			/* log neighbour to be added to the tree search list */
			GString *string = g_string_new("");
			guint size = g_list_length(neighbour->path);
			guint count = 0;
			for (e = neighbour->path; e; e = e->next) {
				count ++;
				OSyncFormatConverter *edge = e->data;
				if (count == 1) {
					  g_string_append(string, osync_objformat_get_name(osync_converter_get_sourceformat(edge)));
					  g_string_append(string, " -> ");
				}
				g_string_append(string, osync_objformat_get_name(osync_converter_get_targetformat(edge)));
				if (size > 1 && count < size) g_string_append(string, " -> ");
			}
			osync_trace(TRACE_INTERNAL, "%s's neighbour : %s (%s)", osync_objformat_get_name(current->format), osync_objformat_get_name(neighbour->format), g_string_free(string, FALSE));

			/* We found a neighbour and insert it sorted in our search queue 
			   If vertices are equals in losses, objtypes and conversions, first registered is inserted before the others 
			   in the same OSyncFormatConverterPathVertice group (vertice_id) */
			tree->search = g_list_insert_sorted(tree->search, neighbour, _compare_vertice_distance); 

			/* Optimization:
			 * We found a possible target. Save it. */
			if (target_fn(fndata, neighbour->format)) {
				osync_trace(TRACE_INTERNAL, "Possible target found.");
				result = neighbour;
			}
		}

		if (osync_error_is_set(error))
			goto error_free_tree;
		
		/* Done, drop the reference to the OSyncFormatConverterPathVertice */
		_vertice_unref(current);
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
	
	/* Drop the reference to the result OSyncFormatConverterPathVertice */
	_vertice_unref(result);
	
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
 * @defgroup OSyncFormatEnvAPI OpenSync Format Environment
 * @ingroup OSyncPublic
 * @brief The public API of the OpenSync Format Environment
 * 
 */
/*@{*/


/*! @brief This will create a new OpenSync Format Environment
 * 
 * The format environment will hold all information about format plugins.
 * 
 * @returns A pointer to a newly allocated format environment. NULL on error.
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
		osync_trace(TRACE_INTERNAL, "FORMAT: %s", osync_objformat_get_name(env->objformats->data));
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
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, __NULLSTR(path), error);
	
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

/*! @brief Register Object Format to the Format Environment 
 * 
 * @param env Pointer to the environment
 * @param format Pointer ot the Object Format which sould be registred
 * 
 */
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

/*! @brief Registers Format Converter or Detector to Format Environment
 * 
 * @param env The format environment
 * @param converter Pointer of the Format Converter or Detector
 */
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

/*! @brief Finds first converter with the given source and target format
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

/*! @brief Returns a list of all converters with the given source and target format
 * 
 * @param env Pointer to the environment
 * @param sourceformat The source format
 * @param targetformat The target format
 * @returns List of OSyncFormatConverter, or NULL if none found
 * 
 */
OSyncList *osync_format_env_find_converters(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat)
{
	OSyncList *r = NULL;
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

		r = osync_list_append(r, converter);
	}

	return r;
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

/*! @brief Register Filter in Format Environment 
 * 
 * @param env The format environment
 * @param filter Pointer of Custom Filter to register
 * 
 */
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
		osync_trace(TRACE_EXIT, "%s: Path has 0 length", __func__);
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

	OSyncData *sourcedata = osync_data_new(NULL, 0, sourceformat, error);
	if (!sourcedata)
		goto error;

	path = _osync_format_env_find_path_fn(env, sourcedata, _target_fn_simple, _target_fn_simple_reached_lastconverter, targetformat, NULL, error);

	osync_data_unref(sourcedata);

	if (!path)
		goto error;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*! @brief Find a conversion path from the source format to a specific format with help of detectors.
 * 
 * This will find a conversion path between two object formats
 * if possible.
 *
 * @param env The format environment to use
 * @param sourcedata The OSyncData object which should be converted and the detectors will run on
 * @param targetformat The target format to be converted to 
 * @param error The error-return location
 * @returns The appropriate conversion path, or NULL if an error occurred.
 * 
 */
OSyncFormatConverterPath *osync_format_env_find_path_with_detectors(OSyncFormatEnv *env, OSyncData *sourcedata, OSyncObjFormat *targetformat, const char *preferred_format, OSyncError **error)
{
	OSyncFormatConverterPath *path = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %p)", __func__, env, sourcedata, targetformat, targetformat ? osync_objformat_get_name(targetformat) : "NONE", error);
	
	path = _osync_format_env_find_path_fn(env, sourcedata, _target_fn_simple, _target_fn_simple_reached_lastconverter, targetformat, preferred_format, error);
	if (!path) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;
}

/*! @brief Find a conversion path from one format to one of a list of formats
 * 
 * @param env The conversion environment to use
 * @param sourceformat The source format to be converted from
 * @param targets List of possible Object Format Sinks
 * @param error The error-return location
 * @returns The appropriate conversion path, or NULL if an error occurred.
 * 
 */
OSyncFormatConverterPath *osync_format_env_find_path_formats(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncList *targets, OSyncError **error)
{
	OSyncFormatConverterPath *path = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, env, sourceformat, targets, error);
	
	OSyncData *sourcedata = osync_data_new(NULL, 0, sourceformat, error);
	if (!sourcedata)
		goto error;

	path = _osync_format_env_find_path_fn(env, sourcedata, _target_fn_format_sinks, _target_fn_format_sinks_reached_lastconverter, targets, NULL, error);

	osync_data_unref(sourcedata);

	if (!path)
		goto error;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*! @brief Find a conversion path from one format to one of a list of formats with the help of detectors
 * 
 * @param env The format environment to use
 * @param sourcedata The OSyncData object which should be converted and the detectors will run on
 * @param targets List of possible Object Format Sinks
 * @param error The error-return location
 * @returns The appropriate conversion path, or NULL if an error occurred.
 * 
 */
OSyncFormatConverterPath *osync_format_env_find_path_formats_with_detectors(OSyncFormatEnv *env, OSyncData *sourcedata, OSyncList *targets, const char *preferred_format, OSyncError **error)
{
	OSyncFormatConverterPath *path = NULL;
	GString *string = g_string_new("");
	OSyncList *t;
	guint size = osync_list_length(targets);
	guint count = 0;
	
	for (t = targets; t; t = t->next) {
		count ++;
		OSyncObjFormatSink *format_sink = t->data;
		g_string_append(string, osync_objformat_sink_get_objformat(format_sink));
		if (size > 1 && count < size) g_string_append(string, " - ");
	}

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p:%s, %s, %p)", __func__, env, sourcedata, targets, g_string_free(string, FALSE), preferred_format ? preferred_format:"NONE", error);
	
	path = _osync_format_env_find_path_fn(env, sourcedata, _target_fn_format_sinks, _target_fn_format_sinks_reached_lastconverter, targets, preferred_format, error);
	if (!path) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, path);
	return path;
}

/*@}*/
