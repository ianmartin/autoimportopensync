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

#ifndef OPENSYNC_HASHTABLE_H_
#define OPENSYNC_HASHTABLE_H_

OSYNC_EXPORT OSyncHashTable *osync_hashtable_new(const char *path, const char *objtype, OSyncError **error);
OSYNC_EXPORT void osync_hashtable_free(OSyncHashTable *table);

OSYNC_EXPORT osync_bool osync_hashtable_slowsync(OSyncHashTable *table, OSyncError **error);

OSYNC_EXPORT int osync_hashtable_num_entries(OSyncHashTable *table);
OSYNC_EXPORT osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int i, char **uid, char **hash);
OSYNC_EXPORT void osync_hashtable_write(OSyncHashTable *table, const char *uid, const char *hash);
OSYNC_EXPORT void osync_hashtable_delete(OSyncHashTable *table, const char *uid);
OSYNC_EXPORT void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChangeType type, const char *uid, const char *hash);

OSYNC_EXPORT void osync_hashtable_report(OSyncHashTable *table, const char *uid);
OSYNC_EXPORT void osync_hashtable_reset_reports(OSyncHashTable *table);

OSYNC_EXPORT char **osync_hashtable_get_deleted(OSyncHashTable *table);
OSYNC_EXPORT OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, const char *uid, const char *hash);
OSYNC_EXPORT char *osync_hashtable_get_hash(OSyncHashTable *table, const char *uid);

#endif /* OPENSYNC_HASHTABLE_H_ */
