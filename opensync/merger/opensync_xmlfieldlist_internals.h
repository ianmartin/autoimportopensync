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
 * @brief Represent a XMLFieldList object
 * @ingroup OSyncXMLFieldListPrivateAPI
 */
struct OSyncXMLFieldList {
	/** The array holds the OSyncXMLField pointers */
	GPtrArray *array;
};

OSyncXMLFieldList *_osync_xmlfieldlist_new(OSyncError **error);
void _osync_xmlfieldlist_add(OSyncXMLFieldList *xmlfieldlist, OSyncXMLField *xmlfield);
void _osync_xmlfieldlist_remove(OSyncXMLFieldList *xmlfieldlist, int index);

#endif /*OPENSYNC_XMLFIELDLIST_INTERNALS_H_*/
