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

#ifndef OPENSYNC_ARCHIVE_INTERNALS_H_
#define OPENSYNC_ARCHIVE_INTERNALS_H_

#include <opensync/opensync_list.h>

/**
 * @defgroup OSyncArchiveInternalAPI OpenSync Archive Internals
 * @ingroup OSyncPrivate
 * @brief The internal part of the OSyncArchive API
 * 
 */
/*@{*/

/**
 * @brief Stores data of an entry in the group archive database (blob).
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry to save.
 * @param objtype The object type of the entry
 * @param data The data to store 
 * @param size Total size of data 
 * @param error Pointer to an error struct
 * @return Returns TRUE on success otherwise FALSE
 */
osync_bool osync_archive_save_data(OSyncArchive *archive, long long int id, const char *objtype, const char *data, unsigned int size, OSyncError **error);

/**
 * @brief Loads data of an entry which is stored in the group archive database (blob).
 *
 * @param archive The group archive
 * @param uid UID of requestd entry
 * @param objtype The objtype type of the entry
 * @param data Pointer to store the requested data 
 * @param size Pointer to store the size of requested data
 * @param error Pointer to an error struct
 * @return Returns 0 if no data is present else 1. On error -1.
 */ 
int osync_archive_load_data(OSyncArchive *archive, const char *uid, const char *objtype, char **data, unsigned int *size, OSyncError **error);

/**
 * @brief Saves an entry in the group archive. 
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry to update (if it already exists), specify 0 to add a new entry.
 * @param uid Reported UID of entry
 * @param objtype Reported object type of entry
 * @param mappingid Mapped ID of entry 
 * @param memberid ID of member which reported entry 
 * @param error Pointer to an error struct
 * @return Returns number of entries in archive group database. 0 on error. 
 */
long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error);

/**
 * @brief Deletes an entry from a group archive.
 *
 * @param archive The group archive
 * @param id Archive (database) id of entry to be deleted
 * @param objtype The object type of the entry
 * @param error Pointer to an error struct
 * @return TRUE if the specified change was deleted successfully, otherwise FALSE
 */
osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, const char *objtype, OSyncError **error);

/**
 * @brief Delete all changes from group archive for a certain object type.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */
osync_bool osync_archive_flush_changes(OSyncArchive *archive, const char *objtype, OSyncError **error);

/**
 * @brief Loads all conficting changes which were ignored in the previous sync. 
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List to store the archive (database) ids of each entry
 * @param changetypes List to store the changetypes for each entry
 * @param error Pointer to an error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */
osync_bool osync_archive_load_ignored_conflicts(OSyncArchive *archive, const char *objtype, OSyncList **mappingsids, OSyncList **changetypes, OSyncError **error);

/**
 * @brief Saves an entry in the ignored conflict list.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param id Mapping Entry ID of entry 
 * @param changetype Changetype of entry 
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */
osync_bool osync_archive_save_ignored_conflict(OSyncArchive *archive, const char *objtype, long long int mappingid, OSyncChangeType changetype, OSyncError **error);

/**
 * @brief Deletes all ignored conflict entries of the changelog with the objtype.
 *
 * @param archive The group archive
 * @param objtype Reported object type of entry
 * @param error Pointer to an error struct
 * @return Returns TRUE on success, FALSE otherwise 
 */
osync_bool osync_archive_flush_ignored_conflict(OSyncArchive *archive, const char *objtype, OSyncError **error);
/*@}*/

#endif /*OPENSYNC_ARCHIVE_INTERNALS_H_*/
