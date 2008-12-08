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

/**
 * @defgroup OSyncPluginConfigAdvancedOptionsAPI OpenSync Plugin Config Advanced Options
 * @ingroup OSyncPublic
 * @brief Functions to get and set a plugin's custom configuration options
 *
 * The AdvancedOptions system allows plugins to store custom (plugin-specific) 
 * configuration settings in a self-describing manner, so that configuration
 * frontends can create corresponding user interfaces on the fly.
 *
 * If desired, AdvancedOptions can also have parameters (sub-options) each 
 * with their own type, name, etc.
 *
 */

/*@{*/

/*! @brief Advanced option value types
 * 
 **/
typedef enum {
	/** None */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_NONE = 0,
	/** bool */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_BOOL,
	/** char */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_CHAR,
	/** double */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_DOUBLE,
	/** int */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_INT,
	/** long */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONG,
	/** long long */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_LONGLONG,
	/** unsigned int */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_UINT,
	/** unsigned long */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONG,
	/** unsigned long long */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_ULONGLONG,
	/** String (char *) */
	OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING
} OSyncPluginAdvancedOptionType;

/* TODO: hide form public API? */
OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_type_string_to_val(const char *typestr);

/* OSyncPluginAdvancedOption */
/*! @brief Create a new OSyncPluginAdvancedOption object
 *
 * @param error Pointer to an error struct
 * @returns the newly created object, or NULL in case of an error.
 *
 */
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_advancedoption_new(OSyncError **error);

/*! @brief Decrease the reference count on an OSyncPluginAdvancedOption object
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_unref(OSyncPluginAdvancedOption *option);

/*! @brief Increase the reference count on an OSyncPluginAdvancedOption object
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns The OSyncPluginAdvancedOption object passed in
 * 
 */
OSYNC_EXPORT OSyncPluginAdvancedOption *osync_plugin_advancedoption_ref(OSyncPluginAdvancedOption *option);


/*! @brief Get a list of the parameters in an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the list of parameters
 * 
 */
OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_get_parameters(OSyncPluginAdvancedOption *option);

/*! @brief Add a parameter to an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param param the parameter to add
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_add_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param);

/*! @brief Remove a parameter from an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param param the parameter to remove
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_remove_parameter(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionParameter *param);


/*! @brief Get the maximum value/length of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the maximum size of the option
 * 
 */
OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_maxsize(OSyncPluginAdvancedOption *option);

/*! @brief Set the maximum value/length of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param maxsize the maximum size to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_maxsize(OSyncPluginAdvancedOption *option, unsigned int maxsize);


/*! @brief Get the minimum value/length of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the minimum value/length of the option
 * 
 */
OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_minsize(OSyncPluginAdvancedOption *option);

/*! @brief Set the minimum value/length of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param minsize the minimum value/length to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_minsize(OSyncPluginAdvancedOption *option, unsigned int minsize);


OSYNC_EXPORT unsigned int osync_plugin_advancedoption_get_maxoccurs(OSyncPluginAdvancedOption *option);
OSYNC_EXPORT void osync_plugin_advancedoption_set_maxoccurs(OSyncPluginAdvancedOption *option, unsigned int maxoccurs);


/*! @brief Get the display name of an option
 * 
 * The display name is intended to be shown in the configuration user interface for the option.
 *
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the display name of the option
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_displayname(OSyncPluginAdvancedOption *option);

/*! @brief Set the display name of an option
 * 
 * The display name is intended to be shown in the configuration user interface for the option.
 *
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param displayname the display name to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_displayname(OSyncPluginAdvancedOption *option, const char *displayname);


/*! @brief Get the name of an option
 * 
 * This name is intended as an internal identifier for the option. It should not be shown in the user interface.
 *
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the name of the option
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_name(OSyncPluginAdvancedOption *option);

/*! @brief Set the name of an option
 * 
 * This name is intended as an internal identifier for the option. It should not be shown in the user interface.
 *
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param name the name to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_name(OSyncPluginAdvancedOption *option, const char *name);


/*! @brief Get the value type of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the value type of the option
 * 
 */
OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_get_type(OSyncPluginAdvancedOption *option);

/*! @brief Get the value type of an option (as text)
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns a string representing the value type of the option
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_type_string(OSyncPluginAdvancedOption *option);

/*! @brief Set the value type of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param type the value type to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_type(OSyncPluginAdvancedOption *option, OSyncPluginAdvancedOptionType type);


/*! @brief Get a list of the enumerated values of an option
 * 
 * For options which accept only a set list of possible values (i.e. an enumeration) this function 
 * returns a list of the possible values.
 *
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the list of enumerated values
 * 
 */
OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_get_valenums(OSyncPluginAdvancedOption *option);

/*! @brief Add an enumerated value to an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param value the value to add to the enumeration
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_add_valenum(OSyncPluginAdvancedOption *option, const char *value);

/*! @brief Remove an enumerated value from an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param value the value to remove from the enumeration
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_remove_valenum(OSyncPluginAdvancedOption *option, const char *value);


/*! @brief Set the value of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @param value the value to set (as a null-terminated string)
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_set_value(OSyncPluginAdvancedOption *option, const char *value);

/*! @brief Get the value of an option
 * 
 * @param option Pointer to the OSyncPluginAdvancedOption object
 * @returns the value of the option (as a null-terminated string)
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_get_value(OSyncPluginAdvancedOption *option);


/* OSyncPluginAdvancedOptionParameter */
/*! @brief Create a new OSyncPluginAdvancedOptionParameter object
 *
 * @param error Pointer to an error struct
 * @returns the newly created object, or NULL in case of an error.
 *
 */
OSYNC_EXPORT OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_new(OSyncError **error);

/*! @brief Decrease the reference count on an OSyncPluginAdvancedOptionParameter object
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_unref(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Increase the reference count on an OSyncPluginAdvancedOptionParameter object
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns The OSyncPluginAdvancedOptionParameter object passed in
 * 
 */
OSYNC_EXPORT OSyncPluginAdvancedOptionParameter *osync_plugin_advancedoption_param_ref(OSyncPluginAdvancedOptionParameter *param);


/*! @brief Get the display name of a parameter
 * 
 * The display name is intended to be shown in the configuration user interface for the parameter.
 *
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns the display name of the parameter
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_displayname(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Set the display name of a parameter
 * 
 * The display name is intended to be shown in the configuration user interface for the parameter.
 *
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param displayname the display name to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_displayname(OSyncPluginAdvancedOptionParameter *param, const char *displayname);


/*! @brief Get the name of a parameter
 * 
 * This name is intended as an internal identifier for the parameter. It should not be shown in the user interface.
 *
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns the name of the parameter
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_name(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Set the name of a parameter
 * 
 * This name is intended as an internal identifier for the parameter. It should not be shown in the user interface.
 *
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param name the name to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_name(OSyncPluginAdvancedOptionParameter *param, const char *name);


/*! @brief Get the value type of a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns the value type of the parameter
 * 
 */
OSYNC_EXPORT OSyncPluginAdvancedOptionType osync_plugin_advancedoption_param_get_type(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Get the value type of a parameter (as text)
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns a string representing the value type of the parameter
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_type_string(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Set the value type of a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param type the value type to set
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_type(OSyncPluginAdvancedOptionParameter *param, OSyncPluginAdvancedOptionType type);

/*! @brief Get a list of the enumerated values of a parameter
 * 
 * For parameters which accept only a set list of possible values (i.e. an enumeration) this function 
 * returns a list of the possible values.
 *
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns the list of enumerated values
 * 
 */
OSYNC_EXPORT OSyncList *osync_plugin_advancedoption_param_get_valenums(OSyncPluginAdvancedOptionParameter *param);

/*! @brief Add an enumerated value to a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param value the value to add to the enumeration
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_add_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value);

/*! @brief Remove an enumerated value from a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param value the value to remove from the enumeration
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_remove_valenum(OSyncPluginAdvancedOptionParameter *param, const char *value);


/*! @brief Set the value of a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @param value the value to set (as a null-terminated string)
 * 
 */
OSYNC_EXPORT void osync_plugin_advancedoption_param_set_value(OSyncPluginAdvancedOptionParameter *param, const char *value);

/*! @brief Get the value of a parameter
 * 
 * @param param Pointer to the OSyncPluginAdvancedOptionParameter object
 * @returns the value of the option (as a null-terminated string)
 * 
 */
OSYNC_EXPORT const char *osync_plugin_advancedoption_param_get_value(OSyncPluginAdvancedOptionParameter *param);

/*@}*/

#endif /*_OPENSYNC_PLUGIN_ADVANCEDOPTIONS_H_*/

