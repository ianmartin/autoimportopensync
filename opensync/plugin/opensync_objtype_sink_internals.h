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

#ifndef OPENSYNC_OBJTYPE_SINK_INTERNALS_H_
#define OPENSYNC_OBJTYPE_SINK_INTERNALS_H_

/**
 * @defgroup OSyncObjTypeSinkPrivateAPI OpenSync Object Type Sink Internals
 * @ingroup OSyncPrivate
 * @brief Internal functions for managing object type sinks
 * 
 */
/*@{*/

/*! @brief Checks if sink has a read single entries function (read)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a read single entries function (read), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_read(OSyncObjTypeSink *sink);

/*! @brief Sets the status of the read sink function
 *
 * @param sink Pointer to sink
 * @param read TRUE if the sink has a read function, FALSE otherwise
 */
void osync_objtype_sink_set_function_read(OSyncObjTypeSink *sink, osync_bool read);


/*! @brief Checks if sink has a get latest changes function (get_changes)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a get latest changes function (get_changes), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_getchanges(OSyncObjTypeSink *sink);

/*! @brief Sets the status of the get_changes sink function
 *
 * @param sink Pointer to sink
 * @param getchanges TRUE if the sink has a get_changes function, FALSE otherwise
 */
void osync_objtype_sink_set_function_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges);

/*! @brief Checks if sink has a write function (commit)
 *
 * @param sink Pointer to the sink
 * @returns TRUE if the sink has a write function (commit), FALSE otherwise
 */
osync_bool osync_objtype_sink_get_function_write(OSyncObjTypeSink *sink);

/*! @brief Sets the status of the write sink function
 *
 * @param sink Pointer to sink
 * @param write TRUE if the sink has a write function, FALSE otherwise
 */
void osync_objtype_sink_set_function_write(OSyncObjTypeSink *sink, osync_bool write);


/*! @brief Get the current or default connect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_connect_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current connect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_connect_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default disconnect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_disconnect_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current disconnect timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_disconnect_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default getchanges timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_getchanges_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current getchanges timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_getchanges_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default commit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_commit_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current commit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_commit_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default batchcommit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_batchcommit_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current batchcommit timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_batchcommit_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default committedall timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_committedall_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current committedall timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_committedall_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default syncdone timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_syncdone_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current syncdone timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_syncdone_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default write timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_write_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current write timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_write_timeout(OSyncObjTypeSink *sink);


/*! @brief Get the current or default read timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_read_timeout_or_default(OSyncObjTypeSink *sink);

/*! @brief Get the current read timeout in seconds 
 * 
 * @param sink Pointer to the sink
 * @return The timeout in seconds 
 * 
 */
unsigned int osync_objtype_sink_get_read_timeout(OSyncObjTypeSink *sink);

/*@}*/

#endif /*OPENSYNC_SINK_INTERNALS_H_*/

