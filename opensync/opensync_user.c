/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include "opensync.h"
#include "opensync_internals.h"

/**
 * @defgroup OSyncEnvUserPrivate OpenSync User Internals
 * @ingroup OSyncPrivate
 * @brief The private API of dealing with users
 * 
 */
/*@{*/


/*! @brief This will create a new user
 * 
 * The user will hold information like uid, gid, home directory etc
 * 
 * @returns A pointer to a newly allocated OSyncUserInfo
 * 
 */
OSyncUserInfo *_osync_user_new(void)
{
	OSyncUserInfo *user = g_malloc0(sizeof(OSyncUserInfo));
	
	user->uid = getuid();
	user->gid = getgid();
	
	user->homedir = g_get_home_dir();
	user->username = g_get_user_name();
	
	user->confdir = g_strdup_printf("%s/.opensync", user->homedir);
	
	osync_debug("OSUSR", 3, "Detected User:\nUID: %i\nGID: %i\nHome: %s\nOSyncDir: %s", user->uid, user->gid, user->homedir, user->confdir);
	
	return user;
}

/*! @brief This will set the configdir for the given user
 * 
 * This will set the configdir for the given user
 * 
 * @param user The user to change
 * @param path The new configdir path
 * 
 */
void _osync_user_set_confdir(OSyncUserInfo *user, const char *path)
{
	g_assert(user);
	
	if (user->confdir)
		g_free(user->confdir);
	
	user->confdir = g_strdup(path);
}

/*! @brief This will get the configdir for the given user
 * 
 * This will set the configdir for the given user
 * 
 * @param user The user to get the path from
 * @returns The configdir path
 * 
 */
const char *_osync_user_get_confdir(OSyncUserInfo *user)
{
	g_assert(user);
	return user->confdir;
}

/*@}*/
