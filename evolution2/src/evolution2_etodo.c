/*
 * evolution2_sync - A plugin for the opensync framework
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
 
#include "evolution2_sync.h"

osync_bool evo2_todo_open(evo_environment *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	ESourceList *sources = NULL;
	ESource *source = NULL;
	GError *gerror = NULL;
	
	if (strcmp(env->tasks_path, "default")) {
	  	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, &gerror)) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get sources for tasks: %s", gerror ? gerror->message : "None");
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
		
		source = evo2_find_source(sources, env->tasks_path);
		if (!source) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find source for tasks");
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
		
		env->tasks = e_cal_new(source, E_CAL_SOURCE_TYPE_TODO);
		if(!env->tasks) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to create new tasks");
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			return FALSE;
		}
		
		if(!e_cal_open(env->tasks, FALSE, &gerror)) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to open tasks: %s", gerror ? gerror->message : "None");
			g_object_unref(env->tasks);
			env->tasks = NULL;
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
	} else {
		if (!e_cal_open_default (&env->tasks, E_CAL_SOURCE_TYPE_TODO, NULL, NULL, &gerror)) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to open default tasks: %s", gerror ? gerror->message : "None");
			env->calendar = NULL;
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
			g_clear_error(&gerror);
			return FALSE;
		}
	}
	
	if (!osync_anchor_compare(env->member, "todo", env->tasks_path))
		osync_member_set_slow_sync(env->member, "todo", TRUE);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void evo2_todo_get_changes(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	GList *changes = NULL;
	GList *l = NULL;
	char *data = NULL;
	const char *uid = NULL;
	int datasize = 0;
	GError *gerror = NULL;
	
	if (osync_member_get_slow_sync(env->member, "todo") == FALSE) {
		osync_debug("EVO2-SYNC", 4, "No slow_sync for todo");
		if (!e_cal_get_changes(env->tasks, env->change_id, &changes, &gerror)) {
			osync_context_send_log(ctx, "Unable to open changed tasks entries");
			osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open changed tasks entries: %s", __func__, gerror ? gerror->message : "None");
			g_clear_error(&gerror);
			return;
		}
		
		for (l = changes; l; l = l->next) {
			ECalChange *ecc = (ECalChange *)l->data;
			e_cal_component_get_uid(ecc->comp, &uid);
			e_cal_component_commit_sequence (ecc->comp);
			e_cal_component_strip_errors(ecc->comp);
			switch (ecc->type) {
				case E_CAL_CHANGE_ADDED:
					data = e_cal_get_component_as_string(env->calendar, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, "todo", "vtodo20", data, datasize, uid, CHANGE_ADDED);
					break;
				case E_CAL_CHANGE_MODIFIED:
					data = e_cal_get_component_as_string(env->calendar, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, "todo", "vtodo20", data, datasize, uid, CHANGE_MODIFIED);
					break;
				case E_CAL_CHANGE_DELETED:
					evo2_report_change(ctx, "todo", "vtodo20", NULL, 0, uid, CHANGE_DELETED);
					break;
			}
		}
	} else {
		osync_debug("EVO2-SYNC", 4, "slow_sync for todo");
        if (!e_cal_get_object_list_as_comp (env->tasks, "(contains? \"any\" \"\")", &changes, &gerror)) {
			osync_context_send_log(ctx, "Unable to get all todos");
			osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get all todos: %s", __func__, gerror ? gerror->message : "None");
			g_clear_error(&gerror);
			return;
        }
		for (l = changes; l; l = l->next) {
			ECalComponent *comp = E_CAL_COMPONENT (l->data);
			char *data = e_cal_get_component_as_string(env->tasks, e_cal_component_get_icalcomponent(comp));
			const char *uid = NULL;
			e_cal_component_get_uid(comp, &uid);
			int datasize = strlen(data) + 1;
			evo2_report_change(ctx, "todo", "vtodo20", data, datasize, uid, CHANGE_ADDED);
			g_object_unref (comp);
		}
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
}


static osync_bool evo2_todo_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	const char *uid = osync_change_get_uid(change);
	char *data = osync_change_get_data(change);
	icalcomponent *icomp = NULL;
	char *returnuid = NULL;
	GError *gerror = NULL;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_cal_remove_object(env->tasks, uid, &gerror)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete todo: %s", gerror ? gerror->message : "None");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to delete todo: %s", __func__, gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			icomp = icalcomponent_new_from_string(data);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert todo");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to convert todo", __func__);
				return FALSE;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VTODO_COMPONENT);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get todo");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get todo", __func__);
				return FALSE;
			}
			
			if (!e_cal_create_object(env->tasks, icomp, &returnuid, &gerror)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to create todo: %s", gerror ? gerror->message : "None");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to create todo: %s", __func__, gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				return FALSE;
			}
			osync_change_set_uid(change, returnuid);
			break;
		case CHANGE_MODIFIED:
			icomp = icalcomponent_new_from_string(data);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert todo2");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to convert todo2", __func__);
				return FALSE;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VTODO_COMPONENT);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get todo2");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get todo2", __func__);
				return FALSE;
			}
			
			icalcomponent_set_uid (icomp, uid);
			if (!e_cal_modify_object(env->tasks, icomp, CALOBJ_MOD_ALL, &gerror)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to modify todo: %s", gerror ? gerror->message : "None");
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to modify todo: %s", __func__, gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				return FALSE;
			}
			break;
		default:
			printf("Error\n");
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

void evo2_tasks_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
	osync_plugin_set_commit_objformat(info, "todo", "vtodo20", evo2_todo_modify);
	osync_plugin_set_access_objformat(info, "todo", "vtodo20", evo2_todo_modify);
}
