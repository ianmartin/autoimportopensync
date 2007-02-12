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

#ifndef OPENSYNC_XMLFIELD_INTERNALS_H_
#define OPENSYNC_XMLFIELD_INTERNALS_H_

/**
 * @brief Represent a XMLField object
 * @ingroup OSyncXMLFieldPrivateAPI
 */
struct OSyncXMLField {
	/** The pointer to the next xmlfield */
	OSyncXMLField *next;
	/** The pointer to the previous xmlfield */
	OSyncXMLField *prev; 
	/** The wrapped xml node */
	xmlNodePtr node;
};

OSyncXMLField *_osync_xmlfield_new(OSyncXMLFormat *xmlformat, xmlNodePtr node, OSyncError **error);
void _osync_xmlfield_free(OSyncXMLField *xmlfield);

void _osync_xmlfield_unlink(OSyncXMLField *xmlfield);

int _osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2);

#endif /*OPENSYNC_XMLFIELD_INTERNALS_H_*/
