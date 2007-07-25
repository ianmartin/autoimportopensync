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

#include "opensync-format.h"
#include "opensync-group.h"
#include "opensync_group_internals.h"
#include "opensync-db.h"


#include "opensync_xml.h"

#ifndef _WIN32
#include <sys/file.h>

#ifdef NOT_HAVE_FLOCK
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8

static int flock(int fd, int operation)
{
       struct flock flock;

       switch (operation & ~LOCK_NB) {
       case LOCK_SH:
               flock.l_type = F_RDLCK;
               break;
       case LOCK_EX:
               flock.l_type = F_WRLCK;
               break;
       case LOCK_UN:
               flock.l_type = F_UNLCK;
               break;
       default:
               errno = EINVAL;
               return -1;
       }

       flock.l_whence = 0;
       flock.l_start = 0;
       flock.l_len = 0;

       return fcntl(fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW, &flock);
}
#endif //NOT_HAVE_FLOCK
#endif //not defined _WIN32

/**
 * @defgroup OSyncGroupPrivateAPI OpenSync Group Internals
 * @ingroup OSyncPrivate
 * @brief The private API of opensync
 * 
 * This gives you an insight in the private API of opensync.
 * 
 */
/*@{*/

/*! @brief Creates a new unique member if in this group
 * 
 * @param group The group
 * @returns A new unique member id
 * 
 */
static long long int _osync_group_create_member_id(OSyncGroup *group)
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

/*! @brief Loads all members of a group
 * 
 * Loads all members of a group
 * 
 * @param group The group
 * @param path The path from which to load the members
 * @param error Pointer to an error struct
 * @returns True if the members were loaded successfully, FALSE otherwise
 * 
 */
static osync_bool _osync_group_load_members(OSyncGroup *group, const char *path, OSyncError **error)
{	
	GDir *dir = NULL;
	GError *gerror = NULL;
	char *member_path = NULL;
	char *filename = NULL;
	OSyncMember *member = NULL;
	const gchar *de = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, group, path ? path : "nil", error);

	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open group configdir %s", gerror->message);
		g_error_free (gerror);
		goto error;
	}

	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s/syncmember.conf", osync_group_get_configdir(group), de);
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			g_free(filename);
			continue;
		}
		g_free(filename);
		
		member = osync_member_new(error);
		if (!member)
			goto error_close;
		
		member_path = g_strdup_printf ("%s/%s", osync_group_get_configdir(group), de);
		if (!osync_member_load(member, member_path, error)) {
			g_free(member_path);
			goto error_free_member;
		}
		g_free(member_path);
		
		osync_group_add_member(group, member);
		osync_member_unref(member);
	}
	g_dir_close(dir);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_member:
	osync_member_unref(member);
error_close:
	g_free(filename);
	g_dir_close(dir);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(error));
	return FALSE;
}

static void _build_list(gpointer key, gpointer value, gpointer user_data)
{
	if (GPOINTER_TO_INT(value) >= 2) {
		GList **l = user_data;
		*l = g_list_append(*l, key);
	}
}

static void _add_one(gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *table = user_data;
	
	int num = GPOINTER_TO_INT(value);
	g_hash_table_replace(table, key, GINT_TO_POINTER(num + 1));
}

/*! @brief Get list of supported object types of the group 
 * 
 * @param group The group
 * @returns List of supported object types 
 * 
 */
static GList *_osync_group_get_supported_objtypes(OSyncGroup *group)
{
	GList *m = NULL;
	GList *ret = NULL;
	GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
    
	int num_data = 0;
	int i;
    
	/* Loop over all members... */
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		int num_member = osync_member_num_objtypes(member);
		/* ... and get the objtype from each of the members. */
		for (i = 0; i < num_member; i++) {
			const char *objtype = osync_member_nth_objtype(member, i);
			if (objtype != NULL) {
				/* For each objtype, add 1 to the hashtable. If the objtype is
			 	* the special objtype "data", add 1 to all objtypes */
				if(!strcmp(objtype, "data"))
					num_data++;
				int num = GPOINTER_TO_INT(g_hash_table_lookup(table, objtype));
				g_hash_table_replace(table, (char *)objtype, GINT_TO_POINTER(num + 1));
			}
		}
	}
	
	for (i = 0; i < num_data; i++)
		g_hash_table_foreach(table, _add_one, table);
	
	if (g_hash_table_size(table) == 0 && num_data >= 2) {
		osync_trace(TRACE_INTERNAL, "No objtype found yet, but data available");
		g_hash_table_replace(table, "data", GINT_TO_POINTER(num_data));
	}
	
	g_hash_table_foreach(table, _build_list, &ret);
	g_hash_table_destroy(table);
	return ret;
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
 * @param error Pointer to an error struct
 * @returns Pointer to a new group
 * 
 */
OSyncGroup *osync_group_new(OSyncError **error)
{
	OSyncGroup *group = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	group = osync_try_malloc0(sizeof(OSyncGroup), error);
	if (!group)
		goto error;
	group->ref_count = 1;

	/* By default Merger is enabled */
	group->use_merger = TRUE;

	/* By default Converter is enabled */
	group->use_converter = TRUE;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, group);
	return group;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(error));
	return NULL;
}


/** @brief Increase the reference count of the group
 * 
 * @param group The group
 * 
 */
void osync_group_ref(OSyncGroup *group)
{
	osync_assert(group);
	
	g_atomic_int_inc(&(group->ref_count));
}

/** @brief Decrease the reference count of the group
 * 
 * @param group The group
 * 
 */
void osync_group_unref(OSyncGroup *group)
{
	osync_assert(group);
		
	if (g_atomic_int_dec_and_test(&(group->ref_count))) {
		if (group->lock_fd)
			osync_group_unlock(group);
		
		while (group->members)
			osync_group_remove_member(group, group->members->data);
		
		if (group->name)
			g_free(group->name);
		
		if (group->configdir)
			g_free(group->configdir);
			
		g_free(group);
	}
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
 * last sync of this group was successful, or if this something crashed.
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
	char *lockfile = NULL;
	osync_bool exists = FALSE;
	osync_bool locked = FALSE;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, group);
	osync_assert(group);
	
	if (!group->configdir) {
		osync_trace(TRACE_EXIT, "%s: OSYNC_LOCK_OK: No configdir", __func__);
		return OSYNC_LOCK_OK;
	}
	
	if (group->lock_fd) {
		osync_trace(TRACE_EXIT, "%s: OSYNC_LOCKED, lock_fd existed", __func__);
		return OSYNC_LOCKED;
	}
	
	lockfile = g_strdup_printf("%s/lock", group->configdir);

	if (g_file_test(lockfile, G_FILE_TEST_EXISTS)) {
		osync_trace(TRACE_INTERNAL, "locking group: file exists");
		exists = TRUE;
	}

	if ((group->lock_fd = g_open(lockfile, O_CREAT | O_WRONLY, 00700)) == -1) {
		group->lock_fd = 0;
		g_free(lockfile);
		osync_trace(TRACE_EXIT, "%s: Unable to open: %s", __func__, g_strerror(errno) ? g_strerror(errno) : "nil");
		return OSYNC_LOCK_STALE;
	} else {
#ifndef _WIN32
		/* Set FD_CLOEXEC flags for the lock file descriptor. We don't want the
		 * subprocesses created by plugins or the engine to keep holding the lock
		 */
		int oldflags = fcntl(group->lock_fd, F_GETFD);
		if (oldflags == -1) {
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Unable to get fd flags");
			return OSYNC_LOCK_STALE;
		}

		if (fcntl(group->lock_fd, F_SETFD, oldflags|FD_CLOEXEC) == -1) {
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, "Unable to set fd flags");
			return OSYNC_LOCK_STALE;
		}

		if (flock(group->lock_fd, LOCK_EX | LOCK_NB) == -1) {
			if (errno == EWOULDBLOCK) {
				osync_trace(TRACE_INTERNAL, "locking group: is locked2");
				locked = TRUE;
				close(group->lock_fd);
				group->lock_fd = 0;
			} else
				osync_trace(TRACE_INTERNAL, "error setting lock: %s", g_strerror(errno) ? g_strerror(errno) : "nil");
		} else
#endif
			osync_trace(TRACE_INTERNAL, "Successfully locked");
	}
	
	g_free(lockfile);
	
	if (exists) {
		if (locked) {
			osync_trace(TRACE_EXIT, "%s: OSYNC_LOCKED", __func__);
			return OSYNC_LOCKED;
		} else {
			osync_trace(TRACE_EXIT, "%s: OSYNC_LOCK_STALE", __func__);
			return OSYNC_LOCK_STALE;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: OSYNC_LOCK_OK", __func__);
	return OSYNC_LOCK_OK;
}

/*! @brief Unlocks a group
 * 
 * @param group The group to unlock
 * 
 */
void osync_group_unlock(OSyncGroup *group)
{
	char *lockfile = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, group);
	osync_assert(group);
	
	if (!group->configdir) {
		osync_trace(TRACE_EXIT, "%s: No configdir", __func__);
		return;
	}
	
	if (!group->lock_fd) {
		osync_trace(TRACE_EXIT, "%s: You have to lock the group before unlocking", __func__);
		return;
	}
    
#ifndef _WIN32
	flock(group->lock_fd, LOCK_UN);
#endif

	close(group->lock_fd);
	
	group->lock_fd = 0;
	
	lockfile = g_strdup_printf("%s/lock", group->configdir);
	
	g_unlink(lockfile);
	g_free(lockfile);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
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
 * @param error Pointer to an error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_save(OSyncGroup *group, OSyncError **error)
{
	char *filename = NULL;
	int i;
	xmlDocPtr doc;
	char *tmstr = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, error);
	osync_assert(group);
	osync_assert(group->configdir);
	
	osync_trace(TRACE_INTERNAL, "Trying to open configdirectory %s to save group %s", group->configdir ? group->configdir : "nil", group->name ? group->name : "nil");
	
	if (!g_file_test(group->configdir, G_FILE_TEST_IS_DIR)) {
		osync_trace(TRACE_INTERNAL, "Creating group configdirectory %s", group->configdir ? group->configdir : "nil");
		if (g_mkdir(group->configdir, 0700)) {
			osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to create directory for group %s\n", group->name);
			goto error;
		}
	}
	
	filename = g_strdup_printf ("%s/syncgroup.conf", group->configdir);
	osync_trace(TRACE_INTERNAL, "Saving group to file %s", filename ? filename : "nil");
	
	doc = xmlNewDoc((xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"syncgroup", NULL);
	
	// TODO: reimplement the filter!
	//The filters
	/*GList *f;
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
	}*/

	xmlNewChild(doc->children, NULL, (xmlChar*)"groupname", (xmlChar*)group->name);

	tmstr = g_strdup_printf("%i", (int)group->last_sync);
	xmlNewChild(doc->children, NULL, (xmlChar*)"last_sync", (xmlChar*)tmstr);
	g_free(tmstr);

	xmlNewChild(doc->children, NULL, (xmlChar*)"enable_merger", (xmlChar*) (group->use_merger ? "true" : "false"));
	xmlNewChild(doc->children, NULL, (xmlChar*)"enable_converter", (xmlChar*) (group->use_converter ? "true" : "false"));


	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);

	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		if (!osync_member_save(member, error))
			goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

/*! @brief Deletes a group from disc
 * 
 * Deletes to group directories
 * 
 * @param group The group
 * @param error Pointer to an error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_delete(OSyncGroup *group, OSyncError **error)
{
	char *delcmd = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, error);
	osync_assert(group);
	
	delcmd = g_strdup_printf("rm -rf %s", group->configdir);
	if (system(delcmd)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to delete group. command %s failed", delcmd);
		g_free(delcmd);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
		return FALSE;
	}
	g_free(delcmd);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/*! @brief Reset all databases of a group (anchor, hashtable and archieve) 
 * 
 * @param group The group
 * @param error Pointer to an error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_reset(OSyncGroup *group, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, error);

	OSyncDB *db = NULL;
	GList *m = NULL;
	char *path = NULL;

	osync_assert(group);

	/* Loop over all members... */
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;

		/* flush hashtable */
		path = g_strdup_printf("%s/hashtable.db", osync_member_get_configdir(member));
		if (!(db = osync_db_new(error)))
			goto error_and_free;

		if (!osync_db_open(db, path, error))
			goto error_and_free;

		osync_db_reset_full(db, error);

		g_free(path);

		/* flush anchor db */ 
		path = g_strdup_printf("%s/anchor.db", osync_member_get_configdir(member));
		if (!(db = osync_db_new(error)))
			goto error_and_free;

		if (!osync_db_open(db, path, error))
			goto error_and_free;

		osync_db_reset_full(db, error);

		g_free(path);

	}

	path = g_strdup_printf("%s/archive.db", osync_group_get_configdir(group));
	if (!(db = osync_db_new(error)))
		goto error_and_free;

	if (!osync_db_open(db, path, error))
		goto error_and_free;

	osync_db_reset_full(db, error);

	g_free(path);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_and_free:
	g_free(path);	
//error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
}

/*! @brief Loads a group from a directory
 * 
 * Loads a group from a directory
 * 
 * @param group The group object to load into
 * @param path The path to the config directory of the group
 * @param error Pointer to an error struct
 * @returns TRUE on success, FALSE otherwise
 * 
 */
osync_bool osync_group_load(OSyncGroup *group, const char *path, OSyncError **error)
{
	char *filename = NULL;
	char *real_path = NULL;
	xmlDocPtr doc;
	xmlNodePtr cur;
	//xmlNodePtr filternode;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, group, path ? path : "nil", error);
	osync_assert(group);
	osync_assert(path);
	
	if (!g_path_is_absolute(path)) {
		char *curdir = g_get_current_dir();
		real_path = g_strdup_printf("%s/%s", curdir, path);
		g_free(curdir);
	} else {
		real_path = g_strdup(path);
	}
	
	osync_group_set_configdir(group, real_path);
	filename = g_strdup_printf("%s/syncgroup.conf", real_path);
	g_free(real_path);
	
	if (!osync_open_xml_file(&doc, &cur, filename, "syncgroup", error)) {
		g_free(filename);
		goto error;
	}
	g_free(filename);
	
	while (cur != NULL) {
		char *str = (char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"groupname"))
				osync_group_set_name(group, str);
	
			if (!xmlStrcmp(cur->name, (const xmlChar *)"last_sync"))
				group->last_sync = (time_t)atoi(str);

			if (!xmlStrcmp(cur->name, (const xmlChar *)"enable_merger"))
				group->use_merger = (!g_ascii_strcasecmp("true", str)) ? TRUE : FALSE;

			if (!xmlStrcmp(cur->name, (const xmlChar *)"enable_converter"))
				group->use_converter = (!g_ascii_strcasecmp("true", str)) ? TRUE : FALSE;

			// TODO: reimplement the filter!
			/*if (!xmlStrcmp(cur->name, (const xmlChar *)"filter")) {
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
			}*/
		
			xmlFree(str);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	
	/* Check for sanity */
	if (!group->name) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Loaded a group without a name");
		goto error;
	}
	
	if (!_osync_group_load_members(group, group->configdir, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, group);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error) ? osync_error_print(error) : "nil");
	return FALSE;
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
	
	if (!osync_member_get_configdir(member)) {
		char *configdir = g_strdup_printf("%s/%lli", group->configdir, _osync_group_create_member_id(group));
		osync_member_set_configdir(member, configdir);
		g_free(configdir);
	}
	
	group->members = g_list_append(group->members, member);
	osync_member_ref(member);
}

/*! @brief Removes a member from the group
 * 
 * @param group The group from which to remove
 * @param member The member to remove
 * 
 */
void osync_group_remove_member(OSyncGroup *group, OSyncMember *member)
{
	osync_assert(group);
	group->members = g_list_remove(group->members, member);
	osync_member_unref(member);
}

/** @brief Searches for a member by its id
 * 
 * @param group The group in which to search
 * @param id The id of the member
 * @returns The member, or NULL if not found
 * 
 */
OSyncMember *osync_group_find_member(OSyncGroup *group, int id)
{
	GList *m = NULL;
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		if (osync_member_get_id(member) == id)
			return member;
	}
	return NULL;
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
	osync_assert(group);
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
	osync_assert(group);
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
	osync_assert(group);
	return group->configdir;
}

/*! @brief Sets the configdir of the group
 * 
 * @param group The group
 * @param directory The new configdir
 * @returns String with the path of the config directory
 * 
 */
void osync_group_set_configdir(OSyncGroup *group, const char *directory)
{
	osync_assert(group);
	if (group->configdir)
		g_free(group->configdir);
	group->configdir = g_strdup(directory);
}

/*! @brief The number of object types of the group
 * 
 * @param group The group
 * @returns Number of object types of the group 
 * 
 */
int osync_group_num_objtypes(OSyncGroup *group)
{
	GList *objs = NULL;
	int len = 0;
	osync_assert(group);
	objs = _osync_group_get_supported_objtypes(group);
	len = g_list_length(objs);
	g_list_free(objs);
	return len;
}

/*! @brief The nth object type of the group
 * 
 * @param group The group
 * @param nth The nth position of the object type list
 * @returns Name of the nth object type of the group
 * 
 */
const char *osync_group_nth_objtype(OSyncGroup *group, int nth)
{
	GList *objs = NULL;
	const char *objtype = NULL;
	osync_assert(group);
	objs = _osync_group_get_supported_objtypes(group);
	objtype = g_list_nth_data(objs, nth);
	g_list_free(objs);
	return objtype;
	
}

/*! @brief Change the status of an object type in the group.
 * 
 * @param group The group
 * @param objtype Name of the object type
 * @param enabled The status of the object type to set. TRUE enable, FALSE diable objtype. 
 * 
 */
void osync_group_set_objtype_enabled(OSyncGroup *group, const char *objtype, osync_bool enabled)
{
	GList *m = NULL;
	osync_assert(group);
	/* Loop over all members... */
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		osync_member_set_objtype_enabled(member, objtype, enabled);
	}
}

/*! @brief Get the status of an object type in the group 
 * 
 * @param group The group
 * @param objtype The name of the object type 
 * @returns TRUE if object type is enabled in this group, otherwise FALSE
 * 
 */
int osync_group_objtype_enabled(OSyncGroup *group, const char *objtype)
{
	GList *m = NULL;
	int enabled = -1;
	
	osync_assert(group);
	
	/* What do to:
	 * 
	 * g -> enabled variable
	 * m = value from member
	 * 
	 *   g  -1 0 1 2
	 * m
	 * -1   -1 0 1 2
	 * 0     0 0 1 1
	 * 1     2 1 1 2
	 * 
	 */
	
	/* Loop over all members... */
	for (m = group->members; m; m = m->next) {
		OSyncMember *member = m->data;
		switch (osync_member_objtype_enabled(member, objtype)) {
			case -1:
				//Do nothing;
				break;
			case 0:
				if (enabled == -1)
					enabled = 0;
				else if (enabled == 2)
					enabled = 1;
				break;
			case 1:
				if (enabled == -1)
					enabled = 2;
				else if (enabled == 0)
					enabled = 1;
				break;
		}
	}
	return enabled;
}

/*! @brief Add filter to the group 
 * 
 * @param group The group
 * @param filter The filter to add 
 * 
 */
void osync_group_add_filter(OSyncGroup *group, OSyncFilter *filter)
{
	osync_assert(group);
	group->filters = g_list_append(group->filters, filter);
	osync_filter_ref(filter);
}

/*! @brief Remove filter from the group 
 * 
 * @param group The group
 * @param filter The filter to remove
 * 
 */
void osync_group_remove_filter(OSyncGroup *group, OSyncFilter *filter)
{
	osync_assert(group);
	group->filters = g_list_remove(group->filters, filter);
	osync_filter_unref(filter);
}

/*! @brief Returns the number of filters registered in a group
 * 
 * @param group The group
 * @returns The number of filters
 * 
 */
int osync_group_num_filters(OSyncGroup *group)
{
	osync_assert(group);
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
	osync_assert(group);
	return g_list_nth_data(group->filters, nth);
}

/*! @brief Sets the last synchronization date of this group
 * 
 * The information will be stored on disc after osync_group_save()
 * 
 * @param group The group in which to save
 * @param last_sync The time info to set
 */
void osync_group_set_last_synchronization(OSyncGroup *group, time_t last_sync)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, group, last_sync);
	osync_assert(group);
	
	group->last_sync = last_sync;
               
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Gets the last synchronization date from this group
 * 
 * The information will available on the group after osync_group_load()
 * 
 * @param group The group
 * @return The synchronization info
 */
time_t osync_group_get_last_synchronization(OSyncGroup *group)
{
	osync_assert(group);
	return group->last_sync;
}

/*! @brief Set fixed conflict resolution for the group for all appearing conflicts 
 * 
 * @param group The group
 * @param res The conflict resolution
 * @param num The Member ID which solves the conflict (winner)
 * 
 */
void osync_group_set_conflict_resolution(OSyncGroup *group, OSyncConflictResolution res, int num)
{
	osync_assert(group);
	group->conflict_resolution = res;
	group->conflict_winner = num;
}

/*! @brief Get fixed conflict resolution for the group for all appearing conflicts 
 * 
 * @param group The group
 * @param res Pointer to set conflict resolution value
 * @param num Pointer to set Member ID value which solves the conflict (winner)
 * 
 */
void osync_group_get_conflict_resolution(OSyncGroup *group, OSyncConflictResolution *res, int *num)
{
	osync_assert(group);
	osync_assert(res);
	osync_assert(num);
	
	*res = group->conflict_resolution;
	*num = group->conflict_winner;
}

/*! @brief Get group configured status of merger use. 
 * 
 * @param group The group
 * @return TRUE if merger is enabled. FALSE if merger is disabled.
 */
osync_bool osync_group_get_use_merger(OSyncGroup *group)
{
	osync_assert(group);

	return group->use_merger;
}

/*! @brief Configure status of merger use. 
 * 
 * @param group The group
 * @param use_merger TRUE enables the merger. FALSE disables the merger. 
 */
void osync_group_set_use_merger(OSyncGroup *group, osync_bool use_merger)
{
	osync_assert(group);

	group->use_merger = use_merger;
}

/*! @brief Get group configured status of converter use. 
 * 
 * @param group The group
 * @return TRUE if converter is enabled. FALSE if converter is disabled.
 */
osync_bool osync_group_get_use_converter(OSyncGroup *group)
{
	osync_assert(group);

	return group->use_converter;
}

/*! @brief Configure status of converter use. 
 * 
 * @param group The group
 * @param use_converter TRUE enables the converter. FALSE disables the converter. 
 */
void osync_group_set_use_converter(OSyncGroup *group, osync_bool use_converter)
{
	osync_assert(group);

	group->use_converter = use_converter;
}

/*@}*/
