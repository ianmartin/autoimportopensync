#ifndef HAVE_osync_H
#define HAVE_osync_H

#include <opensync/opensync.h>

/**************************************************************
 * Enumerations
 *************************************************************/

 
/**************************************************************
 * Structs
 *************************************************************/
typedef struct OSyncEngine OSyncEngine;
typedef struct OSyncClient OSyncClient;
typedef struct OSyncMapping OSyncMapping;

typedef enum {
	MEMBER_CONNECTED = 1,
	MEMBER_SENT_CHANGES = 2,
	MEMBER_DISCONNECTED = 3,
	MEMBER_CONNECT_ERROR = 4,
	MEMBER_GET_CHANGES_ERROR = 5,
	MEMBER_SYNC_DONE_ERROR = 6,
	MEMBER_DISCONNECT_ERROR = 7
} memberupdatetype;

typedef struct MSyncMemberUpdate {
	memberupdatetype type;
	OSyncMember *member;
	OSyncError *error;
} MSyncMemberUpdate;

typedef enum {
	CHANGE_RECEIVED = 1,
	CHANGE_RECEIVED_INFO = 2,
	CHANGE_SENT = 3,
	CHANGE_WRITE_ERROR = 4,
	CHANGE_RECV_ERROR = 5
} changeupdatetype;

typedef struct MSyncChangeUpdate {
	changeupdatetype type;
	OSyncChange *change;
	int member_id;
	int mapping_id;
	OSyncError *error;
} MSyncChangeUpdate;

typedef enum {
	MAPPING_SOLVED = 1,
	MAPPING_SYNCED = 2,
	MAPPING_NEW = 3,
	MAPPING_WRITE_ERROR = 4
} mappingupdatetype;

typedef struct MSyncMappingUpdate {
	mappingupdatetype type;
	long long int winner;
	OSyncMapping *mapping;
	OSyncError *error;
} MSyncMappingUpdate;

typedef enum {
	ENG_ENDPHASE_CON = 1,
	ENG_ENDPHASE_READ = 2,
	ENG_ENDPHASE_WRITE = 3,
	ENG_ENDPHASE_DISCON = 4,
	ENG_ERROR = 5,
	ENG_SYNC_SUCCESSFULL = 6,
	ENG_PREV_UNCLEAN = 7,
	ENG_END_CONFLICTS = 8
} engineupdatetype;

typedef struct OSyncEngineUpdate {
	engineupdatetype type;
	OSyncError *error;
} OSyncEngineUpdate;

/**************************************************************
 * Includes
 *************************************************************/
#include "osengine_engine.h"
#include "osengine_status.h"
#include "osengine_mapping.h"
#include "osengine_debug.h"

/**************************************************************
 * Prototypes
 *************************************************************/

#endif
