#include <opensync.h>
#include "opensync_internals.h"
#include "opensync_member.h"

OSyncMemberFunctions *osync_memberfunctions_new()
{
	OSyncMemberFunctions *functions = g_malloc0(sizeof(OSyncMemberFunctions));
	return functions;
}

OSyncMember *osync_member_new(OSyncGroup *group)
{
	//char *filename = NULL;
	if (group == NULL) return NULL;
	
	OSyncMember *member = g_malloc0(sizeof(OSyncMember));
	osync_group_add_member(group, member);

	//filename = g_strdup_printf("%s/member%i", osync_group_get_configdir(group), g_random_int_range(1, 1000000));
	//member->name = g_path_get_basename(filename);
	//member->configdir = filename;
	//member->hashtable = g_hash_table_new (g_str_hash, g_str_equal);
	member->group = group;
	member->memberfunctions = osync_memberfunctions_new();
	//printf("hashtable: %p\n", member->hashtable);
	osync_debug("OSMEM", 3, "Generated new member");
	//osync_debug("OSMEM", 3, "Name: %s, %p", member->name, member);
	//osync_debug("OSMEM", 3, "Configdirectory: %s", filename);

	return member;
}	

OSyncMemberFunctions *osync_member_get_memberfunctions(OSyncMember *member)
{
	return member->memberfunctions;
}

osync_bool osync_member_instance_plugin(OSyncMember *member, OSyncPlugin *plugin)
{
	osync_debug("OSMEM", 3, "Insstancing plugin %s for member %i", osync_plugin_get_name(plugin), member->id);
	if (plugin->info.is_threadsafe) {
		member->plugin = plugin;
	} else {
		OSyncPlugin *newplugin = osync_plugin_new();
		osync_plugin_load_info(newplugin, plugin->path);
		member->plugin = newplugin;
	}
	return TRUE;
}

const char *osync_member_get_pluginname(OSyncMember *member)
{
	return osync_plugin_get_name(member->plugin);
}

OSyncFormatEnv *osync_member_get_format_env(OSyncMember *member)
{
	g_assert(member);
	return member->group->conv_env;
}

char *osync_member_get_configdir(OSyncMember *member)
{
	return member->configdir;
}

osync_bool osync_member_set_configdir(OSyncMember *member, char *path)
{
	osync_debug("OSMEM", 3, "Setting configdirectory for member %i to %s", member->id, path);
	member->configdir = g_strdup(path);
	return TRUE; //FIXME
}

osync_bool osync_member_read_config(OSyncMember *member, char **data, int *size)
{
	OSyncPluginFunctions functions = member->plugin->info.functions;
	osync_bool ret = FALSE;
	if (functions.get_config) {
		ret = functions.get_config(member->configdir, data, size);
	} else {
		char *filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
		ret = osync_file_read(filename, data, size);
		g_free(filename);
	}
	return ret;
}

osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size)
{
	osync_bool ret = TRUE;

	if (!osync_member_read_config(member, data, size)) {
		char *filename = g_strdup_printf(OPENSYNC_CONFIGDIR"/defaults/%s", osync_plugin_get_name(member->plugin));
		osync_debug("OSMEM", 3, "Reading default config file for member %i", member->id);
		ret = osync_file_read(filename, data, size);
		g_free(filename);
	}
	return ret;
}

osync_bool osync_member_set_config(OSyncMember *member, char *data, int size)
{
	osync_bool ret = FALSE;
	OSyncPluginFunctions functions = member->plugin->info.functions;
	if (!g_file_test(member->configdir, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSMEM", 3, "Creating config directory: %s for member %i", member->configdir, member->id);
		mkdir(member->configdir, 0777);
	}
	osync_debug("OSMEM", 3, "Saving configuration for member %i\n", member->id);
	if (functions.store_config) {
		return functions.store_config(member->configdir, data, size);
	} else {
		char *filename = g_strdup_printf("%s/%s.conf", member->configdir, osync_plugin_get_name(member->plugin));
		ret = osync_file_write(filename, data, size);
		g_free(filename);
	}
	return ret;
}

osync_bool osync_member_initialize(OSyncMember *member)
{
	g_assert(member);
	g_assert(member->plugin);
	OSyncPluginFunctions functions = member->plugin->info.functions;
	g_assert(functions.finalize);
	if (!(member->plugindata = functions.initialize(member)))
		return FALSE;
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
	OSyncObjType *type = osync_change_get_objtype(change);
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncFormatFunctions *functions = osync_plugin_get_objformat_functions(member->plugin, type->name, format->name);

	OSyncContext *context = osync_context_new(member);
	context->callback_function = function;
	context->calldata = user_data;
	functions->commit_change(context, change);
}

osync_bool osync_member_make_random_data(OSyncMember *member, OSyncChange *change)
{
	int retry = 0;
	g_assert(member);
	OSyncFormatEnv *env = osync_member_get_format_env(member);
	
	for (retry = 0; retry < 100; retry++) {
		if (retry > 20)
			return FALSE; //Giving up
		
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
		OSyncPlgAcceptedType *plgtype = osync_plugin_find_accepted_type(&member->plugin->info, objtype->name);
		if (!plgtype)
			continue; //We had a objtype we cannot add
		
		selected = g_random_int_range(0, g_list_length(plgtype->formats));
		format = g_list_nth_data(plgtype->formats, selected);
		if (!osync_conv_convert(env, change, format))
			continue; //Unable to convert to selected format
		break;
	}
	return TRUE;
}

OSyncChange *osync_member_add_random_data(OSyncMember *member)
{
	OSyncContext *context = osync_context_new(member);
	OSyncChange *change = osync_change_new();
	change->changetype = CHANGE_ADDED;
	if (!osync_member_make_random_data(member, change))
		return NULL;
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncFormatFunctions *functions =
			osync_plugin_get_objformat_functions(member->plugin, format->objtype->name, format->name);
	if (functions->access(context, change) == TRUE)
		return change;
	return NULL;
}

osync_bool osync_member_modify_random_data(OSyncMember *member, OSyncChange *change)
{
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_MODIFIED;
	
	char *uid = g_strdup(osync_change_get_uid(change));
	
	if (!osync_member_make_random_data(member, change))
		return FALSE;

	osync_change_set_uid(change, uid);
	
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncFormatFunctions *functions =
			osync_plugin_get_objformat_functions(member->plugin, format->objtype->name, format->name);
	return functions->access(context, change);
}

osync_bool osync_member_delete_data(OSyncMember *member, OSyncChange *change)
{
	OSyncContext *context = osync_context_new(member);
	change->changetype = CHANGE_DELETED;
	
	OSyncObjFormat *format = osync_change_get_objformat(change);
	OSyncFormatFunctions *functions =
			osync_plugin_get_objformat_functions(member->plugin, format->objtype->name, format->name);
	return functions->access(context, change);
}

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
		member = osync_group_get_nth_member(group, i);
		if (member->id == id) {
			return member;
		}
	}
	osync_debug("OSPLG", 0, "Couldnt find the group with the id %i", id);
	return NULL;
}

void osync_member_add_changeentry(OSyncMember *member, OSyncChange *entry)
{
	if (!member)
		return;
	osync_assert(osync_member_uid_is_unique(member, entry->uid), "Member uid is not unique while adding. Did you try to open several mapping tables?\n");

	member->entries = g_list_append(member->entries, entry);
	entry->member = member;
}

void osync_member_remove_changeentry(OSyncMember *member, OSyncChange *entry)
{
	g_assert(member);
	member->entries = g_list_remove(member->entries, entry);
	entry->member = NULL;
}

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

osync_bool osync_member_uid_is_unique(OSyncMember *member, const char *uid)
{
	GList *c = NULL;
	int found = 0;
	for (c = member->entries; c; c = c->next) {
		OSyncChange *entry = c->data;
		if (!strcmp(osync_change_get_uid(entry), uid)) {
			found++;
		}
	}
	if (found <= 1)
		return TRUE;
	return FALSE;
}

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

int osync_member_num_changeentries(OSyncMember *member)
{
	return g_list_length(member->entries);
}

OSyncChange *osync_member_nth_changeentry(OSyncMember *member, int n)
{
	return g_list_nth_data(member->entries, n);
}

void osync_member_load(OSyncMember *member)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	char *filename = NULL;
	osync_debug("OSGRP", 3, "Trying to load member from directory %s", member->configdir);
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	member->id = atoi(g_path_get_basename(member->configdir));

	if (!_osync_open_xml_file(&doc, &cur, filename, "syncmember")) {
		printf("Returning\n");
		return;
	}

	while (cur != NULL) {
		char *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"pluginname")) {
				OSyncPlugin *plugin = osync_plugin_from_name(member->group->env, str);
				if (!plugin) {
					osync_debug("OSPLG", 0, "Couldn't find the plugin %s for member", str);
				} else {
					osync_member_instance_plugin(member, plugin);
				}
			}
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
}

void osync_member_save(OSyncMember *member)
{
	char *filename = NULL;
		
	if (!g_file_test(member->configdir, G_FILE_TEST_IS_DIR)) {
		mkdir(member->configdir, 0777);
	}
	
	filename = g_strdup_printf ("%s/syncmember.conf", member->configdir);
	
	xmlDocPtr doc;

	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, "syncmember", NULL);

	xmlNewChild(doc->children, NULL, "pluginname", osync_plugin_get_name(member->plugin));

	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);
}

unsigned int osync_member_get_id(OSyncMember *member)
{
	g_assert(member != NULL);
	return member->id;
}

void osync_member_create(OSyncMember *member)
{
	member->id = osync_group_create_member_id(member->group);
	member->configdir = g_strdup_printf("%s/%i", member->group->configdir, member->id);
	osync_member_save(member);
}

void osync_member_call_plugin(OSyncMember *member, char *function, void *data)
{
	void (*plgfunc) (void *, void *);
	if (!(plgfunc = osync_plugin_get_function(member->plugin, function)))
		return;
	plgfunc(member->plugindata, data);
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

osync_bool osync_member_objtype_enabled(OSyncMember *member, const char *objtype)
{
	g_assert(member);
	return osync_group_objtype_enabled(member->group, objtype);
}
