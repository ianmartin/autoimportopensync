#include "evolution_sync.h"

osync_bool evo2_calendar_open(evo_environment *env)
{
	ESourceList *sources;
	ESource *source;
	if (!env->calendar_path)
		return FALSE;
	
  	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_EVENT, NULL)) {
  		osync_debug("EVO2-SYNC", 1, "Unable to get sources for cal");
		return FALSE;
	}
	
	source = evo2_find_source(sources, env->calendar_path);
	if (!source) {
		osync_debug("EVO2-SYNC", 1, "Unable to find source for cal");
		return FALSE;
	}
	
	env->calendar = e_cal_new(source, E_CAL_SOURCE_TYPE_EVENT);
	if(!env->calendar) {
		osync_debug("EVO2-SYNC", 1, "failed new calendar");
		return FALSE;
	}
	
	if(!e_cal_open(env->calendar, FALSE, NULL)) {
		osync_debug("EVO2-SYNC", 1, "failed to open calendar");
		return FALSE;
	}
	return TRUE;
}

static osync_bool evo2_calendar_modify(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);
	evo_environment *env = (evo_environment *)osync_context_get_plugin_data(ctx);
	
	char *uid = osync_change_get_uid(change);
	char *data = osync_change_get_data(change);
	icalcomponent *icomp;
	char *returnuid;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!e_cal_remove_object(env->calendar, uid, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete contact");
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			icomp = icalcomponent_new_from_string(data);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			if (!e_cal_create_object(env->calendar, icomp, &returnuid, NULL)) {
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
			if (!e_cal_modify_object(env->calendar, icomp, CALOBJ_MOD_ALL, NULL)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert cal");
				return FALSE;
			}
			break;
		default:
			printf("Error\n");
	}
	
	osync_context_report_success(ctx);
	return TRUE;
}

OSyncFormatFunctions vcalendar_functions = {
	.commit_change = evo2_calendar_modify,
	.access = evo2_calendar_modify,
};

void evo2_calendar_setup(OSyncPluginInfo *info)
{
	osync_plugin_register_accepted_objtype(info, "calendar");
	osync_plugin_register_accepted_objformat(info, "calendar", "vcalendar", &vcalendar_functions);
}
