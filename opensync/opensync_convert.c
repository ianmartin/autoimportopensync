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
 
#include <opensync.h>
#include "opensync_internals.h"

/**
 * @defgroup OSyncConvPrivate OpenSync Conversion Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

osync_bool osync_conv_plugin_load(OSyncFormatEnv *env, char *path, OSyncError **error)
{
	/* Check if this platform supports dynamic
	 * loading of modules */
	osync_debug("OSFRM", 3, "Loading formats plugin from %s", path);
	if (!g_module_supported()) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This platform does not support loading of modules");
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		return FALSE;
	}

	/* Try to open the module or fail if an error occurs */
	GModule *plugin = g_module_open(path, G_MODULE_BIND_LOCAL);
	if (!plugin) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open plugin %s: %s", path, g_module_error());
		osync_debug("OSPLG", 0, "Unable to open plugin %s", path);
		return FALSE;
	}
	
	void (* fct_info)(OSyncFormatEnv *env) = NULL;
	void (** fct_infop)(OSyncFormatEnv *env) = &fct_info;
	if (!g_module_symbol(plugin, "get_info", (void **)fct_infop)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to open format plugin %s: %s", path, g_module_error());
		osync_debug("OSPLG", 0, "Unable to open format plugin %s", path);
		return FALSE;
	}
	env->plugins = g_list_append(env->plugins, plugin);
	
	fct_info(env);
	return TRUE;
}

OSyncFormatConverter *osync_conv_find_converter_objformat(OSyncFormatEnv *env, OSyncObjFormat *fmt_src, OSyncObjFormat *fmt_trg)
{
	GList *element = NULL;
	for (element = env->converters; element; element = element->next) {
		OSyncFormatConverter *converter = element->data;
		if (fmt_src == converter->source_format && fmt_trg == converter->target_format)
			return converter;
	}
	return NULL;
}


OSyncDataDetector *osync_conv_find_detector(OSyncFormatEnv *env, const char *origformat, const char *trgformat)
{
	GList *i;
	for (i = env->data_detectors; i; i = i->next) {
		OSyncDataDetector *det = i->data;
		if (!strcmp(det->sourceformat, origformat) && !strcmp(det->targetformat, trgformat)) {
			return det;
		}
	}
	return NULL;
}

OSyncFormatConverter *osync_conv_find_converter(OSyncFormatEnv *env, const char *sourcename, const char *targetname)
{
	g_assert(env);
	g_assert(sourcename);
	g_assert(targetname);
	
	OSyncObjFormat *fmt_src = osync_conv_find_objformat(env, sourcename);
	if (!fmt_src)
		return NULL;
	OSyncObjFormat *fmt_trg = osync_conv_find_objformat(env, targetname);
	if (!fmt_trg)
		return NULL;
	
	return osync_conv_find_converter_objformat(env, fmt_src, fmt_trg);
}

static osync_bool osync_register_unresolved_converter(OSyncFormatEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func, ConverterFlags flags)
{
	OSyncUnresolvedConverter *conv = g_malloc0(sizeof(OSyncUnresolvedConverter));
	g_assert(conv);

	conv->source_format = sourcename;
	conv->target_format = targetname;
	conv->convert_func = convert_func;
	conv->type = type;
	conv->flags = flags;
	env->unresolved_converters = g_list_append(env->unresolved_converters, conv);

	return TRUE;
}

static osync_bool _osync_conv_register_converter(OSyncFormatEnv *env, ConverterType type, OSyncObjFormat *fmt_src, OSyncObjFormat *fmt_trg, OSyncFormatConvertFunc convert_func, ConverterFlags flags)
{
	OSyncFormatConverter *converter = g_malloc0(sizeof(OSyncFormatConverter));
	g_assert(converter);
	converter->source_format = fmt_src;
	converter->target_format = fmt_trg;
	converter->convert_func = convert_func;
	converter->type = type;
	converter->flags = flags;

	env->converters = g_list_append(env->converters, converter);
	if (fmt_src->objtype)
		fmt_src->objtype->converters = g_list_append(fmt_trg->objtype->converters, converter);

	return TRUE;
}

/** Register converters that can be resolved
 *
 * Try to register the unresolved converters, and
 * remove the converters that were successfully resolved
 * and registered.
 */
static void osync_conv_resolve_converters(OSyncFormatEnv *env)
{
	GList *i, *next;
	for (i = env->unresolved_converters; i ; i = next) {
		OSyncUnresolvedConverter *conv = i->data;
		next = i->next;

		OSyncObjFormat *fmt_src = osync_conv_find_objformat(env, conv->source_format);
		OSyncObjFormat *fmt_trg = osync_conv_find_objformat(env, conv->target_format);
		if (fmt_src && fmt_trg) {
			_osync_conv_register_converter(env, conv->type,
			                               fmt_src, fmt_trg, conv->convert_func, conv->flags);
			env->unresolved_converters = g_list_delete_link(env->unresolved_converters, i);
			g_free(conv);
		}
	}
}

/** Copy the function references for is_like formats
 *
 * Copy the destroy and copy functions for the formats using osync_conv_format_set_like(),
 * after registering the format fmt.
 *
 * This is only needed due to the plugin loading order problem
 *
 * @see OSyncObjFormat::is_like
 */
static void osync_conv_resolve_is_like(OSyncFormatEnv *env, OSyncObjFormat *fmt)
{
	GList *i;
	for (i = env->objformats; i; i = i->next) {
		OSyncObjFormat *f = i->data;
		if (f->is_like && !strcmp(f->is_like, fmt->name)) {
			f->destroy_func = fmt->destroy_func;
			f->copy_func = fmt->copy_func;
		}
	}
}

/** The destroy_func to call free() on a block of data
 */
static void osync_format_malloced_destroy(char *data, size_t size)
{
	if (data)
		free(data);
}

/** The copy_func that does a malloc()/memcpy() on the data
 */
static osync_bool osync_format_malloced_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = malloc(inpsize);
	if (!*output)
		return FALSE;
	memcpy(*output, input, inpsize);
	return TRUE;
}

/** Simple copy converter function
 *
 * @see osync_conv_format_set_like()
 * */
static osync_bool conv_simple_copy(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	/*FIXME: Remove 'const' from input? */
	*output = (char*)input;
	*outpsize = inpsize;
	return TRUE;
}

/** Try to do cheap data conversion
 *
 * Call a converter trying to avoid memory copy and allocation.
 * The returned data may be only a reference to the input data,
 * in this case, *free_data will be set to zero.
 *
 * If the returned data must be destroyed later, *free_data
 * will be set to non-zero.
 */
osync_bool osync_cheap_convert(OSyncFormatConverter *converter, char *input, size_t inpsize, char **output, size_t *outsize, osync_bool *free_data)
{
	char *origdata;
	size_t origsize;
	osync_debug("OSCONV", 4, "Invoking converter to cheap conversion: %s -> %s",
			converter->source_format->name, converter->target_format->name);

	/* Duplicate the data, if the converter will take it to himself */
	if (converter->flags & CONV_TAKEOVER) {
		if (!converter->source_format->copy_func) {
			osync_debug("OSCONV", 0, "Converter %s->%s is CONV_TAKEOVER, but no copy_func", converter->source_format->name, converter->target_format->name);
			return FALSE;
		}
		if (!converter->source_format->copy_func(input, inpsize, &origdata, &origsize))
			return FALSE;
	} else {
		origdata = input;
		origsize = inpsize;
	}
	/* Call the converter */
	if (!converter->convert_func(origdata, origsize, output, outsize, NULL))
		return FALSE;

	/* Don't free the data if we got just a reference to it */
	if (converter->flags & CONV_NOCOPY)
		*free_data = FALSE;
	else
		*free_data = TRUE;

	return TRUE;
}

static void osync_conv_invoke_extensions(OSyncObjFormat *format, osync_bool convert_to, OSyncChange *change)
{
	char *data = NULL;
	int datasize = 0;
	
	GList *e;
	for (e = format->extensions; e; e = e->next) {
		OSyncFormatExtension *ext = e->data;
		if (convert_to)
			ext->conv_to(change->data, change->size, &data, &datasize, NULL);
		else
			ext->conv_from(change->data, change->size, &data, &datasize, NULL);
		if (data) {
			//The extension duplicated the data.
			if (format->destroy_func)
				format->destroy_func(change->data, change->size);
			else
				osync_debug("OSYNC", 1, "Format %s don't have a destroy function. Possible memory leak", format->name);
			change->data = data;
			change->size = datasize;
		}
	}
}

osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncChange *change, OSyncError **error)
{
	char *data = NULL;
	int datasize = 0;
	osync_bool ret = TRUE;
	if (!converter->convert_func)
		return FALSE;
	
	if (change->data) {
		//Invoke the converter and all extensions
		osync_conv_invoke_extensions(converter->source_format, FALSE, change);
		ret = converter->convert_func(change->data, change->size, &data, &datasize, error);
		
		if (converter->flags & CONV_NOCOPY) {
			/* Duplicate the returned data, as the original data will be destroyed */
			if (!converter->target_format->copy_func) {
				/* There is nothing we can do, here. The returned data is a reference, but
				 * we can't copy the data before destroying it
				 */
				osync_debug("OSYNC", 0, "Format %s don't have a copy function, but a no-copy converter was registered", converter->target_format->name);
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Format %s don't have a copy function, but a no-copy converter was registered", converter->target_format->name);
				return FALSE;
			}
			converter->target_format->copy_func(data, datasize, &data, &datasize);
		}
		/* Free the data, unless the converter took the ownership of the data */
		if (!(converter->flags & CONV_TAKEOVER)) {
			if (converter->source_format->destroy_func)
				converter->source_format->destroy_func(change->data, change->size);
			else
				osync_debug("OSYNC", 1, "Format %s don't have a destroy function. Possible memory leak", converter->source_format->name);
		}
		change->data = data;
		change->size = datasize;
		
		osync_conv_invoke_extensions(converter->target_format, TRUE, change);
	}
	osync_debug("OSYNC", 3, "Converting! replacing format %s with %s", converter->source_format->name, converter->target_format->name);
	change->format = converter->target_format;
	change->objtype = change->format->objtype;

	return ret;
}

typedef struct vertice {
	OSyncObjFormat *format;

	/** Converted data: needed
	 * because we may need to run detectors
	 * during the search
	 *
	 * @{
	 */
	char *data;
	size_t datasize;
	osync_bool free_data;
	/** @} */

	/** The converter that needs to be run
	 * on previous->data to get the
	 * vertice data
	 */
	OSyncFormatConverter *converter;

	/** Keep reference counts because
	 * the data returned by converters
	 * can be references to other data,
	 * and we can't free them too early
	 */
	size_t references;

	/** A reference to the previous vertice
	 * on the path
	 *
	 * Used when a conversion is necessary
	 */
	struct vertice *previous;

	/** The path of converters */
	GList *path;

	/** Distance data
	 * @{
	 */
	unsigned losses;
	unsigned objtype_changes;
	unsigned conversions;
	/** @} */

} vertice;

/** Compare the distance of two vertices
 *
 * First, try to minimize the losses. Then,
 * try to minimize the conversions between
 * different objtypes. Then, try to minimize
 * the total number of conversions.
 */
int compare_vertice_distance(const void *a, const void *b)
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

typedef struct conv_tree {
	OSyncFormatEnv *env;
	OSyncObjType *type;

	/* The converters that weren't reached yet */
	GList *unused;
	/* The search queue for the Breadth-first search */
	GList *search;
} conv_tree;


/** Increment a vertice reference count */
static void ref_vertice(vertice *v)
{
	v->references++;
}

/** Dereference an vertice
 */
static void deref_vertice(vertice *vertice)
{
	/* Decrement the reference count,
	 * and just return if we still
	 * have a reference
	 */
	if (--vertice->references > 0)
		return;

	g_list_free(vertice->path);
	if (vertice->free_data) {
		if (!vertice->format->destroy_func)
			osync_debug("OSCONV", 1, "Memory leak: can't free data of type %s", vertice->format->name);
		else {
			osync_debug("OSCONV", 4, "Freeing data of type %s", vertice->format->name);
			vertice->format->destroy_func(vertice->data, vertice->datasize);
		}
	}
	/* Drop the v->previous reference */
	if (vertice->previous)
		deref_vertice(vertice->previous);
	g_free(vertice);
}

/** Calculate vertice data
 *
 * This function MUST be called before any
 * referencs to vertice->data
 */
osync_bool calc_vertice_data(vertice *v)
{
	g_assert(v);

	/* The data was already calculated */
	if (v->data) return TRUE;

	/* Ask the previous vertice to calculate its data, too.
	 *
	 * FIXME: Remove recursive call. It doesn't do too much harm,
	 * but it is better to avoid it
	 */
	calc_vertice_data(v->previous);

	/* Convert the data, trying to keep references, not copies */
	osync_cheap_convert(v->converter, v->previous->data, v->previous->datasize, &v->data, &v->datasize, &v->free_data);

	/* Keep references to old data, if we have done a 'nocopy'
	 * conversion.
	 *
	 * free_data == 0 means that the returned data is a reference
	 * to the original data.
	 *
	 * So, the reference on v->previous will be kept, if
	 * free_data == 0. Otherwise, drop the reference.
	 */
	if (v->free_data) {
		deref_vertice(v->previous);
		v->previous = NULL;
	}

	return TRUE;
}

/** Returns a neighbour of the vertice ve
 *
 * Returns a new reference to te vertice. The reference
 * should be dropped using deref_vertice(), later.
 */
vertice *get_next_vertice_neighbour(OSyncFormatEnv *env, conv_tree *tree, vertice *ve)
{
	GList *c = NULL;
	OSyncObjFormat *detected_fmt = NULL;

	if (ve->format->detect_func) {
		calc_vertice_data(ve);
		ve->format->detect_func(env, ve->data, ve->datasize, &detected_fmt);
	}
		
	for (c = tree->unused; c; c = c->next) {
		OSyncFormatConverter *converter = c->data;
		OSyncObjFormat *fmt_target = converter->target_format;
		
		/* Check only valid converters, from the right format */
		if (strcmp(converter->source_format->name, ve->format->name))
			continue;

		/* If CONV_DETECTFIRST is set, check if the detector for the
		 * format was run, or there is an specific detector for
		 * this pair
		 */
		if (converter->flags & CONV_DETECTFIRST) {
			if (!detected_fmt) {
				/* No format was detected, so we need a detector specific
				 * to this target format
				 */
				const char *srcfrm = converter->source_format->name;
				const char *trgfrm = converter->target_format->name;
				OSyncDataDetector *det = osync_conv_find_detector(env, srcfrm, trgfrm);
				/* Error if no detector was found */
				if (!det) {
					osync_debug("OSCONV", 2, "Converter %s->%s marked as DETECTFIRST, but no detector found", srcfrm, trgfrm);
					continue;
				}
				/* Call the detector, and don't use the converter, if it returns FALSE */
				calc_vertice_data(ve);
				if (!det->detect_func(env, ve->data, ve->datasize))
					continue;
			} else {
				/* Convert only to the detected format, when using CONV_DETECTFIRST
				 * converters
				 */
				if (strcmp(converter->target_format->name, detected_fmt->name))
					continue;
			}
		}


		/* From this point, we already found an edge (i.e. a converter) that may
		 * be used
		 */

		/* Remove the converter from the unused list */
		tree->unused = g_list_remove(tree->unused, converter);

		/* Allocate the new neighbour */
		vertice *neigh = g_malloc0(sizeof(vertice));
		/* Start with a reference count = 1 */
		neigh->references = 1;
		neigh->converter = converter;
		neigh->format = fmt_target;
		neigh->path = g_list_copy(ve->path);
		neigh->path = g_list_append(neigh->path, converter);

		/* Distance calculation */
		neigh->conversions = ve->conversions + 1;
		neigh->losses = ve->losses;
		if (converter->type == CONVERTER_DESENCAP)
			neigh->losses++;
		neigh->objtype_changes = ve->objtype_changes;
		if (converter->source_format->objtype != converter->target_format->objtype)
			neigh->objtype_changes++;

		/* Keep the reference to the previous vertice, as we may need
		 * its data later
		 */
		ref_vertice(ve);
		neigh->previous = ve;

		return neigh;
	}
	return NULL;
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
 * The list returned on path_edges should be freed by the caller.
 *
 * Note: NEVER use the detection/conversion functions on
 *       CHANGE_DELETED changes. Converting and detecting data
 *       on changes that have no data doesn't make sense
 *
 * @see osync_conv_convert_fn(), osync_change_convert(),
 *      osync_conv_convert_fmtlist(), osync_change_convert_member_sink()
 *
 * @see target_fn_fmtlist(), target_fn_fmtnames(),
 *      target_fn_simple(), target_fn_fmtname(),
 *      target_fn_membersink(), target_fn_no_any()
 */
osync_bool osync_conv_find_path_fn(OSyncFormatEnv *env, OSyncChange *start, OSyncPathTargetFn target_fn, const void *fndata, GList/* OSyncConverter * */ **path_edges)
{
	g_assert(start->format);

	/* CHANGE_DELETED changes don't have any data to be converted. Passing
	 * CHANGE_DELETED changes through the conversion code is an error
	 */
	g_assert(start->changetype != CHANGE_DELETED);

	*path_edges = NULL;
	osync_bool ret = FALSE;
	conv_tree *tree = g_malloc0(sizeof(conv_tree));

	tree->unused = g_list_copy(env->converters);

	vertice *result = NULL;
	vertice *begin = g_malloc0(sizeof(vertice));
	begin->format = start->format;
	begin->path = NULL;
	begin->data = start->data;
	begin->datasize = start->size;
	begin->free_data = FALSE;
	begin->references = 1;
	
	tree->search = g_list_append(NULL, begin);
	
	while (g_list_length(tree->search)) {
		vertice *neighbour;

		/* Get the first vertice,
		 * and remove it from the queue
		 */
		vertice *current = g_list_first(tree->search)->data;
		tree->search = g_list_delete_link(tree->search, g_list_first(tree->search));

		osync_debug("OSCONV", 4, "Next vertice: %s. distance: (l: %d, oc: %d, c: %d).", current->format->name,
				current->losses, current->objtype_changes, current->conversions);
		/* Check if we have reached a target format */
		if (target_fn(fndata, current->format)) {
			/* Done. return the result */
			result = current;
			/* Note: the reference to 'current' will be dropped
			 * after getting the path from 'result' at
			 * the end of the function.
			 */
			break;
		}
		osync_debug("OSCONV", 4, "Looking %s neighbours.", current->format->name);
		while ((neighbour = get_next_vertice_neighbour(env, tree, current))) {
			osync_debug("OSCONV", 4, "%s neighbour: %s", current->format->name, neighbour->format->name);
			osync_debug("OSCONV", 4, "Adding %s to queue", neighbour->format->name);
			tree->search = g_list_insert_sorted(tree->search, neighbour, compare_vertice_distance);
		}
		/* Done, drop the reference to the vertice */
		deref_vertice(current);
	}
	/* Remove the references on the search queue */
	g_list_foreach(tree->search, (GFunc)deref_vertice, NULL);
	
	if (result) {
		/* Found it. Copy the conversion path */
		*path_edges = g_list_copy(result->path);
		/* Drop the reference to the result vertice */
		deref_vertice(result);
		ret = TRUE;
	}
	
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
	return ret;
}

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata, OSyncError **error)
{
	g_assert(change);
	g_assert(target_fn);
	OSyncObjFormat *source = change->format;
	osync_assert(source, "Cannot convert! change has no objformat!");
	GList *path = NULL;
	osync_bool ret = TRUE;

	/* Optimization: check if the format is already valid */
	if (target_fn(fndata, source))
		goto out;

	ret = FALSE;
	if (!osync_conv_find_path_fn(env, change, target_fn, fndata, &path)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find a conversion path to the format requested");
		goto out;
	}
	
	for (; path; path = path->next) {
		OSyncFormatConverter *converter = path->data;
		if (!osync_converter_invoke(converter, change, error)) {
			goto out_free_path;
		}
	}

	ret = TRUE;

out_free_path:
	g_list_free(path);
out:
	return ret;
}

/** Function used on path searchs for a format list
 *
 * @see osync_conv_find_path_fn(), osync_conv_convert_fmtlist()
 */
static osync_bool target_fn_fmtlist(const void *data, OSyncObjFormat *fmt)
{
	const GList/*OSyncObjFormat * */ *l = data;
	const GList *i;
	for (i = l; i; i = i->next) {
		OSyncObjFormat *f = i->data;
		if (!strcmp(fmt->name, f->name))
			return TRUE;
	}
	/* else */
	return FALSE;
}

/** Convert a change to the nearest format on a list of formats
 */
osync_bool osync_conv_convert_fmtlist(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets)
{
	return osync_conv_convert_fn(env, change, target_fn_fmtlist, targets, NULL);
}

osync_bool osync_conv_find_path_fmtlist(OSyncFormatEnv *env, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist)
{
	return osync_conv_find_path_fn(env, start, target_fn_fmtlist, targets, retlist);
}

/** Function used on a path search for a format name array
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtnames()
 */
static osync_bool target_fn_fmtnames(const void *data, OSyncObjFormat *fmt)
{
	const char * const *list = data;
	const char * const *i;
	for (i = list; *i; i++) {
		if (!strcmp(fmt->name, *i))
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
static osync_bool target_fn_simple(const void *data, OSyncObjFormat *fmt)
{
	const OSyncObjFormat *target = data;
	return target == fmt;
}

/** Function used on path searchs for a single format name
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_fmtname()
 */
static osync_bool target_fn_fmtname(const void *data, OSyncObjFormat *fmt)
{
	const char *name = data;
	return !strcmp(name, fmt->name);
}

/** Function used on path searchs for a sink on a member
 *
 * @see osync_conv_find_path_fn(), osync_change_convert_member_sink()
 */
static osync_bool target_fn_membersink(const void *data, OSyncObjFormat *fmt)
{
	const OSyncMember *memb = data;
	GList *i;
	for (i = memb->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *sink = i->data;
		if (sink->format == fmt)
			return TRUE;
	}

	/* Not found */
	return FALSE;
}

/** Convert a change to the nearest format sink on a member
 */
osync_bool osync_change_convert_member_sink(OSyncFormatEnv *env, OSyncChange *change, OSyncMember *memb)
{
	return osync_conv_convert_fn(env, change, target_fn_membersink, memb, NULL);
}

osync_bool osync_conv_objtype_is_any(const char *objstr)
{
	if (!strcmp(objstr, "data"))
		return TRUE;
	return FALSE;
}

/** Target function for osync_change_detect_objtype() search
 *
 * Returns true if the objformat is not "data"
 */
osync_bool target_fn_no_any(const void *data, OSyncObjFormat *fmt)
{
	return !osync_conv_objtype_is_any(fmt->objtype->name);
}

/*@}*/

/**
 * @defgroup OSyncConvAPI OpenSync Conversion
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * Miscanellous functions
 * 
 */
/*@{*/

/*! @brief This will create a new opensync format environment
 * 
 * The environment will hold all information about plugins, formats etc
 * 
 * @returns A pointer to a newly allocated environment. NULL on error.
 * 
 */
OSyncFormatEnv *osync_conv_env_new(void)
{
	OSyncFormatEnv *env = g_malloc0(sizeof(OSyncFormatEnv));
	env->pluginpath = g_strdup(OPENSYNC_FORMATSDIR);
	return env;
}

/*! @brief Frees a osync format environment
 * 
 * Frees a osync format environment and all resources.
 * 
 * @param env Pointer to the environment to free
 * 
 */
void osync_conv_env_free(OSyncFormatEnv *env)
{
	g_assert(env);
	//FIXME Free all format etc
	
	g_free(env->pluginpath);
	g_free(env);
}

/*! @brief Loads all format and conversion plugins
 * 
 * This command will load all plugins for the conversion system.
 * If you dont change the path before it will load the plugins
 * from the default location
 * 
 * @param env The format environment
 * @param error The location to return a error to
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_conv_env_load(OSyncFormatEnv *env, OSyncError **oserror)
{
	g_assert(env);
	g_assert(env->pluginpath);
	GDir *dir = NULL;
	GError *error = NULL;
	osync_debug("OSCONV", 3, "Trying to open formats plugin directory %s", env->pluginpath);
	
	if (!g_file_test(env->pluginpath, G_FILE_TEST_EXISTS)) {
		osync_debug("OSCONV", 3, "%s exists, but is no dir", env->pluginpath);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "directory %s does not exist", env->pluginpath);
		return FALSE;
	}
	
	dir = g_dir_open(env->pluginpath, 0, &error);
	if (!dir) {
		osync_debug("OSCONV", 0, "Unable to open format plugin directory %s: %s", env->pluginpath, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open format directory %s: %s", env->pluginpath, error->message);
		g_error_free(error);
		return FALSE;
	}
	
	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		char *filename = NULL;
		filename = g_strdup_printf ("%s/%s", env->pluginpath, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || g_pattern_match_simple("*lib.la", filename) || !g_pattern_match_simple("*.la", filename)) {
			g_free(filename);
			continue;
		}
		
		OSyncError *error = NULL;
		if (!osync_conv_plugin_load(env, filename, &error)) {
			osync_debug("OSCONV", 0, "Unable to load format plugin %s: %s", filename, error->message);
			osync_error_free(&error);
		}
		g_free(filename);
	}
	g_dir_close(dir);
	return TRUE;
}

/*! @brief Unloads a format environment
 * 
 * This unloads all plugins and frees the resources associated
 * with these plugins
 * 
 * @param env Pointer to environment
 * 
 */
void osync_conv_env_unload(OSyncFormatEnv *env)
{
	g_assert(env);
	
	GList *p;
	for (p = env->plugins; p; p = p->next) {
		GModule *plugin = p->data;
		g_module_close(plugin);
	}
	g_list_free(env->plugins);
	env->plugins = NULL;
}

osync_bool osync_conv_set_common_format(OSyncFormatEnv *env, const char *objtypestr, const char *formatname, OSyncError **error)
{
	OSyncObjType *type = osync_conv_find_objtype(env, objtypestr);
	if (!type) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to set a common format: Unable to find the object-type \"%s\"", objtypestr);
		return FALSE;
	}
	OSyncObjFormat *format = osync_conv_find_objformat(env, formatname);
	if (!format) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to set a common format: Unable to find the format \"%s\"", formatname);
		return FALSE;
	}
	type->common_format = format;
	return TRUE;
}

OSyncObjType *osync_conv_find_objtype(OSyncFormatEnv *env, const char *name)
{
	g_assert(env);
	g_assert(name);
	
	GList *element = NULL;
	for (element = env->objtypes; element; element = element->next) {
		OSyncObjType *type = element->data;
		if (!strcmp(type->name, name))
			return type;
	}
	osync_debug("CONV", 1, "Unable to find the requested objtype \"%s\"", name);
	return NULL;
}

int osync_conv_num_objtypes(OSyncFormatEnv *env)
{
	g_assert(env);
	return g_list_length(env->objtypes);
}

OSyncObjType *osync_conv_nth_objtype(OSyncFormatEnv *env, int nth)
{
	g_assert(env);
	return g_list_nth_data(env->objtypes, nth);
}

int osync_conv_num_objformats(OSyncObjType *type)
{
	g_assert(type);
	return g_list_length(type->formats);
}

OSyncObjFormat *osync_conv_nth_objformat(OSyncObjType *type, int nth)
{
	g_assert(type);
	return g_list_nth_data(type->formats, nth);
}

OSyncObjFormat *osync_conv_find_objformat(OSyncFormatEnv *env, const char *name)
{
	g_assert(env);
	g_assert(name);
	
	GList *element = NULL;
	for (element = env->objformats; element; element = element->next) {
		OSyncObjFormat *format = element->data;
		if (!strcmp(format->name, name))
			return format;
	}
	osync_debug("CONV", 1, "Unable to find the requested format \"%s\"", name);
	return NULL;
}

OSyncObjType *osync_conv_register_objtype(OSyncFormatEnv *env, const char *name)
{
	OSyncObjType *type = NULL;
	if (!(type = osync_conv_find_objtype(env, name))) {
		type = g_malloc0(sizeof(OSyncObjType));
		g_assert(type);
		type->name = g_strdup(name);
		type->env = env;
		env->objtypes = g_list_append(env->objtypes, type);
	}
	return type;
}

osync_bool osync_conv_register_converter(OSyncFormatEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func, ConverterFlags flags)
{
	if (osync_conv_find_converter(env, sourcename, targetname))
		return FALSE;

	OSyncObjFormat *fmt_src = osync_conv_find_objformat(env, sourcename);
	OSyncObjFormat *fmt_trg = osync_conv_find_objformat(env, targetname);
	if (!fmt_src || !fmt_trg)
		return osync_register_unresolved_converter(env, type, sourcename, targetname, convert_func, flags);
	else
		return _osync_conv_register_converter(env, type, fmt_src, fmt_trg, convert_func, flags);
}

void osync_conv_register_data_detector(OSyncFormatEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func)
{
	OSyncDataDetector *detector = g_malloc0(sizeof(OSyncDataDetector));
	detector->sourceformat = strdup(sourceformat);
	detector->targetformat = strdup(format);
	detector->detect_func = detect_func;

	env->data_detectors = g_list_append(env->data_detectors, detector);
}

void osync_conv_register_filter_function(OSyncFormatEnv *env, const char *name, const char *objtype, const char *format, OSyncFilterFunction hook)
{
	OSyncCustomFilter *function = g_malloc0(sizeof(OSyncCustomFilter));
	g_assert(function);
	function->name = g_strdup(name);
	function->objtype = g_strdup(objtype);
	function->format = g_strdup(format);
	function->hook = hook;
	
	env->filter_functions = g_list_append(env->filter_functions, function);
}

OSyncObjFormat *osync_conv_register_objformat(OSyncFormatEnv *env, const char *typename, const char *name)
{
	OSyncObjType *type;
	OSyncObjFormat *format = NULL;

	type = osync_conv_find_objtype(env, typename);
	if (!type)
		type = osync_conv_register_objtype(env, typename);

	if (!(format = osync_conv_find_objformat(env, name))) {
		format = g_malloc0(sizeof(OSyncObjFormat));
		g_assert(format);
		format->name = strdup(name);
		format->objtype = type;
		format->env = env;
		env->objformats = g_list_append(env->objformats, format);
		type->formats = g_list_append(type->formats, format);
	}

	/* Some converters may resolve their format names, now */
	osync_conv_resolve_converters(env);
	/* Resolve the pending osync_conv_format_set_like() definitions, too */
	osync_conv_resolve_is_like(env, format);
	return format;
}

void osync_conv_register_extension(OSyncFormatEnv *env, const char *objformatname, OSyncFormatConvertFunc conv_to_func, OSyncFormatConvertFunc conv_from_func)
{
	OSyncObjFormat *format = NULL;

	format = osync_conv_find_objformat(env, objformatname);
	if (!format) {
		osync_debug("OSCONV", 0, "You need to register the objformat first before registering the extension!");
		return;
	}

	OSyncFormatExtension *ext = g_malloc0(sizeof(OSyncFormatExtension));
	ext->format = format;
	ext->conv_to = conv_to_func;
	ext->conv_from = conv_from_func;
	
	format->extensions = g_list_append(format->extensions, ext);
}

void osync_conv_format_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func)
{
	g_assert(format);
	format->cmp_func = cmp_func;
}

void osync_conv_format_set_destroy_func(OSyncObjFormat *format, OSyncFormatDestroyFunc destroy_func)
{
	g_assert(format);
	format->destroy_func = destroy_func;
	osync_conv_resolve_is_like(format->env, format);
}

void osync_conv_format_set_copy_func(OSyncObjFormat *format, OSyncFormatCopyFunc copy_func)
{
	g_assert(format);
	format->copy_func = copy_func;
	osync_conv_resolve_is_like(format->env, format);
}

/** Set the format as a simple malloc()ed block
 *
 * This will tell that this format is a block of data that can be
 * deallocated using simple a call to free(), and copied using a simple
 * call to malloc()/memcpy(). This will set the destroy and copy functions
 * of the format to functions that use free()/malloc()/memcpy(), accordingly.
 */
void osync_conv_format_set_malloced(OSyncObjFormat *format)
{
	g_assert(format);
	osync_conv_format_set_copy_func(format, osync_format_malloced_copy);
	osync_conv_format_set_destroy_func(format, osync_format_malloced_destroy);
}

/** Set the format as behaving like another format
 *
 * This will tell that this format can be converted to/from
 * base_format by just copying the pointer and size of data.
 *
 * Calling this function implies that the copy and destroy functions
 * for the format will be the same of base_format.
 *
 * Converter flags may be specified for the converters that will be
 * registered. to_flags will contain flags for the converter
 * format -> base_format. from_flags will contain flags for the converter
 * base_format -> format. Currently, the only flags allowed on these parameters
 * are CONV_NOTLOSSY and CONV_DETECTFIRST.
 *
 * The common use of this function is for the "plain" format, as:
 *
 * osync_conv_format_set_like(myformat, "plain", CONV_NOTLOSSY, 0);
 *
 * The call above will make the format be easily converted to the "plain"
 * format, marking the myformat -> "plain" conversion as CONV_NOTLOSSY,
 * and marking the "plain" -> myformat as lossy. This is the most common
 * use, as when comparing the data, a single byte change on the block
 * will make a difference, and most of other formats (like "vcard")
 * can represent the same information as a different block of bytes
 * (i.e. the order of the fields doesn't matter).
 */
void osync_conv_format_set_like(OSyncObjFormat *format, const char *base_format, ConverterFlags to_flags, ConverterFlags from_flags)
{
	OSyncObjFormat *base;

	/* Copy the name for osync_conv_resolve_is_like() */
	format->is_like = strdup(base_format);

	/* If the format was already registered, set the functions,
	 * else it will be resolved when registered
	 */
	base = osync_conv_find_objformat(format->env, base_format);
	if (base) osync_conv_resolve_is_like(format->env, base);

	osync_conv_register_converter(format->env, CONVERTER_CONV, format->name, base_format, conv_simple_copy, CONV_NOCOPY|to_flags);
	osync_conv_register_converter(format->env, CONVERTER_CONV, base_format, format->name, conv_simple_copy, CONV_NOCOPY|from_flags);
}

/** Set the detector function for this type
 *
 * A detector function for a given format is different from a the detectors registered
 * using osync_conv_register_data_detector().
 *
 * The osync_conv_format_set_detect_func() is a function designed to return the lower
 * objformat of the data, by looking at it.
 *
 * The osync_conv_register_data_detector() function is a function that checks if a
 * given block of data can be converting to a given format.
 *
 * The osync_conv_format_set_detect_func() is more useful for
 * 'encapsulator'-like formats * that can tell the format of the data below,
 * by just looking at the data. The osync_conv_register_data_detector() functions
 * is more useful for 'generic' formats (like the "plain" format) that each
 * format to which it can be converted to (vcard, vcalendar, etc.) knows
 * how to detect the data by looking into the "plain" block.
 */
void osync_conv_format_set_detect_func(OSyncObjFormat *format, OSyncFormatDetectFunc detect_func)
{
	g_assert(format);
	format->detect_func = detect_func;
}

void osync_conv_format_set_duplicate_func(OSyncObjFormat *format, OSyncFormatDuplicateFunc dupe_func)
{
	g_assert(format);
	format->duplicate_func = dupe_func;
}

void osync_conv_format_set_create_func(OSyncObjFormat *format, OSyncFormatCreateFunc create_func)
{
	g_assert(format);
	format->create_func = create_func;
}

void osync_conv_format_set_print_func(OSyncObjFormat *format, OSyncFormatPrintFunc print_func)
{
	g_assert(format);
	format->print_func = print_func;
}

const char *osync_objtype_get_name(OSyncObjType *type)
{
	g_assert(type);
	return type->name;
}

const char *osync_objformat_get_name(OSyncObjFormat *format)
{
	g_assert(format);
	return format->name;
}

OSyncObjType *osync_objformat_get_objtype(OSyncObjFormat *format)
{
	g_assert(format);
	return format->objtype;
}

osync_bool osync_change_duplicate(OSyncChange *change)
{
	g_assert(change);
	OSyncObjFormat *format = change->format;
	osync_debug("OSCONV", 3, "Duplicating change %s with format %s\n", change->uid, format->name);
	if (!format || !format->duplicate_func)
		return FALSE;
	format->duplicate_func(change);
	return TRUE;
}

OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange)
{
	g_assert(rightchange);
	g_assert(leftchange);

	/*FIXME: Convert data if the formats are different */

	if (rightchange->changetype == leftchange->changetype) {
		if (!(rightchange->data == leftchange->data)) {
			if (!(leftchange->objtype == rightchange->objtype)) {
				osync_debug("OSCONV", 4, "Objtypes do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (leftchange->format != rightchange->format) {
				osync_debug("OSCONV", 4, "Objformats do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (!rightchange->data || !leftchange->data) {
				osync_debug("OSCONV", 4, "One change has no data\n");
				return CONV_DATA_MISMATCH;
			}
			OSyncObjFormat *format = leftchange->format;
			g_assert(format);
			
			return format->cmp_func(leftchange, rightchange);
		} else {
			osync_debug("OSCONV", 4, "OK. data point to same memory: %p, %p\n", rightchange->data, leftchange->data);
			return CONV_DATA_SAME;
		}
	} else {
		osync_debug("OSCONV", 4, "Change types do not match\n");
		return CONV_DATA_MISMATCH;
	}
}

/*! @brief Convert a change to a specific format
 * 
 * This will convert the change with its data to the specified format
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetformat To which format you want to convert to
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_simple, targetformat, error);
}

/*! @brief Convert a change to a specific format with the given name
 * 
 * This will convert the change with its data to the specified format
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetname To which format you want to convert to
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_fmtname(OSyncFormatEnv *env, OSyncChange *change, const char *targetname, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_fmtname, targetname, error);
}

/*! @brief Convert a change to some formats
 * 
 * This will convert the change with its data to one of the specified formats
 * if possible.
 * 
 * @param env The conversion environment to use
 * @param change The change to convert
 * @param targetnames NULL-Terminated array of formatnames
 * @param error The error-return location
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_change_convert_fmtnames(OSyncFormatEnv *env, OSyncChange *change, const char **targetnames, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_fmtnames, targetnames, error);
}

/*! @brief Tries to detect the object type of the given change
 * 
 * This will try to detect the object type of the data on the change
 * and return it, but not set it.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The objecttype on success, NULL otherwise
 * 
 */
OSyncObjType *osync_change_detect_objtype(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	OSyncObjFormat *format = NULL;
	if (!(format = osync_change_detect_objformat(env, change, error)))
		return NULL;
	return format->objtype;
}

/*! @brief Tries to detect the encapsulated object type of the given change
 * 
 * This will try to detect the encapsulated object type of the data on the change
 * and return it, but not set it. This will try to detect the change, deencapsulate
 * it, detect again etc until it cannot deencapsulate further.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The objecttype on success, NULL otherwise
 * 
 */
OSyncObjType *osync_change_detect_objtype_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	OSyncObjFormat *format = NULL;
	if (!(format = osync_change_detect_objformat_full(env, change, error)))
		return NULL;
	return format->objtype;
}

/*! @brief Tries to detect the format of the given change
 * 
 * This will try to detect the format of the data on the change
 * and return it, but not set it.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_change_detect_objformat(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no data");
		return NULL;
	}
	
	GList *d = NULL;
	for (d = env->data_detectors; d; d = d->next) {
		OSyncDataDetector *detector = d->data;
		if (detector->detect_func(env, change->data, change->size)) {
			OSyncObjFormat *sourceformat = osync_conv_find_objformat(env, detector->targetformat);
			return sourceformat;
		}
	}
	
	osync_error_set(error, OSYNC_ERROR_GENERIC, "None of the detectors was able to recognize this data");
	return NULL;
}

/*! @brief Tries to detect the encapsulated format of the given change
 * 
 * This will try to detect the encapsulated format of the data on the change
 * and return it, but not set it. This will try to detect the change, deencapsulate
 * it, detect again etc until it cannot deencapsulate further.
 * 
 * @param env The conversion environment to use
 * @param change The change to detect
 * @param error The error-return location
 * @returns The format on success, NULL otherwise
 * 
 */
OSyncObjFormat *osync_change_detect_objformat_full(OSyncFormatEnv *env, OSyncChange *change, OSyncError **error)
{
	OSyncObjFormat *ret;
	GList *path;
	if (!osync_conv_find_path_fn(env, change, target_fn_no_any, NULL, &path))
		return NULL;

	if (!path)
		ret = change->format;
	else {
		OSyncFormatConverter *last_converter = g_list_last(path)->data;
		g_assert(last_converter);
		ret = last_converter->target_format;
	}
	g_list_free(path);
	return ret;
}

/*@}*/
