OSyncGroup *osync_group_new(OSyncError **error);
void osync_group_unref(OSyncGroup *group);
void osync_group_ref(OSyncGroup *group);

/*! @ingroup OSyncGroupAPI
 * @brief The lock state of a group
 */
typedef enum {
	OSYNC_LOCK_OK,
	OSYNC_LOCKED,
	OSYNC_LOCK_STALE
} OSyncLockState;

typedef enum {
	OSYNC_CONFLICT_RESOLUTION_UNKNOWN,
	OSYNC_CONFLICT_RESOLUTION_DUPLICATE,
	OSYNC_CONFLICT_RESOLUTION_IGNORE,
	OSYNC_CONFLICT_RESOLUTION_NEWER,
	OSYNC_CONFLICT_RESOLUTION_SELECT
} OSyncConflictResolution;

OSyncLockState osync_group_lock(OSyncGroup *group);
void osync_group_unlock(OSyncGroup *group);

void osync_group_set_name(OSyncGroup *group, const char *name);
const char *osync_group_get_name(OSyncGroup *group);
osync_bool osync_group_save(OSyncGroup *group, OSyncError **error);
osync_bool osync_group_load(OSyncGroup *group, const char *path, OSyncError **error);
osync_bool osync_group_delete(OSyncGroup *group, OSyncError **error);
void osync_group_reset(OSyncGroup *group);

void osync_group_add_member(OSyncGroup *group, OSyncMember *member);
void osync_group_remove_member(OSyncGroup *group, OSyncMember *member);
OSyncMember *osync_group_nth_member(OSyncGroup *group, int nth);
int osync_group_num_members(OSyncGroup *group);

const char *osync_group_get_configdir(OSyncGroup *group);
void osync_group_set_configdir(OSyncGroup *group, const char *directory);

long long int osync_group_create_member_id(OSyncGroup *group);
void osync_group_set_slow_sync(OSyncGroup *group, const char *objtype, osync_bool slow_sync);
osync_bool osync_group_get_slow_sync(OSyncGroup *group, const char *objtype);
void osync_group_reset_slow_sync(OSyncGroup *group, const char *objtypestr);
osync_bool osync_group_objtype_enabled(OSyncGroup *group, const char *objtype);
void osync_group_set_objtype_enabled(OSyncGroup *group, const char *objtype, osync_bool enabled);
OSyncFormatEnv *osync_group_get_format_env(OSyncGroup *group);

int osync_group_num_filters(OSyncGroup *group);
OSyncFilter *osync_group_nth_filter(OSyncGroup *group, int nth);
void osync_group_flush_filters(OSyncGroup *group);

osync_bool osync_group_open_changelog(OSyncGroup *group, char ***uids, long long int **memberids, int **changetypes, OSyncError **error);
osync_bool osync_group_save_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error);
osync_bool osync_group_remove_changelog(OSyncGroup *group, OSyncChange *change, OSyncError **error);

void osync_group_set_last_synchronization(OSyncGroup *group, time_t last_sync);
time_t osync_group_get_last_synchronization(OSyncGroup *group);
