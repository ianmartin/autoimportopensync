#include "opensync.h"
#include "opensync_internals.h"

/**
 * @defgroup OSyncGroupPrivateAPI OpenSync Group Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

OSyncEnv *osync_group_get_env(OSyncGroup *group)
{
	return group->env;
}

void *osync_group_get_data(OSyncGroup *group)
{
	return group->data;
}

void osync_group_set_data(OSyncGroup *group, void *data)
{
	group->data = data;
}

long long int osync_group_create_member_id(OSyncGroup *group)
{
	char *filename = NULL;
	long long int i = 0;
	do {
		i++;
		if (filename)
			g_free(filename);
		filename = g_strdup_printf("%s/%lli", group->configdir, i);
	} while (g_file_test(filename, G_FILE_TEST_EXISTS));
	g_free(filename);
	return i;
}

OSyncFormatEnv *osync_group_get_format_env(OSyncGroup *group)
{
	g_assert(group);
	return group->conv_env;
}

/*! @brief Loads all members of a group
 * 
 * Loads all members of a group
 * 
 * @param group The group
 * @param
 * @returns Pointer to a new group
 * 
 */
osync_bool osync_group_load_members(OSyncGroup *group, const char *path, OSyncError **error)
{
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *filename = NULL;
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_debug("OSGRP", 3, "Unable to open group configdir %s", gerror->message);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open group configdir %s", gerror->message);
		g_error_free (gerror);
		return FALSE;
	}

	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", osync_group_get_configdir(group), de);
		if (!g_file_test(filename, G_FILE_TEST_IS_DIR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || g_pattern_match_simple(".*", de) || !strcmp("db", de)) {
			g_free(filename);
			continue;
		}

		if (!osync_member_load(group, filename, error)) {
			osync_debug("OSGRP", 0, "Unable to load one of the members");
			g_free(filename);
			g_dir_close(dir);
			return FALSE;
		}
		g_free(filename);
	}
	g_dir_close(dir);
	return TRUE;
}

/*@}*/

/**
 * @defgroup OSyncGroupAPI OpenSync Groups
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

/*! @brief Creates a new group for the given environment
 * 
 * Creates a newly allocated group
 * 
 * @param env The environment for which to create the group. Might be NULL if you which to not add the group at the point of creation
 * @returns Pointer to a new group
 * 
 */
OSyncGroup *osync_group_new(OSyncEnv *env)
{
	OSyncGroup *group = g_malloc0(sizeof(OSyncGroup));

	if (env) {
		osync_env_append_group(env, group);
		group->env = env;
	}
	
	OSyncError *error = NULL;
	group->conv_env = osync_conv_env_new();
	if (!osync_conv_env_load(group->conv_env, &error)) {
		osync_debug("OSGRP", 0, "Unable to load conversion environment: %s\n", error->message);
		osync_error_free(&error);
		group->conv_env = NULL;
	} else {
		/*FIXME: Remove hardcoded format name here */
		osync_conv_set_common_format(group->conv_env, "contact", "vcard30");
	}
	
	osync_debug("OSGRP", 3, "Generated new group");
	return group;
}

/*! @brief Frees the given group
 * 
 * Frees the given group
 * 
 * @param group The group
 * 
 */
void osync_group_free(OSyncGroup *group)
{
	g_assert(group);
	
	if (group->conv_env) {
		osync_conv_env_unload(group->conv_env);
		osync_conv_env_free(group->conv_env);
	}
	
	while (osync_group_nth_member(group, 0))
		osync_member_free(osync_group_nth_member(group, 0));
	
	if (group->env)
		osync_env_remove_group(group->env, group);
	
	if (group->name)
		g_free(group->name);
	
	if (group->configdir)
		g_free(group->configdir);
		
	g_free(group);
}	

/*! @brief Sets the name for the group
 * 
 * Sets the name for a group
 * 
 * @param group The group
 * @param name The name to set
 * 
 */
void osync_group_set_name(OSyncGroup *group, const char *name)
{
	g_assert(group);
	if (group->name)
		g_free(group->name);
	group->name = g_strdup(name);
}

/*! @brief Returns the name of a group
 * 
 * Returns the name of a group
 * 
 * @param group The group
 * @returns Name of the group
 * 
 */
const char *osync_group_get_name(OSyncGroup *group)
{
	g_assert(group);
	return group->name;
}

/*! @brief Saves the group to disc
 * 
 * Saves the group to disc possibly creating the configdirectory
 * 
 * @param group The group
 * @param error Pointer to a error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_save(OSyncGroup *group, OSyncError **error)
{
	g_assert(group);
	osync_assert(group->env, "You must specify a Environment prior to saving the group");
	
	if (!group->configdir) {
		group->id = _osync_env_create_group_id(group->env);
		group->configdir = g_strdup_printf("%s/group%lli", group->env->configdir, group->id);
	}
		
	char *filename = NULL;
	osync_debug("OSGRP", 3, "Trying to open configdirectory %s to save group %s", group->configdir, group->name);
	int i;
	
	if (!g_file_test(group->configdir, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSGRP", 3, "Creating group configdirectory %s", group->configdir);
		if (mkdir(group->configdir, 0700)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for group %s\n", group->name);
			return FALSE;
		}
	}
	
	filename = g_strdup_printf ("%s/syncgroup.conf", group->configdir);
	osync_debug("OSGRP", 3, "Saving group to file %s", filename);
	
	xmlDocPtr doc;

	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, "syncgroup", NULL);
	
	//The filters
	GList *f;
	for (f = group->filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		xmlNodePtr child = xmlNewChild(doc->children, NULL, "filter", NULL);
		
		if (filter->sourcememberid) {
			char *sourcememberid = g_strdup_printf("%lli", filter->sourcememberid);
			xmlNewChild(child, NULL, "sourcemember", sourcememberid);
			g_free(sourcememberid);
		}
		if (filter->destmemberid) {
			char *destmemberid = g_strdup_printf("%lli", filter->destmemberid);
			xmlNewChild(child, NULL, "destmember", destmemberid);
			g_free(destmemberid);
		}
		if (filter->sourceobjtype)
			xmlNewChild(child, NULL, "sourceobjtype", filter->sourceobjtype);
		if (filter->destobjtype)
			xmlNewChild(child, NULL, "destobjtype", filter->destobjtype);
		if (filter->detectobjtype)
			xmlNewChild(child, NULL, "detectobjtype", filter->detectobjtype);
		if (filter->action) {
			char *action = g_strdup_printf("%i", filter->action);
			xmlNewChild(child, NULL, "action", action);
			g_free(action);
		}
		if (filter->function_name)
			xmlNewChild(child, NULL, "function_name", filter->function_name);
		if (filter->config)
			xmlNewChild(child, NULL, "config", filter->config);
	}

	xmlNewChild(doc->children, NULL, "groupname", group->name);

	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);

	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		if (!osync_member_save(member, error))
			return FALSE;
	}
	
	return TRUE;
}

/*! @brief Deletes a group from disc
 * 
 * Deletes to group directories and removes it from its environment
 * 
 * @param group The group
 * @param error Pointer to a error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_delete(OSyncGroup *group, OSyncError **error)
{
	g_assert(group);
	char *delcmd = g_strdup_printf("rm -rf %s", group->configdir);
	if (system(delcmd)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to delete group. command %s failed", delcmd);
		g_free(delcmd);
		return FALSE;
	}
	g_free(delcmd);
	osync_group_free(group);
	return TRUE;
}

/*! @brief Loads a group from a directory
 * 
 * Loads a group from a directory
 * 
 * @param env The environment in which to create the group. Can be NULL
 * @param path The path to the config directory of the group
 * @param error Pointer to a error struct
 * @returns Pointer to the loaded group
 * 
 */
OSyncGroup *osync_group_load(OSyncEnv *env, const char *path, OSyncError **error)
{
	g_assert(env);
	char *filename = NULL;
	char *real_path = NULL;
	
	osync_debug("OSGRP", 3, "Trying to load group from directory %s", path);
	
	if (!g_path_is_absolute(path)) {
		real_path = g_strdup_printf("%s/%s", g_get_current_dir(), path);
	} else {
		real_path = g_strdup(path);
	}
	filename = g_strdup_printf("%s/syncgroup.conf", real_path);
		
	OSyncGroup *group = osync_group_new(env);
	group->configdir = real_path;

	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlNodePtr filternode;
	
	if (!_osync_open_xml_file(&doc, &cur, filename, "syncgroup", error)) {
		osync_group_free(group);
		g_free(filename);
		return NULL;
	}

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"groupname"))
			group->name = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if (!xmlStrcmp(cur->name, (const xmlChar *)"filter")) {
			filternode = cur->xmlChildrenNode;
			OSyncFilter *filter = osync_filter_new();
			filter->group = group;
			
			while (filternode != NULL) {
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"sourceobjtype"))
					filter->sourceobjtype = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"destobjtype"))
					filter->destobjtype = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"detectobjtype"))
					filter->detectobjtype = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"config"))
					filter->config = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"function_name")) {
					char *str = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					osync_filter_update_hook(filter, group, str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"sourcemember")) {
					char *str = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					filter->sourcememberid = atoll(str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"destmember")) {
					char *str = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					filter->destmemberid = atoll(str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"action")) {
					char *str = xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					filter->action = atoi(str);
					xmlFree(str);
				}
				filternode = filternode->next;
			}
			osync_filter_register(group, filter);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
	
	if (!osync_group_load_members(group, real_path, error)) {
		osync_group_free(group);
		return NULL;
	}

	return group;
}

/*! @brief Appends a member to the group
 * 
 * Appends a member to the group
 * 
 * @param group The group to which to append
 * @param member The member to append
 * 
 */
void osync_group_add_member(OSyncGroup *group, OSyncMember *member)
{
	g_assert(group);
	group->members = g_list_append(group->members, member);
}

/*! @brief Removes a member from the group
 * 
 * @param group The group from which to remove
 * @param member The member to remove
 * 
 */
void osync_group_remove_member(OSyncGroup *group, OSyncMember *member)
{
	g_assert(group);
	group->members = g_list_remove(group->members, member);
}

/*! @brief Returns the nth member of the group
 * 
 * Returns a pointer to the nth member of the group
 * 
 * @param group The group
 * @param nth Which member to return
 * @returns Pointer to the member
 * 
 */
OSyncMember *osync_group_nth_member(OSyncGroup *group, int nth)
{
	g_assert(group);
	return (OSyncMember *)g_list_nth_data(group->members, nth);
}

/*! @brief Counts the members of the group
 * 
 * Returns the number of members in the group
 * 
 * @param group The group
 * @returns Number of members
 * 
 */
int osync_group_num_members(OSyncGroup *group)
{
	g_assert(group);
	return g_list_length(group->members);
}

/*! @brief Returns the configdir for the group
 * 
 * Returns the configdir for the group
 * 
 * @param group The group
 * @returns String with the path of the config directory
 * 
 */
const char *osync_group_get_configdir(OSyncGroup *group)
{
	g_assert(group);
	return group->configdir;
}

/*! @brief Sets if the group requires slow-sync for the given object type
 * 
 * Sets if the group requires slow-sync for the given object type. This will be
 * reset once the group performs a successfull slow-sync
 * 
 * @param group The group
 * @param objtypestr The name of the object type
 * @param slow_sync Set to TRUE if you want to perform a slow-sync, FALSE otherwise
 * 
 */
void osync_group_set_slow_sync(OSyncGroup *group, const char *objtypestr, osync_bool slow_sync)
{
	g_assert(group);
	OSyncFormatEnv *conv_env = group->conv_env;

	if (osync_conv_objtype_is_any(objtypestr)) {
		GList *element;
		for (element = conv_env->objtypes; element; element = element->next) {
			OSyncObjType *objtype = element->data;
			objtype->needs_slow_sync = slow_sync;
		}
	} else {
		OSyncObjType *objtype = osync_conv_find_objtype(conv_env, objtypestr);
		g_assert(objtype);
		objtype->needs_slow_sync = slow_sync;
	}
}

/*! @brief Returns if the group will perform a slow-sync for the object type
 * 
 * Returns if the group will perform a slow-sync for the object type
 * 
 * @param group The group
 * @param objtype The name of the object type
 * @returns TRUE if a slow-sync will be performed, FALSE otherwise
 * 
 */
osync_bool osync_group_get_slow_sync(OSyncGroup *group, const char *objtype)
{
	g_assert(group);
	OSyncFormatEnv *env = group->conv_env;
	g_assert(env);
	
	OSyncObjType *osync_objtype = osync_conv_find_objtype(env, "data");
	if (osync_objtype->needs_slow_sync)
		return TRUE;
	osync_objtype = osync_conv_find_objtype(env, objtype);
	g_assert(osync_objtype);
	return osync_objtype->needs_slow_sync;
}

/*! @brief Returns if the object type is enabled for the group
 * 
 * Returns TRUE if the object type is enabled for the group. Note that this
 * information is saved on a per member basis. If one of the members has this object type enabled
 * this function will return TRUE
 * 
 * @param group The group
 * @param objtype The name of the object type
 * @returns TRUE if the object type is enabled for at least one member. FALSE if for none
 * 
 */
osync_bool osync_group_objtype_enabled(OSyncGroup *group, const char *objtype)
{
	//FIXME We should actually return a 3-state here.
	//0 if none is enabled
	//"0.5" if some are enabled, some are not
	//1 if all are enabled
	g_assert(group);
	GList *m;
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		if (osync_member_objtype_enabled(member, objtype))
			return TRUE;
	}
	return FALSE;
}

/*! @brief Sets if the object type is accepted for ALL members
 * 
 * BUG We loose information if only some members are enabled
 * 
 * @param group The group
 * @param objtypestr The name of the object type
 * @param enabled What do you want to set today?
 * 
 */
void osync_group_set_objtype_enabled(OSyncGroup *group, const char *objtypestr, osync_bool enabled)
{
	g_assert(group);
	GList *m;
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		osync_member_set_objtype_enabled(member, objtypestr, enabled);
	}
}

int osync_group_num_filters(OSyncGroup *group)
{
	g_assert(group);
	return g_list_length(group->filters);
}

OSyncFilter *osync_group_nth_filter(OSyncGroup *group, int nth)
{
	g_assert(group);
	return g_list_nth_data(group->filters, nth);
}

/*@}*/
