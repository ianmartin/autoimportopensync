#ifndef _OPENSYNC_USER_INTERNALS_H_
#define _OPENSYNC_USER_INTERNALS_H_

/**
 * @defgroup OSyncEnvUserPrivate OpenSync User Internals
 * @ingroup OSyncPrivate
 * @brief The private API of dealing with users
 * 
 */
/*@{*/

/*! @brief Represent a user */
struct OSyncUserInfo
{
	uid_t uid;
	gid_t gid;
	G_CONST_RETURN gchar *username;
	G_CONST_RETURN gchar *homedir;
	char *confdir;
};

OSyncUserInfo *_osync_user_new(void);
void _osync_user_set_confdir(OSyncUserInfo *user, const char *path);
const char *_osync_user_get_confdir(OSyncUserInfo *user);
OSyncUserInfo *_osync_get_user(void);

/*@}*/

#endif //_OPENSYNC_USER_INTERNALS_H_
