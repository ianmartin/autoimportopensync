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

#ifndef _OPENSYNC_CHANGE_H_
#define _OPENSYNC_CHANGE_H_

/**
 * @defgroup OSyncChangeAPI OpenSync Change
 * @ingroup OSyncData
 * @brief Handles change objects
 * 
 */
/*@{*/

/*! @brief Create a new change object
 * 
 * @param error An error struct
 * @returns The new change object
 * 
 */
OSYNC_EXPORT OSyncChange *osync_change_new(OSyncError **error);

/*! @brief Increase the reference count on a change object
 * 
 * @param change The change object
 * 
 */
OSYNC_EXPORT OSyncChange *osync_change_ref(OSyncChange *change);

/*! @brief Decrease the reference count on a change object
 * 
 * @param change The change object
 * 
 */
OSYNC_EXPORT void osync_change_unref(OSyncChange *change);

/*! @brief Sets the hash of a change that is used to decide whether a change is new, modified etc.
 * 
 * @param change The change
 * @param hash The hash to set
 * 
 */
OSYNC_EXPORT void osync_change_set_hash(OSyncChange *change, const char *hash);

/*! @brief Gets the hash of a change
 * 
 * @param change The change
 * @returns The hash
 * 
 */
OSYNC_EXPORT const char *osync_change_get_hash(OSyncChange *change);

/*! @brief Sets the uid of a change
 * 
 * @param change The change
 * @param uid The uid to set
 * 
 */
OSYNC_EXPORT void osync_change_set_uid(OSyncChange *change, const char *uid);

/*! @brief Gets the uid of a change
 * 
 * @param change The change
 * @returns The uid
 * 
 */
OSYNC_EXPORT const char *osync_change_get_uid(OSyncChange *change);

/*! @brief Gets the changetype of a change
 * 
 * @param change The change
 * @returns The changetype
 * 
 */
OSYNC_EXPORT OSyncChangeType osync_change_get_changetype(OSyncChange *change);

/*! @brief Sets the changetype of a change
 * 
 * @param change The change
 * @param type The changetype to set
 * 
 */
OSYNC_EXPORT void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type);

/*! @brief Sets the data of a change
 * 
 * @param change The change
 * @param data the data object to set
 * 
 */
OSYNC_EXPORT void osync_change_set_data(OSyncChange *change, OSyncData *data);

/*! @brief Gets the data from a change object
 * 
 * @param change The change
 * @returns the data object
 * 
 */
OSYNC_EXPORT OSyncData *osync_change_get_data(OSyncChange *change);

/*! @brief Gets the object format of a change
 * 
 * @param change The change
 * @returns The object format
 * 
 */
OSYNC_EXPORT OSyncObjFormat *osync_change_get_objformat(OSyncChange *change);

/*! @brief Gets the object type of a change
 * 
 * @param change The change
 * @returns The name of the object type
 * 
 */
OSYNC_EXPORT const char *osync_change_get_objtype(OSyncChange *change);

/*! @brief Sets the object type of a change
 * 
 * @param change The change
 * @param objtype The name of the object type to set
 * 
 */
OSYNC_EXPORT void osync_change_set_objtype(OSyncChange *change, const char *objtype);

/*! @brief Compares two changes
 * 
 * Compares the two given changes and returns:
 * CONV_DATA_MISMATCH if they are not the same
 * CONV_DATA_SIMILAR if the are not the same but look similar
 * CONV_DATA_SAME if they are exactly the same
 * This function does also compare changetypes etc unlike
 * osync_data_compare()
 * 
 * @param leftchange The left change to compare
 * @param rightchange The right change to compare
 * @returns The result of the comparison
 * 
 */
OSYNC_EXPORT OSyncConvCmpResult osync_change_compare(OSyncChange *leftchange, OSyncChange *rightchange);

/*@}*/

#endif /*_OPENSYNC_CHANGE_H_*/
