#include "opensync.h"
#include "opensync_internals.h"

/**
 * @defgroup OSyncMemberPrivateAPI OpenSync Member Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
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
	OSyncPluginFunctions functions = member->plugin->info.functions;
	osync_bool ret = FALSE;
	if (!member->configdir)
		return FALSE;
	
	if (functions.get_config) {
		ret = functions.get_config(member->configdir, data, size);
	} else {
		char *filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
		ret = osync_file_read(filename, data, size, error);
		g_free(filename);
	}
	return ret;
}

/*@}*/

/**
 * @defgroup OSyncMemberAPI OpenSync Member
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
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
	osync_debug("OSMEM", 3, "Generated new member");

	return member;
}

void osync_member_free(OSyncMember *member)
{
	if (member->group)
		osync_group_remove_member(member->group, member);
	
	//Free the plugin if we are not thread-safe
	if (member->plugin && !member->plugin->info.is_threadsafe) {
		osync_plugin_unload(member->plugin);
		osync_plugin_free(member->plugin);
	}
	
	g_free(member->memberfunctions);
	g_free(member);
}	

void osync_member_unload_plugin(OSyncMember *member)
{
	g_assert(member);
	if (!member->plugin)
		return;
		
	if (!member->plugin->info.is_threadsafe) {
		osync_plugin_unload(member->plugin);
		osync_plugin_free(member->plugin);
	}
	
	g_list_free(member->objtype_sinks);
	g_list_free(member->format_sinks);
	//Really free the formats!
	
	member->objtype_sinks = NULL;
	member->format_sinks = NULL;
	member->plugin = NULL;
}

osync_bool osync_member_instance_plugin(OSyncMember *member, OSyncPlugin *plugin, OSyncError **error)
{
	g_assert(member);
	g_assert(plugin);
	osync_debug("OSMEM", 3, "Instancing plugin %s for member %i", plugin->info.name, member->id);
	osync_member_unload_plugin(member);
	
	if (plugin->info.is_threadsafe) {
		member->plugin = plugin;
	} else {
		member->plugin = osync_plugin_load(NULL, plugin->path, error);
		if (!member->plugin)
			return FALSE;
	}
	
	//Prepare the sinks;
	GList *o;
	for (o = member->plugin->accepted_objtypes; o; o = o->next) {
		OSyncObjTypeTemplate *objtemplate = o->data;
		OSyncObjTypeSink *objsink = osync_objtype_sink_from_template(member->group, objtemplate);
		if (!objsink) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to instance plugin. Could not create object type from template");
			return FALSE;
		}
		member->objtype_sinks = g_list_append(member->objtype_sinks, objsink);
		GList *f;
		for (f = objtemplate->formats; f; f = f->next) {
			OSyncObjFormatTemplate *frmtemplate = f->data;
			OSyncObjFormatSink *format_sink = osync_objformat_sink_from_template(member->group, frmtemplate);
			if (!format_sink) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to instance plugin. Could not create format from template");
				return FALSE;
			}
			objsink->formatsinks = g_list_append(objsink->formatsinks, format_sink);
			format_sink->objtype_sink = objsink;
			member->format_sinks = g_list_append(member->format_sinks, format_sink);
		}
	}
	return TRUE;
}

OSyncPlugin *osync_member_get_plugin(OSyncMember *member)
{
	g_assert(member);
	return member->plugin;
}

const char *osync_member_get_pluginname(OSyncMember *member)
{
	g_assert(member);
	return osync_plugin_get_name(member->plugin);
}

const char *osync_member_get_configdir(OSyncMember *member)
{
	g_assert(member);
	return member->configdir;
}

osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size, OSyncError **error)
{
	g_assert(member);
	osync_bool ret = TRUE;

	if (member->configdata) {
		*data = member->configdata;
		*size = member->configsize;
		return TRUE;
	}

	if (!osync_member_read_config(member, data, size, error)) {
		char *filename = g_strdup_printf(OPENSYNC_CONFIGDIR"/defaults/%s", osync_plugin_get_name(member->plugin));
		osync_debug("OSMEM", 3, "Reading default2 config file for member %lli from %s", member->id, filename);
		ret = osync_file_read(filename, data, size, error);
		g_free(filename);
	}
	return ret;
}

void osync_member_set_config(OSyncMember *member, const char *data, int size)
{
	g_assert(member);
	//FIXME free old data
	member->configdata = g_strdup(data);
	member->configsize = size;
}

OSyncMember *osync_member_load(OSyncGroup *group, const char *path, OSyncError **error)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	char *filename = NULL;
	
	osync_debug("OSGRP", 3, "Trying to load member from directory %s", path);
	filename = g_strdup_printf ("%s/syncmember.conf", path);
	
	OSyncMember *member = osync_member_new(group);
	char *basename = g_path_get_basename(path);
	member->id = atoi(basename);
	g_free(basename);
	member->configdir = g_strdup(path);

	if (!_osync_open_xml_file(&doc, &cur, filename, "syncmember", error)) {
		osync_member_free(member);
		return NULL;
	}

	while (cur != NULL) {
		char *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"pluginname")) {
				OSyncPlugin *plugin = osync_env_find_plugin(group->env, str);
				if (!plugin) {
					osync_debug("OSPLG", 0, "Couldn't find the plugin %s for member", str);
					osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to find the plugin %s for the member", str);
					xmlFree(str);
					xmlFreeDoc(doc);
					g_free(filename);
					osync_member_free(member);
					return NULL;
				} else {
					if (!osync_member_instance_plugin(member, plugin, error)) {
						xmlFree(str);
						xmlFreeDoc(doc);
						g_free(filename);
						osync_member_free(member);
						return NULL;
					}
				}
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
	
	return member;
}

osync_bool osync_member_save(OSyncMember *member, OSyncError **error)
{
	char *filename = NULL;

	if (!member->id) {
		member->id = osync_group_create_member_id(member->group);
		member->configdir = g_strdup_printf("%s/%lli", member->group->configdir, member->id);
	}
	osync_debug("OSMEM", 3, "Saving configuration for member %i\n", member->id);
	
	if (!g_file_test(member->configdir, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSMEM", 3, "Creating config directory: %s for member %i", member->configdir, member->id);
		if (mkdir(member->configdir, 0777)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for member %li\n", member->id);
			return FALSE;
		}
	}
	
	//Saving the syncmember.conf
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	xmlDocPtr doc;
	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, "syncmember", NULL);
	//The plugin name
	xmlNewChild(doc->children, NULL, "pluginname", osync_plugin_get_name(member->plugin));
	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);
	
	//Saving the config if it exists
	if (member->configdata) {
		osync_bool ret = TRUE;
		OSyncPluginFunctions functions = member->plugin->info.functions;
		
		if (functions.store_config) {
			ret = functions.store_config(member->configdir, member->configdata, member->configsize);
		} else {
			filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
			if (!osync_file_write(filename, member->configdata, member->configsize, error)) {
				ret = FALSE;
			}
			g_free(filename);
		}
		g_free(member->configdata);
		member->configdata = NULL;
		member->configsize = 0;
		return ret;
	}
	return TRUE;
}

long long int osync_member_get_id(OSyncMember *member)
{
	g_assert(member);
	return member->id;
}

void *osync_member_call_plugin(OSyncMember *member, const char *function, void *data, OSyncError **error)
{
	void *(*plgfunc) (void *, void *);
	if (!(plgfunc = osync_plugin_get_function(member->plugin, function, error)))
		return NULL;
	//FIXME Should we pass the error through?
	return plgfunc(member->plugindata, data);
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
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

osync_bool osync_member_initialize(OSyncMember *member, OSyncError **error)
{
	g_assert(member);
	g_assert(member->plugin);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	g_assert(functions.finalize);
	if (!(member->plugindata = functions.initialize(member))) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "Unable to initialize member");
		return FALSE;
	}
	return TRUE;
}

void osync_member_finalize(OSyncMember *member)
{
	g_assert(member);
	g_assert(member->plugin);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	if (functions.finalize)
		functions.finalize(member->plugindata);
}

void osync_member_get_changeinfo(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.get_changeinfo(context);
}

void osync_member_get_change_data(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	OSyncPluginFunctions functions = member->plugin->info.functions;
	g_assert(change != NULL);
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.get_data(context, change);
}

void osync_member_connect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.connect(context);
}

void osync_member_disconnect(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
	OSyncPluginFunctions functions = member->plugin->info.functions;
	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions.disconnect(context);
}

void osync_member_sync_done(OSyncMember *member, OSyncEngCallback function, void *user_data)
{
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
}

void osync_member_commit_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data)
{
	g_assert(member);
	g_assert(change);

	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;

	OSyncFormatEnv *env = osync_member_get_format_env(member);
	
	OSyncObjType *type = change->objtype;
	
	//osync_run_hook(member->group->before_convert_hook, (change, member));
	//osync_run_hook(member->before_convert_hook, (change, member);

	/* This is an optmization:
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
		return;
	}

	GList *targets = NULL;
	GList *i;
	for (i = member->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *fmtsink = i->data;
		targets = g_list_append(targets, fmtsink->format);
	}
	
	if (!osync_conv_detect_and_convert(env, change, targets)) {
		osync_debug("OSYNC", 0, "Unable to convert to any format on the plugin");
		osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to convert change");
		return;
	}
	
	//Filter the change
	change->destobjtype = g_strdup(sink->objtype->name);
	if (!osync_filter_change_allowed(member, change)) {
		osync_debug("OSYNC", 2, "Change %s filtered out for member %lli", change->uid, member->id);
		osync_context_report_success(context);
		return;
	}

	/*FIXME: use a sane interface to return the frmtsink to be used */
	for (i = member->format_sinks; i; i = i->next) {
		OSyncObjFormatSink *fmtsink = i->data;
		if (!fmtsink->objtype_sink->enabled) {
			osync_context_report_success(context);
			return;
		}
		if (fmtsink->format == osync_change_get_objformat(change)) {
			fmtsink->functions.commit_change(context, change);
			break;
		}
	}
	if (!i)
		osync_context_report_error(context, OSYNC_ERROR_CONVERT, "Unable to send changes");
}

OSyncObjFormatSink *osync_member_make_random_data(OSyncMember *member, OSyncChange *change)
{
	int retry = 0;
	g_assert(member);
	OSyncFormatEnv *env = osync_member_get_format_env(member);
	
	OSyncObjFormatSink *format_sink = NULL;
	
	for (retry = 0; retry < 100; retry++) {
		if (retry > 20)
			return NULL; //Giving up
		
		//Select a random objtype
		int selected = g_random_int_range(0, g_list_length(env->objtypes));
		OSyncObjType *objtype = g_list_nth_data(env->objtypes, selected);
		osync_change_set_objtype(change, objtype);
		
		//Select a random objformat
		if (!g_list_length(objtype->formats))
			continue;
		OSyncObjFormat *format = NULL;
		selected = g_random_int_range(0, g_list_length(objtype->formats));
		format = g_list_nth_data(objtype->formats, selected);
		
		if (!format->create_func)
			continue;
		//Create the data
		format->create_func(change);
		
		osync_change_set_objformat(change, format);
		//Convert the data to a format the plugin understands
		OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, objtype->name);
		if (!objtype_sink)
			continue; //We had a objtype we cannot add
		
		selected = g_random_int_range(0, g_list_length(objtype_sink->formatsinks));
		format_sink = g_list_nth_data(objtype_sink->formatsinks, selected);
		/*FIXME: use multiple sinks, or what? */
		GList *targets = g_list_append(NULL, format_sink->format);
		osync_bool r = osync_conv_detect_and_convert(env, change, targets);
		g_list_free(targets);
		if (!r)
			continue; //Unable to convert to selected format

		break;
	}
	return format_sink;
}

OSyncChange *osync_member_add_random_data(OSyncMember *member)
{
	OSyncContext *context = osync_context_new(member);
	OSyncChange *change = osync_change_new();
	change->changetype = CHANGE_ADDED;
	OSyncObjFormatSink *format_sink;
	if (!(format_sink = osync_member_make_random_data(member, change)))
		return NULL;
	
	if (format_sink->functions.access(context, change) == TRUE)
		return change;
	return NULL;
}

osync_bool osync_member_modify_random_data(OSyncMember *member, OSyncChange *change)
{
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_MODIFIED;
	OSyncObjFormatSink *format_sink;
	char *uid = g_strdup(osync_change_get_uid(change));
	
	if (!(format_sink = osync_member_make_random_data(member, change)))
		return FALSE;

	osync_change_set_uid(change, uid);
	
	return format_sink->functions.access(context, change);
}

osync_bool osync_member_delete_data(OSyncMember *member, OSyncChange *change)
{
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_DELETED;
	
	OSyncObjTypeSink *objtype_sink = osync_member_find_objtype_sink(member, change->objtype->name);
	if (!objtype_sink)
		return FALSE;
	
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncObjFormatSink *format_sink = osync_objtype_find_format_sink(objtype_sink, format->name);
	if (!format_sink)
		return FALSE;
	
	return format_sink->functions.access(context, change);
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

//FIXME Remove this and replace with "views"
void osync_member_add_changeentry(OSyncMember *member, OSyncChange *entry)
{
	g_assert(member);

	member->entries = g_list_append(member->entries, entry);
	entry->member = member;
}

//FIXME Remove this and replace with "views"
void osync_member_remove_changeentry(OSyncMember *member, OSyncChange *entry)
{
	g_assert(member);
	member->entries = g_list_remove(member->entries, entry);
	entry->member = NULL;
}

//FIXME Remove this and replace with "views"
OSyncChange *osync_member_find_change(OSyncMember *member, const char *uid)
{
	int i;
	for (i = 0; i < g_list_length(member->entries); i++) {
		OSyncChange *entry = g_list_nth_data(member->entries, i);
		if (!strcmp(osync_change_get_uid(entry), uid)) {
			return entry;
		}
	}
	return NULL;
}

//FIXME Remove this and replace with "views"
osync_bool osync_member_uid_is_unique(OSyncMember *member, OSyncChange *change, osync_bool spare_deleted)
{
	GList *c = NULL;
	int found = 0;

	for (c = member->entries; c; c = c->next) {
		OSyncChange *entry = c->data;
		if ((change != entry) && (!spare_deleted || (entry->changetype != CHANGE_DELETED)) && !strcmp(entry->uid, change->uid)) {
			found++;
		}
	}
	if (found == 0)
		return TRUE;
	return FALSE;
}

//FIXME Remove this and replace with "views"
osync_bool osync_member_update_change(OSyncMember *member, OSyncChange **change)
{
	OSyncChange *entry;
	if ((entry = osync_member_find_change(member, osync_change_get_uid(*change)))) {
		osync_change_update(*change, entry);
		*change = entry;
		return TRUE;
	}
	return FALSE;
}

//FIXME Remove this and replace with "views"
int osync_member_num_changeentries(OSyncMember *member)
{
	return g_list_length(member->entries);
}

//FIXME Remove this and replace with "views"
OSyncChange *osync_member_nth_changeentry(OSyncMember *member, int n)
{
	return g_list_nth_data(member->entries, n);
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
