/*
 * libopensync - A synchronization framework
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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-data.h"
#include "opensync_data_internals.h"

#include "opensync-format.h"
#include "format/opensync_objformat_internals.h" 

#include "opensync_change_internals.h"
#include "opensync_change_private.h"


OSyncChange *osync_change_new(OSyncError **error)
{
	OSyncChange *change = osync_try_malloc0(sizeof(OSyncChange), error);
	if (!change)
		return NULL;
		
	change->ref_count = 1;
	
	return change;
}

OSyncChange *osync_change_ref(OSyncChange *change)
{
	osync_assert(change);
	
	g_atomic_int_inc(&(change->ref_count));

	return change;
}

void osync_change_unref(OSyncChange *change)
{
	osync_assert(change);
	
	if (g_atomic_int_dec_and_test(&(change->ref_count))) {
		if (change->data)
			osync_data_unref(change->data);
			
		if (change->uid)
			g_free(change->uid);
		
		if (change->hash)
			g_free(change->hash);
		
		g_free(change);
	}
}

OSyncChangeType osync_change_get_changetype(OSyncChange *change)
{
	osync_assert(change);
	return change->changetype;
}

void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type)
{
	osync_assert(change);
	change->changetype = type;
}

OSyncObjFormat *osync_change_get_objformat(OSyncChange *change)
{
	osync_assert(change);
	if (!change->data)
		return NULL;
	return osync_data_get_objformat(change->data);
}

const char *osync_change_get_objtype(OSyncChange *change)
{
	osync_assert(change);
	if (!change->data)
		return NULL;
	return osync_data_get_objtype(change->data);
}

void osync_change_set_objtype(OSyncChange *change, const char *objtype)
{
	osync_assert(change);
	if (!change->data)
		return;
	osync_data_set_objtype(change->data, objtype);
}

void osync_change_set_hash(OSyncChange *change, const char *hash)
{
	osync_assert(change);
	if (change->hash)
		g_free(change->hash);
	change->hash = g_strdup(hash);
}

const char *osync_change_get_hash(OSyncChange *change)
{
	osync_assert(change);
	return change->hash;
}

void osync_change_set_uid(OSyncChange *change, const char *uid)
{
	osync_assert(change);
	osync_assert(uid);

	if (change->uid)
		g_free(change->uid);
	change->uid = g_strdup(uid);
}

const char *osync_change_get_uid(OSyncChange *change)
{
	osync_assert(change);
	return change->uid;
}

void osync_change_set_data(OSyncChange *change, OSyncData *data)
{
	osync_assert(change);
	if (change->data)
		osync_data_unref(change->data);
	change->data = data;
	osync_data_ref(data);
}

OSyncData *osync_change_get_data(OSyncChange *change)
{
	osync_assert(change);
	return change->data;
}

OSyncChange *osync_change_clone(OSyncChange *source, OSyncError **error)
{
	OSyncChange *change = NULL;

	osync_assert(source);

	change = osync_change_new(error);
	if (!change)
		return NULL;

	if (source->data)
		osync_change_set_data(change, source->data);

	if (source->uid)
		change->uid = g_strdup(source->uid);
	
	if (source->hash)
		change->hash = g_strdup(source->hash);

	if (source->changetype)
		change->changetype = osync_change_get_changetype(source);

	return change;
}

OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, leftchange, rightchange);
	osync_assert(rightchange);
	osync_assert(leftchange);

	if (rightchange->changetype == leftchange->changetype) {
		OSyncConvCmpResult ret = osync_data_compare(leftchange->data, rightchange->data);
		osync_trace(TRACE_EXIT, "%s: Compare data: %i", __func__, ret);
		return ret;
	} else {
		osync_trace(TRACE_EXIT, "%s: MISMATCH: Change types do not match", __func__);
		return OSYNC_CONV_DATA_MISMATCH;
	}
}

osync_bool osync_change_duplicate(OSyncChange *change, osync_bool *dirty, OSyncError **error)
{
        OSyncData *data = NULL;
	char *newuid = NULL;
	char *output = NULL;
	unsigned int outsize = 0;
	char *input = NULL;
	unsigned int insize = 0;

	osync_assert(change);
	data = change->data;
	osync_assert(data);
	osync_data_get_data(data, &input, &insize);
	
	if (!osync_objformat_duplicate(osync_data_get_objformat(data), osync_change_get_uid(change), input, insize, &newuid, &output, &outsize, dirty, error))
		return FALSE;
	
	if (newuid) {
		osync_change_set_uid(change, newuid);
		g_free(newuid);
	}
	
	if (output) {
		osync_objformat_destroy(osync_data_get_objformat(data), input, insize);
		osync_data_set_data(data, output, outsize);
	}
	
	return TRUE;
}

