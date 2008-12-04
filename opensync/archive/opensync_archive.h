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

#include <opensync/opensync_list.h>

/**
 * @defgroup OSyncArchive OpenSync Archive Module
 * @ingroup OSyncPublic
 * @defgroup OSyncArchiveAPI OpenSync Archive
 * @ingroup OSyncArchive
 * @brief Functions for manipulating the archive
 * 
 * The archive is a persistent store of all of the objects seen/handled 
 * in a sync group. It is necessary for the merger code to work.
 */
/*@{*/

/**
 * @brief Creates a new archive object
 * @param filename the full path to the archive database file
 * @param error Pointer to an error struct
 * @return The pointer to the newly allocated archive object or NULL in case of error
 */
OSYNC_EXPORT OSyncArchive *osync_archive_new(const char *filename, OSyncError **error);

/**
 * @brief Increments the reference counter
 * @param archive The pointer to an archive object
 */
OSYNC_EXPORT OSyncArchive *osync_archive_ref(OSyncArchive *archive);

/**
 * @brief Decrement the reference counter. The archive object will 
 *  be freed if there is no more reference to it.
 * @param archive The pointer to an archive object
 */
OSYNC_EXPORT void osync_archive_unref(OSyncArchive *archive);

/**
 * @brief Loads all changes from group archive for a certain object type.
 *
 * @param archive The group archive
 * @param objtype Requested object type 
 * @param ids List to store the archive (database) ids of each entry
 * @param uids List to store uids of each entry
 * @param mappingids List to store mappingids for each entry
 * @param memberids List to store member IDs for each entry 
 * @param error Pointer to an error struct
 * @return TRUE on when all changes successfully loaded otherwise FALSE
 */
OSYNC_EXPORT osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error);
/*@}*/

#endif /*OPENSYNC_ARCHIVE_H_*/
