#include "evolution_sync.h"

GList* read_filelist(GList *list, char *dirname, char *textname,
                     char *filename, int depth) {
  DIR *dir;
  if (depth < 0)
    return(list);
  if ((dir = opendir(dirname))) {
    struct dirent *de;
    while ((de = readdir(dir))) {
      if (de->d_type != DT_DIR) {
        if (!strcmp(de->d_name, filename)) {
          evo_sync_file *ef = g_malloc0(sizeof(evo_sync_file));
          ef->name = g_strdup(textname);
          ef->path = g_strdup_printf("%s/%s", dirname, de->d_name);
          list = g_list_append(list, ef);
        }
      }
      if (de->d_type != DT_REG) {
        char *newdir;
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
          newdir = g_strdup_printf("%s/%s", dirname, de->d_name);
          list = read_filelist(list, newdir, de->d_name, filename, depth-1);
          g_free(newdir);
        }
      }
    }
    closedir(dir);
  }
  return(list);
}

GList *evo2_list_calendars(evo_environment *env, void *data, OSyncError **error)
{
	 dirname = g_strdup_printf ("%s/evolution/local", 
			       g_get_home_dir());
    calendarlist = read_filelist(NULL, dirname, NULL, "calendar.ics", 5);
    todolist = read_filelist(NULL, dirname, NULL, "tasks.ics", 5);
    addressbooklist = read_filelist(NULL, dirname, NULL, "addressbook.db", 5);
    g_free(dirname);

    calendarmenu = gtk_menu_new ();
    for (n = 0; n < g_list_length(calendarlist); n++) {
      evo_sync_file *ef = g_list_nth_data(calendarlist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (calendar_selected),
			  (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (calendarmenu), menuitem);
      if ((evoconn->calendarpath && !strcmp(evoconn->calendarpath, ef->path))
	  || (!evoconn->calendarpath && n == 0)) {
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
	gtk_menu_set_active (GTK_MENU(calendarmenu), n);
      }
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
							    "calendarmenu")), 
			      calendarmenu);
    gtk_widget_show_all(GTK_WIDGET(calendarmenu));

    todomenu = gtk_menu_new ();
    for (n = 0; n < g_list_length(todolist); n++) {
      evo_sync_file *ef = g_list_nth_data(todolist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (todo_selected),
			  (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (todomenu), menuitem);
      if ((evoconn->todopath && !strcmp(evoconn->todopath, ef->path))
	  || (!evoconn->todopath && n == 0)) {
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
	gtk_menu_set_active (GTK_MENU(todomenu), n);
      }
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
							    "todomenu")), 
			      todomenu);
    gtk_widget_show_all(GTK_WIDGET(todomenu));

    addressbookmenu = gtk_menu_new ();
    for (n = 0; n < g_list_length(addressbooklist); n++) {
      evo_sync_file *ef = g_list_nth_data(addressbooklist, n);
      menuitem = gtk_menu_item_new_with_label (ef->name);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (addressbook_selected),
			  (gpointer) ef->path);
      gtk_menu_append (GTK_MENU (addressbookmenu), menuitem);
      if ((evoconn->addressbookpath && !strcmp(evoconn->addressbookpath, ef->path))
	  || (!evoconn->addressbookpath && n == 0)) {
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
	gtk_menu_set_active (GTK_MENU(addressbookmenu), n);
      }
    }
    gtk_option_menu_set_menu (GTK_OPTION_MENU(lookup_widget(evowindow,
							    "addressbookmenu")), 
			      addressbookmenu);
    gtk_widget_show_all(GTK_WIDGET(addressbookmenu));

    gtk_widget_show (evowindow);
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_EVENT, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list calendars. Unable to get sources");
		return NULL;
	}

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

GList *evo2_list_tasks(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list tasks. Unable to get sources");
		return NULL;
	}

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
}

GList *evo2_list_addressbooks(evo_environment *env, void *data, OSyncError **error)
{
	GList *paths = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	
	if (!e_book_get_addressbooks(&sources, NULL)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to list addressbooks. Unable to get sources");
		return NULL;
    }

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			evo2_location *path = g_malloc0(sizeof(evo2_location));
			path->uri = g_strdup(e_source_get_uri(source));
			path->name = g_strdup(e_source_peek_name(source));
			paths = g_list_append(paths, path);
		}
	}
	return paths;
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
	char version[256] = "";

	g_type_init();
	pipe(fd);
	if (!fork()) {
		dup2(fd[1],1);
		execlp("evolution", "evolution", "--version", NULL);
		close(fd[1]);
		exit(0);
	}

	if (read(fd[0],version,256) > 0) {
		int majver, minver = 0, micver = 0;
		if (sscanf(version,"Gnome evolution %d.%d.%d",&majver, &minver, &micver) >= 2) {
			osync_debug("EVO-SYNC", 4, "Detected evolution %d.%d.%d.\n", majver, minver, micver));
			if (!(majver > 1 || (majver==1 && minver >= 4))) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Evolution plugin: This plugin requires Evolution 1.4 or greater.");
				return NULL;
			}
		}
	}
	
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
	OSyncGroup *group = osync_member_get_group(member);
	
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
	env->dbs_waiting = 3;
	
	if (osync_member_objtype_enabled(env->member, "contact") &&  env->adressbook_path && strlen(env->adressbook_path)) {
		if (evo_addrbook_open(env)) {
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
			open_any = TRUE;
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

	if (env->addressbook) {
		if (osync_member_get_slow_sync(env->member, "contact")) {
			if (!evo_addr_get_changes(ctx) {
				osync_context_send_log(ctx, "Unable to open changed addressbook entries");
			}
		} else {
			if (!evo_addr_get_all(ctx) {
				osync_context_send_log(ctx, "Unable to open all addressbook entries");
			}
		}
	}
	
	if (env->calendar) {
		if (osync_member_get_slow_sync(env->member, "calendar")) {
			if (!evo_cal_get_changes(ctx) {
				osync_context_send_log(ctx, "Unable to open changed calendar entries");
			}
		} else {
			if (!evo_cal_get_all(ctx) {
				osync_context_send_log(ctx, "Unable to open all calendar entries");
			}
		}
	}
	
	if (env->tasks) {
		if (osync_member_get_slow_sync(env->member, "todo")) {
			if (!evo_todo_get_changes(ctx)) {
				osync_context_send_log(ctx, "Unable to open changed tasks");
			}
		} else {
			if (!evo_todo_get_all(ctx)) {
				osync_context_send_log(ctx, "Unable to open all tasks");
			}
		}
	}
	
	osync_context_report_success(ctx);
}

static void evo2_sync_done(OSyncContext *ctx)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	osync_context_report_success(ctx);
}

static void evo2_disconnect(OSyncContext *ctx)
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
