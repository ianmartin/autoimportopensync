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
 
#include <opensync.h>
#include "opensync_internals.h"

OSyncChange *osync_change_new(void)
{
	OSyncChange *change = g_malloc0(sizeof(OSyncChange));
	change->refcount = 1;
	return change;
}

void osync_change_free(OSyncChange *change)
{
	
	g_assert(change);
	//FIXME cleanly release the change!
	if (change->mapping)
		osync_mapping_remove_entry(change->mapping, change);
	if (change->member)
		osync_member_remove_changeentry(change->member, change);
	if (change->sourceobjtype)
		g_free(change->sourceobjtype);
	if (change->destobjtype)
		g_free(change->destobjtype);
	g_free(change);
}

void osync_change_reset(OSyncChange *change)
{
	g_free(change->hash);
	if (change->sourceobjtype)
		g_free(change->sourceobjtype);
	if (change->destobjtype)
		g_free(change->destobjtype);
	change->hash = NULL;
	//FIXME Release data
	change->data = NULL;
	change->size = 0;
	change->has_data = FALSE;
	change->changetype = CHANGE_UNKNOWN;
	change->sourceobjtype = NULL;
	change->destobjtype = NULL;
	change->is_detected = FALSE;
}

void osync_change_ref(OSyncChange *change)
{
	g_assert(change);
	change->refcount++;
}

void osync_change_decref(OSyncChange *change)
{
	g_assert(change);
	change->refcount--;
	if (change->refcount >= 0)
		osync_change_free(change);
}

OSyncMember *osync_change_get_member(OSyncChange *change)
{
	g_assert(change);
	return change->member;
}

void osync_change_set_member(OSyncChange *change, OSyncMember *member)
{
	g_assert(change);
	change->member = member;
}

OSyncObjType *osync_change_get_objtype(OSyncChange *change)
{
	g_assert(change);
	return change->objtype;
}

const char *osync_change_get_sourceobjtype(OSyncChange *change)
{
	g_assert(change);
	return change->sourceobjtype;
}


void osync_change_set_objtype(OSyncChange *change, OSyncObjType *type)
{
	g_assert(change);
	change->objtype = type;
}

void osync_change_set_objtype_string(OSyncChange *change, const char *name)
{
	OSyncObjType *objtype;
	g_assert(change);
	if ((objtype = osync_conv_find_objtype(osync_member_get_format_env(change->member), name))) {
		osync_change_set_objtype(change, objtype);
	}//FIXME: handle objtype not found
}

OSyncObjFormat *osync_change_get_objformat(OSyncChange *change)
{
	g_assert(change);
	return change->format;
}

void osync_change_set_objformat(OSyncChange *change, OSyncObjFormat *objformat)
{
	g_assert(change);
	change->format = objformat;
	change->objtype = objformat->objtype;

}

osync_bool osync_change_set_objformat_string(OSyncChange *change, const char *name)
{
	OSyncObjFormat *objformat;
	g_assert(change);
	if ((objformat = osync_conv_find_objformat(osync_member_get_format_env(change->member), name))) {
		osync_change_set_objformat(change, objformat);
	} else
		return FALSE;
	return TRUE;
}

OSyncChangeType osync_change_get_changetype(OSyncChange *change)
{
	if (!change) return CHANGE_UNKNOWN;
	return change->changetype;
}

void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type)
{
	g_assert(change);
	change->changetype = type;
}

void osync_change_set_hash(OSyncChange *change, const char *hash)
{
	g_assert(change);
	//FIXME release str
	change->hash = g_strdup(hash);
}

char *osync_change_get_hash(OSyncChange *change)
{
	g_assert(change);
	return change->hash;
}

void osync_change_set_uid(OSyncChange *change, const char *uid)
{
	g_assert(change);
	//FIXME release string
	change->uid = g_strdup(uid);
}

char *osync_change_get_uid(OSyncChange *change)
{
	g_assert(change);
	return change->uid;
}

void osync_change_set_data(OSyncChange *change, char *data, int size, osync_bool has_data)
{
	//FIXME do we need to keep track of refs for the data?
	change->data = data;
	change->size = size;
	change->has_data = has_data;
}

osync_bool osync_change_has_data(OSyncChange *change)
{
	g_assert(change);
	return change->has_data;
}

char *osync_change_get_data(OSyncChange *change)
{
	g_assert(change);
	return change->data;
}

int osync_change_get_datasize(OSyncChange *change)
{
	g_assert(change);
	return change->size;
}

char *osync_change_get_printable(OSyncChange *change)
{
	g_assert(change);
	if (!change->has_data)
		return NULL;
		
	OSyncObjFormat *format = change->format;
	g_assert(format);
	
	if (!format->print_func)
		return g_strndup(change->data, change->size);
		
	return format->print_func(change);
}

OSyncMapping *osync_change_get_mapping(OSyncChange *change)
{
	g_assert(change);
	return change->mapping;
}

void *osync_change_get_engine_data(OSyncChange *change)
{
	g_assert(change);
	return change->engine_data;
}

void osync_change_set_engine_data(OSyncChange *change, void *engine_data)
{
	g_assert(change);
	change->engine_data = engine_data;
}

long long int osync_change_get_id(OSyncChange *change)
{
	g_assert(change);
	return change->id;
}

void osync_change_update(OSyncChange *source, OSyncChange *target)
{
	//FIXME free stuff
	g_assert(source);
	g_assert(target);
	if (!target->uid)
		target->uid = g_strdup(source->uid);
	target->hash = g_strdup(source->hash);
	target->data = g_malloc0(source->size);
	memcpy(target->data, source->data, source->size);
	target->size = source->size;
	target->has_data = source->has_data;
	target->changetype = source->changetype;
	if (source->format)
		target->format = source->format;
	if (source->objtype) {
		target->objtype = source->objtype;
		target->sourceobjtype = g_strdup(source->objtype->name);
	}
	target->is_detected = source->is_detected;
}
