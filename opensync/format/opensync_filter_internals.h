/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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

#ifndef _OPENSYNC_FILTER_INTERNALS_H_
#define _OPENSYNC_FILTER_INTERNALS_H_

/*! @brief Represents a filter to filter changes 
 * @ingroup OSyncFilterPrivate
 **/
struct OSyncFilter {
	char *objtype;
	OSyncFilterAction action;
	OSyncCustomFilter *custom_filter;
	char *config;
	int ref_count;
};

/*! @brief Represents a custom filter that can be used to call hooks
 * @ingroup OSyncFilterPrivate
 **/
struct OSyncCustomFilter {
	char *name;
	char *objtype;
	char *objformat;
	OSyncFilterFunction hook;
	int ref_count;
};

#endif /*_OPENSYNC_FILTER_INTERNALS_H_*/
