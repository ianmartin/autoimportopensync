#include <opensync.h>
#include "opensync_internals.h"
#include "opensync_group.h"

OSyncGroup *osync_group_new(OSyncEnv *osinfo)
{
	OSyncGroup *group = g_malloc0(sizeof(OSyncGroup));
	char *filename = NULL;
	
	filename = g_strdup_printf("%s/group%i", osync_env_get_configdir(osinfo), g_random_int_range(1, 1000000));
	group->configdir = filename;
	group->env = osinfo;
	group->conv_env = osync_conv_env_new();
	osync_conv_env_load(group->conv_env);
	osync_conv_set_common_format(group->conv_env, "contact", "vcard");
	osync_debug("OSGRP", 3, "Generated new group:");
	osync_debug("OSGRP", 3, "Configdirectory: %s", filename);
	return group;
}

void osync_group_free(OSyncGroup *group)
{
	printf("Freeing group\n");
	osync_db_tear_down(group->dbenv);
	g_assert(group);
	g_free(group->name);
	g_free(group->configdir);
	g_free(group);
}	

void osync_group_set_name(OSyncGroup *group, char *name)
{
	osync_debug("OSGRP", 3, "Setting name of group %s to %s", group->name, name);
	group->name = g_strdup(name);
}

char *osync_group_get_name(OSyncGroup *group)
{
	return group->name;
}

void osync_group_save(OSyncGroup *group)
{
	char *filename = NULL;
	osync_debug("OSGRP", 3, "Trying to open configdirectory %s to save group %s", group->configdir, group->name);
	int i;
		
	if (!g_file_test(group->configdir, G_FILE_TEST_IS_DIR)) {
		osync_debug("OSGRP", 3, "Creating configdirectory %s", group->configdir);
		mkdir(group->configdir, 0777);
		char *dbdir = g_strdup_printf("%s/db", group->configdir);
		mkdir(dbdir, 0777);
		g_free(dbdir);
	}
	
	filename = g_strdup_printf ("%s/syncgroup.conf", group->configdir);
	osync_debug("OSGRP", 3, "Saving group to file %s", filename);
	
	xmlDocPtr doc;

	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, "syncgroup", NULL);

	xmlNewChild(doc->children, NULL, "name", group->name);

	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
	g_free(filename);

	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_get_nth_member(group, i);
		osync_member_save(member);
	}
}

OSyncGroup *osync_group_load(OSyncEnv *env, char *path)
{
	OSyncMember *member = NULL;
	char *filename = NULL;
	
	osync_debug("OSGRP", 3, "Trying to load group from directory %s", path);
	OSyncGroup *group = osync_group_new(env);
	if (!g_path_is_absolute(path)) {
		char *abspath = g_strdup_printf("%s/%s", g_get_current_dir(), path); //FIXME Free the string!
		osync_group_set_configdir(group, abspath);
		g_free(abspath);
	} else {
		osync_group_set_configdir(group, path);
	}
	
	filename = g_strdup_printf ("%s/syncgroup.conf", group->configdir);
	
	xmlDocPtr doc;
	xmlNodePtr cur;
	
	if (!_osync_open_xml_file(&doc, &cur, filename, "syncgroup")) {
		osync_group_free(group);
		return NULL;
	}

	while (cur != NULL) {
		char *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (!str)
			continue;
		if (!xmlStrcmp(cur->name, (const xmlChar *)"name")) {
			group->name = g_strdup(str);
		}
		xmlFree(str);
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	g_free(filename);
	
	GDir *dir;
	GError *error = NULL;

	dir = g_dir_open(osync_group_get_configdir(group), 0, &error);
	if (error) {
		osync_debug("OSGRP", 3, "Unable to open group configdir %s", error->message);
		g_error_free (error);
		osync_group_free(group);
		return NULL;
	}
  
	if (dir) {
		const gchar *de = NULL;
		while ((de = g_dir_read_name(dir))) {
			filename = g_strdup_printf ("%s/%s", osync_group_get_configdir(group), de);
			if (!g_file_test(filename, G_FILE_TEST_IS_DIR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || g_pattern_match_simple(".*", de) || !strcmp("db", de)) {
				continue;
			}
			member = osync_member_new(group);
			osync_member_set_configdir(member, filename);
			if (!osync_member_load(member)) {
				osync_debug("OSGRP", 0, "Unable to load one of the members");
				return NULL;
			}
		}
	}
	char *dbdir = g_strdup_printf("%s/db", group->configdir);
	char *logfile = g_strdup_printf("%s/group.log", dbdir);
	FILE *log = fopen(logfile, "rw");
	group->dbenv = osync_db_setup(dbdir, log);
	g_free(dbdir);
	g_free(logfile);
	osync_env_append_group(env, group);
	return group;
}

void osync_group_add_member(OSyncGroup *group, OSyncMember *member)
{
	group->members = g_list_append(group->members, member);
}

OSyncMember *osync_group_get_nth_member(OSyncGroup *group, int nth)
{
	return (OSyncMember *)g_list_nth_data(group->members, nth);
}

int osync_group_num_members(OSyncGroup *group)
{
	return g_list_length(group->members);
}

char *osync_group_get_configdir(OSyncGroup *group)
{
	g_assert(group);
	return group->configdir;
}

void osync_group_set_configdir(OSyncGroup *group, char *path)
{
	osync_debug("OSGRP", 3, "Setting configdirectory of group %s to %s", group->name, path);
	group->configdir = g_strdup(path);
}

/*osync_bool osync_group_initialize(OSyncGroup *group)
{
	GList *element;
	for (element = group->members; element; element = element->next) {
		OSyncMember *member = element->data;
		if (!osync_member_initialize(member))
			return FALSE;
	}
	return TRUE;
}*/

OSyncGroup *osync_group_from_name(OSyncEnv *osinfo, char *name)
{
	OSyncGroup *group;
	int i;
	for (i = 0; i < osync_num_groups(osinfo); i++) {
		group = osync_get_nth_group(osinfo, i);
		if (g_ascii_strcasecmp(group->name, name) == 0) {
			return group;
		}
	}
	osync_debug("OSPLG", 0, "Couldnt find the group with the name %s", name);
	return NULL;
}

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

unsigned int osync_group_create_member_id(OSyncGroup *group)
{
	char *filename = NULL;
	int i = 0;
	do {
		i++;
		if (filename)
			g_free(filename);
		filename = g_strdup_printf("%s/%i", group->configdir, i);
	} while (g_file_test(filename, G_FILE_TEST_EXISTS));
	g_free(filename);
	return i;
}

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

OSyncFormatEnv *osync_group_get_format_env(OSyncGroup *group)
{
	g_assert(group);
	return group->conv_env;
}
