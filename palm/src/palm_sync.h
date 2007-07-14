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
#include <stdio.h>

#include <pi-version.h>
#include <pi-socket.h>
#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-version.h>
#include <pi-address.h>
#include <pi-datebook.h>
#include <pi-todo.h>
#include <pi-memo.h>

#if ((PILOT_LINK_VERSION == 0) && (PILOT_LINK_MAJOR < 12))
#define OLD_PILOT_LINK
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <glib.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#define PILOT_DEVICE_SERIAL 0
#define PILOT_DEVICE_USB_VISOR 1
#define PILOT_DEVICE_IRDA 2
#define PILOT_DEVICE_NETWORK 4

typedef struct PSyncContactEntry PSyncContactEntry;
typedef struct PSyncEventEntry PSyncEventEntry;
typedef struct PSyncTodoEntry PSyncTodoEntry;
typedef struct PSyncNoteEntry PSyncNoteEntry;

typedef struct PSyncEnv PSyncEnv;

typedef struct PSyncDatabase {
#ifdef OLD_PILOT_LINK
	unsigned char buffer[65536];
#else
	pi_buffer_t *buffer;
#endif
	int size;
	int handle;
	PSyncEnv *env;
	struct CategoryAppInfo cai;
	char *name;
} PSyncDatabase;

typedef struct PSyncEntry {
	PSyncDatabase *db;
#ifdef OLD_PILOT_LINK
	unsigned char buffer[65536];
#else
	pi_buffer_t *buffer;
#endif
	recordid_t id;
	int attr;
	int size;
	int category;
	int index;
} PSyncEntry;

struct PSyncEnv {
	char *username;
	int id;
	char *sockaddr;
	int timeout;
	int speed;
	int conntype;
	int popup;
	int mismatch;
	
	int socket;
	
	PSyncDatabase *currentDB;
	struct PilotUser user;
	
	char *codepage;
	
	OSyncObjFormat *contact_format;
	OSyncObjTypeSink *contact_sink;
};

void psyncDBClose(PSyncDatabase *db);
const char *psyncDBCategoryFromId(PSyncDatabase *db, int id, OSyncError **error);
PSyncDatabase *psyncDBOpen(PSyncEnv *env, char *name, OSyncError **error); 
PSyncEntry *psyncDBGetNthEntry(PSyncDatabase *db, int nth, OSyncError **error);
PSyncEntry *psyncDBGetNextModified(PSyncDatabase *db, OSyncError **error);	
void psyncDBClose(PSyncDatabase *db);
unsigned long psyncUidGetID(const char *uid, OSyncError **error);
osync_bool psyncDBWrite(PSyncDatabase *db, PSyncEntry *entry, OSyncError **error);
int psyncDBCategoryToId(PSyncDatabase *db, const char *name, OSyncError **error);
osync_bool psyncDBAdd(PSyncDatabase *db, PSyncEntry *entry, unsigned long *id, OSyncError **error);
osync_bool psyncDBDelete(PSyncDatabase *db, int id, OSyncError **error);
PSyncEntry *psyncDBGetEntryByID(PSyncDatabase *db, unsigned long id, OSyncError **error);
		
void psyncConnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
void psyncDisconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx);
void psyncSyncDone(void *data, OSyncPluginInfo *info, OSyncContext *ctx);

#endif //_PALM_SYNC_H
