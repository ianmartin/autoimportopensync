#include "evolution_sync.h"

osync_bool evo2_tasks_open(evo_environment *env)
{
	ESourceList *sources;
	ESource *source;
	if (!env->tasks_path)
		return FALSE;
	
  	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, NULL)) {
  		osync_debug("EVO2-SYNC", 1, "Unable to get sources for tasks");
		return FALSE;
	}
	
	source = evo2_find_source(sources, env->tasks_path);
	if (!source) {
		osync_debug("EVO2-SYNC", 1, "Unable to find source for tasks");
		return FALSE;
	}
	
	env->tasks = e_cal_new(source, E_CAL_SOURCE_TYPE_TODO);
	if(!env->tasks) {
		osync_debug("EVO2-SYNC", 1, "failed new tasks");
		return FALSE;
	}
	
	if(!e_cal_open(env->tasks, FALSE, NULL)) {
		osync_debug("EVO2-SYNC", 1, "failed to open tasks");
		return FALSE;
	}
	return TRUE;
}

static osync_bool evo2_tasks_modify(OSyncContext *ctx, OSyncChange *change)
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
