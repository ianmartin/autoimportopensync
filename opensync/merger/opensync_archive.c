/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

/**
 * @defgroup OSyncArchivePrivateAPI OpenSync Archive Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncArchive
 * 
 */
/*@{*/

void _osync_archive_db_trace(void *data, const char *query)
{
	osync_trace(TRACE_INTERNAL, "query executed: %s", query);
}

/*@}*/

/**
 * @defgroup OSyncArchiveAPI OpenSync Archive
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncArchive
 * 
 */
/*@{*/

OSyncArchive *osync_archive_new(const char* filename, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, filename, error);
	
	OSyncArchive *archive = osync_try_malloc0(sizeof(OSyncArchive), error);
	if (!archive)
		goto error;
		
	int rc = sqlite3_open(filename, &(archive->db));
	if (rc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot open database: %s", sqlite3_errmsg(archive->db));
		goto error_free;
	}
	sqlite3_trace(archive->db, _osync_archive_db_trace, NULL);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, archive);
	return archive;

error_free:
	g_free(archive);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_archive_close(OSyncArchive *archive)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, archive);
	
	int ret = sqlite3_close(archive->db);
	if (ret)
		osync_trace(TRACE_INTERNAL, "Can't close database: %s", sqlite3_errmsg(archive->db));
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

int osync_archive_store(OSyncArchive *archive, const char* data, unsigned int size)
{
	return 0;	
}

void osync_archive_restore(OSyncArchive *archive, int id, const char** data, unsigned int *size)
{
}

/*@}*/
