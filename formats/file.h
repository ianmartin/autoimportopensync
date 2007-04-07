/*
 * opensync - A file format for opensync
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

#ifndef _FILE_H
#define _FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#define uid_t int
#define gid_t int
#endif //_WIN32

typedef struct OSyncFileFormat {
	/** The mode of this file. See man fstat for explanation */
	mode_t mode;
	/** The id of the user (owner) of this file */
	uid_t userid;
	/** The id of the owning group of this file */
	gid_t groupid;
	/** Time of the last modification */
	time_t last_mod;
    char *path;
    char *data;
    unsigned int size;
} OSyncFileFormat;

#endif //_FILE_H
