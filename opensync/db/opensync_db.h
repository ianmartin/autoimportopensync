/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Gollub <dgollub@suse.de> 
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
 
#ifndef _OPENSYNC_DB_H_
#define _OPENSYNC_DB_H_

OSyncDB *osync_db_new(OSyncError **error);

osync_bool osync_db_open(OSyncDB *db, const char *dbfile, OSyncError **error);
osync_bool osync_db_close(OSyncDB *db, OSyncError **error);

int osync_db_exists(OSyncDB *db, const char *tablename, OSyncError **error);

osync_bool osync_db_reset(OSyncDB *db, const char *tablename, OSyncError **error);
osync_bool osync_db_reset_full(OSyncDB *db, OSyncError **error);

int osync_db_count(OSyncDB *db, const char *query, OSyncError **error);

char *osync_db_query_single_string(OSyncDB *db, const char *query, OSyncError **error);
int osync_db_query_single_int(OSyncDB *db, const char *query, OSyncError **error);
osync_bool osync_db_query(OSyncDB *db, const char *query, OSyncError **error);
GList *osync_db_query_table(OSyncDB *db, const char *query, OSyncError **error);
void osync_db_free_list(GList *list);

osync_bool osync_db_bind_blob(OSyncDB *db, const char *query, const char *data, unsigned int size, OSyncError **error);
int osync_db_get_blob(OSyncDB *db, const char *query, char **data, unsigned int *size, OSyncError **error);

long long int osync_db_last_rowid(OSyncDB *db);

#endif /*_OPENSYNC_DB_H_*/

