#include <opensync.h>
#include "opensync_internals.h"

OSyncFormatEnv *osync_conv_env_new(void)
{
	OSyncFormatEnv *env = g_malloc0(sizeof(OSyncFormatEnv));
	env->pluginpath = OPENSYNC_FORMATSDIR;
	return env;
}

void osync_conv_env_free(OSyncFormatEnv *env)
{
	g_assert(env);
	g_free(env);
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

void osync_conv_open_plugin(OSyncFormatEnv *env, char *path)
{
	/* Check if this platform supports dynamic
	 * loading of modules */
	osync_debug("OSFRM", 3, "Loading formats plugin from %s", path);
	if (!g_module_supported()) {
		osync_debug("OSPLG", 0, "This platform does not support loading of modules");
		return;
	}

	/* Try to open the module or fail if an error occurs */
	GModule *plugin = g_module_open(path, 0);
	if (!plugin) {
		osync_debug("OSPLG", 0, "Unable to open plugin: %s", g_module_error());
		return;
	}
	
	void (* fct_info)(OSyncFormatEnv *env) = NULL;
	void (** fct_infop)(OSyncFormatEnv *env) = &fct_info;
	if (!g_module_symbol(plugin, "get_info", (void **)fct_infop)) {
		osync_debug("OSPLG", 0, "Unable to open format plugin %s: %s", path, g_module_error());
		return;
	}
	
	fct_info(env);
}

void osync_conv_env_load(OSyncFormatEnv *env)
{
	g_assert(env);
	g_assert(env->pluginpath);
	GDir *dir = NULL;
	GError *error = NULL;
	osync_debug("OSPLG", 3, "Trying to open formats plugin directory %s", env->pluginpath);
	
	if (!g_file_test(env->pluginpath, G_FILE_TEST_EXISTS)) {
		return;
	}
	
	dir = g_dir_open(env->pluginpath, 0, &error);
	if (error) {
		osync_debug("OSPLG", 0, "Unable to open formats plugin directory %s: %s", env->pluginpath, error->message);
		g_error_free (error);
		return;
	}
  
	if (dir) {
		const gchar *de = NULL;
		while ((de = g_dir_read_name(dir))) {
			
			char *filename = NULL;
			filename = g_strdup_printf ("%s/%s", env->pluginpath, de);
			
			if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.la", filename)) {
				continue;
			}
			
			/* Try to open the syncgroup dir*/
			osync_conv_open_plugin(env, filename);
		}
	}
}

OSyncObjType *osync_conv_find_objtype(OSyncFormatEnv *env, const char *name)
{
	g_assert(env);
	g_assert(name);
	
	GList *element = NULL;
	for (element = env->objtypes; element; element = element->next) {
		OSyncObjType *type = element->data;
		if (!strcmp(type->name, name) || osync_conv_objtype_is_any(type->name))
			return type;
	}
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

OSyncFormatConverter *osync_conf_find_converter_objformat(OSyncFormatEnv *env, OSyncObjFormat *fmt_src, OSyncObjFormat *fmt_trg)
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
	
	return osync_conf_find_converter_objformat(env, fmt_src, fmt_trg);
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

/** Simple copy converter function
 *
 * @see osync_conv_format_set_like()
 * */
static osync_bool conv_simple_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	/*FIXME: Remove 'const' from input? */
	*output = (char*)input;
	*outpsize = inpsize;
	return TRUE;
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

void osync_conv_set_common_format(OSyncFormatEnv *env, const char *objtypestr, const char *formatname)
{
	OSyncObjType *type = osync_conv_find_objtype(env, objtypestr);
	g_assert(type);
	OSyncObjFormat *format = osync_conv_find_objformat(env, formatname);
	type->common_format = format;
}

osync_bool osync_conv_duplicate_change(OSyncChange *change)
{
	g_assert(change);
	OSyncObjFormat *format = change->format;
	printf("Duplicating change %s with format %s\n", change->uid, format->name);
	if (!format || !format->duplicate_func)
		return FALSE;
	format->duplicate_func(change);
	return TRUE;
}

char *osync_conv_objtype_get_name(OSyncObjType *type)
{
	g_assert(type);
	return type->name;
}

OSyncConvCmpResult osync_conv_compare_changes(OSyncChange *leftchange, OSyncChange *rightchange)
{
	g_assert(rightchange);
	g_assert(leftchange);

	/*FIXME: Convert data if the formats are different */

	if (rightchange->changetype == leftchange->changetype) {
		if (!(rightchange->data == leftchange->data)) {
			if (!(leftchange->objtype == rightchange->objtype)) {
				printf("Objtypes do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (leftchange->format != rightchange->format) {
				printf("Objformats do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (!rightchange->data || !leftchange->data) {
				printf("One change has no data\n");
				return CONV_DATA_MISMATCH;
			}
			OSyncObjFormat *format = leftchange->format;
			g_assert(format);
			
			return format->cmp_func(leftchange, rightchange);
		} else {
			printf("OK. data point to same memory: %p, %p\n", rightchange->data, leftchange->data);
			return CONV_DATA_SAME;
		}
	} else {
		printf("Change types do not match\n");
		return CONV_DATA_MISMATCH;
	}
}

#if 0
osync_bool osync_conv_detect_next_format(OSyncFormatEnv *env, OSyncChange *change)
{
	GList *last = g_list_last(change->objformats);
	OSyncObjFormat *format = last->data;
	g_assert(format);
	if (!format->detect_func)
		return FALSE;
	//Call the format detector now.
	if (format->detect_func(env, change))
		return TRUE;
		
	//Call the property detectors now.
	GList *p = NULL;
	for (p = format->properties; p; p = p->next) {
		OSyncFormatProperty *property = p->data;
		if (property->detect_func)
			property->detect_func(env, change);
		if (change->objtype)
			return TRUE;
	}
	return FALSE;
}

osync_bool osync_conv_detect_objtype(OSyncFormatEnv *env, OSyncChange *change)
{
	g_assert(change);
	if (change->changetype == CHANGE_DELETED)
		return FALSE;
	while (!change->objtype || osync_conv_objtype_is_any(change->objtype->name)) {
		if (!osync_conv_detect_next_format(env, change)) {
			change->objtype = osync_conv_find_objtype(env, "data");
			return FALSE;
		}
	}
	return TRUE;
}
#endif

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
	/* Duplicate the data, if the converter will take it to himself */
	if (converter->flags & CONV_TAKEOVER) {
		if (!converter->source_format->copy_func) {
			osync_debug("OSFRM", 0, "Converter %s->%s is CONV_TAKEOVER, but no copy_func", converter->source_format, converter->target_format);
			return FALSE;
		}
		if (!converter->source_format->copy_func(input, inpsize, &origdata, &origsize))
			return FALSE;
	} else {
		origdata = input;
		origsize = inpsize;
	}
	/* Call the converter */
	if (!converter->convert_func(origdata, origsize, output, outsize))
		return FALSE;

	/* Don't free the data if we got just a reference to it */
	if (converter->flags & CONV_NOCOPY)
		*free_data = FALSE;
	else
		*free_data = TRUE;

	return TRUE;
}

osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncChange *change)
{
	char *data = NULL;
	int datasize = 0;
	osync_bool ret = TRUE;
	if (!converter->convert_func)
		return FALSE;
	
	if (change->data) {
		ret = converter->convert_func(change->data, change->size, &data, &datasize);
		if (converter->flags & CONV_NOCOPY) {
			/* Duplicate the returned data, as the original data will be destroyed */
			if (!converter->target_format->copy_func) {
				/* There is nothing we can do, here. The returned data is a reference, but
				 * we can't copy the data before destroying it
				 */
				osync_debug("OSYNC", 0, "Format %s don't have a copy function, but a no-copy converter was registered", converter->target_format->name);
				return FALSE;
			}
			converter->target_format->copy_func(data, datasize, &data, &datasize);
		}
		/* Free the data, unless the converter took the ownership of the data */
		if (!(converter->flags & CONV_TAKEOVER)) {
			if (converter->source_format->destroy_func)
				converter->source_format->destroy_func(change->data, change->size);
			else
				osync_debug("OSYNC", 1, "Format %s don't have a destroy function. Possible memory leak",
							converter->source_format->name);
		}
		change->data = data;
		change->size = datasize;
	}
	osync_debug("OSYNC", 3, "Converting! replacing format %s with %s", converter->source_format->name, converter->target_format->name);
	change->format = converter->target_format;
	change->objtype = change->format->objtype;

#if 0
	switch (converter->type) {
		case CONVERTER_CONV:
			change->objformats = g_list_remove(change->objformats, converter->source_format);
			change->objformats = g_list_append(change->objformats, converter->target_format);
			break;
		case CONVERTER_ENCAP:
			osync_debug("OSYNC", 3, "Encaping! adding format %s", converter->target_format->name);
			change->objformats = g_list_append(change->objformats, converter->target_format);
			break;
		case CONVERTER_DESENCAP:
			osync_debug("OSYNC", 3, "desencaping! removing format %s", converter->source_format->name);
			change->objformats = g_list_remove(change->objformats, converter->source_format);
			break;
		default:
			g_assert_not_reached();
	}
#endif
	return ret;
}

static osync_bool osync_conv_fmt_in_list(OSyncObjFormat *fmt, GList/*OSyncObjFormat * */ *l)
{
	GList *i;
	for (i = l; i; i = i->next) {
		OSyncObjFormat *f = i->data;
		if (!strcmp(fmt->name, f->name))
			return TRUE;
	}
	/* else */
	return FALSE;
}

typedef struct edge {
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

	/* Keep reference counts because
	 * the data returned by converters
	 * can be references to other data,
	 * and we can't free them too early
	 *
	 * @{
	 */
	struct edge *referenced_data;
	size_t references;
	/** @} */

	/** The path of converters */
	GList *path;

} edge;

typedef struct conv_tree {
	OSyncFormatEnv *env;
	OSyncObjType *type;

	/* The converters that weren't reached yet */
	GList *unused;
	/* The search queue for the Breadth-first search */
	GList *search;
} conv_tree;

edge *get_next_edge_neighbour(OSyncFormatEnv *env, conv_tree *tree, edge *me)
{
	GList *c = NULL;
	OSyncObjFormat *detected_fmt = NULL;
	if (me->format->detect_func)
		me->format->detect_func(env, me->data, me->datasize, &detected_fmt);
		
	for (c = tree->unused; c; c = c->next) {
		OSyncFormatConverter *converter = c->data;
		OSyncObjFormat *fmt_target = converter->target_format;
		
		/* Check only valid converters, from the right format */
		if (strcmp(converter->source_format->name, me->format->name))
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
					osync_debug("OSFRM", 2, "Converter %s->%s marked as DETECTFIRST, but no detector found", srcfrm, trgfrm);
					continue;
				}
				/* Call the detector, and don't use the converter, if it returns FALSE */
				if (!det->detect_func(env, me->data, me->datasize))
					continue;
			} else {
				/* Convert only to the detected format, when using CONV_DETECTFIRST
				 * converters
				 */
				if (strcmp(converter->target_format->name, detected_fmt->name))
					continue;
			}
		}
		tree->unused = g_list_remove(tree->unused, converter);
		edge *neighbour = g_malloc0(sizeof(edge));
		/* Start with a reference count = 1 */
		neighbour->references = 1;
		neighbour->format = fmt_target;
		neighbour->path = g_list_copy(me->path);
		neighbour->path = g_list_append(neighbour->path, converter);

		/* Convert the data, trying to keep references, not copies */
		/*FIXME: we may convert only if we need to run a detector */
		osync_cheap_convert(converter, me->data, me->datasize, &neighbour->data, &neighbour->datasize, &neighbour->free_data);
		/* Keep references to old data, to avoid
		 * destroying the edges that have
		 * references to their data
		 */
		if (!neighbour->free_data) {
			/* !free_data means that the returned data is a reference */
			me->references++;
			neighbour->referenced_data = me;
		}
		return neighbour;
	}
	return NULL;
}

/** Dereference an edge
 */
void put_edge(edge *edge)
{
	/* Decrement the reference count,
	 * and just return if we still
	 * have a reference
	 */
	if (--edge->references > 0)
		return;

	g_list_free(edge->path);
	if (edge->free_data) {
		if (!edge->format->destroy_func)
			osync_debug("OSFRM", 1, "Memory leak: can't free data of type %s", edge->format->name);
		else {
			osync_debug("OSFRM", 4, "Freeing data of type %s", edge->format->name);
			edge->format->destroy_func(edge->data, edge->datasize);
		}
	}
	if (edge->referenced_data)
		/* We are not referencing the data of another edge, anymore */
		put_edge(edge->referenced_data);
	g_free(edge);
}

osync_bool osync_conv_find_shortest_path(OSyncFormatEnv *env, GList *vertices, OSyncChange *start, GList/*OSyncObjFormat * */ *targets, GList **retlist)
{	
	*retlist = NULL;
	osync_bool ret = FALSE;
	conv_tree *tree = g_malloc0(sizeof(conv_tree));
	tree->unused = g_list_copy(vertices);
	edge *current = g_malloc0(sizeof(edge));

	edge *neighbour = NULL;
	current->format = osync_change_get_objformat(start);
	current->path = NULL;
	current->data = start->data;
	current->datasize = start->size;
	current->free_data = FALSE;
	current->references = 1;
	
	while (g_list_length(tree->unused) || g_list_length(tree->search)) {
		osync_debug("OSFRM", 4, "Searching %s neighbours", current->format->name);
		while ((neighbour = get_next_edge_neighbour(env, tree, current))) {
			osync_debug("OSFRM", 4, "%s neighbour: %s", current->format->name, neighbour->format->name);
			/* Check if we have reached a target format */
			if (osync_conv_fmt_in_list(neighbour->format, targets)) {
				//We are done!
				*retlist = neighbour->path;
				ret = TRUE;
				goto reply;
			}
			osync_debug("OSFRM", 4, "Adding %s to queue", neighbour->format->name);
			tree->search = g_list_append(tree->search, neighbour);
		}
		if (!tree->search)
			goto reply;
		put_edge(current);

		/* Get the next item on the queue */
		/*TODO: Use a priority queue to get the
		 * nearest vertices first
		 * (i.e. try to use less lossy converters)
		 */
		current = g_list_first(tree->search)->data;
		tree->search = g_list_remove(tree->search, current);

	}
	
	reply:
	g_list_foreach(tree->search, (GFunc)put_edge, NULL);
	
	if (neighbour)
		g_free(neighbour);
	if (current)
		put_edge(current);
	
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
	return ret;
}

osync_bool osync_conv_find_change_path(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncFormat* */ *formats, GList **retlist)
{
	return osync_conv_find_shortest_path(env, env->converters, change, formats, retlist);
}

#if 0
osync_bool osync_conv_desencap_change(OSyncFormatEnv *env, OSyncChange *change)
{
	GList *l = g_list_last(change->objformats);
	if (!l)
		return FALSE;
	OSyncObjFormat *sourceformat = l->data;
	l = l->prev;
	if (!l)
		return FALSE;
	OSyncObjFormat *targetformat = l->data;
	if (!sourceformat || !targetformat)
		return FALSE;
	OSyncFormatConverter *converter = osync_conf_find_converter_objformat(env, sourceformat, targetformat);
	if (!converter)
		return FALSE;
	return osync_converter_invoke(converter, change);
}
#endif

osync_bool osync_conv_detect_and_convert(OSyncFormatEnv *env, OSyncChange *change, GList/*OSyncObjFormat * */ *targets)
{
	g_assert(change);
	g_assert(targets);
	OSyncObjFormat *source = change->format;
	osync_assert(source, "Cannot convert! change has no objformat!");
	GList *path = NULL;

	if (osync_conv_fmt_in_list(source, targets))
		return TRUE;

	if (!osync_conv_find_change_path(env, change, targets, &path))
		return FALSE;

	for (; path; path = path->next) {
		OSyncFormatConverter *converter = path->data;
		if (!osync_converter_invoke(converter, change))
			return FALSE;
	}

	return TRUE;
}

osync_bool osync_conv_objtype_is_any(const char *objstr)
{
	if (!strcmp(objstr, "data"))
		return TRUE;
	return FALSE;
}

void osync_conv_register_data_detector(OSyncFormatEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func)
{
	OSyncDataDetector *detector = g_malloc0(sizeof(OSyncDataDetector));
	detector->sourceformat = strdup(sourceformat);
	detector->targetformat = strdup(format);
	detector->detect_func = detect_func;

	env->data_detectors = g_list_append(env->data_detectors, detector);
}


#if 0
osync_bool osync_conv_detect_data(OSyncFormatEnv *env, OSyncChange *change, char *data, int size)
{
	GList *d = NULL;
	const char *fmtname;
	if (!change->has_data)
		return FALSE;

	fmtname = osync_change_get_objformat(change)->name;
	for (d = env->data_detectors; d; d = d->next) {
		OSyncDataDetector *detector = d->data;
		if (!strcmp(detector->sourceformat, fmtname)) {
			if (detector->detect_func(env, data, size)) {
				change->objformats = g_list_prepend(change->objformats, osync_conv_find_objformat(env, detector->targetformat));
				return TRUE;
			}
		}
	}
	return FALSE;
}
#endif
