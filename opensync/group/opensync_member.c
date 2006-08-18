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

#include "opensync-plugin.h"
#include "opensync-group.h"
#include "opensync_member_internals.h"

#include "opensync_xml.h"

/**
 * @defgroup OSyncMemberPrivateAPI OpenSync Member Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncMember
 * 
 */
/*@{*/

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
OSyncMember *osync_member_new(OSyncError **error)
{
	OSyncMember *member = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	member = osync_try_malloc0(sizeof(OSyncMember), error);
	if (!member)
		goto error;
	member->starttype = OSYNC_START_TYPE_THREAD;
	member->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, member);
	return member;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(error));
	return NULL;
}

void osync_member_ref(OSyncMember *member)
{
	osync_assert(member);
	
	g_atomic_int_inc(&(member->ref_count));
}

void osync_member_unref(OSyncMember *member)
{
	osync_assert(member);
		
	if (g_atomic_int_dec_and_test(&(member->ref_count))) {
		if (member->pluginname)
			g_free(member->pluginname);
		
		if (member->configdir)
			g_free(member->configdir);
		
		if (member->configdata)
			g_free(member->configdata);
		
		while (member->objtypes) {
			OSyncObjTypeSink *sink = member->objtypes->data;
			osync_objtype_sink_unref(sink);
			member->objtypes = g_list_remove(member->objtypes, member->objtypes->data);
		}
		
		g_free(member);
	}
}

/** @brief Returns the name of the default plugin of the member
 * 
 * @param member The member
 * @returns The name of the plugin
 * 
 */
const char *osync_member_get_pluginname(OSyncMember *member)
{
	osync_assert(member);
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
	osync_assert(member);
	if (member->pluginname)
		g_free(member->pluginname);
	member->pluginname = g_strdup(pluginname);
}

/** @brief Returns the configuration directory where this member is stored
 * 
 * @param member The member
 * @returns The configuration directory
 * 
 */
const char *osync_member_get_configdir(OSyncMember *member)
{
	osync_assert(member);
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
	osync_assert(member);
	if (member->configdir)
		g_free(member->configdir);
	member->configdir = g_strdup(configdir);
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
const char *osync_member_get_config_or_default(OSyncMember *member, OSyncError **error)
{
	char *filename = NULL;
	char *data = NULL;
	const char *config = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	g_assert(member);

	if (member->configdata) {
		osync_trace(TRACE_EXIT, "%s: Configdata already in memory", __func__);
		return member->configdata;
	}
	
	filename = g_strdup_printf("%s"G_DIR_SEPARATOR_S"%s.conf", member->configdir, member->pluginname);
	osync_trace(TRACE_INTERNAL, "Reading %s", filename);
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		if (!osync_file_read(filename, &data, NULL, error))
			goto error_free_filename;
		g_free(filename);
		
		osync_member_set_config(member, data);
		
		/* Free the data and return the const pointer */
		g_free(data);
		const char *config = osync_member_get_config(member, error);
		
		osync_trace(TRACE_EXIT, "%s: Read from member directory", __func__);
		return config;
	}
	g_free(filename);

	filename = g_strdup_printf(OPENSYNC_CONFIGDIR G_DIR_SEPARATOR_S"%s", member->pluginname);
	osync_trace(TRACE_INTERNAL, "Reading default %s", filename);
	if (!osync_file_read(filename, &data, NULL, error))
		goto error_free_filename;
	g_free(filename);
	
	osync_member_set_config(member, data);
	g_free(data);
		
	config = osync_member_get_config(member, error);
		
	osync_trace(TRACE_EXIT, "%s: Read default config", __func__);
	return osync_member_get_config(member, NULL);

error_free_filename:
	g_free(filename);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
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
const char *osync_member_get_config(OSyncMember *member, OSyncError **error)
{
	char *filename = NULL;
	char *data = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	osync_assert(member);

	if (member->configdata) {
		osync_trace(TRACE_EXIT, "%s: Configdata already in memory", __func__);
		return member->configdata;
	}
	
	filename = g_strdup_printf("%s/%s.conf", member->configdir, member->pluginname);
	osync_trace(TRACE_INTERNAL, "Reading config from: %s", filename);
	
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		if (!osync_file_read(filename, &data, NULL, error)) {
			g_free(filename);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return NULL;
		}
		g_free(filename);
		
		osync_member_set_config(member, data);
		g_free(data);
		
		osync_trace(TRACE_EXIT, "%s: Read set config from member", __func__);
		return osync_member_get_config(member, NULL);
	}
	g_free(filename);
	
	osync_error_set(error, OSYNC_ERROR_GENERIC, "Plugin is not configured");
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
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
void osync_member_set_config(OSyncMember *member, const char *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, data);
	g_assert(member);
	
	if (member->configdata)
		g_free(member->configdata);

	member->configdata = g_strdup(data);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static OSyncObjTypeSink *_osync_member_parse_objtype(xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, cur, error);
	OSyncObjTypeSink *sink = osync_objtype_sink_new(NULL, error);
	if (!sink) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"name")) {
				osync_objtype_sink_set_name(sink, str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"enabled")) {
				osync_objtype_sink_set_enabled(sink, atoi(str));
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, sink);
	return sink;
}

/** @brief Loads a member from a directory where it has been saved
 * 
 * @param group The group which is the parent
 * @param path The path of the member
 * @param error Pointer to a error
 * @returns A newly allocated member thats stored in the group or NULL if error
 * 
 */
osync_bool osync_member_load(OSyncMember *member, const char *path, OSyncError **error)
{	xmlDocPtr doc;
	xmlNodePtr cur;
	char *filename = NULL;
	char *basename = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, member, path, error);

	filename = g_strdup_printf ("%s/syncmember.conf", path);
	
	basename = g_path_get_basename(path);
	member->id = atoi(basename);
	g_free(basename);
	osync_member_set_configdir(member, path);
	
	if (!osync_open_xml_file(&doc, &cur, filename, "syncmember", error)) {
		g_free(filename);
		goto error;
	}
	g_free(filename);

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"pluginname")) {
				member->pluginname = g_strdup(str);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"objtype")) {
				OSyncObjTypeSink *sink = _osync_member_parse_objtype(cur->xmlChildrenNode, error);
				if (!sink)
					goto error_free_doc;
				member->objtypes = g_list_append(member->objtypes, sink);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
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
	char *filename = NULL;
	xmlDocPtr doc = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	osync_assert(member);
	osync_assert(member->configdir);
	
	if (!g_file_test(member->configdir, G_FILE_TEST_IS_DIR)) {
		if (mkdir(member->configdir, 0700)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for member %li\n", member->id);
			goto error;
		}
	}
	
	//Saving the syncmember.conf
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	doc = xmlNewDoc((xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"syncmember", NULL);
	
	//The plugin name
	xmlNewChild(doc->children, NULL, (xmlChar*)"pluginname", (xmlChar*)member->pluginname);
	
	//The objtypes
	GList *o = NULL;
	for (o = member->objtypes; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;
		xmlNode *node = xmlNewChild(doc->children, NULL, (xmlChar*)"objtype", NULL);
		
		xmlNewChild(node, NULL, (xmlChar*)"name", (xmlChar*)osync_objtype_sink_get_name(sink));
		xmlNewChild(node, NULL, (xmlChar*)"enabled", osync_objtype_sink_is_enabled(sink) ? (xmlChar*)"1" : (xmlChar*)"0");
		
	}
	
	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);
	
	//Saving the config if it exists
	if (member->configdata) {
		filename = g_strdup_printf("%s/%s.conf", member->configdir, member->pluginname);
		if (!osync_file_write(filename, member->configdata, strlen(member->configdata), 0600, error)) {
			g_free(filename);
			goto error;
		}
		
		g_free(filename);
		
		g_free(member->configdata);
		member->configdata = NULL;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_member_delete(OSyncMember *member, OSyncError **error)
{
	char *delcmd = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);
	osync_assert(member);
	
	delcmd = g_strdup_printf("rm -rf %s", member->configdir);
	if (system(delcmd)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to delete member. command %s failed", delcmd);
		g_free(delcmd);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	g_free(delcmd);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/** @brief Gets the unique id of a member
 * 
 * @param member The member
 * @returns The id of the member thats unique in its group
 * 
 */
long long int osync_member_get_id(OSyncMember *member)
{
	osync_assert(member);
	return member->id;
}

static OSyncObjTypeSink *_osync_member_find_objtype(OSyncMember *member, const char *objtype)
{
	GList *o;
	for (o = member->objtypes; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;
		if (!strcmp(osync_objtype_sink_get_name(sink), objtype))
			return sink;
	}
	return NULL;
}

void osync_member_add_objtype(OSyncMember *member, const char *objtype)
{
	OSyncObjTypeSink *sink = NULL;
	osync_assert(member);
	osync_assert(objtype);

	if (!_osync_member_find_objtype(member, objtype)) {
		sink = osync_objtype_sink_new(objtype, NULL);
		member->objtypes = g_list_append(member->objtypes, sink);
	}
}

int osync_member_num_objtypes(OSyncMember *member)
{
	osync_assert(member);
	return g_list_length(member->objtypes);
}

const char *osync_member_nth_objtype(OSyncMember *member, int nth)
{
	OSyncObjTypeSink *sink = NULL;
	osync_assert(member);
	sink = g_list_nth_data(member->objtypes, nth);
	return osync_objtype_sink_get_name(sink);
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
	OSyncObjTypeSink *sink = NULL;
	osync_assert(member);
	sink = _osync_member_find_objtype(member, objtype);
	if (!sink)
		return FALSE;
	return osync_objtype_sink_is_enabled(sink);
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
void osync_member_set_objtype_enabled(OSyncMember *member, const char *objtype, osync_bool enabled)
{
	OSyncObjTypeSink *sink = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %i)", __func__, member, objtype, enabled);
	osync_assert(member);
	
	sink = _osync_member_find_objtype(member, objtype);
	if (!sink) {
		osync_trace(TRACE_EXIT, "%s: Unable to find objtype", __func__);
		return;
	}
		
	osync_objtype_sink_set_enabled(sink, enabled);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_member_set_start_type(OSyncMember *member, OSyncStartType type)
{
	osync_assert(member);
	member->starttype = type;
}

OSyncStartType osync_member_get_start_type(OSyncMember *member)
{
	osync_assert(member);
	return member->starttype;
}

/*@}*/
