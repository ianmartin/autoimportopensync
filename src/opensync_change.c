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
	g_free(change);
}

void osync_change_reset(OSyncChange *change)
{
	g_free(change->hash);
	change->hash = NULL;
	//FIXME Release data
	change->data = NULL;
	change->size = 0;
	change->has_data = FALSE;
	change->changetype = CHANGE_UNKNOWN;
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

void osync_change_set_objtype(OSyncChange *change, OSyncObjType *type)
{
	g_assert(change);
	change->objtype = type;
}

OSyncObjFormat *osync_change_get_objformat(OSyncChange *change)
{
	g_assert(change);
	if (!change->objformats)
		return NULL;
	return g_list_last(change->objformats)->data;
}

void osync_change_append_objformat(OSyncChange *change, OSyncObjFormat *objformat)
{
	g_assert(change);
	change->objformats = g_list_append(change->objformats, objformat);
}

void osync_change_prepend_objformat(OSyncChange *change, OSyncObjFormat *objformat)
{
	g_assert(change);
	change->objformats = g_list_prepend(change->objformats, objformat);
}

void osync_change_set_objformat(OSyncChange *change, OSyncObjFormat *objformat)
{
	g_assert(change);
	//FIXME Free the prev list
	change->objformats = g_list_append(NULL, objformat);
}

void osync_change_set_objformat_string(OSyncChange *change, const char *name)
{
	OSyncObjFormat *objformat;
	g_assert(change);
	if ((objformat = osync_conv_find_objformat(osync_member_get_format_env(change->member), name))) {
		osync_change_set_objformat(change, objformat);
	}//FIXME: handle objformat not found
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

unsigned long osync_change_get_id(OSyncChange *change)
{
	g_assert(change);
	return change->id;
}

void osync_change_marshal(OSyncChange *change, DBT *dbt)
{
	memset(dbt, 0, sizeof(*dbt));
	unsigned long mapid = 0;
	int objlen = 0;
	int formatlen = 0;
	OSyncObjFormat *format = NULL;
	OSyncMember *member = osync_change_get_member(change);
	OSyncMapping *mapping = osync_change_get_mapping(change);
	if (mapping)
		mapid = osync_mapping_get_id(mapping);
		
	if (g_list_last(change->objformats))
		format = g_list_last(change->objformats)->data;
		
	unsigned int id = osync_member_get_id(member);

	int size = 0;
	size += sizeof(unsigned long); //Mapping id
	size += sizeof(unsigned int); //member id
	size += sizeof(int); //uidlen
	size += sizeof(int); //objlen
	size += sizeof(int); //formatlen
	
	int uidlen = strlen(change->uid) + 1;
	size += uidlen; //uid

	if (change->objtype) {
		objlen = strlen(change->objtype->name) + 1;
		size += objlen; // objtype
	}
	
	if (format) {
		formatlen = strlen(format->name) + 1;
		size += formatlen; // formattype
	}
	
	void *ptr = g_malloc0(size);
	void *p = ptr;
	
	memcpy(p, &mapid, sizeof(unsigned long));
	p += sizeof(unsigned long);
	
	memcpy(p, &id, sizeof(unsigned int));
	p += sizeof(unsigned int);
	
	memcpy(p, &uidlen, sizeof(int));
	p += sizeof(int);
	
	memcpy(p, &objlen, sizeof(int));
	p += sizeof(int);
	
	memcpy(p, &formatlen, sizeof(int));
	p += sizeof(int);
	
	memcpy(p, change->uid, uidlen);
	p += uidlen;
	
	if (change->objtype) {
		memcpy(p, change->objtype->name, objlen);
		p += objlen;
	}
	
	if (format) {
		memcpy(p, format->name, formatlen);
		p += formatlen;
	}
	
	dbt->data = ptr;
	dbt->size = size;
}

void osync_change_unmarshal(OSyncMappingTable *table, OSyncChange *change, const void *data)
{
	int uidlen = 0;
	int objlen = 0;
	int formatlen = 0;
	unsigned long mapid = 0;
	char *objtype = NULL;
	char *objformat = NULL;
	unsigned int id = 0;
	memset(change, 0, sizeof(OSyncChange));
	
	memcpy(&mapid, data, sizeof(unsigned long));
	data += sizeof(unsigned long);
	
	memcpy(&id, data, sizeof(unsigned int));
	data += sizeof(unsigned int);
	
	memcpy(&uidlen, data, sizeof(int));
	data += sizeof(int);
	
	memcpy(&objlen, data, sizeof(int));
	data += sizeof(int);
	
	memcpy(&formatlen, data, sizeof(int));
	data += sizeof(int);
	change->uid = g_malloc0(uidlen);
	memcpy(change->uid, data, uidlen);
	data += uidlen;
	
	if (objlen) {
		objtype = g_malloc0(objlen);
		memcpy(objtype, data, objlen);
		data += objlen;
	}

	if (formatlen) {
		objformat = g_malloc0(formatlen);
		memcpy(objformat, data, formatlen);
		data += formatlen;
	}
	
	if (table && table->group) {
		change->member = osync_member_from_id(table->group, id);
		if (objtype)
			change->objtype = osync_conv_find_objtype(table->group->conv_env, objtype);
		//FIXME: handle object type not found
		if (objformat)
			osync_change_set_objformat(change, osync_conv_find_objformat(table->group->conv_env, objformat));
		//FIXME: handle objformat not found
	}
}

void osync_change_update(OSyncChange *source, OSyncChange *target)
{
	//FIXME free stuff
	g_assert(source);
	g_assert(target);
	target->uid = g_strdup(source->uid);
	target->hash = g_strdup(source->hash);
	target->data = g_malloc0(source->size);
	memcpy(target->data, source->data, source->size);
	target->size = source->size;
	target->has_data = source->has_data;
	target->changetype = source->changetype;
	//FIXME?
	if (source->objformats)
		target->objformats = g_list_append(NULL, g_list_last(source->objformats)->data);
	if (source->objtype)
		target->objtype = source->objtype;
	//target->member = source->member;
	//target->mapping = source->mapping;
	//target->flags = source->flags;
}
