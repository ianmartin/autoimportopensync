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
 
#ifndef OPENSYNC_XMLFORMAT_INTERNALS_H_
#define OPENSYNC_XMLFORMAT_INTERNALS_H_

/**
 * @defgroup OSyncXMLFormatPrivateAPI OpenSync XMLFormat Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFormat
 * 
 */
/*@{*/

/**
 * @brief Get the name of the root node in a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The name of the root node of the xmlformat
 */
const char *osync_xmlformat_root_name(OSyncXMLFormat *xmlformat);

/**
 * @brief Get the objtype of a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The objtype of the xmlformat
 */
const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat);

/*@}*/

#endif /* OPENSYNC_XMLFORMAT_INTERNAL_H_ */

