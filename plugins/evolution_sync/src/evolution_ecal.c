#include "evolution_sync.h"

static void evo_calendar_opened_cb(CalClient *client, CalClientOpenStatus status, gpointer data)
{
	evo_environment *env = data;
  
	if (status == CAL_CLIENT_OPEN_SUCCESS) {
		env->dbs_waiting--;
	} else {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to calendar");
		g_object_unref(G_OBJECT(client));
		env->calendar = NULL;
	}
	
	if (env->dbs_waiting == 0)
		osync_context_report_success(ctx);
}

osync_bool evo_calendar_open(evo_environment *env)
{
	if (!env->calendar_path) {
		env->dbs_waiting--;
		return FALSE;
	}
	
	env->calendar = cal_client_new();
	if (!env->calendar) {
		osync_debug("EVO-SYNC", 1, "Evolution plugin: Could not connect to Evolution!");
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Evolution plugin: Could not connect to Evolution!");
		return FALSE;
	}

	g_signal_connect (env->calendar, "cal_opened", G_CALLBACK(evo_calendar_opened_cb), env);

	g_signal_connect (env->calendar, "obj_removed", G_CALLBACK(evo_obj_removed_cb), env);
	g_signal_connect (env->calendar, "obj_updated", G_CALLBACK(evo_obj_updated_cb), env);

	osync_debug("EVO-SYNC", 1, "Calendar loading `%s'...\n", env->calendar_path));

	if (!cal_client_open_calendar(env->calendar, env->calendar_path, FALSE);) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Evolution plugin: Could not open \"%s\"!", env->calendar_path);
		osync_debug("EVO-SYNC", 1, "Evolution plugin: Could not open \"%s\"!", env->calendar_path);
		return FALSE;
	}
	return TRUE;
}

gboolean evo_cal_modify_one(evolution_connection *conn,
			    changed_object *obj,
			    char **uidret) {
  // UID exists, this is modify
  CalComponent *calcomp = NULL;
  CalClientResult res = 0;
  int modified = 0;

  if (obj->comp) {
    icalcomponent *icalcomp;
    char *tmp = evo_replace(obj->comp, "\r\n", "\n");
    char *start, *end;
    start = strstr(tmp, "BEGIN:VEVENT");
    end = strstr(tmp, "END:VEVENT");
    if (end) {
      end+=strlen("END:VEVENT"+1);
      end[0] = 0;
    }
    if (!start || !end) {
      start = strstr(tmp, "BEGIN:VTODO");
      end = strstr(tmp, "END:VTODO");
      if (end) {
	end+=strlen("END:VTODO"+1);
	end[0] = 0;
      }
    }
    if (!start)
      start = tmp;
    icalcomp = icalcomponent_new_from_string(start);
    g_free(tmp);
    calcomp = cal_component_new ();
    g_assert(calcomp);
    cal_component_set_icalcomponent (calcomp,icalcomp);
    if (obj->uid) {
      cal_component_set_uid (calcomp, obj->uid);
    } else {
      char* uid = cal_component_gen_uid();
      cal_component_set_uid (calcomp, uid);
      if (uidret)
	*uidret = g_strdup(uid);
    }
    if (obj->object_type == SYNC_OBJECT_TYPE_CALENDAR) 
      res = cal_client_update_object (conn->cal_client, calcomp);
    if (obj->object_type == SYNC_OBJECT_TYPE_TODO) 
      res = cal_client_update_object (conn->todo_client, calcomp);
    if (res != CAL_CLIENT_RESULT_SUCCESS && obj->uid) {
      // Modify failed, try adding
      char* uid = cal_component_gen_uid();
      cal_component_set_uid (calcomp, uid);
      if (uidret)
	*uidret = g_strdup(uid);
      if (obj->object_type == SYNC_OBJECT_TYPE_CALENDAR) 
	res = cal_client_update_object (conn->cal_client, calcomp);
      if (obj->object_type == SYNC_OBJECT_TYPE_TODO) 
	res = cal_client_update_object (conn->todo_client, calcomp);
    }
    modified = (res == CAL_CLIENT_RESULT_SUCCESS);
    icalcomponent_free(icalcomp);
  } else {
    if (obj->uid) {
      if (obj->object_type == SYNC_OBJECT_TYPE_CALENDAR) {
	if (cal_client_remove_object(conn->cal_client, obj->uid) ==
	    CAL_CLIENT_RESULT_SUCCESS)
	  modified = 1;
      }
      if (obj->object_type == SYNC_OBJECT_TYPE_TODO) {
	if (cal_client_remove_object(conn->todo_client, obj->uid) ==
	    CAL_CLIENT_RESULT_SUCCESS)
	  modified = 1;
      }
    }
  }
  return(modified);
}

static osync_bool evo_calendar_modify(OSyncContext *ctx, OSyncChange *change)
{
	gboolean evo_cal_modify(gpointer data) {
  evolution_connection *conn = data;
  GList *objects = conn->modify_objects, *results = conn->modify_results;
  while (objects && results) {
    changed_object *object = objects->data;
    syncobj_modify_result *result = results->data;
    if (object->object_type == SYNC_OBJECT_TYPE_CALENDAR ||
	object->object_type == SYNC_OBJECT_TYPE_TODO) {
      if (evo_cal_modify_one(conn, object, &(result->returnuid)))
	result->result = SYNC_MSG_REQDONE;
    }
    objects = objects->next;
    results = results->next;
  }
  if (conn->callback)
    (conn->callback)(NULL, conn); // We have done the modifications
  return(FALSE);
}
}

void evo_cal_modify_done_cb(gpointer data, evolution_connection *conn) {
  conn->callback = evo_addr_modify_done_cb; // Continue and modify addr
  g_idle_add(evo_addr_modify, conn);
}

GList *evo_calendar_get_all (GList *changes, evolution_connection *conn) {
  GList *cal_changes, *known_changes, *l;
  if (!conn->cal_client)
    return(changes);

  cal_changes =  cal_client_get_uids(conn->cal_client, CALOBJ_TYPE_EVENT);
  for (l=cal_changes; l; l = l->next) {
    CalComponent *comp;
    icalcomponent *icalcomp;
    char *uid;

    if (cal_client_get_object(conn->cal_client, l->data, &comp) ==
	CAL_CLIENT_GET_SUCCESS) {
      changed_object* change;
      icalproperty *dtend;

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
      dtend = icalcomponent_get_first_property(icalcomp, 
					       ICAL_DTEND_PROPERTY);
      if (dtend)
	change->removepriority = 
	  g_strdup(icaltime_as_ical_string(icalproperty_get_dtend(dtend)));
      changes = evo_append_change(changes, change);
    }
  }
  known_changes =  cal_client_get_changes(conn->cal_client, 
					  CALOBJ_TYPE_EVENT,
					  conn->changedbname);
  cal_obj_uid_list_free(known_changes);
  cal_obj_uid_list_free(cal_changes);
  return(changes);
}

GList *evo_calendar_get_changes(GList *changes, evolution_connection *conn) {
  GList *cal_changes, *l;
  if (!conn->cal_client)
    return(changes);
  cal_changes =  cal_client_get_changes(conn->cal_client, 
					CALOBJ_TYPE_EVENT, 
					conn->changedbname);
  for (l=cal_changes; l; l = l->next) {
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
  cal_client_change_list_free (cal_changes);
  return(changes);
}

void evo_calendar_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "calendar");
	osync_plugin_accept_objformat(info, "calendar", "vcalendar");
	osync_plugin_set_commit_objformat(info, "calendar", "vcalendar", evo_calendar_modify);
}
