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

#ifndef HAVE_OPENSYNC_H
#define HAVE_OPENSYNC_H

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************
 * Defines
 *************************************************************/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**************************************************************
 * Structs
 *************************************************************/
 
/* Data Component */
typedef struct OSyncData OSyncData;
typedef struct OSyncChange OSyncChange;
 
/* Format Component */
typedef struct OSyncFormatEnv OSyncFormatEnv;
typedef struct OSyncObjType OSyncObjType;
typedef struct OSyncObjFormat OSyncObjFormat;
typedef struct OSyncFormatConverterPath OSyncFormatConverterPath;
typedef struct OSyncFormatConverter OSyncFormatConverter;
typedef struct OSyncFilter OSyncFilter;
typedef struct OSyncObjFormatSink OSyncObjFormatSink;
typedef struct OSyncObjTypeSink OSyncObjTypeSink;

/* Plugin component */
typedef struct OSyncPlugin OSyncPlugin;
typedef struct OSyncPluginEnv OSyncPluginEnv;
typedef struct OSyncModule OSyncModule;

/* Engine component */
typedef struct OSyncEngine OSyncEngine;
typedef struct OSyncClient OSyncClient;

/* Mapping component */
typedef struct OSyncMapping OSyncMapping;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMappingView OSyncMappingView;
typedef struct OSyncMappingEntry OSyncMappingEntry;

/* Helper component */
typedef struct OSyncAnchorDB OSyncAnchorDB;

typedef struct OSyncError OSyncError;
typedef struct OSyncGroup OSyncGroup;
typedef struct OSyncUserInfo OSyncUserInfo;
typedef struct OSyncMember OSyncMember;
typedef struct OSyncContext OSyncContext;
typedef struct OSyncHashTable OSyncHashTable;
typedef struct OSyncFormatProperty OSyncFormatProperty;
typedef struct OSyncCustomFilter OSyncCustomFilter;
typedef struct OSyncMessage OSyncMessage;
typedef struct OSyncQueue OSyncQueue;
typedef struct OSyncDB OSyncDB;
typedef int osync_bool;

#include "opensync-support.h"
#include "opensync-error.h"

#ifdef __cplusplus
}
#endif

#endif
