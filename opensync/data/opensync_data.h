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

#ifndef _OPENSYNC_DATA_H_
#define _OPENSYNC_DATA_H_

/**
 * @defgroup OSyncData OpenSync Data Module
 * @ingroup OSyncPublic
 * @defgroup OSyncDataAPI OpenSync Data
 * @ingroup OSyncData
 * @brief Handles data within changes
 */
/*@{*/

/*! @brief Create a new data object
 * 
 * @param buffer Character buffer containing the data
 * @param size The size of the data contained in the buffer
 * @param format The object format of the data
 * @param error An error struct
 * @returns The new data object
 * 
 */
OSYNC_EXPORT OSyncData *osync_data_new(char *data, unsigned int size, OSyncObjFormat *format, OSyncError **error);

/*! @brief Increase the reference count on a data object
 * 
 * @param data The data object
 * 
 */
OSYNC_EXPORT OSyncData *osync_data_ref(OSyncData *data);

/*! @brief Decrease the reference count on a data object
 * 
 * @param data The data object
 * 
 */
OSYNC_EXPORT void osync_data_unref(OSyncData *data);

/*! @brief Get the object format from a data object
 * 
 * @param data The data object
 * @returns the object format of the data object
 * 
 */
OSYNC_EXPORT OSyncObjFormat *osync_data_get_objformat(OSyncData *data);

/*! @brief Set the object format on a data object
 * 
 * @param data The data object
 * @param objformat The object format to set
 * 
 */
OSYNC_EXPORT void osync_data_set_objformat(OSyncData *data, OSyncObjFormat *objformat);

/*! @brief Get the object type from a data object
 * 
 * @param data The data object
 * @returns the name of the object type of the data object
 * 
 */
OSYNC_EXPORT const char *osync_data_get_objtype(OSyncData *data);

/*! @brief Set the object type of a data object
 * 
 * @param data The data object
 * @param objtype The name of the object type to set
 * 
 */
OSYNC_EXPORT void osync_data_set_objtype(OSyncData *data, const char *objtype);

/*! @brief Get the data from a data object
 * 
 * @param data The data object
 * @param buffer Pointer to a char * that will be set to point to the data if specified. Do not free this buffer.
 * @param size Pointer to an integer variable that will be set to the size of the data if specified
 * 
 */
OSYNC_EXPORT void osync_data_get_data(OSyncData *data, char **buffer, unsigned int *size);

/*! @brief Set the data of a data object
 * 
 * @param data The data object
 * @param buffer The data as a character array. Freeing this buffer will be handled by the data object.
 * @param size The size of the data contained in the buffer
 * 
 */
OSYNC_EXPORT void osync_data_set_data(OSyncData *data, char *buffer, unsigned int size);

/*! @brief Check if the data object has data stored
 * 
 * @param data The data object
 * @returns TRUE if the data object has data, FALSE otherwise
 * 
 */
OSYNC_EXPORT osync_bool osync_data_has_data(OSyncData *data);

/*! @brief Returns a string describing a data object
 * 
 * Some formats cannot be printed directly. To be able to print these
 * objects they should specify a print function.
 * 
 * @param data The data to get printable
 * @returns A string describing the object
 * 
 */
OSYNC_EXPORT char *osync_data_get_printable(OSyncData *data);

/*! @brief Clone a data object
 * 
 * @param source The data object to clone
 * @param error An error struct
 * @returns a copy of the specified data object, or NULL if an error occurred
 * 
 */
OSYNC_EXPORT OSyncData *osync_data_clone(OSyncData *data, OSyncError **error);

/*! @brief Returns the revision of the object
 * 
 * @param data The change to get the revision from
 * @param error An error struct
 * @returns The revision of the object in seconds since the epoch in zulu time
 * 
 */
OSYNC_EXPORT time_t osync_data_get_revision(OSyncData *data, OSyncError **error);
/*@}*/

#endif /* _OPENSYNC_DATA_H_ */

