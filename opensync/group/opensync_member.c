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
#include "opensync-merger.h"
#include "opensync_member_internals.h"

#include "opensync_xml.h"

/**
 * @defgroup OSyncMemberPrivateAPI OpenSync Member Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncMember
 * 
 */
/*@{*/

/** @brief Set Merger of Member 
 * 
 * @param member The Member pointer 
 * 
 */
static void _osync_member_set_merger(OSyncMember *member, OSyncMerger *merger)
{
	osync_assert(member);
	
	if (member->merger)
		osync_merger_unref(member->merger);
	member->merger = merger;
	if(merger)
		osync_merger_ref(member->merger);
}


/** @brief Parser for the "timeout" node in  the member configuration
 * 
 * @param cur Pointer to the xmlNode 
 * @param sink Pointer to the OSyncObjTypeSink object
 * 
 */
void _osync_member_parse_timeout(xmlNode *cur, OSyncObjTypeSink *sink)
{
	osync_assert(sink);

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"connect")) {
				osync_objtype_sink_set_connect_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"disconnect")) {
				osync_objtype_sink_set_disconnect_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"get_changes")) {
				osync_objtype_sink_set_getchanges_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"commit")) {
				osync_objtype_sink_set_commit_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"batch_commit")) {
				osync_objtype_sink_set_batchcommit_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"committed_all")) {
				osync_objtype_sink_set_committedall_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"sync_done")) {
				osync_objtype_sink_set_syncdone_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"write")) {
				osync_objtype_sink_set_write_timeout(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"read")) {
				osync_objtype_sink_set_read_timeout(sink, atoi(str));
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
}

/** @brief Parser for the "objtype" node in  the member configuration
 * 
 * @param cur Pointer to the xmlNode 
 * @param error Pointer to a error
 * @returns Object type sink of the parsed configuration. NULL on error.
 * 
 */
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
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"read")) {
				osync_objtype_sink_set_read(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"getchanges")) {
				osync_objtype_sink_set_getchanges(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"write")) {
				osync_objtype_sink_set_write(sink, atoi(str));
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"objformat")) {
				char *str_name = osync_xml_find_node(cur, "name");
				char *str_config = osync_xml_find_node(cur, "config");
				osync_objtype_sink_add_objformat_with_config(sink, str_name, str_config);
				xmlFree(str_name);
				xmlFree(str_config);
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"timeout")) {
				_osync_member_parse_timeout(cur->xmlChildrenNode, sink);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, sink);
	return sink;
}

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

	member->ref_count = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, member);
	return member;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(error));
	return NULL;
}

/** @brief Increase the reference count of the member
 * 
 * @param member The member
 * 
 */
OSyncMember *osync_member_ref(OSyncMember *member)
{
	osync_assert(member);
	
	g_atomic_int_inc(&(member->ref_count));

	return member;
}

/** @brief Decrease the reference count of the member
 * 
 * @param member The member
 * 
 */
void osync_member_unref(OSyncMember *member)
{
	osync_assert(member);
		
	if (g_atomic_int_dec_and_test(&(member->ref_count))) {
		if (member->pluginname)
			g_free(member->pluginname);

		if (member->name)
			g_free(member->name);

		if (member->configdir)
			g_free(member->configdir);
		
		if (member->configdata)
			g_free(member->configdata);
			
		if (osync_member_get_capabilities(member))
			osync_capabilities_unref(osync_member_get_capabilities(member));
			
		if (osync_member_get_merger(member))
			osync_merger_unref(osync_member_get_merger(member));
		
		osync_member_flush_objtypes(member);

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

/** @brief Returns the inidividual name of the member
 * 
 * @param member The member
 * @returns The name of the plugin
 * 
 */
const char *osync_member_get_name(OSyncMember *member)
{
	osync_assert(member);
	return member->name;
}

/** @brief Sets an individual name of the member
 * 
 * @param member The member
 * @param name The individual name of the member 
 * 
 */
void osync_member_set_name(OSyncMember *member, const char *name)
{
	osync_assert(member);
	if (member->name)
		g_free(member->name);
	member->name = g_strdup(name);
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
 * @param error Pointer to a error
 * @returns The member configuration of the plugin default configuration if the member isn't configuered already 
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

osync_bool osync_member_has_config(OSyncMember *member)
{
	osync_assert(member);
	return member->configdata ? TRUE : FALSE;
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
 * @param error Pointer to a error
 * @returns Member configuration 
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

/** @brief Loads a member from a directory where it has been saved
 * 
 * @param member The Member pointer of the member which gets loaded
 * @param path The path of the member
 * @param error Pointer to a error
 * @returns TRUE on success, FALSE if error
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
	
	if (!osync_xml_open_file(&doc, &cur, filename, "syncmember", error)) {
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
			} else if (!xmlStrcmp(cur->name, (const xmlChar *)"timeout")) {
				/* Sink Function Timeout settings for main sink */
				if (!member->main_sink)
					member->main_sink = osync_objtype_main_sink_new(error);

				if (!member->main_sink) {
					osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
					goto error_free_doc;
				}

				_osync_member_parse_timeout(cur->xmlChildrenNode, member->main_sink);
			}

			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);

	if(osync_capabilities_member_has_capabilities(member))
	{
		OSyncCapabilities* capabilities = osync_capabilities_member_get_capabilities(member, error);
		if(!capabilities)
			goto error;
		if(!osync_member_set_capabilities(member, capabilities, error))
			goto error;
		osync_capabilities_unref(capabilities);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_member_save_sink_add_timeout(xmlNode *cur, const char *timeoutname, unsigned int timeout, OSyncError **error)
{
	/* Skip if no custom timeout got set */
	if (!timeout)
		return TRUE;

	char *str = g_strdup_printf("%u", timeout);
	xmlNewChild(cur, NULL, (xmlChar*)timeoutname, str);
	g_free(str);

	return TRUE;
}

static osync_bool _osync_member_save_sink_timeout(xmlNode *cur, OSyncObjTypeSink *sink, OSyncError **error)
{
	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"timeout", NULL);
	char *timeout = NULL;

	_osync_member_save_sink_add_timeout(node, "connect", osync_objtype_sink_get_connect_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "disconnect", osync_objtype_sink_get_disconnect_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "get_changes", osync_objtype_sink_get_getchanges_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "commit", osync_objtype_sink_get_commit_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "batch_commit", osync_objtype_sink_get_batchcommit_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "committed_all", osync_objtype_sink_get_committedall_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "sync_done", osync_objtype_sink_get_syncdone_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "write", osync_objtype_sink_get_write_timeout(sink), error);
	_osync_member_save_sink_add_timeout(node, "read", osync_objtype_sink_get_read_timeout(sink), error);

	if (!node->children) { 
		xmlUnlinkNode(node);
		xmlFreeNode(node);
	}


	return TRUE;
}

static osync_bool _osync_member_save_sink(xmlDoc *doc, OSyncObjTypeSink *sink, OSyncError **error)
{
	int i = 0;
	xmlNode *node = NULL;

	/* Write main sink stuff in main node. */
	if (sink && !osync_objtype_sink_get_name(sink)) {
		node = doc->children;
	} else {
		node = xmlNewChild(doc->children, NULL, (xmlChar*)"objtype", NULL);
	}

	xmlNewChild(node, NULL, (xmlChar*)"enabled", osync_objtype_sink_is_enabled(sink) ? (xmlChar*)"1" : (xmlChar*)"0");
	xmlNewChild(node, NULL, (xmlChar*)"read", osync_objtype_sink_get_read(sink) ? (xmlChar*)"1" : (xmlChar*)"0");
	xmlNewChild(node, NULL, (xmlChar*)"getchanges", osync_objtype_sink_get_getchanges(sink) ? (xmlChar*)"1" : (xmlChar*)"0");
	xmlNewChild(node, NULL, (xmlChar*)"write", osync_objtype_sink_get_write(sink) ? (xmlChar*)"1" : (xmlChar*)"0");

	/* Check if sink is a Main Sink, if so skip objtype specific stuff */
	if (sink && !osync_objtype_sink_get_name(sink))
		return TRUE;

	/* Objtype specific settings */
	xmlNewChild(node, NULL, (xmlChar*)"name", (xmlChar*)osync_objtype_sink_get_name(sink));

	for (i = 0; i < osync_objtype_sink_num_objformats(sink); i++) {
		const char *format = osync_objtype_sink_nth_objformat(sink, i);
		const char *format_config = osync_objtype_sink_nth_objformat_config(sink, i);
		xmlNode *objformat_node = xmlNewChild(node, NULL, (xmlChar*)"objformat", NULL);
		xmlNewChild(objformat_node, NULL, (xmlChar*)"name", (xmlChar*)format);
		xmlNewChild(objformat_node, NULL, (xmlChar*)"config", (xmlChar*)format_config);
	}

	return _osync_member_save_sink_timeout(node, sink, error);
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
		if (g_mkdir(member->configdir, 0700)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for member %li\n", member->id);
			goto error;
		}
	}
	
	doc = xmlNewDoc((xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"syncmember", NULL);

	char *version_str = g_strdup_printf("%u.%u", MEMBER_MAJOR_VERSION, MEMBER_MINOR_VERSION);
	xmlSetProp(doc->children, (const xmlChar*)"version", (const xmlChar *)version_str);	
	g_free(version_str);

	//The plugin name
	xmlNewChild(doc->children, NULL, (xmlChar*)"pluginname", (xmlChar*)member->pluginname);

	//The main sink
	if (member->main_sink && !_osync_member_save_sink(doc, member->main_sink, error)) {
		xmlFreeDoc(doc);
		goto error;
	}
	
	//The objtypes
	GList *o = NULL;
	for (o = member->objtypes; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;

		if (!_osync_member_save_sink(doc, sink, error)) {
			xmlFreeDoc(doc);
			goto error;
		}
	}
	
	/* TODO Validate file before storing! */

	//Saving the syncmember.conf
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	xmlSaveFormatFile(filename, doc, 1);
	g_free(filename);

	xmlFreeDoc(doc);
	
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
	
	OSyncCapabilities* capabilities = osync_member_get_capabilities(member);
	if(capabilities) {
		if(!osync_capabilities_member_set_capabilities(member, capabilities, error)) {
			goto error;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** @brief Delete a member
 * 
 * @param member The member to delete 
 * @param error Pointer to a error
 * @returns TRUE if the member was deleted successfully, FALSE otherwise
 * 
 */
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

/** @brief Find the object type sink (OSyncObjTypeSink) for the given object type of
 *         a certain member.
 * 
 * @param member The member pointer
 * @param objtype The searched object type 
 * @returns OSyncObjTypeSink pointer if object type sink is avaliable, otherwise NULL 
 * 
 */
OSyncObjTypeSink *osync_member_find_objtype_sink(OSyncMember *member, const char *objtype)
{
	osync_assert(member);

	GList *o;
	for (o = member->objtypes; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;
		if (!strcmp(osync_objtype_sink_get_name(sink), objtype))
			return sink;
	}
	return NULL;
}

/** @brief Add a specifc Object Format to member 
 * 
 * @param member The member pointer
 * @param objtype The searched object type 
 * @param format The name of the Object Format 
 * 
 */
void osync_member_add_objformat(OSyncMember *member, const char *objtype, const char *format)
{
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
	if (!sink)
		return;
	
	osync_objtype_sink_add_objformat(sink, format);
}

/** @brief Add a specifc Object Format with a conversion path config to member 
 * 
 * @param member The member pointer
 * @param objtype The searched object type 
 * @param format The name of the Object Format 
 * 
 */
void osync_member_add_objformat_with_config(OSyncMember *member, const char *objtype, const char *format, const char *format_config)
{
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
	if (!sink)
		return;
	
	osync_objtype_sink_add_objformat_with_config(sink, format, format_config);
}

/** @brief List of all available object formats for a specifc object type of this member 
 * 
 * @param member The member pointer
 * @param objtype The searched object type 
 * @param error Pointer to a error
 * @return List of all object formats of a specific object type of the member
 * 
 */
const OSyncList *osync_member_get_objformats(OSyncMember *member, const char *objtype, OSyncError **error)
{
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
	if (!sink) {
		sink = osync_member_find_objtype_sink(member, "data");
		if (!sink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find objtype %s", objtype);
			return NULL;
		}
	}
	
	return osync_objtype_sink_get_objformats(sink);
}

/** @brief Add an OSyncObjTypeSink object to the member list of supported object types of this member
 * 
 * @param member The member pointer
 * @param sink The OSyncObjTypeSink object to add 
 * 
 */
void osync_member_add_objtype_sink(OSyncMember *member, OSyncObjTypeSink *sink)
{
	osync_assert(member);
	osync_assert(sink);

	member->objtypes = g_list_append(member->objtypes, sink);
	osync_objtype_sink_ref(sink);
}

/** @brief Remove an OSyncObjTypeSink object to the member list of supported object types of this member
 * 
 * @param member The member pointer
 * @param sink The OSyncObjTypeSink object to add 
 * 
 */
void osync_member_remove_objtype_sink(OSyncMember *member, OSyncObjTypeSink *sink)
{
	osync_assert(member);
	osync_assert(sink);

	member->objtypes = g_list_remove(member->objtypes, sink);
	osync_objtype_sink_unref(sink);
}

/** @brief The number of supported object types of this member
 * 
 * @param member The member pointer
 * @returns Number of supported object type of this member
 * 
 */
int osync_member_num_objtypes(OSyncMember *member)
{
	osync_assert(member);
	return g_list_length(member->objtypes);
}

/** @brief The name of the nth supported object type of this member
 * 
 * @param member The member pointer
 * @param nth The nth position of the list of supported object types of this member
 * @returns Name of the nth supported object type
 * 
 */
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
	sink = osync_member_find_objtype_sink(member, objtype);
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
	
	sink = osync_member_find_objtype_sink(member, objtype);
	if (!sink) {
		osync_trace(TRACE_EXIT, "%s: Unable to find objtype", __func__);
		return;
	}
		
	osync_objtype_sink_set_enabled(sink, enabled);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Get the capabilities of the member 
 * 
 * @param member The member
 * @returns The capabilities of this member, NULL if no capabilities are set
 */
OSyncCapabilities *osync_member_get_capabilities(OSyncMember *member)
{
	osync_assert(member);
	return member->capabilities;
}

/** @brief Set the capabilities of the member 
 * 
 * @param member The member
 * @param capabilities The capabilities
 * @param error Pointer to a error
 * @returns TRUE if the capabilities got set successfully, otherwise FALSE 
 */
osync_bool osync_member_set_capabilities(OSyncMember *member, OSyncCapabilities *capabilities, OSyncError **error)
{
	osync_assert(member);
	
	if (member->capabilities)
		osync_capabilities_unref(member->capabilities);
	member->capabilities = capabilities;
	if(capabilities) {
		osync_capabilities_ref(member->capabilities);
		OSyncMerger* merger = osync_merger_new(member->capabilities, error);
		if(!merger)
			return FALSE;
		_osync_member_set_merger(member, merger);
		osync_merger_unref(merger);
	}
	return TRUE;
}

/** @brief Get pointer of the Merger 
 * 
 * @param member The member
 * @returns The pointer of the Merger, NULL if merger is disabled
 */
OSyncMerger *osync_member_get_merger(OSyncMember *member)
{
	osync_assert(member);
	return member->merger;
}

/** @brief Remove all object types from member. 
 * 
 * @param member The member
 *
 * Note: this function should be called to flush the member before discovering.
 *       To detect if something isn't supported anymore.
 *
 */
void osync_member_flush_objtypes(OSyncMember *member)
{
	osync_assert(member);

        while (member->objtypes) {
                OSyncObjTypeSink *sink = member->objtypes->data;
                osync_objtype_sink_unref(sink);
                member->objtypes = g_list_remove(member->objtypes, member->objtypes->data);
        }

	if (member->main_sink) {
		osync_objtype_sink_unref(member->main_sink);
		member->main_sink = NULL;
	}
}

/** @brief Get the main sink of member. 
 * 
 * @param member The member
 * @returns OSyncObjTypeSink pointer of the main sink.
 *
 */
OSyncObjTypeSink *osync_member_get_main_sink(OSyncMember *member)
{
	osync_assert(member);
	return member->main_sink;
}

/** @brief Checks if the member configuration is up to date. 
 * 
 * @param member The member
 * @returns TRUE if member configuration is up to date. 
 *
 */
osync_bool osync_member_config_is_uptodate(OSyncMember *member)
{
	osync_assert(member);
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);

	xmlDocPtr doc;
	xmlNodePtr cur;
	OSyncError *error = NULL;
	unsigned int version_major;
	unsigned int version_minor;
	xmlChar *version_str = NULL;
	osync_bool uptodate = FALSE;

	char *config = g_strdup_printf("%s%c%s",
			osync_member_get_configdir(member),
			G_DIR_SEPARATOR, "syncmember.conf");
	
	/* If syncmember isn't present, we assume that update is required. */
	if (!osync_xml_open_file(&doc, &cur, config, "syncmember", &error))
		goto end;

	version_str = xmlGetProp(cur->parent, (const xmlChar *)"version");

	/* No version node, means very outdated version. */
	if (!version_str)
		goto end;

      	sscanf(version_str, "%u.%u", &version_major, &version_minor);

	osync_trace(TRACE_INTERNAL, "Version: %s (current %u.%u required %u.%u)",
			version_str, version_major, version_minor, 
			OSYNC_MEMBER_MAJOR_VERSION, OSYNC_MEMBER_MINOR_VERSION );

	if (OSYNC_MEMBER_MAJOR_VERSION == version_major 
			&& OSYNC_MEMBER_MINOR_VERSION == version_minor)
		uptodate = TRUE;

	xmlFree(version_str);

end:
	g_free(config);

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	return uptodate;
}

/** @brief Checks if the plugin configuration is up to date. 
 * 
 * @param member The member
 * @returns TRUE if plugin configuration is up to date. 
 *
 */
osync_bool osync_member_plugin_is_uptodate(OSyncMember *member)
{
	osync_assert(member);
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);

	xmlDocPtr doc;
	xmlNodePtr cur;
	OSyncError *error = NULL;
	unsigned int version_major;
	unsigned int version_minor;
	xmlChar *version_str = NULL;
	osync_bool uptodate = FALSE;

	char *config = g_strdup_printf("%s%c%s",
			osync_member_get_configdir(member),
			G_DIR_SEPARATOR, osync_member_get_pluginname(member));
	
	/* If syncmember isn't present, we assume that update is required. */
	if (!osync_xml_open_file(&doc, &cur, config, "plugin", &error))
		goto end;

	version_str = xmlGetProp(cur->parent, (const xmlChar *)"version");

	/* No version node, means very outdated version. */
	if (!version_str)
		goto end;

      	sscanf(version_str, "%u.%u", &version_major, &version_minor);

	osync_trace(TRACE_INTERNAL, "Version: %s (current %u.%u required %u.%u)",
			version_str, version_major, version_minor, 
			OSYNC_PLUGIN_MAJOR_VERSION, OSYNC_PLUGIN_MINOR_VERSION ); 

	if (OSYNC_PLUGIN_MAJOR_VERSION == version_major 
			&& OSYNC_PLUGIN_MINOR_VERSION == version_minor)
		uptodate = TRUE;

	xmlFree(version_str);

end:
	g_free(config);

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	return uptodate;
}

/*@}*/
