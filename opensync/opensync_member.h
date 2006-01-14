
/*! @brief Represents the functions that a member will call to inform the syncengine */
typedef struct OSyncMemberFunctions {
	/** The callback function that will receive the new changes from the plugins */
	void (* rf_change) (OSyncMember *, OSyncChange *, void *);
	/** The callback function that will receive custom message calls from the plugin */
	void *(* rf_message) (OSyncMember *, const char *, void *, osync_bool);
	/** The callback function that will receive sync alerts from the plugin */
	void (* rf_sync_alert) (OSyncMember *);
	/** The callback function that will receive log messages */
	void (*rf_log) (OSyncMember *, char *);
} OSyncMemberFunctions;

typedef void (* OSyncEngCallback)(OSyncMember *, void *, OSyncError **);

OSyncMember *osync_member_new(OSyncGroup *group);
void osync_member_free(OSyncMember *member);

osync_bool osync_member_instance_plugin(OSyncMember *member, const char *pluginname, OSyncError **error);
OSyncPlugin *osync_member_get_plugin(OSyncMember *member);

const char *osync_member_get_configdir(OSyncMember *member);
osync_bool osync_member_get_config(OSyncMember *member, char **data, int *size, OSyncError **error);
osync_bool osync_member_get_config_or_default(OSyncMember *member, char **data, int *size, OSyncError **error);
osync_bool osync_member_need_config(OSyncMember *member, OSyncConfigurationTypes *type, OSyncError **error);

void osync_member_set_config(OSyncMember *member, const char *data, int size);
osync_bool osync_member_has_configuration(OSyncMember *member);
const char *osync_member_get_pluginname(OSyncMember *member);
osync_bool osync_member_initialize(OSyncMember *member, OSyncError **error);
void *osync_member_get_enginedata(OSyncMember *member);
void osync_member_set_enginedata(OSyncMember *member, void *data);
void *osync_member_get_report_function(OSyncMember *member);
void osync_member_set_report_function(OSyncMember *member, void *function);
OSyncGroup *osync_member_get_group(OSyncMember *member);
void *osync_member_get_data(OSyncMember *member);
void osync_member_set_data(OSyncMember *member, void *data);
OSyncMemberFunctions *osync_member_get_memberfunctions(OSyncMember *member);
OSyncMember *osync_member_from_id(OSyncGroup *group, int id);
OSyncChange *osync_member_get_changeentry(OSyncMember *member, char *uid);
int osync_member_num_changeentries(OSyncMember *member);
OSyncChange *osync_member_nth_changeentry(OSyncMember *member, int n);
long long int osync_member_get_id(OSyncMember *member);
OSyncMember *osync_member_load(OSyncGroup *group, const char *path, OSyncError **error);
osync_bool osync_member_save(OSyncMember *member, OSyncError **error);
void osync_member_connect(OSyncMember *member, OSyncEngCallback function, void *user_data);
void osync_member_disconnect(OSyncMember *member, OSyncEngCallback function, void *user_data);
void osync_member_get_changeinfo(OSyncMember *member, OSyncEngCallback function, void *user_data);
void osync_member_read_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data);
void osync_member_committed_all(OSyncMember *member, OSyncEngCallback function, void *user_data);

void *osync_member_call_plugin(OSyncMember *member, const char *function, void *data, OSyncError **error);
void osync_member_commit_change(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data);
void osync_member_get_change_data(OSyncMember *member, OSyncChange *change, OSyncEngCallback function, void *user_data);
OSyncFormatEnv *osync_member_get_format_env(OSyncMember *member);
void osync_member_sync_done(OSyncMember *member, OSyncEngCallback function, void *user_data);
OSyncChange *osync_member_find_change(OSyncMember *member, const char *uid);
void osync_member_add_changeentry(OSyncMember *member, OSyncChange *entry);
void osync_member_request_synchronization(OSyncMember *member);
OSyncChange *osync_member_add_random_data(OSyncMember *member, const char *objtype);
void osync_member_finalize(OSyncMember *member);
void osync_member_remove_changeentry(OSyncMember *member, OSyncChange *entry);
osync_bool osync_member_modify_random_data(OSyncMember *member, OSyncChange *change);
osync_bool osync_member_delete_data(OSyncMember *member, OSyncChange *change);
void osync_member_set_slow_sync(OSyncMember *member, const char *objtypestr, osync_bool slow_sync);
osync_bool osync_member_get_slow_sync(OSyncMember *member, const char *objtypestr);
osync_bool osync_member_objtype_enabled(OSyncMember *member, const char *objtype);
osync_bool osync_member_update_change(OSyncMember *member, OSyncChange **change);
void osync_member_set_objtype_enabled(OSyncMember *member, const char *objtypestr, osync_bool enabled);
void osync_member_set_pluginname(OSyncMember *member, const char *pluginname);
void osync_member_set_configdir(OSyncMember *member, const char *configdir);
void *osync_member_get_plugindata(OSyncMember *member);

void osync_member_accept_objtype(OSyncMember *member, const char *objtypestr);
void osync_member_accept_objformat(OSyncMember *member, const char *objtypestr, const char *formatstr, const char *extension);
void osync_member_clear_accepted_types(OSyncMember *member);

void *osync_member_get_loop(OSyncMember *member);
void osync_member_set_loop(OSyncMember *member, void *loop);
