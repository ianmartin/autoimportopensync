#ifndef _OPENSYNC_CHANGE_H_
#define _OPENSYNC_CHANGE_H_

OSyncChange *osync_change_new(void);
OSyncChangeType osync_change_get_changetype(OSyncChange *change);
void osync_change_set_hash(OSyncChange *change, const char *hash);
void osync_change_set_uid(OSyncChange *change, const char *uid);
void osync_change_set_data(OSyncChange *change, char *data, int size, osync_bool has_data);
void osync_change_set_objformat(OSyncChange *change, OSyncObjFormat *format);
OSyncObjType *osync_change_get_objtype(OSyncChange *change);
const char *osync_change_get_sourceobjtype(OSyncChange *change);
void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type);
const char *osync_change_get_hash(OSyncChange *change);
const char *osync_change_get_uid(OSyncChange *change);
char *osync_change_get_data(OSyncChange *change);
int osync_change_get_datasize(OSyncChange *change);
OSyncObjFormat *osync_change_get_objformat(OSyncChange *change);
long long int osync_change_get_mappingid(OSyncChange *entry);
void *osync_change_get_engine_data(OSyncChange *change);
void osync_change_set_engine_data(OSyncChange *change, void *engine_data);
OSyncMember *osync_change_get_member(OSyncChange *change);
void osync_change_update(OSyncChange *source, OSyncChange *target);
void osync_change_set_objtype(OSyncChange *change, OSyncObjType *type);
void osync_change_set_objtype_string(OSyncChange *change, const char *name);
void osync_change_set_member(OSyncChange *change, OSyncMember *member);
void osync_change_set_objformat_string(OSyncChange *change, const char *name);
void osync_change_prepend_objformat(OSyncChange *change, OSyncObjFormat *objformat);
long long int osync_change_get_id(OSyncChange *change);
osync_bool osync_change_has_data(OSyncChange *change);
void osync_change_free(OSyncChange *change);
void osync_change_reset(OSyncChange *change);
char *osync_change_get_printable(OSyncChange *change);
osync_bool osync_change_save(OSyncChange *change, osync_bool save_format, OSyncError **error);
osync_bool osync_change_delete(OSyncChange *change, OSyncError **error);
osync_bool osync_changes_load(OSyncGroup *group, OSyncChange ***changes, OSyncError **error);
void osync_changes_close(OSyncGroup *group);
void osync_change_free_data(OSyncChange *change);
void osync_change_set_mappingid(OSyncChange *change, long long int mappingid);
void osync_change_set_conv_env(OSyncChange *change, OSyncFormatEnv *env);

#endif //_OPENSYNC_CHANGE_H_