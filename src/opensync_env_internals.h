OSyncUserInfo *_osync_user_new(void);
void _osync_user_set_confdir(OSyncUserInfo *user, const char *path);
const char *_osync_user_get_confdir(OSyncUserInfo *user);
OSyncUserInfo *_osync_get_user(void);
osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error);
long long int _osync_env_create_group_id(OSyncEnv *env);
