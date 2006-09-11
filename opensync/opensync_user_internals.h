#ifndef _OPENSYNC_USER_INTERNALS_H_
#define _OPENSYNC_USER_INTERNALS_H_

/*! @brief Represent a user
 * @ingroup OSyncEnvUserPrivate
 **/
struct OSyncUserInfo
{
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	uid_t uid;
	gid_t gid;
	G_CONST_RETURN gchar *username;
	G_CONST_RETURN gchar *homedir;
	char *confdir;
#endif
};

OSyncUserInfo *osync_user_new(OSyncError **error);
void osync_user_free(OSyncUserInfo *info);
void osync_user_set_confdir(OSyncUserInfo *user, const char *path);
const char *osync_user_get_confdir(OSyncUserInfo *user);

#endif //_OPENSYNC_USER_INTERNALS_H_
