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

void evo2_calendar_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent");
	osync_plugin_set_commit_objformat(info, "event", "vevent", evo2_calendar_modify);
}
