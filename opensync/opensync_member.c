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
 * @defgroup OSyncMemberPrivateAPI OpenSync Member Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncMember
 * 
 */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
OSyncMemberFunctions *osync_memberfunctions_new()
{
	OSyncMemberFunctions *functions = g_malloc0(sizeof(OSyncMemberFunctions));
	return functions;
}

OSyncMemberFunctions *osync_member_get_memberfunctions(OSyncMember *member)
{
	return member->memberfunctions;
}

OSyncFormatEnv *osync_member_get_format_env(OSyncMember *member)
{
	g_assert(member);
	return osync_group_get_format_env(member->group);
}

/** Find the objtype_sink for a member, corresponding to objtypestr
 *
 * Note: Only call this function after calling osync_member_require_sink_info()
 */
OSyncObjTypeSink *osync_member_find_objtype_sink(OSyncMember *member, const char *objtypestr)
{
	GList *o;
	for (o = member->objtype_sinks; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;
		if (osync_conv_objtype_is_any(sink->objtype->name) || !strcmp(sink->objtype->name, objtypestr))
			return sink;
	}
	return NULL;
}

/** Be sure that the sink information for the member is available
 *
 * This function should be used on every code that will access the objtype_sinks
 * or objformat_sinks members on OSyncMember.
 *
 * This function will either load the plugin or load the plugin information
 * for the member, in order to get the objtype_sink list information for
 * the member.
 */
osync_bool osync_member_require_sink_info(OSyncMember *member, OSyncError **error)
{
	// Currently, the only way to get the objtype_sink information
	// is loading the plugin
	if (!osync_member_instance_default_plugin(member, error))
		return FALSE;

	return TRUE;
}

/** Returns the list of objtype_sinks of a member
 *
 * @param member The member
 * @param list_ptr Pointer to where the list will be returned
 * @param error Pointer to error info
 *
 * @returns TRUE on success, FALSE on error
 */
osync_bool osync_member_get_objtype_sinks(OSyncMember *member, GList **list_ptr, OSyncError **error)
{
	if (!osync_member_require_sink_info(member, error))
		return FALSE;

	*list_ptr = member->objtype_sinks;
	return TRUE;
}

/** @brief Reads the configuration data of this member
 * 
 * The config file is read in this order:
 * - If there is a configuration in memory that is not yet saved
 * this is returned
 * - If there is a config file in the member directory this is read
 * and returned
 * 
 * The difference to get_config is that this function will never try to read
 * the default config file and return an error instead. So this function is supposed
 * to be used by the plugins.
 * 
 * @param member The member
 * @param data Return location for the data
 * @param size Return location for the size of the data
 * @param error Pointer to a error
 * @returns TRUE if the config was loaded successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_read_config(OSyncMember *member, char **data, int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_member_read_config(%p, %p, %p, %p)", member, data, size, error);
	
	if (!osync_member_instance_default_plugin(member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	OSyncPluginFunctions functions = member->plugin->info.functions;
	osync_bool ret = FALSE;
	if (!member->configdir) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Member has no config directory set");
		osync_trace(TRACE_EXIT_ERROR, "osync_member_read_config: %i", osync_error_print(error));
		return FALSE;
	}
	
	if (functions.get_config) {
		ret = functions.get_config(member->configdir, data, size);
	} else {
		char *filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
		ret = osync_file_read(filename, data, size, error);
		g_free(filename);
	}
	
	if (ret)
		osync_trace(TRACE_EXIT, "osync_member_read_config: TRUE");
	else
		osync_trace(TRACE_EXIT_ERROR, "osync_member_read_config: %s", osync_error_print(error));
	return ret;
}

#endif

/*@}*/

/**
 * @defgroup OSyncMemberAPI OpenSync Member
 * @ingroup OSyncPublic
 * @brief Used to manipulate members, which represent one device or application in a group
 * 
 */
/*@{*/

/** @brief Creates a new member for a group
 * 
 * @param group The parent group. NULL if none
 * @returns A newly allocated member
 * 
 */
OSyncMember *osync_member_new(OSyncGroup *group)
{
	OSyncMember *member = g_malloc0(sizeof(OSyncMember));
	if (group) {
		osync_group_add_member(group, member);
		member->group = group;
	}
	
	member->memberfunctions = osync_memberfunctions_new();
	member->name = NULL;

	return member;
}

/** @brief Frees a member
 * 
 * @param member The member to free
 * 
 */
void osync_member_free(OSyncMember *member)
{
	osync_assert_msg(member, "You must set a member to free");
	
	if (member->group)
		osync_group_remove_member(member->group, member);
	
	//Free the plugin if we are not thread-safe
	/*if (member->plugin && !member->plugin->info.is_threadsafe) {
		osync_plugin_unload(member->plugin);
		osync_plugin_free(member->plugin);
	}*/
	
	if (member->pluginname)
		g_free(member->pluginname);
	
	g_free(member->memberfunctions);
	g_free(member);
}	

/** @brief Unloads the plugin of a member
 * 
 * @param member The member for which to unload the plugin
 * 
 */
void osync_member_unload_plugin(OSyncMember *member)
{
	g_assert(member);
	if (!member->plugin)
		return;
		
	/*if (!member->plugin->info.is_threadsafe) {
		osync_plugin_unload(member->plugin);
		osync_plugin_free(member->plugin);
	}*/
	
	g_list_free(member->objtype_sinks);
	g_list_free(member->format_sinks);
	//Really free the formats!
	
	member->objtype_sinks = NULL;
	member->format_sinks = NULL;
	member->plugin = NULL;
}

/** @brief Instances a plugin and loads it if necessary
 * 
 * @param member The member
 * @param pluginname The name of the plugin that the member should use
 * @param error Pointer to a error
 * @returns TRUE if the plugin was instanced successfull, FALSE otherwise
 * 
 */
osync_bool osync_member_instance_plugin(OSyncMember *member, const char *pluginname, OSyncError **error)
{
	g_assert(member);
	g_assert(member->group);
	g_assert(pluginname);
	
	OSyncPlugin *plugin = osync_env_find_plugin(member->group->env, pluginname);
	if (!plugin) {
		osync_debug("OSPLG", 0, "Couldn't find the plugin %s for member", pluginname);
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to find the plugin \"%s\"", pluginname);
		return FALSE;
	}
	
	osync_debug("OSMEM", 3, "Instancing plugin %s for member %i", plugin->info.name, member->id);
	osync_member_unload_plugin(member);
	
	//For now we disable the threadsafety feature since dlopen doesnt like it
	member->plugin = plugin;
	/*if (plugin->info.is_threadsafe) {
		member->plugin = plugin;
	} else {
		member->plugin = osync_plugin_load(NULL, plugin->path, error);
		if (!member->plugin)
			return FALSE;
	}*/
	member->pluginname = g_strdup(osync_plugin_get_name(member->plugin));
	
	//Prepare the sinks;
	GList *o;
	for (o = member->plugin->accepted_objtypes; o; o = o->next) {
		OSyncObjTypeTemplate *objtemplate = o->data;
		OSyncObjTypeSink *objsink = osync_objtype_sink_from_template(member->group, objtemplate);
		if (!objsink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not find object type \"%s\"", objtemplate->name);
			return FALSE;
		}
		member->objtype_sinks = g_list_append(member->objtype_sinks, objsink);
		GList *f;
		for (f = objtemplate->formats; f; f = f->next) {
			OSyncObjFormatTemplate *frmtemplate = f->data;
			OSyncObjFormatSink *format_sink = osync_objformat_sink_from_template(member->group, frmtemplate);
			if (!format_sink) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not find format \"%s\"", frmtemplate->name);
				return FALSE;
			}
			objsink->formatsinks = g_list_append(objsink->formatsinks, format_sink);
			format_sink->objtype_sink = objsink;
			member->format_sinks = g_list_append(member->format_sinks, format_sink);
			if (frmtemplate->extension_name)
				member->extension = g_strdup(frmtemplate->extension_name);
		}
	}
	
	member->pluginname = g_strdup(pluginname);
	return TRUE;
}

/** @brief Tries to instance the default plugin of a member (if set)
 * 
 * @param member The member
 * @param error Pointer to a error
 * @returns TRUE if the default plugin was instanced successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_instance_default_plugin(OSyncMember *member, OSyncError **error)
{
	if (member->plugin)
		return TRUE;
	
	if (!member->pluginname) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No default plugin set while instancing");
		return FALSE;
	}
	
	return osync_member_instance_plugin(member, member->pluginname, error);
}

/** @brief Returns the plugin of member
 * 
 * @param member The member
 * @returns The plugin of the member
 * 
 */
OSyncPlugin *osync_member_get_plugin(OSyncMember *member)
{
	g_assert(member);
	osync_member_instance_default_plugin(member, NULL);
	return member->plugin;
}

/** @brief Returns the name of the default plugin of the member
 * 
 * @param member The member
 * @returns The name of the plugin
 * 
 */
const char *osync_member_get_pluginname(OSyncMember *member)
{
	g_assert(member);
	return member->pluginname;
}

/** @brief Sets the name of the default plugin of a member
 * 
 * @param member The member
 * @param pluginname The name of the default plugin
 * 
 */
void osync_member_set_pluginname(OSyncMember *member, const char *pluginname)
{
	g_assert(member);
	if (member->pluginname)
		g_free(member->pluginname);
	member->pluginname = g_strdup(pluginname);
}

/** @brief Returns the custom data set to the OSyncPluginInfo
 * 
 * You can set custom data to the OSyncPluginInfo struct using
 * info->plugin_data = something; you can then query this data later
 * using this function.
 * 
 * @param member The member
 * @returns The plugin data set before, or NULL if none was set
 * 
 */
void *osync_member_get_plugindata(OSyncMember *member)
{
	g_assert(member);
	OSyncPlugin *plugin = osync_member_get_plugin(member);
	return osync_plugin_get_plugin_data(plugin);
}

/** @brief Returns the configuration directory where this member is stored
 * 
 * @param member The member
 * @returns The configuration directory
 * 
 */
const char *osync_member_get_configdir(OSyncMember *member)
{
	g_assert(member);
	return member->configdir;
}

/** @brief Sets the directory where a member is supposed to be stored
 * 
 * @param member The member
 * @param configdir The name of the directory
 * 
 */
void osync_member_set_configdir(OSyncMember *member, const char *configdir)
{
	g_assert(member);
	if (member->configdir)
		g_free(member->configdir);
	member->configdir = g_strdup(configdir);
}

osync_bool osync_member_need_config(OSyncMember *member, OSyncConfigurationTypes *type, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, type, error);
	g_assert(member);
	g_assert(type);
	*type = NO_CONFIGURATION;
	
	if (!osync_member_instance_default_plugin(member, error))
		goto error;
	
	*type = member->plugin->info.config_type;
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, *type);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** @brief Gets the configuration data of this member
 * 
 * The config file is read in this order:
 * - If there is a configuration in memory that is not yet saved
 * this is returned
 * - If there is a config file in the member directory this is read
 * and returned
 * - Otherwise the default config file is loaded from one the opensync
 * directories
 * 
 * @param member The member
 * @param data Return location for the data
 * @param size Return location for the size of the data
 * @param error Pointer to a error
 * @returns TRUE if the config was loaded successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_get_config_or_default(OSyncMember *member, char **data, int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, data, size, error);
	g_assert(member);
	osync_bool ret = TRUE;

	if (member->configdata) {
		*data = member->configdata;
		*size = member->configsize;
		osync_trace(TRACE_EXIT, "%s: Configdata already in memory", __func__);
		return TRUE;
	}

	if (!osync_member_read_config(member, data, size, error)) {
		if (osync_error_is_set(error)) {
			osync_trace(TRACE_INTERNAL, "Read config not successfull: %s", osync_error_print(error));
			osync_error_free(error);
		}
		
		char *filename = g_strdup_printf(OPENSYNC_CONFIGDIR"/%s", member->pluginname);
		osync_debug("OSMEM", 3, "Reading default2 config file for member %lli from %s", member->id, filename);
		ret = osync_file_read(filename, data, size, error);
		g_free(filename);
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

/** @brief Gets the configuration data of this member
 * 
 * The config file is read in this order:
 * - If there is a configuration in memory that is not yet saved
 * this is returned
 * - If there is a config file in the member directory this is read
 * and returned
 * - Otherwise the default config file is loaded from one the opensync
 * directories (but only if the plugin specified that it can use the default
 * configuration)
 * 
 * @param member The member
 * @param data Return location for the data
 * @param size Return location for the size of the data
 * @param error Pointer to a error
 * @returns TRUE if the config was loaded successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, data, size, error);
	g_assert(member);
	osync_bool ret = TRUE;

	if (!osync_member_instance_default_plugin(member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (member->plugin->info.config_type == NO_CONFIGURATION) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "This member has no configuration options");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (member->configdata) {
		*data = member->configdata;
		*size = member->configsize;
		osync_trace(TRACE_EXIT, "%s: Configdata already in memory", __func__);
		return TRUE;
	}

	if (!osync_member_read_config(member, data, size, error)) {
		if (osync_error_is_set(error)) {
			osync_trace(TRACE_INTERNAL, "Read config not successfull: %s", osync_error_print(error));
			osync_error_free(error);
		}
		
		if (member->plugin->info.config_type == NEEDS_CONFIGURATION) {
			//We dont load the default config for these
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Member has not been configured");
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
		
		char *filename = g_strdup_printf(OPENSYNC_CONFIGDIR"/%s", member->pluginname);
		osync_debug("OSMEM", 3, "Reading default2 config file for member %lli from %s", member->id, filename);
		ret = osync_file_read(filename, data, size, error);
		g_free(filename);
	}
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

/** @brief Sets the config data for a member
 * 
 * Note that this does not save the config data
 * 
 * @param member The member
 * @param data The new config data
 * @param size The size of the data
 * 
 */
void osync_member_set_config(OSyncMember *member, const char *data, int size)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i)", __func__, member, data, size);
	g_assert(member);
	//FIXME free old data
	member->configdata = g_strdup(data);
	member->configsize = size;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Gets the loop in which the member is dispatched
 * 
 * @param member The member
 * @returns The loop
 * 
 */
void *osync_member_get_loop(OSyncMember *member)
{
	g_assert(member);
	osync_trace(TRACE_INTERNAL, "%s: %p %p", __func__, member, member->loop);
	return member->loop;
}

/** @brief Sets the loop in which the member is dispatched
 * 
 * @param member The member
 * @param loop The pointer to the loop
 * 
 */
void osync_member_set_loop(OSyncMember *member, void *loop)
{
	g_assert(member);
	osync_trace(TRACE_INTERNAL, "%s: %p %p", __func__, member, loop);
	member->loop = loop;
}


/** @brief Returns if the member has configuation options
 * 
 * @param member The member
 * @return TRUE if member needs to be configured, FALSE otherwise
 * 
 */
osync_bool osync_member_has_configuration(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	g_assert(member);
	
	osync_bool ret = (member->plugin->info.config_type == NEEDS_CONFIGURATION || member->plugin->info.config_type == OPTIONAL_CONFIGURATION);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, ret);
	return ret;
}

/** @brief Loads a member from a directory where it has been saved
 * 
 * @param group The group which is the parent
 * @param path The path of the member
 * @param error Pointer to a error
 * @returns A newly allocated member thats stored in the group or NULL if error
 * 
 */
OSyncMember *osync_member_load(OSyncGroup *group, const char *path, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, group, path, error);
	xmlDocPtr doc;
	xmlNodePtr cur;
	char *filename = NULL;
	
	filename = g_strdup_printf ("%s/syncmember.conf", path);
	
	OSyncMember *member = osync_member_new(group);
	char *basename = g_path_get_basename(path);
	member->id = atoi(basename);
	g_free(basename);
	member->configdir = g_strdup(path);

	if (!_osync_open_xml_file(&doc, &cur, filename, "syncmember", error)) {
		osync_member_free(member);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	while (cur != NULL) {
		char *str = (char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"pluginname"))
				member->pluginname = g_strdup(str);
			if (!xmlStrcmp(cur->name, (const xmlChar *)"name"))
				member->name = g_strdup(str);
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s: Loaded member: %p", __func__, member);
	return member;
}

/** @brief Saves a member to it config directory
 * 
 * @param member The member to save
 * @param error Pointer to a error
 * @returns TRUE if the member was saved successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_save(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p:(%lli), %p)", __func__, member, member ? member->id : 0, error);
	char *filename = NULL;

	if (!osync_member_instance_default_plugin(member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (!member->id) {
		member->id = osync_group_create_member_id(member->group);
		member->configdir = g_strdup_printf("%s/%lli", member->group->configdir, member->id);
	}
	
	g_assert(member->configdir);
	if (!g_file_test(member->configdir, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSMEM", 3, "Creating config directory: %s for member %i", member->configdir, member->id);
		if (mkdir(member->configdir, 0700)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for member %li\n", member->id);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
	}
	
	//Saving the syncmember.conf
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	xmlDocPtr doc;
	doc = xmlNewDoc((xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"syncmember", NULL);
	//The plugin name
	xmlNewTextChild(doc->children, NULL, (xmlChar*)"pluginname", (xmlChar*)member->pluginname);
  //The name
	xmlNewTextChild(doc->children, NULL, (xmlChar*)"name", (xmlChar*)member->name);
	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);
	
	//Saving the config if it exists
	osync_bool ret = TRUE;
	if (member->configdata) {
		OSyncPluginFunctions functions = member->plugin->info.functions;
		
		if (functions.store_config) {
			ret = functions.store_config(member->configdir, member->configdata, member->configsize);
		} else {
			filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
			if (!osync_file_write(filename, member->configdata, member->configsize, 0600, error)) {
				ret = FALSE;
			}
			g_free(filename);
		}
		g_free(member->configdata);
		member->configdata = NULL;
		member->configsize = 0;
	}
	
	osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(error));
	return ret;
}

/** @brief Gets the unique id of a member
 * 
 * @param member The member
 * @returns The id of the member thats unique in its group
 * 
 */
long long int osync_member_get_id(OSyncMember *member)
{
	g_assert(member);
	return member->id;
}

/** @brief Makes a custom call to the plugin that the member has instanced
 * 
 * A custom function on the plugin must have the form (void *, void *, OSyncError **)
 * 
 * @param member The member
 * @param function The name of the function on the plugin to call
 * @param data The custom data to pass as the second arg to the function on the plugin
 * @param error A pointer to a error
 * @returns The return value of the function call
 * 
 */
void *osync_member_call_plugin(OSyncMember *member, const char *function, void *data, OSyncError **error)
{
	if (!osync_member_instance_default_plugin(member, error))
		return FALSE;
	
	void *(*plgfunc) (void *, void *, OSyncError **);
	if (!(plgfunc = osync_plugin_get_function(member->plugin, function, error)))
		return NULL;
	return plgfunc(member->plugindata, data, error);
}

/** @brief Sets the slow-sync for a given object type on a member
 * 
 * @param member The member
 * @param objtypestr The name of the object type for which to set slow-sync
 * @param slow_sync Set to TRUE if you want slow-sync, to FALSE if you want normal fast-sync (or remove slow-sync)
 * 
 */
void osync_member_set_slow_sync(OSyncMember *member, const char *objtypestr, osync_bool slow_sync)
{
	g_assert(member);	
	OSyncGroup *group = osync_member_get_group(member);
	g_assert(group);

	osync_group_set_slow_sync(group, objtypestr, slow_sync);
}

/** @brief Returns if slow-sync has been set for a object type
 * 
 * @param member The member
 * @param objtypestr The name of the object type to look up
 * @returns TRUE if slow-sync is enabled, FALSE otherwise
 * 
 */
osync_bool osync_member_get_slow_sync(OSyncMember *member, const char *objtypestr)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, member, objtypestr);
	g_assert(member);	
	OSyncGroup *group = osync_member_get_group(member);
	g_assert(group);

	osync_bool needs_slow_sync = osync_group_get_slow_sync(group, objtypestr);
	
	osync_trace(TRACE_EXIT, "%s: %i", __func__, needs_slow_sync);
	return needs_slow_sync;
}

/** @brief Requests synchronization from the sync engine
 * 
 * @param member The member
 * 
 */
void osync_member_request_synchronization(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	g_assert(member);
	
	if (member->memberfunctions->rf_sync_alert)
		member->memberfunctions->rf_sync_alert(member);
	else {
		osync_trace(TRACE_EXIT_ERROR, "%s: Alert not handled", __func__);
		return;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}


/** @brief Makes random data of a object type that could be writen to the given member
 * 
 * @param member The member
 * @param change The change that will receive the random data
 * @param objtypename The name of the object type for which to create random data
 * @returns The sink of the member that could store this data
 * 
 */
OSyncObjFormatSink *osync_member_make_random_data(OSyncMember *member, OSyncChange *change, const char *objtypename)
{
	int retry = 0;
	g_assert(member);
	OSyncFormatEnv *env = osync_member_get_format_env(member);
	
	OSyncObjFormatSink *format_sink = NULL;
	
	for (retry = 0; retry < 100; retry++) {
		if (retry > 20) {
			osync_trace(TRACE_INTERNAL, "%s: Giving up", __func__);
			return NULL; //Giving up
		}
		
		//Select a random objtype
		OSyncObjType *objtype = NULL;
		int selected = 0;
		if (!objtypename) {
			selected = g_random_int_range(0, g_list_length(env->objtypes));
			objtype = g_list_nth_data(env->objtypes, selected);
		} else
			objtype = osync_conv_find_objtype(member->group->conv_env, objtypename);
		osync_change_set_objtype(change, objtype);
		
		//Select a random objformat
		if (!g_list_length(objtype->formats)) {
			osync_trace(TRACE_INTERNAL, "%s: Next. No formats", __func__);
			continue;
		}
		OSyncObjFormat *format = NULL;
		selected = g_random_int_range(0, g_list_length(objtype->formats));
		format = g_list_nth_data(objtype->formats, selected);
		
		if (!format->create_func) {
			osync_trace(TRACE_INTERNAL, "%s: Next. Format %s has no create function", __func__, format->name);
			continue;
		}
		//Create the data
		format->create_func(change);
		
		osync_change_set_objformat(change, format);
		//Convert the data to a format the plugin understands
		OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, objtype->name);
		if (!objtype_sink) {
			osync_trace(TRACE_INTERNAL, "%s: Next. No objtype sink for %s", __func__, objtype->name);
			continue; //We had a objtype we cannot add
		}
		
		selected = g_random_int_range(0, g_list_length(objtype_sink->formatsinks));
		format_sink = g_list_nth_data(objtype_sink->formatsinks, selected);
		/*FIXME: use multiple sinks, or what? */
		OSyncError *error = NULL;
		if (!osync_change_convert(env, change, format_sink->format, &error)) {
			osync_trace(TRACE_INTERNAL, "%s: Next. Unable to convert: %s", __func__, osync_error_print(&error));
			continue; //Unable to convert to selected format
		}
		
		break;
	}
	return format_sink;
}


/** @brief Returns the custom data of a member
 * 
 * @param member The member
 * @returns The custom data
 * 
 */
void *osync_member_get_data(OSyncMember *member)
{
	g_assert(member);
	return member->enginedata;
}

/** @brief Sets the custom data on a member
 * 
 * @param member The member
 * @param data The custom data
 * 
 */
void osync_member_set_data(OSyncMember *member, void *data)
{
	g_assert(member);
	member->enginedata = data;
}

/** @brief Gets the group in which the member is stored
 * 
 * @param member The member
 * @returns The parental group
 * 
 */
OSyncGroup *osync_member_get_group(OSyncMember *member)
{
	g_assert(member);
	return member->group;
}

/** @brief Searches for a member by its id
 * 
 * @param group The group in which to search
 * @param id The id of the member
 * @returns The member, or NULL if not found
 * 
 */
OSyncMember *osync_member_from_id(OSyncGroup *group, int id)
{
	OSyncMember *member;
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		member = osync_group_nth_member(group, i);
		if (member->id == id) {
			return member;
		}
	}
	osync_debug("OSPLG", 0, "Couldnt find the member with the id %i", id);
	return NULL;
}



/** @brief Returns if a certain object type is enabled on this member
 * 
 * @param member The member
 * @param objtype The name of the object type to check
 * @returns TRUE if the object type is enabled, FALSE otherwise
 * 
 */
osync_bool osync_member_objtype_enabled(OSyncMember *member, const char *objtype)
{
	g_assert(member);
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
	g_assert(sink);
	return sink->enabled;
}

/** @brief Enables or disables a object type on a member
 * 
 * @param member The member
 * @param objtypestr The name of the object type to change
 * @param enabled Set to TRUE if you want to sync the object type, FALSE otherwise
 * 
 * Note: this function should be called only after sink information for the member
 *       is available (osync_member_require_sink_info())
 *
 * @todo Change function interface to not require the plugin to be instanced manually.
 *       See comments on osync_group_set_objtype_enabled()
 */
void osync_member_set_objtype_enabled(OSyncMember *member, const char *objtypestr, osync_bool enabled)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i)", __func__, member, objtypestr, enabled);
	OSyncObjTypeSink *sink = NULL;
	g_assert(member);
	
	if (osync_conv_objtype_is_any(objtypestr)) {
		GList *o = NULL;
		for (o = member->objtype_sinks; o; o = o->next) {
			OSyncObjTypeSink *sink = o->data;
			sink->enabled = enabled;
		}
	} else {
		sink = osync_member_find_objtype_sink(member, objtypestr);
		if (!sink) {
			osync_trace(TRACE_EXIT_ERROR, "Unable to find sink with name \"%s\"", objtypestr);
			return;
		}
		sink->enabled = enabled;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*@}*/

/**
 * @defgroup OSyncMemberFunctions OpenSync Member Functions
 * @ingroup OSyncPublic
 * @brief The functions that can be used to access the device that a member represents
 * 
 */
/*@{*/

/** @brief Initialize a member
 * 
 * Calls the initialize function on a plugin
 * 
 * @param member The member
 * @param error A pointer to a error
 * @returns TRUE if the plugin initialized successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_initialize(OSyncMember *member, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	if (!osync_member_instance_default_plugin(member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	g_assert(member);
	g_assert(member->plugin);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	g_assert(functions.initialize);
	if (!(member->plugindata = functions.initialize(member, error))) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/** @brief Finalizes a plugin
 * 
 * Calls the finalize function on a plugin
 * 
 * @param member The member
 * 
 */
void osync_member_finalize(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	g_assert(member);
	g_assert(member->plugin);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	if (functions.finalize)
		functions.finalize(member->plugindata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Queries a plugin for the changed objects since the last sync
 * 
 * Calls the get_changeinfo function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_get_changeinfo(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	if (!functions.get_changeinfo) {
		osync_context_report_error(context, OSYNC_ERROR_GENERIC, "No get_changeinfo function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No get_changeinfo function was given", __func__);
		return;
	}
	functions.get_changeinfo(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Reads a single object by its uid
 * 
 * Calls the read_change function on the plugin
 * 
 * @param member The member
 * @param change The change to read. The change must have the uid set
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_read_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, change, function, user_data);
	
	g_assert(change);
	g_assert(change->uid);
	g_assert(osync_change_get_objformat(change));
	
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	
	//search for the right formatsink
	GList *i;
	osync_debug("OSYNC", 2, "Searching for sink");
	for (i = member->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *fmtsink = i->data;

		if (fmtsink->format == osync_change_get_objformat(change)) {
			//Read the change
			g_assert(fmtsink->functions.read != NULL);
			fmtsink->functions.read(context, change);
			osync_trace(TRACE_EXIT, "%s", __func__);
			return;
		}
	}
	
	osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to send changes");
	osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find a sink", __func__);
}

/** @brief Checks if the member has a read method for the given objtype
 * 
 * @param member The member
 * @param objtype The objtype for which to check the read methid
 * @return TRUE if the member has read function, FALSE otherwise
 * 
 */
osync_bool osync_member_has_read_function(OSyncMember *member, OSyncObjType *objtype)
{
	GList *i;
	for (i = member->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *fmtsink = i->data;

		if (osync_objformat_get_objtype(fmtsink->format) == objtype)
			return fmtsink->functions.read ? TRUE : FALSE;
	}
	return FALSE;
}

/** @brief Gets the "real" data of a object
 * 
 * Calls the get_data function on the plugin
 * 
 * @param member The member
 * @param change The change. The must be returned from a call to get_changeinfo
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_get_change_data(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, change, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	g_assert(change != NULL);
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.get_data(context, change);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Connects a member to its device
 * 
 * Calls the connect function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_connect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	if (!functions.connect) {
		osync_context_report_error(context, OSYNC_ERROR_GENERIC, "No connect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No connect function was given", __func__);
		return;
	}
	functions.connect(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Disconnects a member from its device
 * 
 * Calls the disconnect function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_disconnect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	if (!functions.disconnect) {
		osync_context_report_error(context, OSYNC_ERROR_GENERIC, "No disconnect function was given");
		osync_trace(TRACE_EXIT_ERROR, "%s: No disconnect function was given", __func__);
		return;
	}
	functions.disconnect(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Tells the plugin that the sync was successfull
 * 
 * Calls the sync_done function on a plugin
 * 
 * @param member The member
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_sync_done(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	osync_member_set_slow_sync(member, "data", FALSE);
	if (functions.sync_done) {
		functions.sync_done(context);
	} else {
		osync_context_report_success(context);
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Commits a change to the device
 * 
 * Calls the commit_change function on a plugin
 * 
 * @param member The member
 * @param change The change to write
 * @param function The function that will receive the answer to this call
 * @param user_data User data that will be passed on to the callback function
 * 
 */
void osync_member_commit_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, change, function, user_data);
	g_assert(member);
	g_assert(change);

	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	
	
	OSyncObjType *type = osync_change_get_objtype(change);
	
	/* This is just an optmization:
	 *
	 * the path search function will avoid doing
	 * cross-objtype conversions, so
	 * if we already have a sink for the original objtype,
	 * and it is disabled, we can drop the change
	 * without doing detection/conversion first.
	 *
	 * If the objtype will change during conversion,
	 * we check the right objtype sink later,
	 * anyway
	 */
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, type->name);
	if (sink && !sink->enabled) {
		osync_context_report_success(context);
		osync_trace(TRACE_EXIT, "%s: Sink not enabled", __func__);
		return;
	}


	//The destobjtype is the objtype of the format to which
	//the change was just converted
	change->destobjtype = g_strdup(osync_change_get_objtype(change)->name);
	
	//Filter the change.
	if (!osync_filter_change_allowed(member, change)) {
		osync_context_report_success(context);
		osync_trace(TRACE_EXIT, "%s: Change filtered", __func__);
		return;
	}

	//search for the right formatsink, now that
	//the change was converted
	GList *i;
	osync_debug("OSYNC", 2, "Searching for sink");
	for (i = member->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *fmtsink = i->data;

		osync_debug("OSYNC", 2, "Comparing change %s with sink %s", osync_change_get_objformat(change)->name, fmtsink->format ? fmtsink->format->name : "None");
		if (fmtsink->format == osync_change_get_objformat(change)) {
			if (fmtsink->functions.batch_commit) {
				//Append to the stored changes
				fmtsink->commit_changes = g_list_append(fmtsink->commit_changes, change);
				fmtsink->commit_contexts = g_list_append(fmtsink->commit_contexts, context);
				osync_trace(TRACE_EXIT, "%s: Waiting for batch processing", __func__);
				return;
			} else {
				// Send the change
				if (!fmtsink->functions.commit_change) {
					osync_context_report_error(context, OSYNC_ERROR_GENERIC, "No commit_change function was given");
					osync_trace(TRACE_EXIT_ERROR, "%s: No commit_change function was given", __func__);
					return;
				}
				fmtsink->functions.commit_change(context, change);
				osync_trace(TRACE_EXIT, "%s", __func__);
				return;
			}
		}
	}

	osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to send changes");
	osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find a sink", __func__);
}

/** @brief Tells the plugin that all changes have been committed
 * 
 * Calls the committed_all function on a plugin or the batch_commit function
 * depending on which function the plugin wants to use.
 * 
 * @param member The member
 * @param function The callback that will receive the answer
 * @param user_data The userdata to pass to the callback
 * 
 */
void osync_member_committed_all(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	OSyncChange **changes = NULL;
	OSyncContext **contexts = NULL;

	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;

	GList *pendingchanges = NULL;
	GList *pendingcontexts = NULL;

	GList *f;
	for (f = member->format_sinks; f; f = f->next) {
		OSyncObjFormatSink *fmtsink = f->data;
		osync_debug("OSYNC", 2, "Sending changes to sink %p:%s", fmtsink, fmtsink->format ? fmtsink->format->name : "None");

		OSyncFormatFunctions functions = fmtsink->functions;

		if (functions.batch_commit) {
			pendingchanges = g_list_concat(pendingchanges, fmtsink->commit_changes);
			pendingcontexts = g_list_concat(pendingcontexts, fmtsink->commit_contexts);
			
			fmtsink->commit_changes = NULL;
			fmtsink->commit_contexts = NULL;
		}
	}
	
	f = member->format_sinks;
	if (f) {
		OSyncObjFormatSink *fmtsink = f->data;
		osync_debug("OSYNC", 2, "Sending committed all to sink %p:%s", fmtsink, fmtsink->format ? fmtsink->format->name : "None");

		OSyncFormatFunctions functions = fmtsink->functions;
	
		if (functions.batch_commit) {
			int i = 0;
			changes = g_malloc0(sizeof(OSyncChange *) * (g_list_length(pendingchanges) + 1));
			contexts = g_malloc0(sizeof(OSyncContext *) * (g_list_length(pendingcontexts) + 1));;
			GList *o = pendingcontexts;
			GList *c = NULL;
			
			for (c = pendingchanges; c && o; c = c->next) {
				OSyncChange *change = c->data;
				OSyncContext *context = o->data;
				
				changes[i] = change;
				contexts[i] = context;
				
				i++;
				o = o->next;
			}
			
			g_list_free(pendingchanges);
			g_list_free(pendingcontexts);
			
			functions.batch_commit(context, contexts, changes);
			
			g_free(changes);
			g_free(contexts);
		} else if (functions.committed_all) {
			functions.committed_all(context);
		} else {
			osync_context_report_success(context);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_member_set_name(OSyncMember *member, const char *name)
{
	g_assert(member);
	if (member->name)
		g_free(member->name);
	member->name = g_strdup(name);
}

const char *osync_member_get_name(OSyncMember *member)
{
	return member->name;
}

/** @brief Adds random data to a member
 * 
 * Generates random data and writes it to the plugin. The plugin must support the access
 * function. This function is mainly used for testing plugins.
 * 
 * @param member The member on which to add random data
 * @param objtype The name of the object type to add
 * @returns The change that was added or NULL if adding the data was not successfull
 * 
 */
OSyncChange *osync_member_add_random_data(OSyncMember *member, const char *objtype)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	OSyncContext *context = osync_context_new(member);
	OSyncChange *change = osync_change_new();
	change->changetype = CHANGE_ADDED;
	OSyncObjFormatSink *format_sink;
	if (!(format_sink = osync_member_make_random_data(member, change, objtype))) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to make random data", __func__);
		return NULL;
	}
	
	if (!format_sink->functions.access) {
		osync_trace(TRACE_EXIT_ERROR, "%s: No access function", __func__);
		return NULL;
	}
	
	if (!format_sink->functions.access(context, change)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to write change", __func__);
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;
}

/** @brief Modifies random data on a member
 * 
 * The plugin must support the access
 * function. This function is mainly used for testing plugins.
 * 
 * @param member The member on which to add random data
 * @param change The change that should be modified. It must have the uid set.
 * @returns TRUE if the changes was modified successfully, FALSE otherwise
 * 
 */
osync_bool osync_member_modify_random_data(OSyncMember *member, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, change);
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_MODIFIED;
	OSyncObjFormatSink *format_sink;
	char *uid = g_strdup(osync_change_get_uid(change));
	
	if (!(format_sink = osync_member_make_random_data(member, change, osync_change_get_objtype(change)->name))) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to make random data", __func__);
		return FALSE;
	}

	osync_change_set_uid(change, uid);
	
	if (!format_sink->functions.access) {
		osync_trace(TRACE_EXIT_ERROR, "%s: No access function", __func__);
		return FALSE;
	}
	
	if (!format_sink->functions.access(context, change)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to modify change", __func__);
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/** @brief Deletes data from a device
 * 
 * The plugin must support the access function. This is mainly
 * used for testing plugins.
 * 
 * @param member The member from which to delete
 * @param change The change to delete. The uid must be set
 * @returns TRUE if the change was deleted, FALSE otherwise
 * 
 */
osync_bool osync_member_delete_data(OSyncMember *member, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, change);
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_DELETED;
	
	OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, osync_change_get_objtype(change)->name);
	if (!objtype_sink) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find objsink for %s", __func__, osync_change_get_objtype(change)->name);
		return FALSE;
	}
	
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncObjFormatSink *format_sink = osync_objtype_find_format_sink(objtype_sink, format->name);
	if (!format_sink) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find format sink for %s", __func__, format->name);
		return FALSE;
	}
	
	if (format_sink->functions.batch_commit) {
		//Append to the stored changes
		format_sink->commit_changes = g_list_append(format_sink->commit_changes, change);
		format_sink->commit_contexts = g_list_append(format_sink->commit_contexts, context);
		osync_trace(TRACE_EXIT, "%s: Waiting for batch processing", __func__);
		return TRUE;
	} else {
		if (!format_sink->functions.access) {
			osync_trace(TRACE_EXIT_ERROR, "%s: No access function", __func__);
			return FALSE;
		}
		
		if (!format_sink->functions.access(context, change)) {
			osync_trace(TRACE_EXIT_ERROR, "%s: Unable to modify change", __func__);
			return FALSE;
		}
	}
		
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*@}*/


/** Write the list of objtypes that had slow-sync set to a message */
void osync_member_write_sink_info(OSyncMember *member, OSyncMessage *message)
{
	GList *obj = NULL;
	for (osync_member_get_objtype_sinks(member, &obj, NULL); obj; obj = obj->next) {
		OSyncObjTypeSink *sink = obj->data;
		int slow;
		slow = osync_member_get_slow_sync(member, sink->objtype->name);
		osync_message_write_string(message, sink->objtype->name);
		osync_message_write_int(message, sink->read);
		osync_message_write_int(message, sink->write);
		osync_message_write_int(message, sink->enabled);
		osync_message_write_int(message, slow);
	}
	osync_message_write_string(message, NULL);
}

/** Read sink info for member
 *
 * @see osync_member_read_sink_info_full(), osync_member_read_sink_info()
 */
int __sync_member_read_sink_info(OSyncMember *member, OSyncMessage *message)
{
	char *objtypestr;
	int r = 0;
	for (;;) {
		int read, write, enabled, slow;
		osync_message_read_string(message, &objtypestr);
		if (!objtypestr)
			break;

		osync_message_read_int(message, &read);
		osync_message_read_int(message, &write);
		osync_message_read_int(message, &enabled);
		osync_message_read_int(message, &slow);

		OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtypestr);
		if (!sink)
			continue;

		sink->read = read;
		sink->write = write;
		sink->enabled = enabled;

		if (slow) {
			osync_member_set_slow_sync(member, objtypestr, TRUE);
			r++;
		}

		free(objtypestr);
	}
	return r;
}


/** Read a list of objtypes that had slow-sync set, replacing old settings
 *
 * Please notice that this function will reset any old
 * slow-sync setting that was set before. So, this should
 * be used only for messages that is known to contain the
 * complete slow-sync settings, not only for a member.
 *
 * i.e. this function may be used to read the slow-sync settings
 * from the engine to osplugin, but not to read the slow-sync
 * settings from osplugin to the engine, because osplugin doesn't
 * know about the slow-sync settings of other members in the sync
 * group.
 */
void osync_member_read_sink_info_full(OSyncMember *member, OSyncMessage *message)
{
	osync_group_reset_slow_sync(member->group, "data");
	__sync_member_read_sink_info(member, message);
}

/** Read a list of objtypes that had slow-sync set, keeping old settings
 *
 * Please notice that this function won't reset
 * the list of slow-sync settings, like
 * osync_message_read_slow_sync_full_list(), but instead
 * it will just set slow-sync for the objtypes received
 * in the list.
 *
 * This function is supposed to be used when handling messages
 * from osplugin to the engine, but NOT for messages from the engine
 * to osplugin.
 */
void osync_member_read_sink_info(OSyncMember *member, OSyncMessage *message)
{
	int set_for_any;

	set_for_any = __sync_member_read_sink_info(member, message);

	if (set_for_any) {
		/* FIXME: We must force slow-sync in "data" when some
		 * objtype is marked to slow-sync
		 *
		 * (See ticket #1011)
		 */
		osync_member_set_slow_sync(member, "data", TRUE);
	}
}

