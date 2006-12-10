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

/*! @brief Represent a hashtable which can be used to check if changes have been modifed or deleted */
struct OSyncHashTable {
	OSyncDB *dbhandle;
	GHashTable *used_entries;
	char *tablename;
};

OSyncHashTable *osync_hashtable_new(const char *path, const char *objtype, OSyncError **error);
void osync_hashtable_free(OSyncHashTable *table);

void osync_hashtable_reset(OSyncHashTable *table);
int osync_hashtable_num_entries(OSyncHashTable *table);
osync_bool osync_hashtable_nth_entry(OSyncHashTable *table, int i, char **uid, char **hash);
void osync_hashtable_write(OSyncHashTable *table, const char *uid, const char *hash);
void osync_hashtable_delete(OSyncHashTable *table, const char *uid);
void osync_hashtable_update_hash(OSyncHashTable *table, OSyncChangeType type, const char *uid, const char *hash);
void osync_hashtable_report(OSyncHashTable *table, const char *uid);
char **osync_hashtable_get_deleted(OSyncHashTable *table);
OSyncChangeType osync_hashtable_get_changetype(OSyncHashTable *table, const char *uid, const char *hash);

#endif /* OPENSYNC_HASHTABLE_H_ */
