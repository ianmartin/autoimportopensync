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

typedef struct conv_tree {
	OSyncFormatEnv *env;
	OSyncObjType *type;

	/* The converters that weren't reached yet */
	GList *unused;
	/* The search queue for the Breadth-first search */
	GList *search;
} conv_tree;

typedef struct vertice {
	OSyncObjFormat *format;
	
	/* The invoke_decap will return a new change everytime
	 * we run it so that the original change does not get
	 * changed. We also need to track if the data of this change
	 * should be freed or if it contains a reference into data of
	 * a previous change */
	OSyncChange *change;
	osync_bool free_change_data;
	osync_bool free_change;
	/** Converted data: needed
	 * because we may need to run detectors
	 * during the search
	 *
	 * @{
	 */
	//char *data;
	//size_t datasize;
	//osync_bool free_data;
	/** @} */

	/** The converter that needs to be run
	 * on previous->data to get the
	 * vertice data
	 */
	//OSyncFormatConverter *converter;

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
	//struct vertice *previous;

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

static OSyncFormatConverter *osync_conv_find_converter_objformat(OSyncFormatEnv *env, OSyncObjFormat *fmt_src, OSyncObjFormat *fmt_trg)
{
	GList *element = NULL;
	for (element = env->converters; element; element = element->next) {
		OSyncFormatConverter *converter = element->data;
		if (fmt_src == converter->source_format && fmt_trg == converter->target_format)
			return converter;
	}
	return NULL;
}

/*static void osync_conv_invoke_extensions(OSyncObjFormat *format, osync_bool convert_to, OSyncChange *change)
{
	char *data = NULL;
	int datasize = 0;
	
	GList *e;
	for (e = format->extensions; e; e = e->next) {
		OSyncFormatExtension *ext = e->data;
		osync_bool free_input;
		if (convert_to)
			ext->conv_to(change->data, change->size, &data, &datasize, &free_input, NULL);
		else
			ext->conv_from(change->data, change->size, &data, &datasize, &free_input, NULL);
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
}*/

osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncChange *change, void *converter_data, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_converter_invoke(%p, %p, %p)", converter, change, error);
	osync_trace(TRACE_INTERNAL, "converter: Type: %i, source: %s, target %s", converter->type, converter->source_format->name, converter->target_format->name);
	char *data = NULL;
	int datasize = 0;
	osync_bool ret = TRUE;
	if ((converter->type != CONVERTER_DETECTOR && !converter->convert_func) || (converter->type == CONVERTER_DETECTOR && !converter->detect_func)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid converter");
		osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke: %s", osync_error_print(error));
		return FALSE;
	}


	if (!converter->convert_func) {
		osync_trace(TRACE_INTERNAL, "osync_converter_invoke: no convert func, just keep the same data");
		ret = TRUE;
	} else if (change->data) {
		//Invoke the converter and all extensions
		//osync_conv_invoke_extensions(converter->source_format, FALSE, change);
		osync_bool free_input = TRUE;
		if ((ret = converter->convert_func(converter_data, change->data, change->size, &data, &datasize, &free_input, error))) {
		
			if (converter->type == CONVERTER_DECAP) {
				if (!free_input) {
					/* Duplicate the returned data, as the original data will be destroyed */
					if (!converter->target_format->copy_func) {
						/* There is nothing we can do, here. The returned data is a reference, but
						 * we can't copy the data before destroying it
						 */
						osync_debug("OSYNC", 0, "Format %s don't have a copy function, but a no-copy %s->%s converter was registered", converter->target_format->name, converter->source_format->name, converter->target_format->name);
						osync_error_set(error, OSYNC_ERROR_GENERIC, "Format %s don't have a copy function, but a no-copy converter was registered", converter->target_format->name);
						osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke: %s", osync_error_print(error));
						return FALSE;
					}
					converter->target_format->copy_func(data, datasize, &data, &datasize);
				}
			}
			/* Free the data, unless the converter took the ownership of the data */
			if (free_input) {
				if (converter->source_format->destroy_func) {
					converter->source_format->destroy_func(change->data, change->size);
				} else
					osync_debug("OSYNC", 1, "Format %s don't have a destroy function. Possible memory leak", converter->source_format->name);
			}
			change->data = data;
			change->size = datasize;
			
			//osync_conv_invoke_extensions(converter->target_format, TRUE, change);
		}
	}
	
	if (ret) {
		osync_debug("OSYNC", 3, "Converting! replacing format %s with %s", converter->source_format->name, converter->target_format->name);
		change->format = converter->target_format;
		change->objtype = osync_change_get_objformat(change)->objtype;
		osync_trace(TRACE_EXIT, "osync_converter_invoke: TRUE");
	} else
		osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke: %s", osync_error_print(error));
	return ret;
}

static OSyncChange *osync_converter_invoke_decap(OSyncFormatConverter *converter, OSyncChange *change, osync_bool *free_output)
{
	osync_trace(TRACE_ENTRY, "osync_converter_invoke_decap(%p, %p, %p)", converter, change, free_output);
	
	*free_output = FALSE;
	
	if (!converter->convert_func) {
		osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke_decap: No convert function");
		return NULL;
	}
	
	if (converter->type != CONVERTER_DECAP) {
		osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke_decap: Not a decap");
		return NULL;
	}
	
	OSyncChange *new_change = osync_change_new();
	
	
	if (change->data) {
		//Invoke the converter and all extensions
		OSyncError *error = NULL;
		if (!converter->convert_func(NULL, change->data, change->size, &(new_change->data), &(new_change->size), free_output, &error)) {
			osync_trace(TRACE_EXIT_ERROR, "osync_converter_invoke_decap", osync_error_print(&error));
			osync_error_free(&error);
			return NULL;
		}
		new_change->has_data = change->has_data;
	}
	osync_debug("OSYNC", 3, "Converting! replacing format %s with %s", converter->source_format->name, converter->target_format->name);
	new_change->format = converter->target_format;
	new_change->objtype = osync_change_get_objformat(new_change)->objtype;
	osync_trace(TRACE_EXIT, "osync_converter_invoke_decap: %p", new_change);
	return new_change;
}

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

/** Increment a vertice reference count */
/*static void ref_vertice(vertice *v)
{
	v->references++;
}*/

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
	if (vertice->change && vertice->free_change) {
		if (vertice->free_change_data)
			osync_change_free_data(vertice->change);
		osync_change_free(vertice->change);
	}

	g_free(vertice);
}

/** Calculate vertice data
 *
 * This function MUST be called before any
 * referencs to vertice->data
 */
/*osync_bool calc_vertice_data(vertice *v)
{
	g_assert(v);


	if (v->data) return TRUE;


	calc_vertice_data(v->previous);

	osync_cheap_convert(v->converter, v->previous->data, v->previous->datasize, &v->data, &v->datasize, &v->free_data);


	if (v->free_data) {
		deref_vertice(v->previous);
		v->previous = NULL;
	}

	return TRUE;
}*/

/** Returns a neighbour of the vertice ve
 *
 * Returns a new reference to te vertice. The reference
 * should be dropped using deref_vertice(), later.
 */
vertice *get_next_vertice_neighbour(OSyncFormatEnv *env, conv_tree *tree, vertice *ve)
{
	GList *c = NULL;
	osync_trace(TRACE_ENTRY, "get_next_vertice_neighbour(%p, %p, %p:%s)", env, tree, ve, ve->format ? ve->format->name : "None");
	//OSyncObjFormat *detected_fmt = NULL;

	/*if (ve->format->detect_func) {
		calc_vertice_data(ve);
		ve->format->detect_func(env, ve->data, ve->datasize, &detected_fmt);
	}*/
		
	for (c = tree->unused; c; c = c->next) {
		OSyncFormatConverter *converter = c->data;
		OSyncObjFormat *fmt_target = converter->target_format;
		
		/* Check only valid converters, from the right format */
		if (strcmp(converter->source_format->name, ve->format->name))
			continue;

		// If the converter type is a detector we need to know wether the input is correct
		if (converter->detect_func) {
			if (!ve->change) {
				osync_trace(TRACE_INTERNAL, "Detector for converter from %s to %s would be called, but we haven't converted the data to the source format for the converter", converter->source_format->name, converter->target_format->name);
				continue;
			}
			if (!converter->detect_func(env, ve->change->data, ve->change->size)) {
				osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: FALSE", converter->source_format->name, converter->target_format->name);
				continue;
			}
			osync_trace(TRACE_INTERNAL, "Invoked detector for converter from %s to %s: TRUE", converter->source_format->name, converter->target_format->name);
		}

		OSyncChange *new_change = NULL;
		osync_bool free_output = FALSE;
		if (converter->type == CONVERTER_DECAP) {
			if (!ve->change) {
				osync_trace(TRACE_INTERNAL, "Desencapsulator from %s to %s would be called, but we haven't converted the data to the source format for the converter", converter->source_format->name, converter->target_format->name);
				continue;
			}
			if (!(new_change = osync_converter_invoke_decap(converter, ve->change, &free_output)))
				continue;
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
		neigh->format = fmt_target;
		neigh->path = g_list_copy(ve->path);
		neigh->path = g_list_append(neigh->path, converter);
		if (new_change) {
			neigh->change = new_change;
			neigh->free_change_data = free_output;
		}

		/* Distance calculation */
		neigh->conversions = ve->conversions + 1;
		neigh->losses = ve->losses;
		if (converter->type == CONVERTER_DECAP)
			neigh->losses++;
		neigh->objtype_changes = ve->objtype_changes;
		if (converter->source_format->objtype != converter->target_format->objtype)
			neigh->objtype_changes++;

		osync_trace(TRACE_EXIT, "get_next_vertice_neighbour: %p:%s", neigh, neigh->format ? neigh->format->name : "None");
		return neigh;
	}
	osync_trace(TRACE_EXIT, "get_next_vertice_neighbour: None found");
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
static osync_bool osync_conv_find_path_fn(OSyncFormatEnv *env, OSyncChange *start, OSyncPathTargetFn target_fn, const void *fndata, GList/* OSyncConverter * */ **path_edges)
{
	osync_trace(TRACE_ENTRY, "osync_conv_find_path_fn(%p, %p, %p, %p, %p)", env, start, target_fn, fndata, path_edges);
	
	g_assert(start->format);

	*path_edges = NULL;
	osync_bool ret = FALSE;
	vertice *result = NULL;

	//Vertice = Spitze = Format
	//edge = Kante = Converter

	//Make a new search tree
	conv_tree *tree = g_malloc0(sizeof(conv_tree));
	tree->unused = g_list_copy(env->converters);
	
	//We make our starting point (which is the current format of the
	//change of course
	vertice *begin = g_malloc0(sizeof(vertice));
	begin->format = start->format;
	begin->path = NULL;
	begin->references = 1;
	begin->change = start;
	begin->free_change_data = FALSE;
	begin->free_change = FALSE;
	
	tree->search = g_list_append(NULL, begin);
	
	while (g_list_length(tree->search)) {
		vertice *neighbour = NULL;

		//Get the first vertice and remove it from the queue
		vertice *current = tree->search->data;
		tree->search = g_list_remove(tree->search, current);
		
		osync_debug("OSCONV", 4, "Next vertice: %s.", current->format->name);
		/* Check if we have reached a target format */
		if (target_fn(fndata, current->format)) {
			/* Done. return the result */
			result = current;
			break;
		}
		osync_debug("OSCONV", 4, "Looking at %s's neighbours.", current->format->name);
		while ((neighbour = get_next_vertice_neighbour(env, tree, current))) {
			osync_debug("OSCONV", 4, "%s's neighbour: %s", current->format->name, neighbour->format->name);
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
		goto free_tree;
	}
	
free_tree:
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
	if (ret)
		osync_trace(TRACE_EXIT, "osync_conv_find_path_fn: TRUE");
	else
		osync_trace(TRACE_EXIT_ERROR, "osync_conv_find_path_fn: FALSE");
	return ret;
}

osync_bool osync_conv_convert_fn(OSyncFormatEnv *env, OSyncChange *change, OSyncPathTargetFn target_fn, const void *fndata, const char *extension_name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_conv_convert_fn(%p, %p, %p, %p, %p)", env, change, target_fn, fndata, error);
	g_assert(change);
	g_assert(target_fn);
	OSyncObjFormat *source = change->format;
	osync_assert(source, "Cannot convert! change has no objformat!");
	GList *path = NULL;
	osync_bool ret = TRUE;

	/* Optimization: check if the format is already valid */
	if (target_fn(fndata, source)) {
		osync_trace(TRACE_EXIT, "osync_conv_convert_fn: Target already valid");
		return TRUE;
	}

	//We can convert the deleted change directly since it has no data
	if (change->changetype == CHANGE_DELETED) {
		change->format = osync_change_get_initial_objformat(change);
		change->objtype = osync_change_get_objformat(change)->objtype;
		osync_trace(TRACE_EXIT, "osync_conv_convert_fn: converted deleted change");
		return TRUE;
	}
	
	ret = FALSE;
	if (!osync_conv_find_path_fn(env, change, target_fn, fndata, &path)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find a conversion path to the format requested");
		osync_trace(TRACE_EXIT_ERROR, "osync_conv_convert_fn: %s", osync_error_print(error));
		goto out;
	}
	
	for (; path; path = path->next) {
		OSyncFormatConverter *converter = path->data;

		osync_trace(TRACE_INTERNAL, "initialize converter: %p", converter->init_func);
		
		//Initialize the converter
		void *converter_data = NULL;
		if (converter->init_func)
			converter_data = converter->init_func();
		
		if (extension_name) {
			osync_trace(TRACE_INTERNAL, "initialize extension: %s", extension_name);
			
			//Initialize the requested extension
			OSyncFormatExtension *extension = osync_conv_find_extension(env, converter->source_format, converter->target_format, extension_name);
			osync_trace(TRACE_INTERNAL, "extension: %p", extension);
			if (extension)
				extension->init_func(converter_data);
		} else {
			
			osync_trace(TRACE_INTERNAL, "initialize all extensions");
			//Initialize all available from extensions
			GList *e;
			for (e = env->extensions; e; e = e->next) {
				OSyncFormatExtension *extension = e->data;
				osync_trace(TRACE_INTERNAL, "extension: %s", extension->name);
				osync_trace(TRACE_INTERNAL, "%p:%p %p:%p", extension->from_format, converter->source_format, extension->to_format, converter->target_format);
				if (extension->from_format == converter->source_format && extension->to_format == converter->target_format)
					extension->init_func(converter_data);
			}
		}
		
		if (!osync_converter_invoke(converter, change, converter_data, error)) {
			osync_trace(TRACE_EXIT_ERROR, "osync_conv_convert_fn: %s", osync_error_print(error));
			goto out_free_path;
		}
		
		//Finalize the converter data
		if (converter->fin_func)
			converter->fin_func(converter_data);
		
	}
	ret = TRUE;

	osync_trace(TRACE_EXIT, "osync_conv_convert_fn: TRUE");
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
	return osync_conv_convert_fn(env, change, target_fn_fmtlist, targets, NULL, NULL);
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

/** Target function for osync_change_detect_objtype() search
 *
 * Returns true if the objformat is not "data"
 */
/*static osync_bool target_fn_no_any(const void *data, OSyncObjFormat *fmt)
{
	return !osync_conv_objtype_is_any(fmt->objtype->name);
}*/

/** Convert a change to the nearest format sink on a member
 */
osync_bool osync_change_convert_member_sink(OSyncFormatEnv *env, OSyncChange *change, OSyncMember *member, OSyncError **error)
{
	return osync_conv_convert_fn(env, change, target_fn_membersink, member, member->extension, error);
}

osync_bool osync_conv_objtype_is_any(const char *objstr)
{
	if (!strcmp(objstr, "data"))
		return TRUE;
	return FALSE;
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
OSyncFormatEnv *osync_conv_env_new(OSyncEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	OSyncFormatEnv *conv_env = g_malloc0(sizeof(OSyncFormatEnv));
	GList *o;
	
	//Now we resolve all format plugin stuff for the conv env
	//First the objecttypes
	OSyncObjType *type = NULL;
	for (o = env->objtype_templates; o; o = o->next) {
		OSyncObjTypeTemplate *otempl = o->data;
		type = g_malloc0(sizeof(OSyncObjType));
		type->name = g_strdup(otempl->name);
		type->env = conv_env;
		conv_env->objtypes = g_list_append(conv_env->objtypes, type);
	}
	
	//The formats
	GList *f = NULL;
	for (f = env->format_templates; f; f = f->next) {
		OSyncObjFormatTemplate *ftempl = f->data;
		OSyncObjType *type = osync_conv_find_objtype(conv_env, ftempl->objtype);
		g_assert(type);
		OSyncObjFormat *format = g_malloc0(sizeof(OSyncObjFormat));
		format->env = conv_env;
		format->name = g_strdup(ftempl->name);
		format->objtype = type;
		
		format->cmp_func = ftempl->cmp_func;
		format->merge_func = ftempl->merge_func;
		format->duplicate_func = ftempl->duplicate_func;
		format->copy_func = ftempl->copy_func;
		format->create_func = ftempl->create_func;
		format->destroy_func = ftempl->destroy_func;
		format->print_func = ftempl->print_func;
		type->formats = g_list_append(type->formats, format);
		conv_env->objformats = g_list_append(conv_env->objformats, format);
	}
	
	//The extension
	GList *i;
	for (i = env->extension_templates; i; i = i->next) {
		OSyncFormatExtensionTemplate *extension_template = i->data;
		OSyncObjFormat *from_format = osync_conv_find_objformat(conv_env, extension_template->from_formatname);
		OSyncObjFormat *to_format = osync_conv_find_objformat(conv_env, extension_template->to_formatname);
		if (!from_format || !to_format)
			continue;

		OSyncFormatExtension *extension = g_malloc0(sizeof(OSyncFormatExtension));
		extension->name = g_strdup(extension_template->name);
		extension->init_func = extension_template->init_func;
		extension->from_format = from_format;
		extension->to_format = to_format;
		
		conv_env->extensions = g_list_append(conv_env->extensions, extension);
	}
	
	//Converter templates
	for (i = env->converter_templates; i; i = i->next) {
		OSyncConverterTemplate *convtmpl = i->data;

		osync_trace(TRACE_INTERNAL, "New converter from %s to %s", convtmpl->source_format, convtmpl->target_format);

		OSyncObjFormat *fmt_src = osync_conv_find_objformat(conv_env, convtmpl->source_format);
		OSyncObjFormat *fmt_trg = osync_conv_find_objformat(conv_env, convtmpl->target_format);
		if (!fmt_src || !fmt_trg)
			continue;
		OSyncFormatConverter *converter = g_malloc0(sizeof(OSyncFormatConverter));
		converter->source_format = fmt_src;
		converter->target_format = fmt_trg;
		converter->convert_func = convtmpl->convert_func;
		converter->type = convtmpl->type;
		converter->init_func = convtmpl->init_func;
		
		conv_env->converters = g_list_append(conv_env->converters, converter);
	}
	
	//The detectors
	for (i = env->data_detectors; i; i = i->next) {
		OSyncDataDetector *detector = i->data;
		OSyncFormatConverter *converter = osync_conv_find_converter(conv_env, detector->sourceformat, detector->targetformat);
		if (!converter) {
			OSyncObjFormat *fmt_src = osync_conv_find_objformat(conv_env, detector->sourceformat);
			OSyncObjFormat *fmt_trg = osync_conv_find_objformat(conv_env, detector->targetformat);
			if (!fmt_src || !fmt_trg)
				continue;
			converter = g_malloc0(sizeof(OSyncFormatConverter));
			converter->source_format = fmt_src;
			converter->target_format = fmt_trg;
			converter->type = CONVERTER_DETECTOR;
		}
		converter->detect_func = detector->detect_func;
		conv_env->converters = g_list_append(conv_env->converters, converter);
	}
		
	//The filters
	conv_env->filter_functions = g_list_copy(env->filter_functions);

	osync_conv_set_common_format(conv_env, "contact", "xml-contact", NULL);
	osync_conv_set_common_format(conv_env, "event", "xml-event", NULL);
	osync_conv_set_common_format(conv_env, "todo", "xml-todo", NULL);
	osync_conv_set_common_format(conv_env, "note", "xml-note", NULL);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, conv_env);
	return conv_env;
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

	//We need to go through the loaded objtypes and free them.

	g_free(env);
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

OSyncFormatExtension *osync_conv_find_extension(OSyncFormatEnv *env, OSyncObjFormat *from_format, OSyncObjFormat *to_format, const char *extension_name)
{
	g_assert(env);
	g_assert(extension_name);

	GList *i = NULL;
	for (i = env->extensions; i; i = i->next) {
		OSyncFormatExtension *extension = i->data;
		osync_trace(TRACE_INTERNAL, "comparing format %p:%p %p:%p name %s:%s", extension->from_format, from_format, extension->to_format, to_format, extension->name, extension_name);
		if ((extension->from_format == from_format || !from_format) && (extension->to_format == to_format || !to_format) && !strcmp(extension->name, extension_name))
			return extension;
	}
	return NULL;
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
	osync_trace(TRACE_ENTRY, "osync_change_compare(%p, %p)", leftchange, rightchange);
	
	g_assert(rightchange);
	g_assert(leftchange);

	OSyncError *error = NULL;
	if (!osync_change_convert_to_common(leftchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare: %s", osync_error_print(&error));
		osync_error_free(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}
	if (!osync_change_convert_to_common(rightchange, &error)) {
		osync_trace(TRACE_INTERNAL, "osync_change_compare: %s", osync_error_print(&error));
		osync_error_free(&error);
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Could not convert leftchange to common format");
		return CONV_DATA_MISMATCH;
	}

	if (rightchange->changetype == leftchange->changetype) {
		if (!(rightchange->data == leftchange->data)) {
			if (!(osync_change_get_objtype(leftchange) == osync_change_get_objtype(rightchange))) {
				osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Objtypes do not match");
				return CONV_DATA_MISMATCH;
			}
			if (leftchange->format != rightchange->format) {
				osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Objformats do not match");
				return CONV_DATA_MISMATCH;
			}
			if (!rightchange->data || !leftchange->data) {
				osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: One change has no data");
				return CONV_DATA_MISMATCH;
			}
			OSyncObjFormat *format = leftchange->format;
			g_assert(format);
			
			OSyncConvCmpResult ret = format->cmp_func(leftchange, rightchange);
			osync_trace(TRACE_EXIT, "osync_change_compare: %i", ret);
			return ret;
		} else {
			osync_trace(TRACE_EXIT, "osync_change_compare: SAME: OK. data point to same memory");
			return CONV_DATA_SAME;
		}
	} else {
		osync_trace(TRACE_EXIT, "osync_change_compare: MISMATCH: Change types do not match");
		return CONV_DATA_MISMATCH;
	}
}

osync_bool osync_change_convert_extension(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat, const char *extension_name, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_convert(%p, %p, %p:%s, %s, %p)", env, change, targetformat, targetformat ? targetformat->name : "NONE", extension_name, error);
	if (osync_conv_convert_fn(env, change, target_fn_simple, targetformat, extension_name, error)) {
		osync_trace(TRACE_EXIT, "osync_change_convert: TRUE");
		return TRUE;
	} else {
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert: %s", osync_error_print(error));
		return FALSE;
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
	return osync_change_convert_extension(env, change, targetformat, NULL, error);
}

osync_bool osync_change_copy_data(OSyncChange *source, OSyncChange *target, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_copy_data(%p, %p, %p)", source, target, error);
	
	OSyncObjFormat *format = NULL;
	format = source->format;
	if (!format)
		format = target->format;
	
	if (!format || !format->copy_func) {
		osync_trace(TRACE_INTERNAL, "We cannot copy the change, falling back to memcpy");
		target->data = g_malloc0(sizeof(char) * source->size);
		memcpy(target->data, source->data, source->size);
		target->size = source->size;
	} else {
		if (!format->copy_func(source->data, source->size, &(target->data), &(target->size))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Something went wrong during copying");
			osync_trace(TRACE_EXIT_ERROR, "osync_change_copy_data: %s", osync_error_print(error));
			return FALSE;
		}
	}
	
	osync_trace(TRACE_EXIT, "osync_change_copy_data");
	return TRUE;
}

OSyncChange *osync_change_copy(OSyncChange *source, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_copy(%p, %p)", source, error);
	g_assert(source);
	
	OSyncChange *newchange = osync_change_new();
	newchange->uid = g_strdup(source->uid);
	newchange->hash = g_strdup(source->hash);
	
	newchange->has_data = source->has_data;
	newchange->changetype = source->changetype;
	newchange->format = osync_change_get_objformat(source);
	newchange->objtype = osync_change_get_objtype(source);
	newchange->sourceobjtype = g_strdup(osync_change_get_objtype(source)->name);
	newchange->is_detected = source->is_detected;
	newchange->changes_db = source->changes_db;
	
	if (!osync_change_copy_data(source, newchange, error)) {
		osync_change_free(newchange);
		osync_trace(TRACE_EXIT_ERROR, "osync_change_copy: %s", osync_error_print(error));
		return NULL;
	}

	osync_trace(TRACE_EXIT, "osync_change_copy: %p", newchange);
	return newchange;
}

osync_bool osync_change_convert_to_common(OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_change_convert_to_common(%p, %p)", change, error);
	
	if (!osync_change_get_objtype(change)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no objtype");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert_to_common: %s", osync_error_print(error));
		return FALSE;
	}
	OSyncFormatEnv *env = osync_change_get_objtype(change)->env;
	
	if (!osync_change_get_objtype(change)->common_format) {
		osync_trace(TRACE_EXIT, "osync_change_convert_to_common: No common format set");
		return TRUE;
	}
	
	osync_trace(TRACE_INTERNAL, "Converting from %s to %s", osync_change_get_objformat(change)->name, osync_change_get_objtype(change)->common_format->name);
	
	if (osync_change_convert(env, change, osync_change_get_objtype(change)->common_format, error)) {
		osync_trace(TRACE_EXIT, "osync_change_convert_to_common: TRUE");
		return TRUE;
	} else {
		osync_trace(TRACE_EXIT_ERROR, "osync_change_convert_to_common: %s", osync_error_print(error));
		return FALSE;
	}
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
	return osync_conv_convert_fn(env, change, target_fn_fmtname, targetname, NULL, error);
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
	return osync_conv_convert_fn(env, change, target_fn_fmtnames, targetnames, NULL, error);
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
	osync_trace(TRACE_ENTRY, "osync_change_detect_objformat(%p, %p, %p)", env, change, error);
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no data");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
		return NULL;
	}
	
	//Run all datadetectors for our source type
	GList *d = NULL;
	for (d = env->converters; d; d = d->next) {
		OSyncFormatConverter *converter = d->data;
		if (!strcmp(converter->source_format->name, osync_change_get_objformat(change)->name)) {
			if (converter->detect_func && converter->detect_func(env, change->data, change->size)) {
				osync_trace(TRACE_EXIT, "osync_change_detect_objformat: %p:%s", converter->target_format, converter->target_format->name);
				return converter->target_format;
			}
		}
	}
	
	osync_error_set(error, OSYNC_ERROR_GENERIC, "None of the detectors was able to recognize this data");
	osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
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
	osync_trace(TRACE_ENTRY, "osync_change_detect_objformat_full(%p, %p, %p)", env, change, error);
	if (!change->has_data) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "The change has no data");
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat: %s", osync_error_print(error));
		return NULL;
	}
	OSyncChange *new_change = change;
	
	//Try to decap the change as far as possible
	while (1) {
		GList *d = NULL;
		for (d = env->converters; d; d = d->next) {
			OSyncFormatConverter *converter = d->data;
			if (!strcmp(converter->source_format->name, osync_change_get_objformat(change)->name) && converter->type == CONVERTER_DECAP) {
				osync_bool free_output = FALSE;
				if (!(new_change = osync_converter_invoke_decap(converter, new_change, &free_output))) {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to decap the change");
					osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat_full: %s", osync_error_print(error));
					return NULL;
				}
				continue;
			}
		}
		break;
	}

	OSyncObjFormat *ret = osync_change_detect_objformat(env, new_change, error);
	if (!ret)
		osync_trace(TRACE_EXIT_ERROR, "osync_change_detect_objformat_full: %s", osync_error_print(error));
	else
		osync_trace(TRACE_EXIT, "osync_change_detect_objformat_full: %p:%s", ret, ret->name);
	return ret;
}

/*@}*/
