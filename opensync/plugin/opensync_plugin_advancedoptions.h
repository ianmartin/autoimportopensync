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

#ifndef _OPENSYNC_PLUGIN_ADVANCEDOPTIONS_H_
#define _OPENSYNC_PLUGIN_ADVANCEDOPTIONS_H_

typedef enum {
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_NONE = 0,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_DOUBLE,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONG,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONGLONG,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_UINT,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONG,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONGLONG,
        OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING
} OSyncPluginAdvancedOptionType;

/* TODO: hide form public API? */
OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_type_string_to_val(const char *typestr);

/* OSyncPluginAdvancedOption */
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_advancedoption_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_advancedoption_unref(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_advancedoption_ref(OSyncPluginAdvancedOption *option);

OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_get_parameters(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_add_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT void osync_plugin_advancedoption_remove_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param);

OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_maxsize(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_maxsize(OSyncPluginAdvancedOption *option, unsigned int maxsize);

OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_minsize(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_minsize(OSyncPluginAdvancedOption *option, unsigned int minsize);

OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_maxoccurs(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_maxoccurs(OSyncPluginAdvancedOption *option, unsigned int maxoccurs);

OSYNC_EXPORT const char *osync_plugin_advancedoption_get_displayname(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_displayname(OSyncPluginAdvancedOption *option, const char *displayname);

OSYNC_EXPORT const char *osync_plugin_advancedoption_get_name(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_name(OSyncPluginAdvancedOption *option, const char *name);

OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_get_type(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_type_string(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_type(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionType type);

OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_get_valenums(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_add_valenum(OSyncPluginAdvancedOption *option, const char *value);
OSYNC_EXPORT void osync_plugin_advancedoption_remove_valenum(OSyncPluginAdvancedOption *option, const char *value);

OSYNC_EXPORT void osync_plugin_advancedoption_set_value(OSyncPluginAdvancedOption *option, const char *value);
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_value(OSyncPluginAdvancedOption *option);

/* OSyncPluginAdvancedOptionParameter */
OSYNC_EXPORT OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_new(OSyncError **error);
OSYNC_EXPORT void osync_plugin_advancedoption_param_unref(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_ref(OSyncPluginAdvancedOptionParameter *param);

OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_displayname(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_displayname(OSyncPluginAdvancedOptionParameter *param, const char *displayname);

OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_name(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_name(OSyncPluginAdvancedOptionParameter *param, const char *name);

OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_param_get_type(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_type_string(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_type(OSyncPluginAdvancedOptionParameter *param, OSyncPluginAdvancedOptionType type);

OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_param_get_valenums(OSyncPluginAdvancedOptionParameter *param);
OSYNC_EXPORT void osync_plugin_advancedoption_param_add_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value);
OSYNC_EXPORT void osync_plugin_advancedoption_param_remove_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value);

OSYNC_EXPORT void osync_plugin_advancedoption_param_set_value(OSyncPluginAdvancedOptionParameter *param, const char *value);
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_value(OSyncPluginAdvancedOptionParameter *param);

#endif /*_OPENSYNC_PLUGIN_ADVANCEDOPTIONS_H_*/

