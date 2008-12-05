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

#include <opensync/opensync_list.h>

/**
 * @defgroup OSyncDB OpenSync Database Module
 * @ingroup OSyncPublic
 * @defgroup OSyncDBAPI OpenSync Database
 * @ingroup OSyncDB
 *@{*/

OSYNC_EXPORT OSyncDB *osync_db_new(OSyncError **error);

/**
 * @brief Open a database
 * 
 * @param db Pointer to database struct
 * @param dbfile Full file path to database file
 * @param error Pointer to a error struct 
 * @return If database opened successfully then TRUE else FALSE
 */
OSYNC_EXPORT osync_bool osync_db_open(OSyncDB *db, const char *dbfile, OSyncError **error);

/**
 * @brief Close a sqlite3 database file
 * 
 * @param db Pointer to database struct
 * @param error Pointer to a error struct 
 * @return If database closed successfully then TRUE else FALSE
 */
OSYNC_EXPORT osync_bool osync_db_close(OSyncDB *db, OSyncError **error);

/**
 * @brief Checks if requested table exists in database
 *
 * @param db Pointer to database struct
 * @param tablename Name of table which gets reseted
 * @param error Pointer to a error struct 
 * @return If the table exist 1 else 0. On error -1.
 */
OSYNC_EXPORT int osync_db_table_exists(OSyncDB *db, const char *tablename, OSyncError **error);

/**
 * @brief Full reset of a sqlite3 table. 
 *
 * @param db Pointer to database struct
 * @param tablename Name of table which gets reseted
 * @param error Pointer to a error struct 
 * @return TRUE on success otherwise FALSE
 */
OSYNC_EXPORT osync_bool osync_db_reset_table(OSyncDB *db, const char *tablename, OSyncError **error);
OSYNC_EXPORT osync_bool osync_db_reset_full(OSyncDB *db, OSyncError **error);

/**
 * @brief Counts result of a database query 
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns number of query result or -1 on error 
 */
OSYNC_EXPORT int osync_db_count(OSyncDB *db, const char *query, OSyncError **error);

/**
 * @brief Execute a SQL query for a single string (char *). Do only use this
 *        function when you expect a single return. The function will return
 *        NULL if there are more then one results! 
 *
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns result string from database. NULL on more then one result or on error (the caller is responsible for freeing)
 */
OSYNC_EXPORT char *osync_db_query_single_string(OSyncDB *db, const char *query, OSyncError **error);

/**
 * @brief Execute a SQL query for a single integer. Do only use this
 *        function when you expect a single return. The function will return
 *        NULL if there are more then one results! 
 *
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns result integer from database.  On more then one result or on error -1 
 */
OSYNC_EXPORT int osync_db_query_single_int(OSyncDB *db, const char *query, OSyncError **error);

/**
 * @brief Executes a simple SQL query 
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns TRUE on success otherwise FALSE 
 */
OSYNC_EXPORT osync_bool osync_db_query(OSyncDB *db, const char *query, OSyncError **error);

/**
 * @brief Exectues a SQL query and fill all requested data in a OSyncList. 
 *        Check error with osync_error_is_set().
 * 
 * @param db Pointer to database struct
 * @param query SQL database query 
 * @param error Pointer to a error struct 
 * @return Returns pointer to OSyncList which contains the each result as another OSyncList ptr. Freeing is recommend with osync_db_free_list() 
 */
OSYNC_EXPORT OSyncList *osync_db_query_table(OSyncDB *db, const char *query, OSyncError **error);

/**
 * @brief Frees the full result of osync_db_query_table().
 *
 * @param list Result OSyncList pointer of osync_db_query_table()
 */
OSYNC_EXPORT void osync_db_free_list(OSyncList *list);

/**
 * @brief Insert a data (blob) in database. 
 *
 * @param db Pointer to database struct
 * @param query SQL query to bind the data blob
 * @param data Pointer to the data
 * @param size The size of the data which should be binded
 * @param error Pointer to a error struct 
 * @return TRUE on success otherwise FALSE
 */
OSYNC_EXPORT osync_bool osync_db_bind_blob(OSyncDB *db, const char *query, const char *data, unsigned int size, OSyncError **error);

/**
 * @brief Get a data (blob) from the database. 
 *
 * @param db Pointer to database struct
 * @param query SQL query to get the data blob
 * @param data Pointer where to store the data
 * @param size Pointer to store the size of data
 * @param error Pointer to a error struct 
 * @return 1 on success, 0 no result, -1 on error.
 */
OSYNC_EXPORT int osync_db_get_blob(OSyncDB *db, const char *query, char **data, unsigned int *size, OSyncError **error);

OSYNC_EXPORT long long int osync_db_last_rowid(OSyncDB *db);
OSYNC_EXPORT char *osync_db_sql_escape(const char *query);

/*@}*/
#endif /* _OPENSYNC_DB_H_ */

