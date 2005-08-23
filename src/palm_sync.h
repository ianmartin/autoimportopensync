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

#ifndef _PALM_SYNC_H
#define _PALM_SYNC_H

#include <opensync/opensync.h>

#include <pi-socket.h>
#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-version.h>
#include <pi-address.h>
#include <pi-datebook.h>
#include <pi-todo.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <glib.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define PILOT_DEVICE_SERIAL 0
#define PILOT_DEVICE_USB_VISOR 1
#define PILOT_DEVICE_IRDA 2
#define PILOT_DEVICE_NETWORK 4

typedef struct PSyncPalmEntry PSyncPalmEntry;

typedef struct PSyncEnv {
	OSyncMember *member;
	char *statefile;
	char *username;
	int id;
	char *sockaddr;
	int timeout;
	int speed;
	int conntype;
	int debuglevel;
	int socket;
	int database;
	int popup;
	int mismatch;
	char *databasename;
	char *codepage;
} PSyncEnv;

#endif //_PALM_SYNC_H
