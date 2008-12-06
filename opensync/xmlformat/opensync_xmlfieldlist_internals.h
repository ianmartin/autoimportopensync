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

#ifndef OPENSYNC_XMLFIELDLIST_INTERNALS_H_
#define OPENSYNC_XMLFIELDLIST_INTERNALS_H_

/**
 * @defgroup OSyncXMLFieldListPrivateAPI OpenSync XMLFieldList Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFieldList
 * 
 */
/*@{*/

/**
 * @brief Creates a new xmlfieldlist object
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlfieldlist object or NULL in case of error
 */
OSyncXMLFieldList *osync_xmlfieldlist_new(OSyncError **error);

/**
 * @brief Adds a xmlfield pointer to the end of the xmlfieldlist
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @param xmlfield The xmlfield pointer to add to the xmlfieldlist 
 */
void osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield);

/**
 * @brief Removes the xmlfield pointer at the given index from the xmlfieldlist.
 * @param xmlfieldlist The pointer to a xmlfieldlist object
 * @param index The index of the xmlfield pointer to remove 
 */
void osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index);

/*@}*/

#endif /*OPENSYNC_XMLFIELDLIST_INTERNALS_H_*/
