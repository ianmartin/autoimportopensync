#include <opensync.h>
#include "opensync_internals.h"

struct OSyncUserInfo
{
	uid_t uid;
	gid_t gid;
	G_CONST_RETURN gchar *username;
	G_CONST_RETURN gchar *homedir;
	char *confdir;
};

OSyncUserInfo *osync_user_new(void)
{
	OSyncUserInfo *user = g_malloc0(sizeof(OSyncUserInfo));
	
	user->uid = getuid();
	user->gid = getgid();
	
	user->homedir = g_get_home_dir();
	user->username = g_get_user_name();
	
	user->confdir = g_strdup_printf("%s/.opensync", user->homedir);
	
	_osync_debug("OSUSR", 3, "Detected User:\nUID: %i\nGID: %i\nHome: %s\nOSyncDir: %s", user->uid, user->gid, user->homedir, user->confdir);
	
	return user;
}

void osync_user_set_confdir(OSyncUserInfo *user, char *path)
{
	if (!user) return;
	
	if (user->confdir)
		free(user->confdir);
	
	user->confdir = g_strdup(path);
}

char *osync_user_get_confdir(OSyncUserInfo *user)
{
	if (!user) return NULL;
	
	return user->confdir;
}
