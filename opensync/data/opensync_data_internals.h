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

#ifndef _OPENSYNC_DATA_INTERNALS_H_
#define _OPENSYNC_DATA_INTERNALS_H_

/**
 * @defgroup OSyncDataInternalAPI OpenSync Data Internals
 * @ingroup OSyncDataPrivate
 * @brief The internal part of the OSyncData API
 */
/*@{*/

/*! @brief Get the data from a data object and then clear the data object's pointers to it
 * 
 * @param data The data object
 * @param buffer Pointer to a char * that will be set to point to the data. The caller is responsible for freeing this after calling.
 * @param size Pointer to an integer variable that will be set to the size of the data
 * 
 */
void osync_data_steal_data(OSyncData *data, char **buffer, unsigned int *size);

/*! @brief Compares two data objects
 * 
 * Compares the two given data objects and returns:
 * CONV_DATA_MISMATCH if they are not the same
 * CONV_DATA_SIMILAR if the are not the same but look similar
 * CONV_DATA_SAME if they are exactly the same
 * 
 * @param leftdata The left data to compare
 * @param rightdata The right data to compare
 * @returns The result of the comparison
 * 
 */
OSyncConvCmpResult osync_data_compare(OSyncData *leftdata, OSyncData *rightdata);
/*@}*/

#endif /* _OPENSYNC_DATA_INTERNALS_H_ */

