/*
 * libopensync - A synchronization framework
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */

#ifndef OPENSYNC_XMLFIELDLIST_H_
#define OPENSYNC_XMLFIELDLIST_H_

/**
 * @defgroup OSyncXMLFieldListAPI OpenSync XMLFieldList
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFieldList
 * 
 */
/*@{*/

/**
 * @brief Frees all memory which was allocated for an xmlfieldlist
 * @param xmlfieldlist Pointer to the xmlfieldlist object to free
 */
OSYNC_EXPORT void osync_xmlfieldlist_free(OSyncXMLFieldList *xmlfieldlist);

/**
 * @brief Get the length of an xmlfieldlist
 * @param xmlfieldlist Pointer to the xmlfieldlist object
 * @return The number of xmlfield items in the xmlfieldlist 
 */
OSYNC_EXPORT int osync_xmlfieldlist_get_length(OSyncXMLFieldList *xmlfieldlist);

/**
 * @brief Get the xmlfield at the given index of an xmlfieldlist 
 * @param xmlfieldlist Pointer to the xmlfieldlist object
 * @param index The index of the xmlfield pointer to return
 * @return The item at the specified index, or NULL in case of error.
 */
OSYNC_EXPORT OSyncXMLField *osync_xmlfieldlist_item(OSyncXMLFieldList *xmlfieldlist, int index);

/*@}*/

#endif /*OPENSYNC_XMLFIELDLIST_H_*/
