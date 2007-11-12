#ifndef _OPENSYNC_CHANGE_H_
#define _OPENSYNC_CHANGE_H_

OSYNC_EXPORT OSyncChange *osync_change_new(OSyncError **error);
OSYNC_EXPORT OSyncChange *osync_change_ref(OSyncChange *change);
OSYNC_EXPORT void osync_change_unref(OSyncChange *change);

OSYNC_EXPORT OSyncChange *osync_change_clone(OSyncChange *source, OSyncError **error);

OSYNC_EXPORT void osync_change_set_hash(OSyncChange *change, const char *hash);
OSYNC_EXPORT const char *osync_change_get_hash(OSyncChange *change);

OSYNC_EXPORT void osync_change_set_uid(OSyncChange *change, const char *uid);
OSYNC_EXPORT const char *osync_change_get_uid(OSyncChange *change);

OSYNC_EXPORT OSyncChangeType osync_change_get_changetype(OSyncChange *change);
OSYNC_EXPORT void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type);

OSYNC_EXPORT void osync_change_set_data(OSyncChange *change, OSyncData *data);
OSYNC_EXPORT OSyncData *osync_change_get_data(OSyncChange *change);

OSYNC_EXPORT OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);
OSYNC_EXPORT osync_bool osync_change_duplicate(OSyncChange *change, osync_bool *dirty, OSyncError **error);

OSYNC_EXPORT OSyncObjFormat *osync_change_get_objformat(OSyncChange *change);
OSYNC_EXPORT const char *osync_change_get_objtype(OSyncChange *change);
OSYNC_EXPORT void osync_change_set_objtype(OSyncChange *change, const char *objtype);

#endif //_OPENSYNC_CHANGE_H_
