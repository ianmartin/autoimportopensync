
/** @brief The types of status updates for members
 * @ingroup OSEnginePublic
 **/
typedef enum {
	/** The member just connected */
	MEMBER_CONNECTED = 1,
	/** The member just sent its changes */
	MEMBER_SENT_CHANGES = 2,
	/** The member just disconnected */
	MEMBER_DISCONNECTED = 3,
	/** The member had problems connecting */
	MEMBER_CONNECT_ERROR = 4,
	/** The member had problems getting the changes */
	MEMBER_GET_CHANGES_ERROR = 5,
	/** The member had problems during sync_done */
	MEMBER_SYNC_DONE_ERROR = 6,
	/** There was an error while disconnecting */
	MEMBER_DISCONNECT_ERROR = 7
} memberupdatetype;

/** @brief The types of status updates for changes
 * @ingroup OSEnginePublic
 **/
typedef enum {
	/** The change was just received */
	CHANGE_RECEIVED = 1,
	/** The change was just received (Only info) */
	CHANGE_RECEIVED_INFO = 2,
	/** The change was just written */
	CHANGE_SENT = 3,
	/** There was an problem writing */
	CHANGE_WRITE_ERROR = 4,
	/** There was an problem receiving the change */
	CHANGE_RECV_ERROR = 5
} changeupdatetype;

/** @brief The types of status updates for mappings
 * @ingroup OSEnginePublic
 **/
typedef enum {
	/** The mapping has just been solved */
	MAPPING_SOLVED = 1,
	/** The mapping has just been completely synced */
	MAPPING_SYNCED = 2,
	/** There was an error writing on of the changes */
	MAPPING_WRITE_ERROR = 3
} mappingupdatetype;

/** @brief The types of status updates for members
 * @ingroup OSEnginePublic
 **/
typedef enum {
	/** All clients have connected or had an error during connection */
	ENG_ENDPHASE_CON = 1,
	/** All clients have sent their changes to the syncengine */
	ENG_ENDPHASE_READ = 2,
	/** All clients have written their changes */
	ENG_ENDPHASE_WRITE = 3,
	/** All clients have disconnected */
	ENG_ENDPHASE_DISCON = 4,
	/** There was an error */
	ENG_ERROR = 5,
	/** The sync is done and was successfull (My favorite message) */
	ENG_SYNC_SUCCESSFULL = 6,
	/** The previous sync was unclean and the engine will perform a slow-sync now */
	ENG_PREV_UNCLEAN = 7,
	/** All conflicts have been reported. */
	ENG_END_CONFLICTS = 8
} engineupdatetype;


/*! @brief Struct for the member status callback
 * @ingroup OSEnginePublic
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
 * @ingroup OSEnginePublic
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
 * @ingroup OSEnginePublic
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
 * @ingroup OSEnginePublic
 */
typedef struct OSyncEngineUpdate {
	/** The type of the status update */
	engineupdatetype type;
	/** If the status was a error, this error will be set */
	OSyncError *error;
} OSyncEngineUpdate;


void osync_status_conflict(OSyncEngine *engine, OSyncMapping *mapping);
void osync_status_update_member(OSyncEngine *engine, OSyncClient *client, memberupdatetype type, OSyncError **error);
void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, changeupdatetype type, OSyncError **error);
void osync_status_update_mapping(OSyncEngine *engine, OSyncMapping *mapping, mappingupdatetype type, OSyncError **error);
void osync_status_update_engine(OSyncEngine *engine, engineupdatetype type, OSyncError **error);
