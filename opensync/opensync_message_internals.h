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
	/** Message is a error reply to a method call */
  OSYNC_MESSAGE_REPLY,
	OSYNC_MESSAGE_ERRORREPLY
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
        void *replysender;
        int timeout;
        gboolean (*timeoutfunc)(gpointer);
} timeout_info;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*! @brief A OSyncMessage
 * 
 */
struct OSyncMessage {
	unsigned int refCount;
	/** The type of this message */
	OSyncMessageCommand cmd;
	/** The name of the message*/
	long long id;
	/** Where should the reply be received? */
	OSyncMessageHandler callback;
	/** The user data */
	gpointer user_data;
	/** The error if this is a error message */
	OSyncError *error;
	/** The timeout associated with this message */
	timeout_info *to_info;
	/** If this message has already been answered */
	osync_bool is_answered;
};

/*@}*/

OSyncMessage *osync_message_new(OSyncMessageCommand cmd, OSyncError **error);
void osync_message_set_handler(OSyncMessage *message, OSyncMessageHandler handler, gpointer user_data);
OSyncMessage *osync_message_new_reply(OSyncMessage *message);
OSyncMessage *osync_message_new_errorreply(OSyncMessage *message);
void osync_message_set_error(OSyncMessage *message, OSyncError *error);
OSyncError *osync_message_get_error(OSyncMessage *message);
gboolean osync_message_is_error(OSyncMessage *message);
void osync_message_send_message(OSyncMessage *message);
OSyncMessageCommand osync_message_get_command(OSyncMessage *message);
long long osync_message_get_id(OSyncMessage *message);
void osync_message_reset_timeout(OSyncMessage *message);
osync_bool osync_message_is_answered(OSyncMessage *message);
void osync_message_set_answered(OSyncMessage *message);
