#include <opensync.h>
#include "opensync_internals.h"

OSyncEnv *osync_env_new(void)
{
	OSyncEnv *os_env = g_malloc0(sizeof(OSyncEnv));

	OSyncUserInfo *user = osync_user_new();
	os_env->configdir = g_strdup(osync_user_get_confdir(user));
	os_env->plugindir = g_strdup(OPENSYNC_PLUGINDIR"/plugins");

	osync_debug("os_env", 3, "Generating new os_env:");
	osync_debug("os_env", 3, "Configdirectory: %s", os_env->configdir);
	return os_env;
}

void osync_env_free(OSyncEnv *env)
{
	g_assert(env);
	//FIXME Free user info
	g_free(env->configdir);
	g_free(env->plugindir);
	g_free(env);
}

void osync_env_append_group(OSyncEnv *os_env, OSyncGroup *group)
{
	os_env->groups = g_list_append(os_env->groups, group);
}

osync_bool osync_env_initialize(OSyncEnv *os_env)
{
	g_assert(os_env != NULL);
	
	//Load all available shared libraries (plugins)
	if (!g_file_test(os_env->plugindir, G_FILE_TEST_EXISTS)) {
		osync_debug("OSGRP", 3, "%s exists, but is no dir", os_env->plugindir);
		return FALSE;
	}
	
	return osync_plugin_load_dir(os_env, os_env->plugindir);
}

void osync_env_finalize(OSyncEnv *os_env)
{
	g_assert(os_env);
	printf("finalizing\n");
	GList *plugins = g_list_copy(os_env->plugins);
	GList *p;
	for (p = plugins; p; p = p->next) {
		OSyncPlugin *plugin = p->data;
		osync_plugin_unload(plugin);
		osync_plugin_free(plugin);
	}
	g_list_free(plugins);
}

osync_bool osync_env_load_groups_dir(OSyncEnv *osyncinfo)
{
	GDir *dir;
	GError *error = NULL;
	osync_debug("OSGRP", 3, "Trying to open main confdir %s to load groups", osync_env_get_configdir(osyncinfo));
	
	if (!g_file_test(osync_env_get_configdir(osyncinfo), G_FILE_TEST_EXISTS)) {
		mkdir(osync_env_get_configdir(osyncinfo), 0777);
	} else {
		if (!g_file_test(osync_env_get_configdir(osyncinfo), G_FILE_TEST_IS_DIR)) {
			osync_debug("OSGRP", 3, "%s exists, but is now dir", osync_env_get_configdir(osyncinfo));
			return FALSE;
		}
	}
	
	dir = g_dir_open(osync_env_get_configdir(osyncinfo), 0, &error);
	if (error) {
		osync_debug("OSGRP", 3, "Unable to open main configdir %s: %s", osync_env_get_configdir(osyncinfo), error->message);
		g_error_free (error);
		return FALSE;
	}
  
	if (dir) {
		const gchar *de = NULL;
		while ((de = g_dir_read_name(dir))) {
			
			char *filename = NULL;
			filename = g_strdup_printf ("%s/%s", osync_env_get_configdir(osyncinfo), de);
			
			if (!g_file_test(filename, G_FILE_TEST_IS_DIR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK)) {
				continue;
			}
			
			/* Try to open the confdir*/
			osync_group_load(osyncinfo, filename);
			g_free(filename);
		}
	}
	return TRUE;
}

int osync_env_num_plugins (OSyncEnv *os_env)
{
	return g_list_length(os_env->plugins);
}

OSyncPlugin *osync_env_get_nth_plugin(OSyncEnv *os_env, int nth)
{
	return (OSyncPlugin *)g_list_nth_data(os_env->plugins, nth);
}

int osync_num_groups (OSyncEnv *os_env)
{
	return g_list_length(os_env->groups);
}

OSyncGroup *osync_get_nth_group(OSyncEnv *os_env, int nth)
{
	OSyncGroup *group = (OSyncGroup *)g_list_nth_data(os_env->groups, nth);
	group = (OSyncGroup *)g_list_nth_data(os_env->groups, nth);
	return group;
}

void osync_env_remove_group(OSyncEnv *os_env, OSyncGroup *group)
{
	os_env->groups = g_list_remove(os_env->groups, group);
}

char *osync_env_get_configdir(OSyncEnv *os_env)
{
	if (!os_env) return NULL;
	return os_env->configdir;
}

void osync_env_set_configdir(OSyncEnv *os_env, char *path)
{
	if (!os_env) return;
	os_env->configdir = g_strdup(path);
}

osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, char *path, char *topentry)
{
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		osync_debug("OSXML", 1, "File %s does not exist", path);
		return FALSE;
	}
	
	*doc = xmlParseFile(path);

	if (!*doc) {
		osync_debug("OSXML", 1, "Could not open: %s", path);
		return FALSE;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		osync_debug("OSXML", 0, "%s seems to be empty", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		osync_debug("OSXML", 0, "%s seems not to be a valid configfile.\n", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	*cur = (*cur)->xmlChildrenNode;
	return TRUE;
}

osync_bool osync_file_write(char *filename, char *data, int size)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	if (!filename)
		return FALSE;
	GIOChannel *chan = g_io_channel_new_file(filename, "w", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to open file %s for writing: %s", filename, error->message);
		return FALSE;
	}
	gsize writen;
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, data, size, &writen, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to read contents of file %s: %s", filename, error->message);
	} else {
		g_io_channel_flush(chan, NULL);
		ret = TRUE;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}

osync_bool osync_file_read(char *filename, char **data, int *size)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	if (!filename)
		return FALSE;
	GIOChannel *chan = g_io_channel_new_file(filename, "r", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to read file %s: %s", filename, error->message);
		return FALSE;
	}
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_read_to_end(chan, data, size, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to read contents of file %s: %s", filename, error->message);
	} else {
		ret = TRUE;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}
