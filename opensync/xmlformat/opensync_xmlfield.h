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
 
#ifndef OPENSYNC_XMLFIELD_H_
#define OPENSYNC_XMLFIELD_H_

/**
 * @defgroup OSyncXMLFieldAPI OpenSync XMLField
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLField
 * 
 */
/*@{*/

 /**
 * @brief Creates a new xmlfield object which will be added to the end of xmlfields of the xmlformat object.
 *  The returned object will be freed with the xmlformat object.
 * @param xmlformat Pointer to the xmlformat object
 * @param name The name of the xmlfield
 * @param error The error which will hold the info in case of an error
 * @return A pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSYNC_EXPORT OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error);


/**
 * @brief Get the name of an xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @return The name of the xmlfield
 */
OSYNC_EXPORT const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield);

/**
 * @brief Set the name of an xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param name The name to set
 */
OSYNC_EXPORT void osync_xmlfield_set_name(OSyncXMLField *xmlfield, const char *name);

/**
 * @brief Get the next xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @return A pointer to the next xmlfield or NULL if there is no more xmlfield
 */
OSYNC_EXPORT OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield);

/**
 * @brief Get the value of a attribute of an xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param attr The name of the attribute
 * @return The value of the attribute of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr);

/**
 * @brief Set the attribute name and the attribute value of an xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param attr The name of the attribute
 * @param value The value of the attribute
 */
OSYNC_EXPORT void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value);

/**
 * @brief Get the count of attributes of an xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @return The count of attributes in the xmlfield
 */
OSYNC_EXPORT int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield);

/**
 * @brief Get the name of the nth attribute of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param nth The index of the attribute
 * @return The name of the nth attribute of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_name(OSyncXMLField *xmlfield, int nth);

/**
 * @brief Get the value of the nth attribute of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param nth The index of the attribute
 * @return The value of the nth attribute of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth);

/**
 * @brief Get the value of a key/value pair of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param key The key of the key/value pair
 * @return The value of the key/value pair of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key);

/**
 * @brief Set the key/value pair of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param key The key of the key/value pair
 * @param value The value of the key/value pair
 */
OSYNC_EXPORT void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);

/**
 * @brief Add the key/value pair to a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param key The key of the key/value pair
 * @param value The value of the key/value pair
 */
OSYNC_EXPORT void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);

/**
 * @brief Get the count of keys of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @return The count of keys in the xmlfield
 */
OSYNC_EXPORT int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield);

/**
 * @brief Get the name of the nth key of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param nth The index of the key
 * @return The name of the nth key of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_name(OSyncXMLField *xmlfield, int nth);

/**
 * @brief Get the value of the nth key of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param nth The index of the key
 * @return The value of the nth key of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth);

/**
 * @brief Set the value of the nth key of a xmlfield
 * @param xmlfield Pointer to the xmlfield object
 * @param nth The index of the key
 * @param value The value of the key/value pair
 * @return The value of the nth key of the xmlfield or NULL in case of error
 */
OSYNC_EXPORT void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value);

/**
 * @brief Sort all key nodes of an xmlfield. This function needs to be called
 *  if the xmlfield was build in an unsorted way. Sorting is not necessary if the
 *  xmlfield was built in a sorted way.
 * @param xmlfield Pointer to the xmlfield object
 */
OSYNC_EXPORT void osync_xmlfield_sort(OSyncXMLField *xmlfield);

/*@}*/

#endif /*OPENSYNC_XMLFIELD_H_*/
