#include "evolution_sync.h"

static void evo_tasks_opened_cb(CalClient *client, CalClientOpenStatus status, gpointer data)
{
	evo_environment *env = data;
  
	if (status == CAL_CLIENT_OPEN_SUCCESS) {
		env->dbs_waiting--;
	} else {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to tasks");
		g_object_unref(G_OBJECT(client));
		env->tasks = NULL;
	}
	
	if (env->dbs_waiting == 0)
		osync_context_report_success(ctx);
}

osync_bool evo_tasks_open(evo_environment *env)
{
	if (!env->tasks_path) {
		env->dbs_waiting--;
		return FALSE;
	}
	
	env->tasks = cal_client_new();
	if (!env->tasks) {
		osync_debug("EVO-SYNC", 1, "Evolution plugin: Could not connect to Evolution!");
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Evolution plugin: Could not connect to Evolution!");
		return FALSE;
	}

	g_signal_connect (env->tasks, "cal_opened", G_CALLBACK(evo_tasks_opened_cb), env);

	g_signal_connect (env->tasks, "obj_removed", G_CALLBACK(evo_obj_removed_cb), env);
	g_signal_connect (env->tasks, "obj_updated", G_CALLBACK(evo_obj_updated_cb), env);

	osync_debug("EVO-SYNC", 1, "tasks loading `%s'...\n", env->tasks_path));

	if (!cal_client_open_tasks(env->tasks, env->tasks_path, FALSE);) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Evolution plugin: Could not open \"%s\"!", env->tasks_path);
		osync_debug("EVO-SYNC", 1, "Evolution plugin: Could not open \"%s\"!", env->tasks_path);
		return FALSE;
	}
	return TRUE;
}

List *evo_todo_get_changes(GList *changes, evolution_connection *conn) {
  GList *todo_changes, *l;
  if (!conn->todo_client)
    return(changes);
  todo_changes =  cal_client_get_changes(conn->todo_client, 
					CALOBJ_TYPE_TODO, 
					conn->changedbname);
  for (l=todo_changes; l; l = l->next) {
    CalClientChange *clientchange;
    CalComponent *comp;
    icalcomponent *icalcomp;
    
    clientchange = l->data;
    comp = clientchange->comp;
    if (comp) {
      char *uid = NULL;
      changed_object* change;
      icalproperty *dtend;

      change = g_malloc0(sizeof(changed_object));
      g_assert(change);
      cal_component_get_uid(comp, (const char**) &uid);
      if (uid) 
	change->uid = g_strdup(uid);
      icalcomp = cal_component_get_icalcomponent (comp);
      if (icalcomp) {
	change->comp = g_strdup_printf("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n%s"
				       "END:VCALENDAR\r\n",
				       cal_component_get_as_string(comp));
	if (clientchange->type & CAL_CLIENT_CHANGE_DELETED)
	  change->change_type = SYNC_OBJ_HARDDELETED;
	else if (clientchange->type & CAL_CLIENT_CHANGE_MODIFIED)
	  change->change_type = SYNC_OBJ_MODIFIED;
	else
	  change->change_type = SYNC_OBJ_ADDED;
	dtend = icalcomponent_get_first_property(icalcomp, 
						 ICAL_DTEND_PROPERTY);
	if (dtend)
	  change->removepriority = 
	    g_strdup(icaltime_as_ical_string(icalproperty_get_dtend(dtend)));
	change->object_type = object_type_from_component(comp);
	changes = evo_append_change(changes, change);
      }
    }
  }
  cal_client_change_list_free (todo_changes);
  return(changes);
}  

GList *evo_tasks_get_all (GList *changes, evolution_connection *conn) {
  GList *todo_changes, *known_changes, *l;
  if (!conn->todo_client)
    return(changes);
  
  todo_changes =  cal_client_get_uids(conn->todo_client, CALOBJ_TYPE_TODO);
  
  for (l=todo_changes; l; l = l->next) {
    CalComponent *comp;
    icalcomponent *icalcomp;
    char *uid;

    if (cal_client_get_object(conn->todo_client, l->data, &comp) ==
	CAL_CLIENT_GET_SUCCESS) {
      changed_object* change;

      change = g_malloc0(sizeof(changed_object));
      g_assert(change);
      cal_component_get_uid(comp, (const char**) &uid);
      if (uid) 
	change->uid = g_strdup(uid);
      icalcomp = cal_component_get_icalcomponent (comp);

      change->comp = g_strdup_printf("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n%s"
				     "END:VCALENDAR\r\n",
				     cal_component_get_as_string(comp));
      change->change_type = SYNC_OBJ_MODIFIED;
      change->object_type = object_type_from_component(comp);
      changes = evo_append_change(changes, change);
    }
  }
  known_changes =  cal_client_get_changes(conn->todo_client, 
					  CALOBJ_TYPE_TODO, 
					  conn->changedbname);
  cal_obj_uid_list_free(known_changes);
  cal_obj_uid_list_free(todo_changes);
  return(changes);
}

static osync_bool evo_tasks_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	char *uid = osync_change_get_uid(change);
	char *data = osync_change_get_data(change);
	icalcomponent *icomp;
	char *returnuid;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_cal_remove_object(env->tasks, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			icomp = icalcomponent_new_from_string(data);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			if (!e_cal_create_object(env->tasks, icomp, &returnuid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			osync_change_set_uid(change, returnuid);
			break;
		case CHANGE_MODIFIED:
			icomp = icalcomponent_new_from_string(data);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			if (!e_cal_modify_object(env->tasks, icomp, CALOBJ_MOD_ALL, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			break;
		default:
			printf("Error4\n");
	}
	
	osync_context_report_success(ctx);
	return FALSE;
}

void evo2_tasks_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo");
	osync_plugin_set_commit_objformat(info, "todo", "vtodo", evo2_tasks_modify);
}
