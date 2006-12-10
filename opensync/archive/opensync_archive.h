/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Armin Bauer <armin.bauer@opensync.org>
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */
 
#ifndef OPENSYNC_ARCHIVE_H_
#define OPENSYNC_ARCHIVE_H_

OSYNC_EXPORT OSyncArchive *osync_archive_new(const char *filename, OSyncError **error);
OSYNC_EXPORT void osync_archive_ref(OSyncArchive *archive);
OSYNC_EXPORT void osync_archive_unref(OSyncArchive *archive);

OSYNC_EXPORT osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_load_data(OSyncArchive *archive, const char *uid, char **data, unsigned int *size, OSyncError **error);

OSYNC_EXPORT long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error);
char *osync_archive_get_objtype(OSyncArchive *archive, long long int memberid, const char *uid, OSyncError **error);

#endif /*OPENSYNC_ARCHIVE_H_*/
