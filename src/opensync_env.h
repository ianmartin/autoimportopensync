OSyncEnv *osync_env_new	(void);
void osync_env_free(OSyncEnv *env);

osync_bool osync_env_initialize(OSyncEnv *osstruct);
void osync_env_finalize(OSyncEnv *os_env);

int osync_env_num_plugins (OSyncEnv *osstruct);
OSyncPlugin *osync_env_get_nth_plugin(OSyncEnv *osstruct, int nth);
void osync_remove_nth_group(OSyncEnv *osstruct, int nth);
void osync_remove_group(OSyncEnv *osstruct, OSyncGroup *group);
char *osync_env_get_configdir(OSyncEnv *osinfo);
void osync_env_set_configdir(OSyncEnv *osinfo, char *path);
int osync_num_groups (OSyncEnv *osinfo);
OSyncGroup *osync_get_nth_group(OSyncEnv *osinfo, int nth);
int osync_add_group(OSyncEnv *osinfo, OSyncGroup *group);
void *osync_get_data(OSyncEnv *osync);
void osync_set_data(OSyncEnv *osync, void *data);
void osync_env_append_group(OSyncEnv *os_env, OSyncGroup *group);
osync_bool osync_env_load_groups_dir(OSyncEnv *osyncinfo);

OSyncUserInfo *osync_user_new(void);
void osync_user_set_confdir(OSyncUserInfo *user, char *path);
char *osync_user_get_confdir(OSyncUserInfo *user);
osync_bool osync_file_write(char *filename, char *data, int size);
osync_bool osync_file_read(char *filename, char **data, int *size);
