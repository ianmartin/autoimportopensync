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

OSyncFlag *osync_flag_new(OSyncFlag *parent)
{
	OSyncFlag *flag = g_malloc0(sizeof(OSyncFlag));
	flag->is_set = FALSE;
	if (parent) {
		flag->comb_flag = parent;
		parent->num_not_set++;
		osync_flag_calculate_comb(parent);
	}
	return flag;
}

void osync_flag_free(OSyncFlag *flag)
{
	g_free(flag);
}

OSyncFlag *osync_comb_flag_new(osync_bool any, osync_bool default_val)
{
	OSyncFlag *flag = osync_flag_new(NULL);
	flag->is_comb = TRUE;
	flag->is_any = any;
	flag->default_val = default_val;
	flag->is_set = default_val;
	return flag;
}

void osync_flag_attach(OSyncFlag *flag, OSyncFlag *target)
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

osync_bool osync_flag_is_attached(OSyncFlag *flag)
{
	if (flag->comb_flag)
		return TRUE;
	return FALSE;
}

void osync_flag_detach(OSyncFlag *flag)
{
	OSyncFlag *target = flag->comb_flag;
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

void osync_flag_set_pos_trigger(OSyncFlag *flag, OSyncFlagTriggerFunc func, void *data1, void *data2)
{
	flag->pos_trigger_func = func;
	flag->pos_user_data1 = data1;
	flag->pos_user_data2 = data2;
}

void osync_flag_set_neg_trigger(OSyncFlag *flag, OSyncFlagTriggerFunc func, void *data1, void *data2)
{
	flag->neg_trigger_func = func;
	flag->neg_user_data1 = data1;
	flag->neg_user_data2 = data2;
}

void osync_flag_calculate_comb(OSyncFlag *flag)
{
	if (!flag->is_comb)
		return;
	
	if (!flag->num_not_set && !flag->num_set) {
		if (flag->default_val)
			osync_flag_set(flag);
		else
			osync_flag_unset(flag);
		return;
	}
	
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

osync_bool osync_flag_is_set(OSyncFlag *flag)
{
	if (flag->is_set == TRUE && flag->is_changing == FALSE)
		return TRUE;
	return FALSE;
}

osync_bool osync_flag_is_not_set(OSyncFlag *flag)
{
	if (flag->is_set == FALSE && flag->is_changing == FALSE)
		return TRUE;
	return FALSE;
}

void osync_comb_flag_update(OSyncFlag *combflag, OSyncFlag *flag, osync_bool prev_state)
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

void osync_flag_changing(OSyncFlag *flag)
{
	flag->is_changing = TRUE;
}

void osync_flag_cancel(OSyncFlag *flag)
{
	flag->is_changing = FALSE;
}

void osync_flag_unset(OSyncFlag *flag)
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

void osync_flag_set(OSyncFlag *flag)
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

void osync_flag_set_state(OSyncFlag *flag, osync_bool state)
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

osync_bool osync_flag_get_state(OSyncFlag *flag)
{
	return flag->is_set;
}

void osync_flag_calc_trigger(OSyncFlag *flag, osync_bool oldstate)
{
	if (flag->is_set != oldstate) {
		if (flag->is_set == TRUE) {
			if (flag->pos_trigger_func) {
				flag->pos_trigger_func(flag->pos_user_data1, flag->pos_user_data2);
			}
		} else {
			if (flag->neg_trigger_func) {
				flag->neg_trigger_func(flag->neg_user_data1, flag->neg_user_data2);
			}
		}
	}
}
