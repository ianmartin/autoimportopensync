/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#include <opensync/opensync_list.h>

typedef void (*OSyncHashtableForEach) (const char *uid, const char *hash, void *user_data);

OSYNC_EXPORT OSyncHashTable *osync_hashtable_new(const char *path, const char *objtype, OSyncError **error);

OSYNC_EXPORT OSyncHashTable *osync_hashtable_ref(OSyncHashTable *table);
OSYNC_EXPORT void osync_hashtable_unref(OSyncHashTable *table);

OSYNC_EXPORT osync_bool osync_hashtable_load(OSyncHashTable *table, OSyncError **error);
OSYNC_EXPORT osync_bool osync_hashtable_save(OSyncHashTable *table, OSyncError **error);

OSYNC_EXPORT osync_bool osync_hashtable_slowsync(OSyncHashTable *table, OSyncError **error);

OSYNC_EXPORT unsigned int osync_hashtable_num_entries(OSyncHashTable *table);
OSYNC_EXPORT void osync_hashtable_foreach(OSyncHashTable *table, OSyncHashtableForEach func, void *user_data);

OSYNC_EXPORT void osync_hashtable_update_change(OSyncHashTable *table, OSyncChange *change);

//OSYNC_EXPORT void osync_hashtable_report(OSyncHashTable *table, OSyncChange *change);
//OSYNC_EXPORT void osync_hashtable_reset_reports(OSyncHashTable *table);

OSYNC_EXPORT OSyncList *osync_hashtable_get_deleted(OSyncHashTable *table);
OSYNC_EXPORT OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, OSyncChange *change);
OSYNC_EXPORT const char *osync_hashtable_get_hash(OSyncHashTable *table, const char *uid);

#endif /* OPENSYNC_HASHTABLE_H_ */
