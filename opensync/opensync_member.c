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

osync_bool osync_member_read_config(OSyncMember *member, char **data, int *size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "osync_member_read_config(%p, %p, %p, %p)", member, data, size, error);
	if (!osync_member_instance_default_plugin(member, error)) {
		osync_trace(TRACE_EXIT_ERROR, "osync_member_read_config: %i", osync_error_print(error));
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

/*@}*/

/**
 * @defgroup OSyncMemberAPI OpenSync Member
 * @ingroup OSyncPublic
 * @brief Used to manipulate members, which represent one device or application in a group
 * 
 */
/*@{*/

OSyncMember *osync_member_new(OSyncGroup *group)
{
	OSyncMember *member = g_malloc0(sizeof(OSyncMember));
	if (group) {
		osync_group_add_member(group, member);
		member->group = group;
	}
	
	member->memberfunctions = osync_memberfunctions_new();

	return member;
}

void osync_member_free(OSyncMember *member)
{
	osync_assert(member, "You must set a member to free");
	
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

OSyncPlugin *osync_member_get_plugin(OSyncMember *member)
{
	g_assert(member);
	osync_member_instance_default_plugin(member, NULL);
	return member->plugin;
}

const char *osync_member_get_pluginname(OSyncMember *member)
{
	g_assert(member);
	return member->pluginname;
}

void osync_member_set_pluginname(OSyncMember *member, const char *pluginname)
{
	g_assert(member);
	if (member->pluginname)
		g_free(member->pluginname);
	member->pluginname = g_strdup(pluginname);
}

const char *osync_member_get_configdir(OSyncMember *member)
{
	g_assert(member);
	return member->configdir;
}

void osync_member_set_configdir(OSyncMember *member, const char *configdir)
{
	g_assert(member);
	if (member->configdir)
		g_free(member->configdir);
	member->configdir = g_strdup(configdir);
}

osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size, OSyncError **error)
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

void osync_member_set_config(OSyncMember *member, const char *data, int size)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i)", __func__, member, data, size);
	g_assert(member);
	//FIXME free old data
	member->configdata = g_strdup(data);
	member->configsize = size;
	osync_trace(TRACE_EXIT, "%s", __func__);
}

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
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
	
	osync_trace(TRACE_EXIT, "%s: Loaded member: %p", __func__, member);
	return member;
}

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
	xmlNewChild(doc->children, NULL, (xmlChar*)"pluginname", (xmlChar*)member->pluginname);
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

long long int osync_member_get_id(OSyncMember *member)
{
	g_assert(member);
	return member->id;
}

void *osync_member_call_plugin(OSyncMember *member, const char *function, void *data, OSyncError **error)
{
	if (!osync_member_instance_default_plugin(member, error))
		return FALSE;
	
	void *(*plgfunc) (void *, void *, OSyncError **);
	if (!(plgfunc = osync_plugin_get_function(member->plugin, function, error)))
		return NULL;
	return plgfunc(member->plugindata, data, error);
}

void osync_member_set_slow_sync(OSyncMember *member, const char *objtypestr, osync_bool slow_sync)
{
	g_assert(member);	
	OSyncGroup *group = osync_member_get_group(member);
	g_assert(group);

	osync_group_set_slow_sync(group, objtypestr, slow_sync);
}

osync_bool osync_member_get_slow_sync(OSyncMember *member, const char *objtypestr)
{
	g_assert(member);	
	OSyncGroup *group = osync_member_get_group(member);
	g_assert(group);

	osync_bool needs_slow_sync = osync_group_get_slow_sync(group, objtypestr);
	return needs_slow_sync;
}

void osync_member_request_synchronization(OSyncMember *member)
{
	g_assert(member);
	
	if (member->memberfunctions->rf_sync_alert)
		member->memberfunctions->rf_sync_alert(member);
}

/*@}*/

/**
 * @defgroup OSyncMemberFunctions OpenSync Member Functions
 * @ingroup OSyncPublic
 * @brief The functions that can be used to access the device that a member represents
 * 
 */
/*@{*/

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
	g_assert(functions.finalize);
	if (!(member->plugindata = functions.initialize(member, error))) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

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

void osync_member_get_changeinfo(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.get_changeinfo(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

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

void osync_member_connect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.connect(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_member_disconnect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, function, user_data);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.disconnect(context);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

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

void osync_member_commit_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, member, change, function, user_data);
	g_assert(member);
	g_assert(change);

	OSyncFormatEnv *env = osync_member_get_format_env(member);
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	
	
	OSyncObjType *type = change->objtype;
	
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


	OSyncError *error = NULL;
	if (!osync_change_convert_member_sink(env, change, member, &error)) {
		osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to convert change");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
		osync_error_free(&error);
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
				fmtsink->functions.commit_change(context, change);
				osync_trace(TRACE_EXIT, "%s", __func__);
				return;
			}
		}
	}

	osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to send changes");
	osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find a sink", __func__);
}

void osync_member_committed_all(OSyncMember *member)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);

	GList *f;
	for (f = member->format_sinks; f; f = f->next) {
		OSyncObjFormatSink *fmtsink = f->data;
		osync_debug("OSYNC", 2, "Sending committed all to sink %s", fmtsink->format ? fmtsink->format->name : "None");

		OSyncFormatFunctions functions = fmtsink->functions;

		if (functions.batch_commit) {
			GList *o = fmtsink->commit_contexts;
			GList *c = NULL;
			
			int i = 0;
			OSyncChange **changes = g_malloc0(sizeof(OSyncChange *) * (g_list_length(fmtsink->commit_changes) + 1));
			OSyncContext **contexts = g_malloc0(sizeof(OSyncContext *) * (g_list_length(fmtsink->commit_changes) + 1));;
			
			for (c = fmtsink->commit_changes; c && o; c = c->next) {
				OSyncChange *change = c->data;
				OSyncContext *context = o->data;
				
				changes[i] = change;
				contexts[i] = context;
				
				i++;
				o = o->next;
			}
			
			changes[i] = NULL;
			contexts[i] = NULL;
			functions.batch_commit(member->plugindata, contexts, changes);
			
			g_free(changes);
			g_free(contexts);
			
			g_list_free(fmtsink->commit_changes);
			fmtsink->commit_changes = NULL;
			
			g_list_free(fmtsink->commit_contexts);
			fmtsink->commit_contexts = NULL;
		} else if (functions.committed_all) {
			functions.committed_all(member->plugindata);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

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

/*@}*/

void *osync_member_get_data(OSyncMember *member)
{
	return member->enginedata;
}

void osync_member_set_data(OSyncMember *member, void *data)
{
	member->enginedata = data;
}

OSyncGroup *osync_member_get_group(OSyncMember *member)
{
	return member->group;
}

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

osync_bool osync_member_objtype_enabled(OSyncMember *member, const char *objtype)
{
	g_assert(member);
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtype);
	g_assert(sink);
	return sink->enabled;
}

void osync_member_set_objtype_enabled(OSyncMember *member, const char *objtypestr, osync_bool enabled)
{
	if (osync_conv_objtype_is_any(objtypestr))
		g_assert_not_reached();
	
	g_assert(member);
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtypestr);
	g_assert(sink);
	
	sink->enabled = enabled;
}

/*void osync_member_set_read_only(OSyncMember *member, const char *objtypestr, osync_bool read_only)
{
	if (osync_conv_objtype_is_any(objtypestr))
		g_assert_not_reached();
	
	g_assert(member);
	OSyncObjTypeSink *sink = osync_member_find_objtype_sink(member, objtypestr);
	g_assert(sink);
	
	sink->read_only = read_only;
}*/

void osync_member_set_format(OSyncMember *member, const char *objtypestr, const char *objformatstr)
{
	g_assert(member);
	OSyncObjTypeSink *objsink = osync_member_find_objtype_sink(member, objtypestr);
	if (!objsink)
		return;
	OSyncObjFormatSink *frmsink = osync_objtype_find_format_sink(objsink, objformatstr);
	if (!frmsink)
		return;
	objsink->selected_format = frmsink;
}

void osync_member_select_format(OSyncMember *member, OSyncObjTypeSink *objsink)
{
	g_assert(member);
	g_assert(objsink);

	//FIXME For now we just select the first one
	OSyncObjFormatSink *frmsink = g_list_nth_data(objsink->formatsinks, 0);
	objsink->selected_format = frmsink;
}
