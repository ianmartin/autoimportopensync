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
 
#ifndef OPENSYNC_XMLFORMAT_H_
#define OPENSYNC_XMLFORMAT_H_

/**
 * @defgroup OSyncXMLFormatAPI OpenSync XMLFormat
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFormat
 * 
 */
/*@{*/

/**
 * @brief Creates a new xmlformat object
 * @param objtype The name of the objtype (e.g.: contact)
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_new(const char *objtype, OSyncError **error);

/**
 * @brief Creates a new xmlformat object from a xml document. 
 * @param buffer The pointer to the xml document
 * @param size The size of the xml document
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error);

/**
 * @brief Increments the reference counter
 * @param xmlformat The pointer to the xmlformat object
 */
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_ref(OSyncXMLFormat *xmlformat);

/**
 * @brief Decrement the reference counter. The xmlformat object will 
 *  be freed if there is no more reference to it.
 * @param xmlformat The pointer to the xmlformat object
 */
OSYNC_EXPORT void osync_xmlformat_unref(OSyncXMLFormat *xmlformat);

/**
 * @brief Get the first field of a xmlformat
 * @param xmlformat The pointer to the xmlformat object
 * @return The first field of the xmlformat
 */
OSYNC_EXPORT OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat);

/**
 * @brief Search for xmlfields in the given xmlformat. It is up to the caller to
 *  free the returned list with OSyncXMLFieldList::osync_xmlfieldlist_free
 * @param xmlformat The pointer to the xmlformat object 
 * @param name The name of the xmlfields to search for
 * @param error The error which will hold the info in case of an error
 * @param ... If the xmlfield should have an attribute with special value,
 *  then it is possible to specify the attribute name and the attribute 
 *  value. Both parameters must always be set. There can be more 
 *  than one attribute pair. The last parameter must always be NULL.
 * @return A pointer to the xmlfieldlist which holds all found xmlfields
 *  or NULL in case of error
 */
OSYNC_EXPORT OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error, ...);

/**
 * @brief Dump the xmlformat into a buffer.
 * @param xmlformat The pointer to the xmlformat object 
 * @param buffer The pointer to the buffer which will hold the xml document. It is up 
 *  to the caller to free this buffer.
 * @param size The pointer to the buffer which will hold the size of the xml document
 * @return Always returns TRUE.
 */
OSYNC_EXPORT osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, unsigned int *size);

/**
 * @brief Sort all xmlfields of the xmlformat.
 *
 *  Calling this function is very expensive - try to avoid using it if possible.
 *  The recommended approach is to assemble the xmlformat in a sorted way instead.
 *
 * @param xmlformat The pointer to the xmlformat object
 */
OSYNC_EXPORT void osync_xmlformat_sort(OSyncXMLFormat *xmlformat);

/**
 * @brief Check if all xmlfields of an xmlformat are sorted.
 * @param xmlformat The pointer to a xmlformat object
 * @returns TRUE if sorted, FALSE otherwise
 */
OSYNC_EXPORT osync_bool osync_xmlformat_is_sorted(OSyncXMLFormat *xmlformat);

/**
 * @brief Copy data from one xmlformat to another.
 *
 * @return True if the copy succeeded, false otherwise
 */ 
OSYNC_EXPORT osync_bool osync_xmlformat_copy(OSyncXMLFormat *source, OSyncXMLFormat **destination, OSyncError **error);

/**
 * @brief Returns the size of the OSyncXMLFormat struct.
 *
 * This is needed since the struct itself is private.
 *
 * @return The size of OSyncXMLFormat struct. 
 */
OSYNC_EXPORT unsigned int osync_xmlformat_size();

/*@}*/

#endif /* OPENSYNC_XMLFORMAT_H_ */

