#ifndef _OPENSYNC_CHANGE_H_
#define _OPENSYNC_CHANGE_H_

OSyncChange *osync_change_new(OSyncError **error);
void osync_change_ref(OSyncChange *change);
void osync_change_unref(OSyncChange *change);

void osync_change_set_hash(OSyncChange *change, const char *hash);
const char *osync_change_get_hash(OSyncChange *change);

void osync_change_set_uid(OSyncChange *change, const char *uid);
const char *osync_change_get_uid(OSyncChange *change);

OSyncChangeType osync_change_get_changetype(OSyncChange *change);
void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type);

void osync_change_set_data(OSyncChange *change, OSyncData *data);
OSyncData *osync_change_get_data(OSyncChange *change);

OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);
osync_bool osync_change_duplicate(OSyncChange *change, osync_bool *dirty, OSyncError **error);

OSyncObjFormat *osync_change_get_objformat(OSyncChange *change);
const char *osync_change_get_objtype(OSyncChange *change);

#endif //_OPENSYNC_CHANGE_H_
