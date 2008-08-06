/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#ifndef _OPENSYNC_PLUGIN_RESOURCE_INTERNALS_H_
#define _OPENSYNC_PLUGIN_RESOURCE_INTERNALS_H_

/*! @brief Gives information about resource
 * 
 * @ingroup OSyncPluginResourcePrivateAPI 
 **/
struct OSyncPluginResource {
	/** If resource is enabled */
	osync_bool enabled;
	/** Human readable identifier/name of resource */
	char *name;
	/** MIME type of this resource */
	char *mime;
	/** Objtype of the resource */
	char *objtype;
	/** OSyncObjFormatSink List of this resource */
	OSyncList *objformatsinks;
	/** Filesystem path */
	char *path;
	/** URL */
	char *url;

	/** Object reference counting */
	int ref_count;
};

#endif /*_OPENSYNC_PLUGIN_RESOURCE_INTERNALS_H_*/

