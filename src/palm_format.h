/*
 * libopensync-palm-plugin - A palm plugin for opensync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * 
 */

#ifndef _PALM_FORMAT_H
#define _PALM_FORMAT_H

struct PSyncContactEntry {
	struct Address address;
	char *codepage;
	char *uid;
	GList *categories;
};

struct PSyncEventEntry {
	struct Appointment appointment;
	char *codepage;
	char *uid;
	GList *categories;
};

struct PSyncTodoEntry {
	struct ToDo todo;
	char *codepage;
	char *uid;
	GList *categories;
};

#endif /* _PALM_FORMAT_H */
