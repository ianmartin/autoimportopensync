#include "evolution_sync.h"

#define EVODIR "~/evolution/local"

GList* read_filelist(GList *list, const char *dirname, const char *targetname)
{         	
	GDir *dir = NULL;
	char *filename = NULL;
	
	dir = g_dir_open(dirname, 0, NULL);
	if (!dir)
		return NULL;

	const gchar *de = NULL;
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", dirname, de);
		if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
			list = read_filelist(list, filename, targetname);
			continue;
		} else if (!strcmp(de, targetname)) {
			//Found a new directory containing our target
			evo_location *loc = g_malloc0(sizeof(evo_location));
			loc->uri = g_strdup(filename);
			loc->name = g_path_get_basename(dirname);
			list = g_list_append(list, loc);
			continue;
		}
		g_free(filename);
	}
	g_dir_close(dir);
	return list;
}

GList *evo_list_calendars(evo_environment *env, void *data, OSyncError **error)
{
	return read_filelist(NULL, EVODIR, "calendar.ics");
}

GList *evo_list_tasks(evo_environment *env, void *data, OSyncError **error)
{
	return read_filelist(NULL, EVODIR, "tasks.ics");
}

GList *evo_list_addressbooks(evo_environment *env, void *data, OSyncError **error)
{
	return read_filelist(NULL, EVODIR, "addressbook.db");
}

void evo_obj_updated_cb(CalClient *client, const char *uid, gpointer data)
{
	evo_environment *env = data;
	osync_member_request_synchronization(env->member);
}

void evo_obj_removed_cb(CalClient *client, const char *uid, gpointer data)
{
	evo_environment *env = data;
	osync_member_request_synchronization(env->member);
}

static void *evo_initialize(OSyncMember *member, OSyncError **error)
{
	osync_debug("EVO-SYNC", 4, "start: %s", __func__);
	int fd[2];

	g_type_init();
	pipe(fd);
	if (!fork()) {
		dup2(fd[1],1);
		execlp("evolution", "evolution", "--version", NULL);
		close(fd[1]);
		exit(0);
	}

	/*if (read(fd[0],version,256) > 0) {
		int majver, minver = 0, micver = 0;
		if (sscanf(version,"Gnome evolution %d.%d.%d",&majver, &minver, &micver) >= 2) {
			osync_debug("EVO-SYNC", 4, "Detected evolution %d.%d.%d.\n", majver, minver, micver));
			if (!(majver > 1 || (majver==1 && minver >= 4))) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Evolution plugin: This plugin requires Evolution 1.4 or greater.");
				return NULL;
			}
		}
	}*/
	
	char *configdata = NULL;
	int configsize = 0;
	
	evo_environment *env = g_malloc0(sizeof(evo_environment));
	if (!env)
		goto error_ret;
	if (!osync_member_get_config(member, &configdata, &configsize, error)) 
		goto error_free;
	if (!evo2_parse_settings(env, configdata, configsize)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for evo2 plugin");
		goto error_free_data;
	}
	env->member = member;
	
	g_free(configdata);
	return (void *)env;
	
	error_free_data:
		g_free(configdata);
	error_free:
		g_free(env);
	error_ret:
		return NULL;
}

static void evo_connect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	env->dbs_to_load = 3;
	
	if (osync_member_objtype_enabled(env->member, "contact") &&  env->adressbook_path && strlen(env->adressbook_path)) {
		if (evo_addrbook_open(env, ctx)) {
			if (!osync_anchor_compare(env->member, "contact", env->adressbook_path))
				osync_member_set_slow_sync(env->member, "contact", TRUE);
		} else {
			osync_context_send_log(ctx, "Unable to open addressbook");
		}
	}

	if (osync_member_objtype_enabled(env->member, "calendar") &&  env->calendar_path && strlen(env->calendar_path)) {
		if (evo_calendar_open(env)) {
			if (!osync_anchor_compare(env->member, "calendar", env->calendar_path))
				osync_member_set_slow_sync(env->member, "calendar", TRUE);
		} else {
			osync_context_send_log(ctx, "Unable to open calendar");
		}
	}

	if (osync_member_objtype_enabled(env->member, "todo") && env->tasks_path && strlen(env->tasks_path)) {
		if (evo_tasks_open(env)) {
			if (!osync_anchor_compare(env->member, "tasks", env->tasks_path))
				osync_member_set_slow_sync(env->member, "tasks", TRUE);
		} else {
			osync_context_send_log(ctx, "Unable to open tasks");
		}
	}

	srand(time(NULL));
}

static void evo_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("EVO-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	evo_addrbook_get_changes(ctx);

	if (env->addressbook) {
		if (osync_member_get_slow_sync(env->member, "contact"))
			evo_addrbook_get_changes(ctx);
		else
			evo_addrbook_get_all(ctx);
	}
	
	if (env->calendar) {
		if (osync_member_get_slow_sync(env->member, "calendar"))
			evo_cal_get_changes(ctx);
		else
			evo_cal_get_all(ctx);
	}
	
	if (env->tasks) {
		if (osync_member_get_slow_sync(env->member, "tasks"))
			evo_tasks_get_changes(ctx);
		else
			evo_tasks_get_all(ctx);
	}
	
	osync_context_report_success(ctx);
}

static void evo_sync_done(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	osync_context_report_success(ctx);
}

static void evo_disconnect(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);

	if (env->calendar)
		g_object_unref(G_OBJECT(env->calendar));
	env->calendar = NULL;

	if (env->tasks)
		g_object_unref(G_OBJECT(env->tasks));
	env->tasks = NULL;
	
	if (env->addressbook) {
		if (env->ebookview)
			g_object_unref(G_OBJECT(env->ebookview));
		e_book_unload_uri(env->addressbook); 
		g_object_unref(G_OBJECT(env->addressbook));
	}
	env->addressbook = NULL;
	
	osync_context_report_success(ctx);
}

static void evo2_finalize(void *data)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)data;
	
	g_free(env);
}

void get_info(OSyncPluginInfo *info) {
	info->name = "evo-sync";
	info->version = 1;
	info->is_threadsafe = FALSE;
	
	info->functions.initialize = evo_initialize;
	info->functions.connect = evo_connect;
	info->functions.get_changeinfo = evo_get_changeinfo;
	info->functions.sync_done = evo_sync_done;
	info->functions.disconnect = evo_disconnect;
	info->functions.finalize = evo_finalize;

	evo_addrbook_setup(info);
	evo_calendar_setup(info);
	evo_tasks_setup(info);
}
