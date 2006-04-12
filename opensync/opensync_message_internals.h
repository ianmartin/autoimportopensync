#ifndef _OPENSYNC_MESSAGES_INTERNALS_H
#define _OPENSYNC_MESSAGES_INTERNALS_H

/**
 * @defgroup OSEngineMessage OpenSync Message Internals
 * @ingroup OSEnginePrivate
 * @brief A Message used by the inter thread messaging library
 * 
 */

/*@{*/

/*! @brief The Type of the message
 * 
 */
typedef enum {
	OSYNC_MESSAGE_NOOP,
	OSYNC_MESSAGE_CONNECT,
	OSYNC_MESSAGE_DISCONNECT,
	OSYNC_MESSAGE_GET_CHANGES,
	OSYNC_MESSAGE_GET_CHANGEDATA,
	OSYNC_MESSAGE_COMMIT_CHANGE,
	OSYNC_MESSAGE_COMMITTED_ALL,
	OSYNC_MESSAGE_SYNC_DONE,
	OSYNC_MESSAGE_CALL_PLUGIN,
	OSYNC_MESSAGE_NEW_CHANGE,
	OSYNC_MESSAGE_REPLY,
	OSYNC_MESSAGE_ERRORREPLY,
	OSYNC_MESSAGE_INITIALIZE,
	OSYNC_MESSAGE_FINALIZE,
	OSYNC_MESSAGE_SYNCHRONIZE,
	OSYNC_MESSAGE_ENGINE_CHANGED,
	OSYNC_MESSAGE_MAPPING_CHANGED,
	OSYNC_MESSAGE_MAPPINGENTRY_CHANGED,
	OSYNC_MESSAGE_ERROR
} OSyncMessageCommand;

/*! @brief Function which can receive messages
 * 
 * @param sender The sender of the received reply
 * @param message The reply that is being received.
 * @param user_data The userdata which was set previously
 * 
 */
typedef void (*OSyncMessageHandler)(OSyncMessage *message, void *user_data);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct timeout_info {
        OSyncQueue *sendingqueue;
        OSyncMessage *message;
        GSource *source;
        void *replysender;
        OSyncQueue *replyqueue;
        int timeout;
        gboolean (*timeoutfunc)(gpointer);
} timeout_info;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*! @brief A OSyncMessage
 * 
 */
struct OSyncMessage {
	gint refCount;
	/** The type of this message */
	OSyncMessageCommand cmd;
	/** The name of the message*/
	long long int id1;
	int id2;
	/** Where should the reply be received? */
	OSyncMessageHandler callback;
	/** The user data */
	gpointer user_data;
	/** The timeout associated with this message */
	timeout_info *to_info;
	/** If this message has already been answered */
	osync_bool is_answered;
	/** The pointer to the internal **/
	GByteArray *buffer;
	/** The current read position **/
	int buffer_read_pos;
};

/*@}*/

OSyncMessage *osync_message_new(OSyncMessageCommand cmd, int size, OSyncError **error);
OSyncMessage *osync_message_new_reply(OSyncMessage *message, OSyncError **error);
OSyncMessage *osync_message_new_errorreply(OSyncMessage *message, OSyncError **error);
void osync_message_ref(OSyncMessage *message);
void osync_message_unref(OSyncMessage *message);

void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, gpointer user_data);
gboolean osync_message_is_error(OSyncMessage *message);
osync_bool osync_message_send_message(OSyncMessage *message, OSyncError **error);
osync_bool osync_message_send_with_timeout(OSyncMessage *message, OSyncQueue *queue, OSyncQueue *replyQueue, int timeout, OSyncError **error);
OSyncMessageCommand osync_message_get_command(OSyncMessage *message);
void osync_message_reset_timeout(OSyncMessage *message);
osync_bool osync_message_is_answered(OSyncMessage *message);
void osync_message_set_answered(OSyncMessage *message);

void osync_message_write_int(OSyncMessage *message, int value);
void osync_message_write_long_long_int(OSyncMessage *message, long long int value);
void osync_message_write_string(OSyncMessage *message, const char *value);
void osync_message_write_data(OSyncMessage *message, const void *value, int size);

void osync_message_read_int(OSyncMessage *message, int *value);
void osync_message_read_long_long_int(OSyncMessage *message, long long int *value);
void osync_message_read_string(OSyncMessage *message, char **value);
void osync_message_read_data(OSyncMessage *message, void *value, int size);

#endif
