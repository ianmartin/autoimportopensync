/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#include "opensync-plugin.h"
#include "opensync_plugin_advancedoptions_private.h"


static int _list_string_compare(void *a, void *b)
{
	return strcmp((const char *) a, (const char *) b);
}

static const char *_osync_plugin_advancedoption_get_type_string(OSyncPluginAdvancedOptionType typeval)
{
	const char *type = NULL;

	switch (typeval) {
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_NONE:
			type = "none";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL:
			type = "bool";
			break;
       		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR:
			type = "char";
			break;
       		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_DOUBLE:
			type = "double";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT:
			type = "int";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONG:
			type = "long";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONGLONG:
			type = "longlong";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_UINT:
			type = "uint";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONG:
			type = "ulong";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONGLONG:
			type = "ulonglong";
			break;
		case OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING:
			type = "string";
			break;
	}

	return type;
}

OSyncPluginAdvancedOptionType osync_plugin_advancedoption_type_string_to_val(const char *typestr)
{
	osync_assert(typestr);

	if (!strcmp(typestr, "bool"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL;
	else if (!strcmp(typestr, "char"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR;
	else if (!strcmp(typestr, "double"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_DOUBLE;
	else if (!strcmp(typestr, "int"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT;
	else if (!strcmp(typestr, "long"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONG;
	else if (!strcmp(typestr, "longlong"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONGLONG;
	else if (!strcmp(typestr, "uint"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_UINT;
	else if (!strcmp(typestr, "ulong"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONG;
	else if (!strcmp(typestr, "ulonglong"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONGLONG;
	else if (!strcmp(typestr, "string"))
		return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING;

	
	return OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_NONE;
}

/* OSyncPluginAdvancedOption */

OSyncPluginAdvancedOption *osync_plugin_advancedoption_new(OSyncError **error)
{
	OSyncPluginAdvancedOption *option = osync_try_malloc0(sizeof(OSyncPluginAdvancedOption), error);
	if (!option)
		return NULL;

	option->ref_count = 1;
	option->minsize = 0;
	option->maxsize = UINT_MAX;
	option->maxoccurs = UINT_MAX;

	return option;
}

OSyncPluginAdvancedOption *osync_plugin_advancedoption_ref(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	
	g_atomic_int_inc(&(option->ref_count));

	return option;
}

void osync_plugin_advancedoption_unref(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	
	if (g_atomic_int_dec_and_test(&(option->ref_count))) {

		if (option->displayname)
			g_free(option->displayname);
			
		if (option->name)
			g_free(option->name);

		while (option->parameters) {
			osync_plugin_advancedoption_param_unref(option->parameters->data);
			option->parameters = osync_list_remove(option->parameters, option->parameters->data);
		}

		while (option->valenum) {
			g_free(option->valenum->data);
			option->valenum = osync_list_remove(option->valenum, option->valenum->data);
		}

		if (option->value)
			g_free(option->value);

		g_free(option);
	}
}

OSyncList *osync_plugin_advancedoption_get_parameters(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->parameters;
}

void osync_plugin_advancedoption_add_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(option);
	osync_assert(param);

	if (osync_list_find(option->parameters, param))
		return;

	osync_plugin_advancedoption_param_ref(param);
	option->parameters = osync_list_prepend(option->parameters, param);
}

void osync_plugin_advancedoption_remove_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(option);
	osync_assert(param);

	option->parameters = osync_list_remove(option->parameters, param);
	osync_plugin_advancedoption_param_unref(param);
}

const char *osync_plugin_advancedoption_get_displayname(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->displayname;
}

void osync_plugin_advancedoption_set_displayname(OSyncPluginAdvancedOption *option, const char *displayname)
{
	osync_assert(option);
	if (option->displayname)
		g_free(option->displayname);

	option->displayname = g_strdup(displayname);
}

unsigned int osync_plugin_advancedoption_get_maxoccurs(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->maxoccurs;
}

void osync_plugin_advancedoption_set_maxoccurs(OSyncPluginAdvancedOption *option, unsigned int maxoccurs)
{
	osync_assert(option);
	option->maxoccurs = maxoccurs;
}

unsigned int osync_plugin_advancedoption_get_maxsize(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->maxsize;
}

void osync_plugin_advancedoption_set_maxsize(OSyncPluginAdvancedOption *option, unsigned int maxsize)
{
	osync_assert(option);
	option->maxsize = maxsize;
}

unsigned int osync_plugin_advancedoption_get_minsize(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->minsize;
}

void osync_plugin_advancedoption_set_minsize(OSyncPluginAdvancedOption *option, unsigned int minsize)
{
	osync_assert(option);
	option->minsize = minsize;
}

const char *osync_plugin_advancedoption_get_name(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->name;
}

void osync_plugin_advancedoption_set_name(OSyncPluginAdvancedOption *option, const char *name)
{
	osync_assert(option);
	if (option->name)
		g_free(option->name);

	option->name = g_strdup(name);

}

OSyncPluginAdvancedOptionType osync_plugin_advancedoption_get_type(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->type;
}

const char *osync_plugin_advancedoption_get_type_string(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return _osync_plugin_advancedoption_get_type_string(option->type); 
}

void osync_plugin_advancedoption_set_type(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionType type)
{
	osync_assert(option);
	option->type = type;
}

OSyncList *osync_plugin_advancedoption_get_valenums(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->valenum;
}

void osync_plugin_advancedoption_add_valenum(OSyncPluginAdvancedOption *option, const char *value)
{
	osync_assert(option);
	osync_assert(value);

	/* Don't add duplicated values */
	OSyncList *data = osync_list_find_custom(option->valenum, (void *) value, _list_string_compare);
	if (data)
		return;

	option->valenum = osync_list_prepend(option->valenum, g_strdup(value));
}

void osync_plugin_advancedoption_remove_valenum(OSyncPluginAdvancedOption *option, const char *value)
{
	osync_assert(option);
	osync_assert(value);

	OSyncList *data = osync_list_find_custom(option->valenum, (void *) value, _list_string_compare);
	if (!data)
		return;

	g_free(data->data);
	option->valenum = osync_list_remove(option->valenum, data->data);
}

const char *osync_plugin_advancedoption_get_value(OSyncPluginAdvancedOption *option)
{
	osync_assert(option);
	return option->value;
}

void osync_plugin_advancedoption_set_value(OSyncPluginAdvancedOption *option, const char *value)
{
	osync_assert(option);
	if (option->value)
		g_free(option->value);

	option->value = g_strdup(value);
}

/* OSyncPluginAdvancedOptionParameter */

OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_new(OSyncError **error)
{
	OSyncPluginAdvancedOptionParameter *param = osync_try_malloc0(sizeof(OSyncPluginAdvancedOptionParameter), error);
	if (!param)
		return NULL;

	param->ref_count = 1;

	return param;
}

OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_ref(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	
	g_atomic_int_inc(&(param->ref_count));

	return param;
}

void osync_plugin_advancedoption_param_unref(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	
	if (g_atomic_int_dec_and_test(&(param->ref_count))) {

		if (param->displayname)
			g_free(param->displayname);
			
		if (param->name)
			g_free(param->name);

		if (param->value)
			g_free(param->value);

		while (param->valenum) {
			g_free(param->valenum->data);
			param->valenum = osync_list_remove(param->valenum, param->valenum->data);
		}

		g_free(param);
	}
}

const char *osync_plugin_advancedoption_param_get_displayname(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	return param->displayname;
}

void osync_plugin_advancedoption_param_set_displayname(OSyncPluginAdvancedOptionParameter *param, const char *displayname)
{
	osync_assert(param);
	if (param->displayname)
		g_free(param->displayname);

	param->displayname = g_strdup(displayname);
}

const char *osync_plugin_advancedoption_param_get_name(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	return param->name;
}

void osync_plugin_advancedoption_param_set_name(OSyncPluginAdvancedOptionParameter *param, const char *name)
{
	osync_assert(param);
	if (param->name)
		g_free(param->name);

	param->name = g_strdup(name);

}

OSyncPluginAdvancedOptionType osync_plugin_advancedoption_param_get_type(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	return param->type;
}

const char *osync_plugin_advancedoption_param_get_type_string(OSyncPluginAdvancedOptionParameter *param)
{
	return _osync_plugin_advancedoption_get_type_string(param->type);
}

void osync_plugin_advancedoption_param_set_type(OSyncPluginAdvancedOptionParameter *param, OSyncPluginAdvancedOptionType type)
{
	osync_assert(param);
	param->type = type;
}

OSyncList *osync_plugin_advancedoption_param_get_valenums(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	return param->valenum;
}

void osync_plugin_advancedoption_param_add_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value)
{
	osync_assert(param);
	osync_assert(value);

	/* Don't add duplicated values */
	OSyncList *data = osync_list_find_custom(param->valenum, (void *) value, _list_string_compare);
	if (data)
		return;

	param->valenum = osync_list_prepend(param->valenum, g_strdup(value));
}

void osync_plugin_advancedoption_param_remove_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value)
{
	osync_assert(param);
	osync_assert(value);

	OSyncList *data = osync_list_find_custom(param->valenum, (void *) value, _list_string_compare);
	if (!data)
		return;

	g_free(data->data);
	param->valenum = osync_list_remove(param->valenum, data->data);
}

const char *osync_plugin_advancedoption_param_get_value(OSyncPluginAdvancedOptionParameter *param)
{
	osync_assert(param);
	return param->value;
}

void osync_plugin_advancedoption_param_set_value(OSyncPluginAdvancedOptionParameter *param, const char *value)
{
	osync_assert(param);
	if (param->value)
		g_free(param->value);

	param->value = g_strdup(value);

}

