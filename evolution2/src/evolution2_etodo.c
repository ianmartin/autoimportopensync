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

static void evo2_etodo_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        OSyncError *error = NULL;
        GError *gerror = NULL;
        ESourceList *sources = NULL;
        ESource *source = NULL;

        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        if (!env->tasks_path) {
                osync_error_set(&error, OSYNC_ERROR_GENERIC, "no tasks path set");
                goto error;
        }

        if (strcmp(env->tasks_path, "default")) {
                if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get sources for tasks: %s", gerror ? gerror->message : "None");
                        goto error;
                }
                
                if (!(source = evo2_find_source(sources, env->tasks_path))) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", env->tasks_path);
                        goto error;
                }
 
                if (!(env->tasks = e_cal_new(source, E_CAL_SOURCE_TYPE_TODO))) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to create new tasks");
			goto error;
		}

		if(!e_cal_open(env->tasks, FALSE, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open tasks: %s", gerror ? gerror->message : "None");
                        goto error_free_todo;
                }
        } else {
                osync_trace(TRACE_INTERNAL, "Opening default tasks\n");
                if (!e_cal_open_default(&env->tasks, E_CAL_SOURCE_TYPE_TODO, NULL, NULL, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open default tasks: %s", gerror ? gerror->message : "None");
                        goto error_free_todo;
                }
        }

        char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
        if (!osync_anchor_compare(anchorpath, "todo", env->tasks_path))
                osync_objtype_sink_set_slowsync(sink, TRUE);
        g_free(anchorpath);


        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error_free_todo:
                g_object_unref(env->tasks);
                env->tasks = NULL;
error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

static void evo2_etodo_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        if (env->tasks) {
                g_object_unref(env->tasks);
                env->tasks = NULL;
        }

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
}

static void evo2_etodo_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
        osync_anchor_update(anchorpath, "todo", env->tasks_path);
        g_free(anchorpath);


        GList *changes = NULL;
        e_cal_get_changes(env->tasks, env->change_id, &changes, NULL);
 
        osync_context_report_success(ctx);
        
        osync_trace(TRACE_EXIT, "%s", __func__);
}

void evo2_etodo_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype)
{
        OSyncError *error = NULL;

        OSyncChange *change = osync_change_new(&error);
        if (!change) {
                osync_context_report_osyncwarning(ctx, error);
                osync_error_unref(&error);
                return;
        }

        osync_change_set_uid(change, uid);
        osync_change_set_changetype(change, changetype);

        OSyncData *odata = osync_data_new(data, size, format, &error);
        if (!odata) {
                osync_change_unref(change);
                osync_context_report_osyncwarning(ctx, error);
                osync_error_unref(&error);
                return;
        }

        osync_change_set_data(change, odata);
        osync_data_unref(odata);

        osync_context_report_change(ctx, change);

        osync_change_unref(change);
}


static void evo2_etodo_get_changes(void *indata, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, indata, info, ctx);
        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        OSyncEvoEnv *env = (OSyncEvoEnv *)indata;
        OSyncError *error = NULL;

        GList *changes = NULL;
        ECalChange *ecc = NULL;
        GList *l = NULL;
        char *data = NULL;
        const char *uid = NULL;
        int datasize = 0;
        GError *gerror = NULL;

        if (osync_objtype_sink_get_slowsync(sink) == FALSE) {
                osync_trace(TRACE_INTERNAL, "No slow_sync for tasks");
                if (!e_cal_get_changes(env->tasks, env->change_id, &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open changed tasks entries: %s", gerror ? gerror->message : "None");
                        goto error;
                }
                osync_trace(TRACE_INTERNAL, "Found %i changes for change-ID %s", g_list_length(changes), env->change_id);

                for (l = changes; l; l = l->next) {
                        ecc = (ECalChange *)l->data;
			e_cal_component_get_uid(ecc->comp, &uid);
			e_cal_component_commit_sequence (ecc->comp);
			e_cal_component_strip_errors(ecc->comp);
			switch (ecc->type) {
				case E_CAL_CHANGE_ADDED:
					data = e_cal_get_component_as_string(env->tasks, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_etodo_report_change(ctx, env->tasks_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
					break;
				case E_CAL_CHANGE_MODIFIED:
					data = e_cal_get_component_as_string(env->tasks, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_etodo_report_change(ctx, env->tasks_format, data, datasize, uid, OSYNC_CHANGE_TYPE_MODIFIED);
					break;
				case E_CAL_CHANGE_DELETED:
					evo2_etodo_report_change(ctx, env->tasks_format, NULL, 0, uid, OSYNC_CHANGE_TYPE_DELETED);
					break;
			}
                }
        } else {
                osync_trace(TRACE_INTERNAL, "slow_sync for tasks");
	        if (!e_cal_get_object_list_as_comp (env->tasks, "(contains? \"any\" \"\")", &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get changes from tasks: %s", gerror ? gerror->message : "None");
                        goto error;
        	}
		for (l = changes; l; l = l->next) {
			ECalComponent *comp = E_CAL_COMPONENT (l->data);
			char *data = e_cal_get_component_as_string(env->tasks, e_cal_component_get_icalcomponent(comp));
			const char *uid = NULL;
			e_cal_component_get_uid(comp, &uid);
			int datasize = strlen(data) + 1;
			evo2_etodo_report_change(ctx, env->tasks_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
			g_object_unref (comp);
		}
	}

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

static void evo2_etodo_modify(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        const char *uid = osync_change_get_uid(change);
	icalcomponent *icomp = NULL;
	char *returnuid = NULL;
        GError *gerror = NULL;
        OSyncError *error = NULL;
        OSyncData *odata = NULL;
        char *plain = NULL;

        switch (osync_change_get_changetype(change)) {
                case OSYNC_CHANGE_TYPE_DELETED:
                        if (!e_cal_remove_object(env->tasks, uid, &gerror)) {
                                osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete todo: %s", gerror ? gerror->message : "None");
                                goto error;
                        }
                        break;
                case OSYNC_CHANGE_TYPE_ADDED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);
			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert todo");
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VTODO_COMPONENT);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get todo");
				goto error;
			}
			
			if (!e_cal_create_object(env->tasks, icomp, &returnuid, &gerror)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create todo: %s", gerror ? gerror->message : "None");
				goto error;
			}
			osync_change_set_uid(change, returnuid);
                        break;
                case OSYNC_CHANGE_TYPE_MODIFIED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);

			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to convert todo2");
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VTODO_COMPONENT);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get todo2");
				goto error;
			}
			
			icalcomponent_set_uid (icomp, uid);
			if (!e_cal_modify_object(env->tasks, icomp, CALOBJ_MOD_ALL, &gerror)) {
				osync_trace(TRACE_INTERNAL, "unable to mod todo: %s", gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				if (!e_cal_create_object(env->tasks, icomp, &returnuid, &gerror)) {
					osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create todo: %s", gerror ? gerror->message : "None");
					goto error;
				}
			}
                        break;
                default:
                        printf("Error\n");
        }

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

osync_bool evo2_etodo_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
        OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
        env->tasks_format = osync_format_env_find_objformat(formatenv, "vtodo20");

        if (!env->tasks_format) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vtodo20 object format. vformat plugin installed?");
                return FALSE;
        }


        env->tasks_sink = osync_objtype_sink_new("todo", error);
        if (!env->tasks_sink){
		printf("tasks sink failed to initialize\n");
                return FALSE;
	}

        osync_objtype_sink_add_objformat(env->tasks_sink, "vtodo20");

        /* All sinks have the same functions of course */
        OSyncObjTypeSinkFunctions functions;
        memset(&functions, 0, sizeof(functions));
        functions.connect = evo2_etodo_connect;
        functions.disconnect = evo2_etodo_disconnect;
        functions.get_changes = evo2_etodo_get_changes;
        functions.commit = evo2_etodo_modify;
        functions.sync_done = evo2_etodo_sync_done;

        /* We pass the OSyncFileDir object to the sink, so we dont have to look it up
         * again once the functions are called */
        osync_objtype_sink_set_functions(env->tasks_sink, functions, NULL);
        osync_plugin_info_add_objtype(info, env->tasks_sink);
	return TRUE;
}

