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
	
	void (* fct_info)(OSyncFormatEnv *env);
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
	GDir *dir;
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
	
	GList *element;
	for (element = env->objtypes; element; element = element->next) {
		OSyncObjType *type = element->data;
		if (!strcmp(type->name, name))
			return type;
	}
	return NULL;
}

OSyncObjType *osync_conv_register_objtype(OSyncFormatEnv *env, const char *name)
{
	OSyncObjType *type;
	if (!(type = osync_conv_find_objtype(env, name))) {
		type = g_malloc0(sizeof(OSyncObjType));
		g_assert(type);
		type->name = g_strdup(name);
		type->env = env;
		type->write = TRUE;
		type->read = TRUE;
		type->enabled = TRUE;
		env->objtypes = g_list_append(env->objtypes, type);
	}
	return type;
}

OSyncFormatConverter *osync_conf_find_converter_objformat(OSyncFormatEnv *env, OSyncObjFormat *fmt_src, OSyncObjFormat *fmt_trg)
{
	GList *element;
	for (element = env->converters; element; element = element->next) {
		OSyncFormatConverter *converter = element->data;
		if (fmt_src == converter->source_format && fmt_trg == converter->target_format)
			return converter;
	}
	return NULL;
}

OSyncFormatConverter *osync_conv_find_converter(OSyncFormatEnv *env, char *sourcename, char *targetname)
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

OSyncFormatConverter *osync_conv_register_converter(OSyncObjType *type, ConverterType convtype, char *sourcename, char *targetname, OSyncFormatConvertFunc convert_func)
{
	OSyncFormatConverter *converter;
	if (!(converter = osync_conv_find_converter(type->env, sourcename, targetname))) {
		converter = g_malloc0(sizeof(OSyncFormatConverter));
		g_assert(converter);
		OSyncObjFormat *fmt_src = osync_conv_find_objformat(type->env, sourcename);
		if (!fmt_src)
			return NULL;
		OSyncObjFormat *fmt_trg = osync_conv_find_objformat(type->env, targetname);
		if (!fmt_trg)
			return NULL;
		
		converter->source_format = fmt_src;
		converter->target_format = fmt_trg;
		converter->convert_func = convert_func;
		converter->type = convtype;
		type->env->converters = g_list_append(type->env->converters, converter);
	}
	type->converters = g_list_append(type->converters, converter);
	return converter;
}

OSyncObjFormat *osync_conv_find_objformat(OSyncFormatEnv *env, const char *name)
{
	g_assert(env);
	g_assert(name);
	
	GList *element;
	for (element = env->objformats; element; element = element->next) {
		OSyncObjFormat *format = element->data;
		if (!strcmp(format->name, name))
			return format;
	}
	return NULL;
}

OSyncObjFormat *osync_conv_register_objformat(OSyncObjType *type, const char *name)
{
	OSyncObjFormat *format;
	if (!(format = osync_conv_find_objformat(type->env, name))) {
		format = g_malloc0(sizeof(OSyncObjFormat));
		g_assert(format);
		format->name = strdup(name);
		type->env->objformats = g_list_append(type->env->objformats, format);
	}
	type->formats = g_list_append(type->formats, format);
	return format;
}

void osync_conv_format_set_compare_func(OSyncObjFormat *format, OSyncFormatCompareFunc cmp_func)
{
	g_assert(format);
	format->cmp_func = cmp_func;
}

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

void osync_conv_format_set_functions(OSyncObjFormat *format, OSyncFormatFunctions functions)
{
	g_assert(format);
	format->functions = functions;
}

void osync_conv_set_common_format(OSyncFormatEnv *env, const char *formatname)
{
	
}

void osync_conv_duplicate_change(OSyncChange *change)
{
	GList *l = g_list_last(change->objformats);
	OSyncObjFormat *format = l->data;
	if (!format || !format->duplicate_func)
			return;
	do {
		format->duplicate_func(change);
	} while (!osync_member_uid_is_unique(change->member, change->uid));
}

char *osync_conv_objtype_get_name(OSyncObjType *type)
{
	g_assert(type);
	return type->name;
}

/*OSyncConvCmpResult osync_conv_compare(OSyncObjFormat *format, const char *leftinput, int leftinpsize, char *rightinput, int rightinpsize)
{
	g_assert(format);
	g_assert(format->cmp_func);
	return format->cmp_func(leftinput, leftinpsize, rightinput, rightinpsize);
}*/

OSyncConvCmpResult osync_conv_compare_changes(OSyncChange *leftchange, OSyncChange *rightchange)
{
	if (rightchange->changetype == leftchange->changetype) {
		if (!(rightchange->data == leftchange->data)) {
			if (!(leftchange->objtype == rightchange->objtype)) {
				printf("Objtypes do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (!(g_list_last(leftchange->objformats)->data == g_list_last(leftchange->objformats)->data)) {
				printf("Objformats do not match\n");
				return CONV_DATA_MISMATCH;
			}
			if (!rightchange->data || !leftchange->data) {
				printf("One change has no data\n");
				return CONV_DATA_MISMATCH;
			}
			OSyncObjFormat *format;
			format = g_list_last(leftchange->objformats)->data;
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

osync_bool osync_conv_detect_next_format(OSyncFormatEnv *env, OSyncChange *change)
{
	GList *last = g_list_last(change->objformats);
	OSyncObjFormat *format = last->data;
	g_assert(format);
	g_assert(format->detect_func);
	//Call the format detector now.
	format->detect_func(env, change);
		
	if (change->objtype)
		return TRUE;
		
	//Call the property detectors now.
	GList *p;
	for (p = format->properties; p; p = p->next) {
		OSyncFormatProperty *property = p->data;
		if (property->detect_func)
			property->detect_func(env, change);
		if (change->objtype)
			return TRUE;
	}
	return FALSE;
}

typedef struct edge {
	OSyncObjFormat *format;
	GList *path;
} edge;

typedef struct conv_tree {
	OSyncObjType *type;
	GList *unused;
	GList *search;
} conv_tree;

edge *next_edge_neighbour(conv_tree *tree, edge *me)
{
	GList *c = NULL;
	for (c = tree->unused; c; c = c->next) {
		OSyncFormatConverter *converter = c->data;
		OSyncObjFormat *fmt_target = converter->target_format;
		if (!strcmp(converter->source_format->name, me->format->name)) {
			tree->unused = g_list_remove(tree->unused, converter);
			edge *neighbour = g_malloc0(sizeof(edge));
			neighbour->format = fmt_target;
			neighbour->path = g_list_copy(me->path);
			neighbour->path = g_list_append(neighbour->path, converter);
			return neighbour;
		}
	}
	return NULL;
}

void free_edge(edge *edge)
{
	g_list_free(edge->path);
	g_free(edge);
}

osync_bool osync_conv_find_shortest_path(GList *vertices, OSyncObjFormat *start, OSyncObjFormat *end, GList **retlist)
{	
	*retlist = NULL;
	osync_bool ret = FALSE;
	conv_tree *tree = g_malloc0(sizeof(conv_tree));
	tree->unused = vertices;
	edge *current = g_malloc0(sizeof(edge));

	edge *neighbour = NULL;
	current->format = start;
	current->path = NULL;
	
	while (g_list_length(tree->unused) || g_list_length(tree->search)) {
		while ((neighbour = next_edge_neighbour(tree, current))) {
			if (!strcmp(neighbour->format->name, end->name)) {
				//We are done!
				*retlist = neighbour->path;
				ret = TRUE;
				goto reply;
			}
			tree->search = g_list_append(tree->search, neighbour);
		}
		if (!tree->search)
			goto reply;
		free_edge(current);
		current = g_list_first(tree->search)->data;
		tree->search = g_list_remove(tree->search, current);			
	}
	
	reply:
	g_list_foreach(tree->search, (GFunc)free_edge, NULL);
	
	if (neighbour)
		g_free(neighbour);
	if (current)
		free_edge(current);
	
	g_list_free(tree->unused);
	g_list_free(tree->search);
	g_free(tree);
	return ret;
}

osync_bool osync_conv_find_change_path(OSyncFormatEnv *env, OSyncObjFormat *sourceformat, OSyncObjFormat *targetformat, GList **retlist)
{
	OSyncFormatConverter *converter;
	GList *vertices = g_list_copy(env->converters);
	//Remove all desencaps
	GList *v;
	for (v = vertices; v; v = v->next) {
		converter = v->data;
		if (converter->type == CONVERTER_DESENCAP)
			v = vertices = g_list_remove(vertices, converter);
	}

	return osync_conv_find_shortest_path(vertices, sourceformat, targetformat, retlist);
}

osync_bool osync_converter_invoke(OSyncFormatConverter *converter, OSyncChange *change)
{
	char *data;
	int size;
	osync_bool ret;
	if (!converter->convert_func)
		return FALSE;
	ret = converter->convert_func(change->data, change->size, &data, &size);
	//Fixme free prev data
	change->data = data;
	change->size = size;
	switch (converter->type) {
		case CONVERTER_CONV:
			printf("Converting! replacing format %s with %s\n", converter->source_format->name, converter->target_format->name);
			change->objformats = g_list_remove(change->objformats, converter->source_format);
			change->objformats = g_list_append(change->objformats, converter->target_format);
			break;
		case CONVERTER_ENCAP:
			printf("Encaping! adding format %s\n", converter->target_format->name);
			change->objformats = g_list_append(change->objformats, converter->target_format);
			break;
		case CONVERTER_DESENCAP:
			printf("desencaping! removing format %s\n", converter->source_format->name);
			change->objformats = g_list_remove(change->objformats, converter->source_format);
			break;
		default:
			g_assert_not_reached();
	}
	return ret;
}

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

osync_bool osync_conv_convert(OSyncFormatEnv *env, OSyncChange *change, OSyncObjFormat *targetformat)
{
	GList *l = g_list_last(change->objformats);
	OSyncObjFormat *source = l->data;
	GList *path;
	if (!strcmp(source->name, targetformat->name))
		return TRUE;
	while (!osync_conv_find_change_path(env, g_list_last(change->objformats)->data, targetformat, &path)) {
		if (!g_list_last(change->objformats)->prev) {
			if (!osync_conv_detect_next_format(env, change))
				return FALSE;
		}
		if (!osync_conv_desencap_change(env, change))
			return FALSE;
			
		if (g_list_last(change->objformats)->data == targetformat)
			return TRUE;
	}
	for (; path; path = path->next) {
		OSyncFormatConverter *converter = path->data;
		if (!osync_converter_invoke(converter, change))
			return FALSE;
	}
	return TRUE;
}
