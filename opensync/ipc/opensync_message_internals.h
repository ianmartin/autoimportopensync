#ifndef _OPENSYNC_MESSAGES_INTERNALS_H
#define _OPENSYNC_MESSAGES_INTERNALS_H

/**
 * @defgroup OSEngineMessage OpenSync Message Internals
 * @ingroup OSEnginePrivate
 * @brief A Message used by the inter thread messaging library
 * 
 */

/*@{*/

typedef struct timeout_info {
        OSyncQueue *sendingqueue;
        OSyncMessage *message;
        GSource *source;
        void *replysender;
        OSyncQueue *replyqueue;
        int timeout;
        gboolean (*timeoutfunc)(gpointer);
} timeout_info;

/*! @brief A OSyncMessage
 * 
 */
struct OSyncMessage {
	gint refCount;
	/** The type of this message */
	OSyncMessageCommand cmd;
	/** The name of the message*/
	long long int id;
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

#endif
