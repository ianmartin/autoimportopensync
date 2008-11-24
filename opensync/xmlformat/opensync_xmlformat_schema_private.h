/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@gmail.com>
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
 * Author: Bjoern Ricks <bjoern.ricks@gmail.com>
 * 
 */
 
#ifndef OPENSYNC_XMLFORMAT_SCHEMA_PRIVATE_H_
#define OPENSYNC_XMLFORMAT_SCHEMA_PRIVATE_H_

#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>
#include <libxml/tree.h>

/** 
 * @brief Represents a Schema object
 * @ingroup OSyncXMLFormatPrivateAPI
 */
struct OSyncXMLFormatSchema {
	/** The schema object */
	xmlSchemaPtr schema;
	/** The schema validation context */
	xmlSchemaValidCtxtPtr context;
	/** The object type of OSyncXMLFormat */
	char *objtype;
	/** The reference counter for this object */
	int ref_count;
};

#endif /* OPENSYNC_XMLFORMAT_SCHEMA_PRIVATE_H_ */
