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
	ITMESSAGE_SIGNAL = 1,
	ITMESSAGE_METHODCALL = 2,
	ITMESSAGE_METHODREPLY = 3,
	ITMESSAGE_ERROR = 4,
	ITMESSAGE_ERRORREPLY = 5
} ITMessageType;

/*! @brief Function which can receive messages
 * 
 * @param sender The sender of the received reply
 * @param message The reply that is being received.
 * @param user_data The userdata which was set previously
 * 
 */
typedef void (*ITMessageHandler)(gpointer sender, ITMessage *message, gpointer user_data);

/*! @brief A ITMessage
 * 
 */
struct ITMessage {
	ITMessageType msgtype;
	char *msgname;
	GHashTable *payload;
	ITMessageHandler callback;
	gpointer user_data;
	int timeout_id;
	ITMQueue *replyqueue;
	gpointer parent;
	OSyncError **error;
};

/*@}*/

ITMessage *itm_message_new(gpointer parent, char *msgname, ITMessageType type);
ITMessage *itm_message_new_signal(gpointer parent, char *msgname);
ITMessage *itm_message_new_methodcall(gpointer parent, char *msgname);
void itm_message_set_handler(ITMessage *message, ITMQueue *replyqueue, ITMessageHandler handler, gpointer user_data);
ITMessage *itm_message_new_methodreply (gpointer parent, ITMessage *message);
ITMessage *itm_message_new_errorreply(gpointer parent, ITMessage *message);
void itm_message_set_error(ITMessage *message, OSyncError **error);
OSyncError **itm_message_get_error(ITMessage *message);
gboolean itm_message_is_methodcall(ITMessage *message, char *msgname);
gboolean itm_message_is_error(ITMessage *message);
gboolean itm_message_is_type(ITMessage *message, ITMessageType type);
void itm_message_send_reply(ITMessage *reply);
void itm_message_set_data(ITMessage *message, const char *name, void *data);
gpointer itm_message_get_data(ITMessage *message, char *name);
void itm_message_move_data(ITMessage *source, ITMessage *target);
const char *itm_message_get_msgname(ITMessage *message);
gboolean itm_message_is_signal(ITMessage *message, char *msgname);
