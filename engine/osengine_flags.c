/*
 * libosengine - A synchronization engine for the opensync framework
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
 
#include "engine.h"
#include "engine_internals.h"

MSyncFlag *osync_flag_new(MSyncFlag *parent)
{
	MSyncFlag *flag = g_malloc0(sizeof(MSyncFlag));
	flag->is_set = FALSE;
	if (parent) {
		flag->comb_flag = parent;
		parent->num_not_set++;
		osync_flag_calculate_comb(parent);
	}
	return flag;
}

void osync_flag_free(MSyncFlag *flag)
{
	g_free(flag);
}

MSyncFlag *osync_comb_flag_new(osync_bool any)
{
	MSyncFlag *flag = osync_flag_new(NULL);
	flag->is_comb = TRUE;
	flag->is_any = any;
	return flag;
}

void osync_flag_attach(MSyncFlag *flag, MSyncFlag *target)
{
	if (flag->comb_flag)
		return;
	g_assert(target->is_comb);
	flag->comb_flag = target;
	if (flag->is_set) {
		target->num_set++;
	} else {
		target->num_not_set++;
	}
	osync_flag_calculate_comb(target);
}

void osync_flag_detach(MSyncFlag *flag)
{
	MSyncFlag *target = flag->comb_flag;
	if (!target)
		return;
	if (flag->is_set) {
		target->num_set--;
	} else {
		target->num_not_set--;
	}
	flag->comb_flag = NULL;
	osync_flag_calculate_comb(target);
}

/*static void trigger_send_mapping_changed(OSyncMapping *mapping)
{
	printf("+++++++++++ trigger send mapping %p\n", mapping);
	MSyncMappingFlags *flags = osync_mapping_get_engine_data(mapping);
	printf("+++++++++++ trigger send mapping %p\n", flags);
	printf("+++++++++++ trigger send mapping2 %p\n", flags->engine);
	if (!flags)
		return;
	//send_mapping_changed(flags->engine, mapping);
	printf("done trigger send mapping\n");
}*/

MSyncMappingFlags *osync_mapping_create_flags(OSyncMapping *mapping)
{
	MSyncMappingFlags *flag = g_malloc0(sizeof(MSyncMappingFlags));
	OSyncGroup *group = osync_mapping_get_group(mapping);
	OSyncEngine *engine = osync_group_get_data(group);
	osync_mapping_set_engine_data(mapping, flag);
	flag->engine = engine;
	flag->fl_solved = osync_flag_new(NULL);
	flag->cmb_has_data = osync_comb_flag_new(FALSE);
	//osync_flag_set_pos_trigger(flag->cmb_has_data, (MSyncFlagTriggerFunc)trigger_send_mapping_changed, mapping);
	flag->cmb_has_info = osync_comb_flag_new(FALSE);
	flag->cmb_synced = osync_comb_flag_new(FALSE);
	flag->cmb_deleted = osync_comb_flag_new(FALSE);
	osync_flag_attach(flag->cmb_synced, engine->cmb_synced);
	osync_flag_set(flag->cmb_synced);
	return flag;
}

void osync_mapping_free_flags(OSyncMapping *mapping)
{
	MSyncMappingFlags *flags = osync_mapping_get_flags(mapping);
	osync_flag_free(flags->fl_solved);
	osync_flag_free(flags->cmb_has_data);
	osync_flag_free(flags->cmb_has_info);
	osync_flag_free(flags->cmb_synced);
	osync_flag_free(flags->cmb_deleted);
	g_free(flags);
	osync_mapping_set_engine_data(mapping, NULL);
}

MSyncChangeFlags *osync_change_create_flags(OSyncChange *change)
{
	MSyncChangeFlags *flag = g_malloc0(sizeof(MSyncChangeFlags));
	flag->fl_has_data = osync_flag_new(NULL);
	flag->fl_dirty = osync_flag_new(NULL);
	flag->fl_mapped = osync_flag_new(NULL);
	flag->fl_has_info = osync_flag_new(NULL);
	flag->fl_synced = osync_flag_new(NULL);
	flag->fl_deleted = osync_flag_new(NULL);
	osync_flag_set(flag->fl_synced);
	osync_flag_set_pos_trigger(flag->fl_dirty, (MSyncFlagTriggerFunc)send_change_changed, change);
	return flag;
}

void osync_change_free_flags(OSyncChange *change)
{
	MSyncChangeFlags *flags = osync_change_get_flags(change);
	osync_flag_free(flags->fl_has_data);
	osync_flag_free(flags->fl_dirty);
	osync_flag_free(flags->fl_mapped);
	osync_flag_free(flags->fl_has_info);
	osync_flag_free(flags->fl_synced);
	osync_flag_free(flags->fl_deleted);
	g_free(flags);
	osync_change_set_engine_data(change, NULL);
}

void osync_change_flags_attach(OSyncChange *change, OSyncMapping *mapping)
{
	MSyncChangeFlags *chflags = osync_change_get_flags(change);
	MSyncMappingFlags *mapflags = osync_mapping_get_flags(mapping);
	osync_flag_attach(chflags->fl_has_data, mapflags->cmb_has_data);
	osync_flag_attach(chflags->fl_has_info, mapflags->cmb_has_info);
	osync_flag_attach(chflags->fl_synced, mapflags->cmb_synced);
	osync_flag_attach(chflags->fl_deleted, mapflags->cmb_deleted);
}

void osync_change_flags_detach(OSyncChange *change)
{
	MSyncChangeFlags *chflags = osync_change_get_flags(change);
	osync_flag_detach(chflags->fl_has_data);
	osync_flag_detach(chflags->fl_has_info);
	osync_flag_detach(chflags->fl_synced);
	osync_flag_detach(chflags->fl_deleted);
}

MSyncChangeFlags *osync_change_get_flags(OSyncChange *change)
{
	MSyncChangeFlags *flags = osync_change_get_engine_data(change);
	if (!flags) {
		flags = osync_change_create_flags(change);
		osync_change_set_engine_data(change, flags);
		//Set default values
	}
	return flags;
}

MSyncMappingFlags *osync_mapping_get_flags(OSyncMapping *mapping)
{
	g_assert(mapping);
	MSyncMappingFlags *flags = osync_mapping_get_engine_data(mapping);
	if (!flags)
		flags = osync_mapping_create_flags(mapping);
	return flags;
}

void osync_flag_set_pos_trigger(MSyncFlag *flag, MSyncFlagTriggerFunc func, void *data)
{
	flag->pos_trigger_func = func;
	flag->pos_user_data = data;
}

void osync_flag_set_neg_trigger(MSyncFlag *flag, MSyncFlagTriggerFunc func, void *data)
{
	flag->neg_trigger_func = func;
	flag->neg_user_data = data;
}

void osync_flag_calculate_comb(MSyncFlag *flag)
{
	if (!flag->is_comb)
		return;
	
	if (!flag->is_any) {
		if (!flag->num_not_set && flag->num_set) {
			osync_flag_set(flag);
		} else {
			osync_flag_unset(flag);
		}
	} else {
		if (flag->num_set) {
			osync_flag_set(flag);
		} else {
			osync_flag_unset(flag);
		}
	}
}

osync_bool osync_flag_is_set(MSyncFlag *flag)
{
	if (flag->is_set == TRUE && flag->is_changing == FALSE)
		return TRUE;
	return FALSE;
}

osync_bool osync_flag_is_not_set(MSyncFlag *flag)
{
	if (flag->is_set == FALSE && flag->is_changing == FALSE)
		return TRUE;
	return FALSE;
}

void osync_comb_flag_update(MSyncFlag *combflag, MSyncFlag *flag, osync_bool prev_state)
{
	if (prev_state == flag->is_set)
		return;
	if (flag->is_set) {
		combflag->num_not_set--;
		combflag->num_set++;
	} else {
		combflag->num_not_set++;
		combflag->num_set--;
	}
}

void osync_flag_changing(MSyncFlag *flag)
{
	flag->is_changing = TRUE;
}

void osync_flag_cancel(MSyncFlag *flag)
{
	flag->is_changing = FALSE;
}

void osync_flag_unset(MSyncFlag *flag)
{
	osync_bool oldstate = flag->is_set;
	flag->is_set = FALSE;
	flag->is_changing = FALSE;
	osync_flag_calc_trigger(flag, oldstate);
	if (flag->comb_flag) {
		osync_comb_flag_update(flag->comb_flag, flag, oldstate);
		osync_flag_calculate_comb(flag->comb_flag);
	}
}

void osync_flag_set(MSyncFlag *flag)
{
	osync_bool oldstate = flag->is_set;
	flag->is_set = TRUE;
	flag->is_changing = FALSE;
	osync_flag_calc_trigger(flag, oldstate);
	if (flag->comb_flag) {
		osync_comb_flag_update(flag->comb_flag, flag, oldstate);
		osync_flag_calculate_comb(flag->comb_flag);
	}
}

void osync_flag_set_state(MSyncFlag *flag, osync_bool state)
{
	osync_bool oldstate = flag->is_set;
	flag->is_set = state;
	flag->is_changing = FALSE;
	if (flag->comb_flag) {
		osync_comb_flag_update(flag->comb_flag, flag, oldstate);
	}
	if (flag->is_comb) {
		//flag->num_not_set = 0;
		//flag->num_set = 0;
	}
}

osync_bool osync_flag_get_state(MSyncFlag *flag)
{
	return flag->is_set;
}

void osync_flag_calc_trigger(MSyncFlag *flag, osync_bool oldstate)
{
	if (flag->is_set != oldstate) {
		if (flag->is_set == TRUE) {
			if (flag->pos_trigger_func) {
				flag->pos_trigger_func(flag->pos_user_data);
			}
		} else {
			if (flag->neg_trigger_func) {
				flag->neg_trigger_func(flag->neg_user_data);
			}
		}
	}
}
