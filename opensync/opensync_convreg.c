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

/**
 * @defgroup OSyncConvPrivate OpenSync Conversion Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

/** The destroy_func to call free() on a block of data
 */
/*static void osync_format_malloced_destroy(char *data, size_t size)
{
	if (data)
		free(data);
}*/

/** The copy_func that does a malloc()/memcpy() on the data
 */
/*static osync_bool osync_format_malloced_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	*output = malloc(inpsize);
	if (!*output)
		return FALSE;
	memcpy(*output, input, inpsize);
	return TRUE;
}*/

/** Simple copy converter function
 *
 * @see osync_env_format_set_like()
 * */
/*static osync_bool conv_simple_copy(char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	*free_input = FALSE;
	*output = (char*)input;
	*outpsize = inpsize;
	return TRUE;
}*/

OSyncObjFormatTemplate *osync_env_find_format_template(OSyncEnv *env, const char *name)
{
	GList *o;
	for (o = env->format_templates; o; o = o->next) {
		OSyncObjFormatTemplate *tmpl = o->data;
		if (!strcmp(tmpl->name, name))
			return tmpl;
	}
	return NULL;
}

OSyncObjTypeTemplate *osync_env_find_objtype_template(OSyncEnv *env, const char *name)
{
	GList *o;
	for (o = env->objtype_templates; o; o = o->next) {
		OSyncObjTypeTemplate *tmpl = o->data;
		if (!strcmp(tmpl->name, name))
			return tmpl;
	}
	return NULL;
}

OSyncDataDetector *osync_env_find_detector(OSyncEnv *env, const char *sourcename, const char *targetname)
{
	GList *o;
	for (o = env->data_detectors; o; o = o->next) {
		OSyncDataDetector *tmpl = o->data;
		if (!strcmp(tmpl->sourceformat, sourcename) && !strcmp(tmpl->targetformat, targetname))
			return tmpl;
	}
	return NULL;
}

OSyncConverterTemplate *osync_env_find_converter_template(OSyncEnv *env, const char *sourcename, const char *targetname)
{
	GList *o;
	for (o = env->converter_templates; o; o = o->next) {
		OSyncConverterTemplate *tmpl = o->data;
		if (!strcmp(tmpl->source_format, sourcename) && !strcmp(tmpl->target_format, targetname))
			return tmpl;
	}
	return NULL;
}

/*OSyncFormatExtensionTemplate *osync_env_find_extension_template(OSyncEnv *env, const char *formatname)
{
	GList *i;
	for (i = env->extension_templates; i; i = i->next) {
		OSyncFormatExtensionTemplate *ext_templ = i->data;
		if (!strcmp(ext_templ->formatname, formatname))
			return ext_templ;
	}
	return NULL;
}*/

/*@}*/

void osync_env_register_detector(OSyncEnv *env, const char *sourceformat, const char *format, OSyncFormatDetectDataFunc detect_func)
{
	g_assert(detect_func);
	OSyncDataDetector *detector = g_malloc0(sizeof(OSyncDataDetector));
	detector->sourceformat = strdup(sourceformat);
	detector->targetformat = strdup(format);
	detector->detect_func = detect_func;

	//Register the "inverse" detector which of course will always work
	env->data_detectors = g_list_append(env->data_detectors, detector);
	detector = g_malloc0(sizeof(OSyncDataDetector));
	detector->sourceformat = strdup(format);
	detector->targetformat = strdup(sourceformat);
	detector->detect_func = NULL;

	env->data_detectors = g_list_append(env->data_detectors, detector);
}

void osync_env_register_filter_function(OSyncEnv *env, const char *name, const char *objtype, const char *format, OSyncFilterFunction hook)
{
	OSyncCustomFilter *function = g_malloc0(sizeof(OSyncCustomFilter));
	function->name = g_strdup(name);
	function->objtype = g_strdup(objtype);
	function->format = g_strdup(format);
	function->hook = hook;
	
	env->filter_functions = g_list_append(env->filter_functions, function);
}

void osync_env_register_objformat(OSyncEnv *env, const char *typename, const char *name)
{
	OSyncObjFormatTemplate *format = NULL;
	if (!(format = osync_env_find_format_template(env, name))) {
		format = g_malloc0(sizeof(OSyncObjFormatTemplate));
		format->name = strdup(name);
		format->objtype = g_strdup(typename);
		//We default to malloc style!
		//format->copy_func = osync_format_malloced_copy;
		//format->destroy_func = osync_format_malloced_destroy;
		env->format_templates = g_list_append(env->format_templates, format);
	}
}

void osync_env_register_objtype(OSyncEnv *env, const char *name)
{
	OSyncObjTypeTemplate *type = NULL;
	if (!(type = osync_env_find_objtype_template(env, name))) {
		type = g_malloc0(sizeof(OSyncObjTypeTemplate));
		type->name = g_strdup(name);
		env->objtype_templates = g_list_append(env->objtype_templates, type);
	}
}

void osync_env_register_converter(OSyncEnv *env, ConverterType type, const char *sourcename, const char *targetname, OSyncFormatConvertFunc convert_func)
{
	OSyncConverterTemplate *converter = g_malloc0(sizeof(OSyncConverterTemplate));

	converter->source_format = sourcename;
	converter->target_format = targetname;
	converter->convert_func = convert_func;
	converter->type = type;
	env->converter_templates = g_list_append(env->converter_templates, converter);
}

void osync_env_converter_set_init(OSyncEnv *env, const char *sourcename, const char *targetname, OSyncFormatConverterInitFunc init_func, OSyncFormatConverterFinalizeFunc fin_func)
{
	OSyncConverterTemplate *converter = osync_env_find_converter_template(env, sourcename, targetname);
	osync_assert(converter != NULL, "You need to register the converter first");
	
	converter->init_func = init_func;
	converter->fin_func = fin_func;
}

void osync_env_register_extension(OSyncEnv *env, const char *from_format, const char *to_format, const char *extension_name, OSyncFormatExtInitFunc init_func)
{
	OSyncFormatExtensionTemplate *ext = g_malloc0(sizeof(OSyncFormatExtensionTemplate));
	ext->from_formatname = g_strdup(from_format);
	ext->to_formatname = g_strdup(to_format);
	ext->name = g_strdup(extension_name);
	ext->init_func = init_func;
	
	env->extension_templates = g_list_append(env->extension_templates, ext);
}

void osync_env_format_set_compare_func(OSyncEnv *env, const char *formatname, OSyncFormatCompareFunc cmp_func)
{
	osync_trace(TRACE_INTERNAL, "osync_env_format_set_compare_func(%p, %s, %p)", env, formatname, cmp_func);
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->cmp_func = cmp_func;
}

void osync_env_format_set_destroy_func(OSyncEnv *env, const char *formatname, OSyncFormatDestroyFunc destroy_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->destroy_func = destroy_func;
}

void osync_env_format_set_copy_func(OSyncEnv *env, const char *formatname, OSyncFormatCopyFunc copy_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->copy_func = copy_func;
}

/** Set the detector function for this type
 *
 * A detector function for a given format is different from a the detectors registered
 * using osync_env_register_data_detector().
 *
 * The osync_env_format_set_detect_func() is a function designed to return the lower
 * objformat of the data, by looking at it.
 *
 * The osync_env_register_data_detector() function is a function that checks if a
 * given block of data can be converting to a given format.
 *
 * The osync_env_format_set_detect_func() is more useful for
 * 'encapsulator'-like formats * that can tell the format of the data below,
 * by just looking at the data. The osync_env_register_data_detector() functions
 * is more useful for 'generic' formats (like the "plain" format) that each
 * format to which it can be converted to (vcard, vcalendar, etc.) knows
 * how to detect the data by looking into the "plain" block.
 */
/*void osync_env_format_set_detect_func(OSyncEnv *env, const char *formatname, OSyncFormatDetectFunc detect_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->detect_func = detect_func;
}*/

void osync_env_format_set_duplicate_func(OSyncEnv *env, const char *formatname, OSyncFormatDuplicateFunc dupe_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->duplicate_func = dupe_func;
}

void osync_env_format_set_create_func(OSyncEnv *env, const char *formatname, OSyncFormatCreateFunc create_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->create_func = create_func;
}

void osync_env_format_set_print_func(OSyncEnv *env, const char *formatname, OSyncFormatPrintFunc print_func)
{
	g_assert(env);
	OSyncObjFormatTemplate *format = osync_env_find_format_template(env, formatname);
	osync_assert(format, "You need to register the formattype first");
	format->print_func = print_func;
}
