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
 
#ifndef OPENSYNC_VERSION_PRIVATE_H_
#define OPENSYNC_VERSION_PRIVATE_H_

/**
 * @brief Represent a Version object
 * @ingroup OSyncVersionPrivateAPI
 */
struct OSyncVersion{
	/** The reference counter for this object */
	int ref_count;
	char *plugin;
	char *priority;
	char *vendor;
	char *modelversion;
	char *firmwareversion;
	char *softwareversion;
	char *hardwareversion;
	char *identifier;
};

#endif /* OPENSYNC_VERSION_PRIVATE_H_ */

