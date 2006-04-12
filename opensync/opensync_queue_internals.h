#ifndef _OPENSYNC_QUEUE_INTERNALS_H
#define _OPENSYNC_QUEUE_INTERNALS_H

/**
 * @defgroup OSEngineQueue OpenSync Message Queues Internals
 * @ingroup OSEnginePrivate
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

/*! @brief Represents a Queue which can be used to receive messages
 */
struct OSyncQueue {
	/** The real asynchronous queue from glib **/
	int fd;
	/** The path name of this queue **/
	char *name;
	/** The message handler for this queue **/
	OSyncMessageHandler message_handler;
	/** The user_data associated with this queue **/
	gpointer user_data;
	/** The source associated with this queue */
	GSource *source;
	/** The context in which this queue is dispatched */
	GMainContext *context;
	GMainContext *incomingContext;
	
	OSyncThread *thread;
	
	GAsyncQueue *incoming;
	GAsyncQueue *outgoing;
	
	OSyncError *error;
	
	GList *pendingReplies;
};

/*@}*/

OSyncQueue *osync_queue_new(const char *name, osync_bool run, OSyncError **error);
osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error);

void osync_queue_free(OSyncQueue *queue);
osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error);
osync_bool osync_queue_exists(OSyncQueue *queue);

osync_bool osync_queue_connect(OSyncQueue *queue, int flags, OSyncError **error);
osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error);

void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data);
osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error);
osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, int timeout, OSyncError **error);

osync_bool osync_queue_start_thread(OSyncQueue *queue, OSyncError **error);
void osync_queue_stop_thread(OSyncQueue *queue);

void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context);
osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error);

osync_bool osync_queue_data_available(OSyncQueue *queue);

OSyncMessage *osync_queue_get_message(OSyncQueue *queue);

osync_bool osync_queue_is_alive(OSyncQueue *queue);

#endif
