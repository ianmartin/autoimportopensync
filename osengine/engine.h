/**
 * @defgroup OSEnginePublic OpenSync Engine API
 * @ingroup PublicAPI
 * @brief The API of the syncengine available to everyone
 * 
 * This gives you an insight in the public API of the opensync sync engine.
 * 
 */
/*@{*/

#ifndef HAVE_ENGINE_H
#define HAVE_ENGINE_H

#include <opensync/opensync.h>

/**************************************************************
 * Enumerations
 *************************************************************/

typedef enum {
	MEMBER_CONNECTED = 1,
	MEMBER_SENT_CHANGES = 2,
	MEMBER_DISCONNECTED = 3,
	MEMBER_CONNECT_ERROR = 4,
	MEMBER_GET_CHANGES_ERROR = 5,
	MEMBER_SYNC_DONE_ERROR = 6,
	MEMBER_DISCONNECT_ERROR = 7
} memberupdatetype;

typedef enum {
	CHANGE_RECEIVED = 1,
	CHANGE_RECEIVED_INFO = 2,
	CHANGE_SENT = 3,
	CHANGE_WRITE_ERROR = 4,
	CHANGE_RECV_ERROR = 5
} changeupdatetype;

typedef enum {
	MAPPING_SOLVED = 1,
	MAPPING_SYNCED = 2,
	MAPPING_NEW = 3,
	MAPPING_WRITE_ERROR = 4
} mappingupdatetype;

/*! @brief The Type of the message
 * 
 */
typedef enum {
	ENG_ENDPHASE_CON = 1, /** All clients have connected or had an error during connection */
	ENG_ENDPHASE_READ = 2, /** All clients have sent their changes to the syncengine */
	ENG_ENDPHASE_WRITE = 3, /** All clients have written their changes */
	ENG_ENDPHASE_DISCON = 4, /** All clients have disconnected */
	ENG_ERROR = 5, /** There was an error */
	ENG_SYNC_SUCCESSFULL = 6, /** The sync is done and was successfull (My favorite message) */
	ENG_PREV_UNCLEAN = 7, /** The previous sync was unclean and the engine will perform a slow-sync now */
	ENG_END_CONFLICTS = 8 /** All conflicts have been reported. */
} engineupdatetype;

/**************************************************************
 * Structs
 *************************************************************/
typedef struct OSyncEngine OSyncEngine;
typedef struct OSyncClient OSyncClient;
typedef struct OSyncMapping OSyncMapping;

/*! @brief Struct for the member status callback
 */
typedef struct MSyncMemberUpdate {
	/** The type of the status update */
	memberupdatetype type;
	/** The member for which the status update is */
	OSyncMember *member;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} MSyncMemberUpdate;

/*! @brief Struct for the change status callback
 */
typedef struct MSyncChangeUpdate {
	/** The type of the status update */
	changeupdatetype type;
	/** The change for which the status update is */
	OSyncChange *change;
	/** The id of the member which sent this change */
	int member_id;
	/** The id of the mapping to which this change belongs if any */
	int mapping_id;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} MSyncChangeUpdate;

/*! @brief Struct for the mapping status callback
 */
typedef struct MSyncMappingUpdate {
	/** The type of the status update */
	mappingupdatetype type;
	/** If the mapping was already solved, this will have the id if the winning entry */
	long long int winner;
	/** The mapping for which the status update is */
	OSyncMapping *mapping;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} MSyncMappingUpdate;

/*! @brief Struct for the engine status callback
 */
typedef struct OSyncEngineUpdate {
	/** The type of the status update */
	engineupdatetype type;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncEngineUpdate;

/**************************************************************
 * Includes
 *************************************************************/
#include "osengine_engine.h"
#include "osengine_status.h"
#include "osengine_mapping.h"
#include "osengine_debug.h"

#endif
/*@}*/
