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
#include <errno.h>
#include <sys/file.h>

extern int errno;

/**
 * @defgroup OSyncGroupPrivateAPI OpenSync Group Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

/*! @brief Returns the environment in which a group is registered
 * 
 * @param group The group
 * @returns The environment
 * 
 */
OSyncEnv *osync_group_get_env(OSyncGroup *group)
{
	return group->env;
}

/*! @brief Gets the custom data of a group
 * 
 * @param group The group
 * @returns The custom data of this group
 * 
 */
void *osync_group_get_data(OSyncGroup *group)
{
	return group->data;
}

/*! @brief Sets the custom data of a group
 * 
 * @param group The group
 * @param data The custom data
 * 
 */
void osync_group_set_data(OSyncGroup *group, void *data)
{
	group->data = data;
}

/*! @brief Creates a new unique member if in this group
 * 
 * @param group The group
 * @returns A new unique member id
 * 
 */
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

/*! @brief Returns the format environment of a group
 * 
 * @param group The group
 * @returns The format environment
 * 
 */
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
 * @param path The path from which to load the members
 * @param error Pointer to a error
 * @returns True if the members were loaded successfully, FALSE otherwise
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
 * @brief A groups represent several device or application that should be synchronized
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
	group->conv_env = osync_conv_env_new(env);
	
	if (env) {
		osync_env_append_group(env, group);
		group->env = env;
	}
	
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
	
	if (group->conv_env)
		osync_conv_env_free(group->conv_env);
	
	if (group->lock_fd)
		osync_group_unlock(group, FALSE);
	
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

/*! @brief Locks a group
 * 
 * Tries to acquire a lock for the given group.
 * 
 * If the lock was successfully acquired, OSYNC_LOCK_OK will
 * be returned.
 * 
 * If the lock was acquired, but a old lock file was detected,
 * OSYNC_LOCK_STALE will be returned. Use this to detect if the
 * last sync of this group was successfull, or if this something crashed.
 * If you get this answer you should perform a slow-sync
 * 
 * If the group is locked, OSYNC_LOCKED is returned
 * 
 * @param group The group
 * @returns if the lockfile was acquired
 * 
 */
OSyncLockState osync_group_lock(OSyncGroup *group)
{
	osync_trace(TRACE_ENTRY, "osync_group_lock(%p)", group);
	g_assert(group);
	g_assert(group->configdir);
	
	osync_bool exists = FALSE;
	osync_bool locked = FALSE;
	
	if (group->lock_fd) {
		osync_trace(TRACE_EXIT, "osync_group_lock: OSYNC_LOCKED, lock_fd existed");
		return OSYNC_LOCKED;
	}
	
	char *lockfile = g_strdup_printf("%s/lock", group->configdir);
	osync_debug("GRP", 4, "locking file %s", lockfile);

	if (g_file_test(lockfile, G_FILE_TEST_EXISTS)) {
		osync_debug("GRP", 4, "locking group: file exists");
		exists = TRUE;
	}
	
	if ((group->lock_fd = open(lockfile, O_CREAT | O_WRONLY, 00700)) == -1) {
		group->lock_fd = 0;
		osync_debug("GRP", 1, "error opening file: %s", strerror(errno));
		g_free(lockfile);
		osync_trace(TRACE_EXIT_ERROR, "osync_group_lock: %s", strerror(errno));
		return OSYNC_LOCK_STALE;
	} else {
		if (flock(group->lock_fd, LOCK_EX | LOCK_NB) == -1) {
			if (errno == EWOULDBLOCK) {
				osync_debug("GRP", 4, "locking group: is locked2");
				locked = TRUE;
				close(group->lock_fd);
				group->lock_fd = 0;
			} else
				osync_debug("GRP", 1, "error setting lock: %s", strerror(errno));
		} else
			osync_debug("GRP", 4, "Successfully locked");
	}
	g_free(lockfile);
	
	if (!exists) {
		osync_trace(TRACE_EXIT, "osync_group_lock: OSYNC_LOCK_OK");
		return OSYNC_LOCK_OK;
	} else {
		if (locked) {
			osync_trace(TRACE_EXIT, "osync_group_lock: OSYNC_LOCKED");
			return OSYNC_LOCKED;
		} else {
			osync_trace(TRACE_EXIT, "osync_group_lock: OSYNC_LOCK_STALE");
			return OSYNC_LOCK_STALE;
		}
	}
}

/*! @brief Unlocks a group
 * 
 * if you set remove = FALSE, the lock file will not be removed
 * and the next call to osync_lock_group() for this group will
 * return OSYNC_LOCK_STALE.
 * 
 * @param group The group
 * @param remove If the lockfile should be removed
 * 
 */
void osync_group_unlock(OSyncGroup *group, osync_bool remove)
{
	g_assert(group);
	g_assert(group->configdir);
	osync_debug("GRP", 4, "unlocking group %s", group->name);
	
	if (!group->lock_fd) {
		osync_debug("GRP", 1, "You have to lock the group before unlocking");
		return;
	}
    
	if (flock(group->lock_fd, LOCK_UN) == -1) {
		osync_debug("GRP", 1, "error releasing lock: %s", strerror(errno));
		return;
	}
	
	fsync(group->lock_fd);
	close(group->lock_fd);
	
	group->lock_fd = 0;
	
	if (remove) {
		char *lockfile = g_strdup_printf("%s/lock", group->configdir);
		unlink(lockfile);
		g_free(lockfile);
	}
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
		group->configdir = g_strdup_printf("%s/group%lli", group->env->groupsdir, group->id);
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

	doc = xmlNewDoc((xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"syncgroup", NULL);
	
	//The filters
	GList *f;
	for (f = group->filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		xmlNodePtr child = xmlNewChild(doc->children, NULL, (xmlChar*)"filter", NULL);
		
		if (filter->sourcememberid) {
			char *sourcememberid = g_strdup_printf("%lli", filter->sourcememberid);
			xmlNewChild(child, NULL, (xmlChar*)"sourcemember", (xmlChar*)sourcememberid);
			g_free(sourcememberid);
		}
		if (filter->destmemberid) {
			char *destmemberid = g_strdup_printf("%lli", filter->destmemberid);
			xmlNewChild(child, NULL, (xmlChar*)"destmember", (xmlChar*)destmemberid);
			g_free(destmemberid);
		}
		if (filter->sourceobjtype)
			xmlNewChild(child, NULL, (xmlChar*)"sourceobjtype", (xmlChar*)filter->sourceobjtype);
		if (filter->destobjtype)
			xmlNewChild(child, NULL, (xmlChar*)"destobjtype", (xmlChar*)filter->destobjtype);
		if (filter->detectobjtype)
			xmlNewChild(child, NULL, (xmlChar*)"detectobjtype", (xmlChar*)filter->detectobjtype);
		if (filter->action) {
			char *action = g_strdup_printf("%i", filter->action);
			xmlNewChild(child, NULL, (xmlChar*)"action", (xmlChar*)action);
			g_free(action);
		}
		if (filter->function_name)
			xmlNewChild(child, NULL, (xmlChar*)"function_name", (xmlChar*)filter->function_name);
		if (filter->config)
			xmlNewChild(child, NULL, (xmlChar*)"config", (xmlChar*)filter->config);
	}

	xmlNewChild(doc->children, NULL, (xmlChar*)"groupname", (xmlChar*)group->name);

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
	
	osync_trace(TRACE_ENTRY, "osync_group_load(%p, %s, %p)", env, path, error);
	
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
		osync_trace(TRACE_EXIT_ERROR, "osync_group_load");
		return NULL;
	}

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"groupname"))
			group->name = (char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if (!xmlStrcmp(cur->name, (const xmlChar *)"filter")) {
			filternode = cur->xmlChildrenNode;
			OSyncFilter *filter = osync_filter_new();
			filter->group = group;
			
			while (filternode != NULL) {
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"sourceobjtype"))
					filter->sourceobjtype = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"destobjtype"))
					filter->destobjtype = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"detectobjtype"))
					filter->detectobjtype = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"config"))
					filter->config = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"function_name")) {
					char *str = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					osync_filter_update_hook(filter, group, str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"sourcemember")) {
					char *str = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					filter->sourcememberid = atoll(str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"destmember")) {
					char *str = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
					if (!str) {
						filternode = filternode->next;
						continue;
					}
					filter->destmemberid = atoll(str);
					xmlFree(str);
				}
				
				if (!xmlStrcmp(filternode->name, (const xmlChar *)"action")) {
					char *str = (char*)xmlNodeListGetString(doc, filternode->xmlChildrenNode, 1);
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
	
	//Check for sanity
	if (!group->name) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Loaded a group without a name");
		osync_debug("OSGRP", 0, "Loaded a group without a name");
		osync_group_free(group);
		osync_trace(TRACE_EXIT_ERROR, "osync_group_load");
		return NULL;
	}
	
	if (!osync_group_load_members(group, real_path, error)) {
		osync_group_free(group);
		osync_trace(TRACE_EXIT_ERROR, "osync_group_load");
		return NULL;
	}
	
	osync_trace(TRACE_EXIT, "osync_group_load");
	return group;
}

/*! @brief Resets all databases of a group
 * 
 * This will reset all databases of a group. So all anchors, mappings
 * hashtables etc will be forgotten (as if the group was never synced)
 * 
 * @param group The group to reset
 * 
 */
void osync_group_reset(OSyncGroup *group)
{
	OSyncError *error = NULL;
	osync_db_reset_group(group, &error);
	
	GList *m = NULL;
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		osync_db_reset_member(member, &error);
	}
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

/*! @brief Returns the number of filters registered in a group
 * 
 * @param group The group
 * @returns The number of filters
 * 
 */
int osync_group_num_filters(OSyncGroup *group)
{
	g_assert(group);
	return g_list_length(group->filters);
}

/*! @brief Gets the nth filter of a group
 * 
 * Note that you should not add or delete filters while
 * iterating over them
 * 
 * @param group The group
 * @param nth Which filter to return
 * @returns The filter or NULL if not found
 * 
 */
OSyncFilter *osync_group_nth_filter(OSyncGroup *group, int nth)
{
	g_assert(group);
	return g_list_nth_data(group->filters, nth);
}

/*! @brief Flushes the list of filters for a group
 *
 * Clean the list of filters on the group
 */
void osync_group_flush_filters(OSyncGroup *group)
{
	g_assert(group);
	while (group->filters) {
		OSyncFilter *f = g_list_nth_data(group->filters, 0);
		osync_filter_free(f);

		/* Delete the first item */
		group->filters = g_list_delete_link(group->filters, group->filters);
	}
}

/*! @brief Can be used to load all items from the changelog. Loaded items will be removed
 * 
 * @param group The group for which to load the log
 * @param uids Place to return an array with the saved uids
 * @param memberids Place to return an array with the saved memberids
 * @param changetypes Place to return an array with the saved changetypes. Same size as uids
 * @param error Place to return the error
 * @returns TRUE if successfull, FALSE otherwise
 */
osync_bool osync_group_open_changelog(OSyncGroup *group, char ***uids, long long int **memberids, int **changetypes, OSyncError **error)
{
	return osync_db_open_changelog(group, uids, memberids, changetypes, error);
}

/*! @brief Saves a change to the changelog.
 * 
 * @param group The group in which to save
 * @param change The change to save
 * @param error Place to return the error
 * @returns TRUE if successfull, FALSE otherwise
 */
osync_bool osync_group_save_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error)
{
	return osync_db_save_changelog(group, change, error);
}

/*! @brief Removes a change from the changelog.
 * 
 * @param group The group in which to save
 * @param change The change to remove
 * @param error Place to return the error
 * @returns TRUE if successfull, FALSE otherwise
 */
osync_bool osync_group_remove_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error)
{
	return osync_db_remove_changelog(group, change, error);
}

/*@}*/
