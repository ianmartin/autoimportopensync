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

#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#ifdef __cplusplus

#define OPENSYNC_BEGIN_DECLS extern "C" {
#define OPENSYNC_END_DECLS }

#else

#define OPENSYNC_BEGIN_DECLS
#define OPENSYNC_END_DECLS

#endif

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define __func__ __FUNCTION__
#define OSYNC_EXPORT __declspec(dllexport)

#elif __GNUC__ 
#include <unistd.h>
#define OSYNC_EXPORT __attribute__ ((visibility("default")))

#elif __sun 
#include <unistd.h>
#define OSYNC_EXPORT __global 

#else
#define OSYNC_EXPORT
#endif

OPENSYNC_BEGIN_DECLS

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
 * Enums
 *************************************************************/

typedef enum {
	OSYNC_START_TYPE_UNKNOWN,
	OSYNC_START_TYPE_PROCESS,
	OSYNC_START_TYPE_THREAD,
	OSYNC_START_TYPE_EXTERNAL
} OSyncStartType;

/*! @ingroup OSyncChangeCmds
 * @brief The possible returns of a change comparison
 */
typedef enum {
	/** The result is unknown, there was a error */
	OSYNC_CONV_DATA_UNKNOWN = 0,
	/** The changes are not the same */
	OSYNC_CONV_DATA_MISMATCH = 1,
	/** The changs are not the same but look similar */
	OSYNC_CONV_DATA_SIMILAR = 2,
	/** The changes are exactly the same */
	OSYNC_CONV_DATA_SAME = 3
} OSyncConvCmpResult;

/*! 
 * @ingroup OSyncChange
 * @brief The changetypes of a change object */
typedef enum  {
	/** Unknown changetype */
	OSYNC_CHANGE_TYPE_UNKNOWN = 0,
	/** Object was added */
	OSYNC_CHANGE_TYPE_ADDED = 1,
	/** Object is unmodifed */
	OSYNC_CHANGE_TYPE_UNMODIFIED = 2,
	/** Object is deleted */
	OSYNC_CHANGE_TYPE_DELETED = 3,
	/** Object has been modified */
	OSYNC_CHANGE_TYPE_MODIFIED = 4
} OSyncChangeType;

/**************************************************************
 * Structs
 *************************************************************/
 
/* Data Component */
typedef struct OSyncData OSyncData;
typedef struct OSyncChange OSyncChange;
 
/* Format Component */
typedef struct OSyncFormatEnv OSyncFormatEnv;
typedef struct OSyncObjFormat OSyncObjFormat;
typedef struct OSyncFormatConverterPath OSyncFormatConverterPath;
typedef struct OSyncFormatConverter OSyncFormatConverter;
typedef struct OSyncFilter OSyncFilter;
typedef struct OSyncObjFormatSink OSyncObjFormatSink;

/* Plugin component */
typedef struct OSyncPlugin OSyncPlugin;
typedef struct OSyncPluginInfo OSyncPluginInfo;
typedef struct OSyncPluginEnv OSyncPluginEnv;
typedef struct OSyncModule OSyncModule;
typedef struct OSyncObjTypeSink OSyncObjTypeSink;

/* Engine component */
typedef struct OSyncEngine OSyncEngine;
typedef struct OSyncObjEngine OSyncObjEngine;
typedef struct OSyncClient OSyncClient;
typedef struct OSyncClientProxy OSyncClientProxy;

/* Mapping component */
typedef struct OSyncMapping OSyncMapping;
typedef struct OSyncMappingTable OSyncMappingTable;
typedef struct OSyncMappingView OSyncMappingView;
typedef struct OSyncMappingEntry OSyncMappingEntry;

/* Helper component */
typedef struct OSyncAnchorDB OSyncAnchorDB;

/* Group component */
typedef struct OSyncGroup OSyncGroup;
typedef struct OSyncGroupEnv OSyncGroupEnv;
typedef struct OSyncMember OSyncMember;

/* Merger component */
typedef struct OSyncArchive OSyncArchive;
typedef struct OSyncCapabilities OSyncCapabilities;
typedef struct OSyncCapability OSyncCapability;
typedef struct OSyncXMLFormat OSyncXMLFormat;
typedef struct OSyncXMLField OSyncXMLField;
typedef struct OSyncXMLFieldList OSyncXMLFieldList;
typedef struct OSyncMerger OSyncMerger;
typedef struct OSyncVersion OSyncVersion;

typedef struct OSyncError OSyncError;
typedef struct OSyncUserInfo OSyncUserInfo;
typedef struct OSyncContext OSyncContext;
typedef struct OSyncHashTable OSyncHashTable;
typedef struct OSyncFormatProperty OSyncFormatProperty;
typedef struct OSyncCustomFilter OSyncCustomFilter;
typedef struct OSyncMessage OSyncMessage;
typedef struct OSyncQueue OSyncQueue;
typedef struct OSyncDB OSyncDB;
typedef int osync_bool;

OPENSYNC_END_DECLS

#include "opensync-support.h"
#include "opensync-error.h"

#endif
