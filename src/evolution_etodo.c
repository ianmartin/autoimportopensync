#include "evolution_sync.h"

gboolean evo2_tasks_open(evo_environment *env)
{
	ESourceList *sources;
	ESource *source;
	if (!env->tasks_path)
		return FALSE;
	
  	if (!e_cal_get_sources(&sources, E_CAL_SOURCE_TYPE_TODO, NULL)) {
  		evo_debug(env, 1, "Unable to get sources for tasks");
		return FALSE;
	}
	
	source = find_source(sources, env->tasks_path);
	if (!source) {
		evo_debug(env, 1, "Unable to find source for tasks");
		return FALSE;
	}
	
	env->tasks = e_cal_new(source, E_CAL_SOURCE_TYPE_TODO);
	if(!env->tasks) {
		evo_debug(env, 1, "failed new tasks");
		return FALSE;
	}
	
	if(!e_cal_open(env->tasks, FALSE, NULL)) {
		evo_debug(env, 1, "failed to open tasks");
		return FALSE;
	}
}

gboolean evo2_tasks_modify(evo_environment *env, OSyncChange *change)
{
	char *uid = osync_change_get_uid(change);
	char *data = osync_change_get_data(change);
	ECalComponent *cal;
	icalcomponent *icomp;
	char *returnuid;
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (e_cal_remove_object(env->tasks, uid, NULL))
				return TRUE;
			break;
		case CHANGE_ADDED:
			icomp = icalcomponent_new_from_string(data);
			if(!icomp)
				return FALSE;
			if (e_cal_create_object(env->tasks, icomp, &returnuid, NULL)) {
				osync_change_set_uid(change, returnuid);
				return TRUE;
			}
			break;
		case CHANGE_MODIFIED:
			icomp = icalcomponent_new_from_string(data);
			if(!icomp)
				return FALSE;
			if (e_cal_modify_object(env->tasks, icomp, CALOBJ_MOD_ALL, NULL))
				return TRUE;
			break;
		default:
			printf("Error4\n");
	}
	return FALSE;
}
