#include "engine.h"
#include "engine_internals.h"

/**
 * @defgroup OSEngineMessage OpenSync Message Internals
 * @ingroup OSEnginePrivate
 * @brief A Message used by the inter thread messaging library
 * 
 */
 
/*@{*/

/*! @brief Creates a new message of the given type
 * 
 * This function will create a new message of the given type, with
 * the given parent and signal name. The parent will be passed to the ITMessageHandler
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param msgname The name of the message
 * @param type The type of this message
 * @returns Pointer to a newly allocated message
 * 
 */
ITMessage *itm_message_new(gpointer parent, char *msgname, ITMessageType type)
{
	ITMessage *message = g_malloc0(sizeof(ITMessage));
	message->msgname = g_strdup(msgname);
	message->msgtype = type;
	message->parent = parent;
	message->payload = g_hash_table_new(g_str_hash, g_str_equal);
	return message;
}

/*! @brief Creates a new message of the signal type
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param msgname The name of the message
 * @returns Pointer to a newly allocated message
 * 
 */
ITMessage *itm_message_new_signal(gpointer parent, char *msgname)
{
	return itm_message_new(parent, msgname, ITMESSAGE_SIGNAL);
}

/*! @brief Creates a new message of the method call
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param msgname The name of the method to call
 * @returns Pointer to a newly allocated message
 * 
 */
ITMessage *itm_message_new_methodcall(gpointer parent, char *msgname)
{
	return itm_message_new(parent, msgname, ITMESSAGE_METHODCALL);
}

/*! @brief Sets the handler that will receive the reply
 * 
 * @param message The message to work on
 * @param replyqueue Which queue should receive the reply
 * @param handler Which handler should be called when the reply is received
 * @param user_data Which user data should be passed to the handler
 * 
 */
void itm_message_set_handler(ITMessage *message, ITMQueue *replyqueue, ITMessageHandler handler, gpointer user_data)
{
	message->user_data = user_data;
	message->callback = handler;
	message->replyqueue = replyqueue;
}

/*! @brief Creates a new reply
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param message The message to which you wish to reply
 * @returns Pointer to a newly allocated message
 * 
 */
ITMessage *itm_message_new_methodreply(gpointer parent, ITMessage *message)
{
	ITMessage *reply = itm_message_new(parent, NULL, ITMESSAGE_METHODREPLY);
	//va_list *arglist = g_malloc0(sizeof(va_list));
	//va_start(*arglist, message);
	//reply->payload = arglist;
	
	reply->callback = message->callback;
	reply->user_data = message->user_data;
	reply->replyqueue = message->replyqueue;
	return reply;
}

/*! @brief Creates a new error reply
 * 
 * @param parent Who send this message. Can be any pointer.
 * @param message The message to which you wish to reply
 * @param msg The error message.
 * @returns Pointer to a newly allocated message
 */
ITMessage *itm_message_new_errorreply(gpointer parent, ITMessage *message)
{
	ITMessage *reply = itm_message_new(parent, "", ITMESSAGE_ERRORREPLY);
	
	reply->callback = message->callback;
	reply->user_data = message->user_data;
	reply->replyqueue = message->replyqueue;
	return reply;
}

void itm_message_set_error(ITMessage *message, char *msg, int errortype)
{
	itm_message_set_data(message, "message", msg);
	itm_message_set_data(message, "errortype", (void *)errortype);
}

const char *itm_message_get_errormsg(ITMessage *message)
{
	return itm_message_get_data(message, "message");
}

int itm_message_get_errortype(ITMessage *message)
{
	return (int)itm_message_get_data(message, "errortype");
}

/*! @brief Checks if the message is of type methodcall with the given name
 * 
 * @param message The message to check
 * @param msgname The name to check for
 * @return #TRUE if the message is of correct type and name matches, #FALSE otherwise
 * 
 */
gboolean itm_message_is_methodcall(ITMessage *message, char *msgname)
{
	if (message->msgtype == ITMESSAGE_METHODCALL && !strcmp(message->msgname, msgname))
		return TRUE;
	return FALSE;
}

/*! @brief Checks if the message is of type signal with the given name
 * 
 * @param message The message to check
 * @param msgname The name to check for
 * @return #TRUE if the message is of correct type and name matches, #FALSE otherwise
 * 
 */
gboolean itm_message_is_signal(ITMessage *message, char *msgname)
{
	if (message->msgtype == ITMESSAGE_SIGNAL && !strcmp(message->msgname, msgname))
		return TRUE;
	return FALSE;
}

/*! @brief Checks if the message is a error
 * 
 * @param message The message to check
 * @return #TRUE if the message is a error, #FALSE otherwise
 * 
 */
gboolean itm_message_is_error(ITMessage *message)
{
	if (message->msgtype == ITMESSAGE_ERROR || message->msgtype == ITMESSAGE_ERRORREPLY)
		return TRUE;
	return FALSE;
}

/*! @brief Checks if the message is of a certain type
 * 
 * @param message The message to check
 * @param type The type to check for
 * @return #TRUE if the message is a that type, #FALSE otherwise
 * 
 */
gboolean itm_message_is_type(ITMessage *message, ITMessageType type)
{
	if (message->msgtype == type)
		return TRUE;
	return FALSE;
}

/*! @brief Sends a reply to a message
 * 
 * This function will send a reply to a message to the sender of the
 * original message. It also removes any timeout for the original message.
 * 
 * @param reply The reply to send
 * 
 */
void itm_message_send_reply(ITMessage *reply)
{
	itm_queue_send(reply->replyqueue, reply);
	if (reply->timeout_id) {
		GSource *source = g_main_context_find_source_by_id(NULL, reply->timeout_id);
		if (source)
			g_source_remove(reply->timeout_id);
			//FIXME Free it
	}
}

/*! @brief Sets the data on a message
 * 
 * This function will set data on a message with that given name
 * 
 * @param message The message
 * @param name The name of the data
 * @param data A pointer to the data
 */
void itm_message_set_data(ITMessage *message, const char *name, void *data)
{
	g_hash_table_insert(message->payload, g_strdup(name), data);
}

/*! @brief Gets the data from a message
 * 
 * This function will return the data that was previously set via the itm_message_set_data()
 * function.
 * 
 * @param message The message
 * @param name The name of the data to fetch
 * @returns pointer to data
 */
gpointer itm_message_get_data(ITMessage *message, char *name)
{
	return g_hash_table_lookup(message->payload, name);
}

/*! @brief Gets the msgname from a message
 * 
 * This function will return the name of a message
 * 
 * @param message The message
 * @returns pointer to name. Dont free it!
 */
const char *itm_message_get_msgname(ITMessage *message)
{
	g_assert(message);
	return message->msgname;
}

/*! @brief Moves the data from one message to another
 * 
 * This function will move all data from one message to another. You cannot
 * request any data from source packet afterwards.
 * 
 * @param source The source message
 * @param target The target message
 * 
 */
void itm_message_move_data(ITMessage *source, ITMessage *target)
{
	g_hash_table_destroy(target->payload);
	target->payload = source->payload;
	target->timeout_id = source->timeout_id;
	source->payload = NULL;
}

/*@}*/
