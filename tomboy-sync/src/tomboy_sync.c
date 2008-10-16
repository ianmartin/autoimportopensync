/*
 * tomboy-sync - A plugin for the opensync framework
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@googlemail.com>
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

#include "config.h"

#include "tomboy_sync.h"

#include <opensync/file.h>
#include <opensync/opensync-version.h>
#include <uuid/uuid.h>

#include "tomboy_sync_file.h"
#include "tomboy_sync_dbus.h"

static void free_dir(OSyncTomboyDir *dir)
{
	if (dir->sink)
		osync_objtype_sink_unref(dir->sink);

	if (dir->hashtable)
		osync_hashtable_unref(dir->hashtable);

	g_free(dir);
}

/*
 * validates a uuid and returns true if the uuid has the right format
 */
osync_bool osync_tomboysync_validate_uuid(char *uuid) {

	/* e.g. 34d5dd78-b416-4956-8ce6-dfbb25848857 */
	return g_regex_match_simple("[A-Za-z0-9]{8}-[A-Za-z0-9]{4}-[A-Za-z0-9]{4}-[A-Za-z0-9]{4}-[A-Za-z0-9]{12}", uuid, 0, 0);
}

/* use content from <last-change-date> and <last-metadata-change-date> as hash instead of the stat information
 * because tomboy (re-)writes a note everytime when it is opened  */
char *osync_tomboysync_generate_hash(char *data, int size)
{
	xmlDocPtr doc;
	xmlParserCtxtPtr ctxt;
	xmlNodePtr node;
	xmlNodePtr cur;
	char *last_change = NULL;
	char *last_metadata_change = NULL;
		
	ctxt = xmlNewParserCtxt();
	if ( ctxt == NULL ) {
		osync_trace(TRACE_ERROR, "Unable to create xml parser context.");
		goto error;
	}
	doc = xmlCtxtReadMemory(ctxt,data,size,NULL,NULL,0);
	if ( doc == NULL ) {
		osync_trace(TRACE_ERROR,"Unable to parse xml doc tree. Size %d content \"%s\".",size, __NULLSTR(data));
		goto error_free_context;
	}
	node = xmlDocGetRootElement(doc)->children;
	if ( node == NULL ) {
		osync_trace(TRACE_ERROR,"Unable to generate hash. Xml doc content is empty.");
		goto error_free_doc;
	}
	for (cur = node; cur != NULL; cur = cur->next ) {
		if ( xmlStrEqual(cur->name, BAD_CAST "last-change-date") && cur->children != NULL ) { // <last-change-date> found
			last_change = (char *)xmlNodeGetContent(cur);
		}
		else if ( xmlStrEqual(cur->name, BAD_CAST "last-metadata-change-date") && cur->children != NULL ) { // <last-metadata-change-date> found
			last_metadata_change = (char *)xmlNodeGetContent(cur);
		}
	}
	if (!last_change || !last_metadata_change) {
		g_free(last_change);
		g_free(last_metadata_change);
		goto error_free_doc;
	}
	
	char *hash = g_strdup_printf("%s-%s", last_change, last_metadata_change);

	g_free(last_change);
	g_free(last_metadata_change);
	xmlFreeDoc(doc);
	xmlFreeParserCtxt(ctxt);
	return hash;
	
error_free_doc:
	xmlFreeParserCtxt(ctxt);
error_free_context:
	xmlFreeParserCtxt(ctxt);
error:
	return NULL;
}

char *osync_tomboysync_generate_uuid()
{
	char * retval = NULL;
	uuid_t uuid;
	uuid_generate(uuid);
	retval = g_malloc0(37);
	/*
	 *  The  uuid_unparse  function converts the supplied UUID uuid from the binary representation into a 36-byte string (plus tailing ’\0’) of the form 1b4e28ba-2fa1-11d2-883f-0016d3cca427 and stores this
     *  value in the character string pointed to by out.
	 */
	uuid_unparse(uuid, retval);

	return retval;
}

/* In initialize, we get the config for the plugin. Here we also must register
 * all _possible_ objtype sinks. */
static void *osync_tomboysync_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);
	
	osync_bool usedbus = FALSE;

	OSyncTomboyEnv *tomboyenv = osync_try_malloc0(sizeof(OSyncTomboyEnv), error);
	if (!tomboyenv) {
		goto error;
	}

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	osync_assert(config);

	OSyncList *optslist = osync_plugin_config_get_advancedoptions(config);
	for (; optslist; optslist = optslist->next) {
		OSyncPluginAdvancedOption *option = optslist->data;

		const char *val = osync_plugin_advancedoption_get_value(option);
		const char *name = osync_plugin_advancedoption_get_name(option);
		osync_assert(name);
		osync_assert(val);
		if (!strcmp(name,"UseDbus")) {
			if (!strcmp(val, "true")) {
				usedbus = TRUE;
			}
		}
	}
	int i, numobjs = osync_plugin_info_num_objtypes(info);
	for (i = 0; i < numobjs; i++) {
		OSyncTomboyDir *dir = osync_try_malloc0(sizeof(OSyncTomboyDir), error);
		if (!dir) {
			goto error;
		}

		dir->sink = osync_plugin_info_nth_objtype(info, i);
		osync_assert(dir->sink);

		const char *objtype = osync_objtype_sink_get_name(dir->sink);
		OSyncPluginResource *res = osync_plugin_config_find_active_resource(config, objtype);
		/* get homedir from config. if no dir is set read homedir from the environment */
		dir->homedir_path = osync_plugin_resource_get_path(res);
		if (!dir->homedir_path) {
			osync_trace(TRACE_INTERNAL, "tomboy-sync home directory not set. Using default.");
			dir->homedir_path = g_getenv ("HOME");
			if (!dir->homedir_path) {
				dir->homedir_path = g_get_home_dir();
			}
		}
		osync_trace(TRACE_INTERNAL, "tomboy-sync home directory is \"%s\".", dir->homedir_path);

		OSyncList *s = osync_plugin_resource_get_objformat_sinks(res);
		OSyncObjFormatSink *fsink = s->data; // there could be only one sink
		const char *objformat = osync_objformat_sink_get_objformat(fsink);
		osync_assert(objformat);
		osync_trace(TRACE_INTERNAL, "tomboy-sync objtype %s has objformat %s", objtype, objformat);
		if (strcmp(objformat, "tomboy-note")) {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Format \"%s\" is not supported by tomboy-sync. Only Format \"tomboy-note\" is supported by the tomboy-sync plugin.", objformat);
			goto error;
		}

		/* All sinks have the same functions of course */
		OSyncObjTypeSinkFunctions functions;
		memset(&functions, 0, sizeof(functions));
#ifdef ENABLE_DBUS
		if ( usedbus ) {
			osync_trace(TRACE_INTERNAL, "using dbus for sync.");
			functions.get_changes = osync_tomboysync_dbus_get_changes;
			functions.commit = osync_tomboysync_dbus_commit_change;
			functions.read = osync_tomboysync_dbus_read;
			functions.write = osync_tomboysync_dbus_write;
			functions.sync_done = osync_tomboysync_dbus_sync_done;
		}
		else {
#endif /* ENABLE_DBUS */
		osync_trace(TRACE_INTERNAL, "using file for sync.");
		functions.get_changes = osync_tomboysync_file_get_changes;
		functions.commit = osync_tomboysync_file_commit_change;
		functions.read = osync_tomboysync_file_read;
		functions.write = osync_tomboysync_file_write;
		functions.sync_done = osync_tomboysync_file_sync_done;
#ifdef ENABLE_DBUS
		}
#endif /* ENABLE_DBUS */

		/* We pass the OSyncTomboyDir object to the sink, so we dont have to look it up
		 * again once the functions are called */
		osync_objtype_sink_set_functions(dir->sink, functions, dir);

		osync_trace(TRACE_INTERNAL, "The configdir: %s", osync_plugin_info_get_configdir(info));
		char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
		dir->hashtable = osync_hashtable_new(tablepath, objtype, error);
		g_free(tablepath);

		if (!dir->hashtable)
			goto error;

		if (!osync_hashtable_load(dir->hashtable, error))
			goto error;

		/* eventuell den anchor ändern ??? */
		char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
		char *path_field = g_strdup_printf("path_%s", osync_objtype_sink_get_name(dir->sink));
		if (!osync_anchor_compare(anchorpath, path_field, dir->homedir_path))
			osync_objtype_sink_set_slowsync(dir->sink, TRUE);
		g_free(anchorpath);
		g_free(path_field);
		
		tomboyenv->dir = dir;

#ifdef ENABLE_DBUS
		if (usedbus) {
			if (!osync_tomboysync_dbus_initalize(tomboyenv,error)) {
				goto error;
			}
		}
		else {
#endif /* ENABLE_DBUS */
		if (!osync_tomboysync_file_initalize(tomboyenv, error)) {
			goto error;
		}
#ifdef ENABLE_DBUS
		}
#endif /* ENABLE_DBUS */
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__);
	/* return e.g. a struct for state maintanance */
	return (void*)tomboyenv;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void osync_tomboysync_finalize(void *data)
{

}

/* Here we actually tell opensync which sinks are available. For this plugin, we
 * just report all objtype as available. Since the resource are configured like this. */
static osync_bool osync_tomboysync_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "note");
	osync_assert(sink);

	osync_trace(TRACE_INTERNAL, "set objtype %s available", osync_objtype_sink_get_name(sink));
	osync_objtype_sink_set_available(sink, TRUE);

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "tomboy-sync");
	osync_version_set_modelversion(version, "0.1");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	/* we can set here the capabilities, but for the file-sync
	 * plugin they are static and shipped with opensync */

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;

	osync_plugin_set_name(plugin, "tomboy-sync");
	osync_plugin_set_longname(plugin, "Tomboy Notes Synchronization Plugin");
	osync_plugin_set_description(plugin, "Plugin to synchronize notes from Tomboy");

	osync_plugin_set_initialize(plugin, osync_tomboysync_initialize);
	osync_plugin_set_finalize(plugin, osync_tomboysync_finalize);
	osync_plugin_set_discover(plugin, osync_tomboysync_discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	return TRUE;

error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
