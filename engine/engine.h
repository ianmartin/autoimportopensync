#ifndef HAVE_osync_H
#define HAVE_osync_H

#include <glib.h>
#include "opensync.h"

/**************************************************************
 * Enumerations
 *************************************************************/

 
/**************************************************************
 * Structs
 *************************************************************/
typedef struct OSyncEngine OSyncEngine;
typedef struct MSyncClient MSyncClient;

typedef enum {
	MEMBER_CONNECTED = 1,
	MEMBER_DISCONNECTED = 2,
	MEMBER_ERROR = 5
} memberupdatetype;

typedef struct MSyncMemberUpdate {
	memberupdatetype type;
	OSyncMember *member;
} MSyncMemberUpdate;

typedef enum {
	CHANGE_RECEIVED = 1,
	CHANGE_SENT = 2
} changeupdatetype;

typedef struct MSyncChangeUpdate {
	changeupdatetype type;
	OSyncChange *change;
	int member_id;
	int mapping_id;
} MSyncChangeUpdate;

typedef enum {
	MAPPING_SOLVED = 1,
	MAPPING_SYNCED = 2,
	MAPPING_NEW = 4
} mappingupdatetype;

typedef struct MSyncMappingUpdate {
	mappingupdatetype type;
	int winner;
	int mapping_id;
} MSyncMappingUpdate;

typedef enum {
	ENG_ENDPHASE_CON = 1,
	ENG_ENDPHASE_READ = 2,
	ENG_ENDPHASE_WRITE = 4,
	ENG_ENDPHASE_DISCON = 8
} engineupdatetype;

typedef struct OSyncEngineUpdate {
	engineupdatetype type;
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
