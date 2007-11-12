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

static void evo2_memo_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        OSyncError *error = NULL;
        GError *gerror = NULL;
        ESourceList *sources = NULL;
        ESource *source = NULL;

        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        if (!env->memos_path) {
                osync_error_set(&error, OSYNC_ERROR_GENERIC, "no memos path set");
                goto error;
        }

        if (strcmp(env->memos_path, "default")) {
                if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_JOURNAL, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get sources for memos: %s", gerror ? gerror->message : "None");
                        goto error;
                }
                
                if (!(source = evo2_find_source(sources, env->memos_path))) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", env->memos_path);
                        goto error;
                }
 
                if (!(env->memos = e_cal_new(source, E_CAL_SOURCE_TYPE_JOURNAL))) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to create new memos");
			goto error;
		}

		if(!e_cal_open(env->memos, FALSE, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open memos: %s", gerror ? gerror->message : "None");
                        goto error_free_todo;
                }
        } else {
                osync_trace(TRACE_INTERNAL, "Opening default memos\n");
                if (!e_cal_open_default(&env->memos, E_CAL_SOURCE_TYPE_JOURNAL, NULL, NULL, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open default memos: %s", gerror ? gerror->message : "None");
                        goto error_free_todo;
                }
        }

        char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
        if (!osync_anchor_compare(anchorpath, "note", env->memos_path))
                osync_objtype_sink_set_slowsync(sink, TRUE);
        g_free(anchorpath);


        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error_free_todo:
                g_object_unref(env->memos);
                env->memos = NULL;
error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

static void evo2_memo_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        if (env->memos) {
                g_object_unref(env->memos);
                env->memos = NULL;
        }

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
}

static void evo2_memo_sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
        OSyncEvoEnv *env = (OSyncEvoEnv *)data;

        char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
        osync_anchor_update(anchorpath, "note", env->memos_path);
        g_free(anchorpath);


        GList *changes = NULL;
        e_cal_get_changes(env->memos, env->change_id, &changes, NULL);
 
        osync_context_report_success(ctx);
        
        osync_trace(TRACE_EXIT, "%s", __func__);
}

void evo2_memo_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype)
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


static void evo2_memo_get_changes(void *indata, OSyncPluginInfo *info, OSyncContext *ctx)
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
                osync_trace(TRACE_INTERNAL, "No slow_sync for memos");
                if (!e_cal_get_changes(env->memos, env->change_id, &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open changed memos entries: %s", gerror ? gerror->message : "None");
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
					data = e_cal_get_component_as_string(env->memos, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_memo_report_change(ctx, env->memos_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
					break;
				case E_CAL_CHANGE_MODIFIED:
					data = e_cal_get_component_as_string(env->memos, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_memo_report_change(ctx, env->memos_format, data, datasize, uid, OSYNC_CHANGE_TYPE_MODIFIED);
					break;
				case E_CAL_CHANGE_DELETED:
					evo2_memo_report_change(ctx, env->memos_format, NULL, 0, uid, OSYNC_CHANGE_TYPE_DELETED);
					break;
			}
                }
        } else {
                osync_trace(TRACE_INTERNAL, "slow_sync for memos");
	        if (!e_cal_get_object_list_as_comp (env->memos, "(contains? \"any\" \"\")", &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get changes from memos: %s", gerror ? gerror->message : "None");
                        goto error;
        	}
		for (l = changes; l; l = l->next) {
			ECalComponent *comp = E_CAL_COMPONENT (l->data);
			char *data = e_cal_get_component_as_string(env->memos, e_cal_component_get_icalcomponent(comp));
			const char *uid = NULL;
			e_cal_component_get_uid(comp, &uid);
			int datasize = strlen(data) + 1;
			evo2_memo_report_change(ctx, env->memos_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
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

static void evo2_memo_modify(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
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
                        if (!e_cal_remove_object(env->memos, uid, &gerror)) {
                                osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete journal: %s", gerror ? gerror->message : "None");
                                goto error;
                        }
                        break;
                case OSYNC_CHANGE_TYPE_ADDED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);
			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert journal");
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VJOURNAL_COMPONENT);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get journal");
				goto error;
			}
			
			if (!e_cal_create_object(env->memos, icomp, &returnuid, &gerror)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create journal: %s", gerror ? gerror->message : "None");
				goto error;
			}
			osync_change_set_uid(change, returnuid);
                        break;
                case OSYNC_CHANGE_TYPE_MODIFIED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);

			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to convert journal2");
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, ICAL_VJOURNAL_COMPONENT);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get journal2");
				goto error;
			}
			
			icalcomponent_set_uid (icomp, uid);
			if (!e_cal_modify_object(env->memos, icomp, CALOBJ_MOD_ALL, &gerror)) {
				osync_trace(TRACE_INTERNAL, "unable to mod journal: %s", gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				if (!e_cal_create_object(env->memos, icomp, &returnuid, &gerror)) {
					osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create journal: %s", gerror ? gerror->message : "None");
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

osync_bool evo2_memo_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
        OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
        env->memos_format = osync_format_env_find_objformat(formatenv, "vjournal");

        if (!env->memos_format) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find vjournal object format. vformat plugin installed?");
                return FALSE;
        }


        env->memos_sink = osync_objtype_sink_new("note", error);
        if (!env->memos_sink){
		printf("memos sink failed to initialize\n");
                return FALSE;
	}

        osync_objtype_sink_add_objformat(env->memos_sink, "vjournal");

        /* All sinks have the same functions of course */
        OSyncObjTypeSinkFunctions functions;
        memset(&functions, 0, sizeof(functions));
        functions.connect = evo2_memo_connect;
        functions.disconnect = evo2_memo_disconnect;
        functions.get_changes = evo2_memo_get_changes;
        functions.commit = evo2_memo_modify;
        functions.sync_done = evo2_memo_sync_done;

        /* We pass the OSyncFileDir object to the sink, so we dont have to look it up
         * again once the functions are called */
        osync_objtype_sink_set_functions(env->memos_sink, functions, NULL);
        osync_plugin_info_add_objtype(info, env->memos_sink);
	return TRUE;
}

