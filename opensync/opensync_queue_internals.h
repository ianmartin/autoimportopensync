#ifndef _OPENSYNC_QUEUE_INTERNALS_H
#define _OPENSYNC_QUEUE_INTERNALS_H

/**
 * @defgroup OSEngineQueue OpenSync Message Queues Internals
 * @ingroup OSEnginePrivate
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

typedef enum {
	OSYNC_QUEUE_EVENT_NONE,
	OSYNC_QUEUE_EVENT_READ,
	OSYNC_QUEUE_EVENT_ERROR,
	OSYNC_QUEUE_EVENT_HUP
} OSyncQueueEvent;

typedef enum {
	OSYNC_QUEUE_SENDER,
	OSYNC_QUEUE_RECEIVER
} OSyncQueueType;

/*! @brief Represents a Queue which can be used to receive messages
 */
struct OSyncQueue {
	OSyncQueueType type;
	/** The real asynchronous queue from glib **/
	int fd;
	/** The path name of this queue **/
	char *name;
	/** The message handler for this queue **/
	OSyncMessageHandler message_handler;
	/** The user_data associated with this queue **/
	gpointer user_data;
	/** The source associated with this queue */
	GSourceFuncs *incoming_functions;
	GSource *incoming_source;
	/** The context in which the IO of the queue is dispatched */
	GMainContext *context;
	GMainContext *incomingContext;
	
	OSyncThread *thread;
	
	GAsyncQueue *incoming;
	GAsyncQueue *outgoing;
	
	GList *pendingReplies;
	GMutex *pendingLock;
	
	GSourceFuncs *write_functions;
	GSource *write_source;
	
	GSourceFuncs *read_functions;
	GSource *read_source;
	
	osync_bool connected;
};

/*@}*/

int _osync_queue_write_data(OSyncQueue *queue, const void *vptr, size_t n, OSyncError **error);
osync_bool _osync_queue_write_long_long_int(OSyncQueue *queue, const long long int message, OSyncError **error);
osync_bool _osync_queue_write_int(OSyncQueue *queue, const int message, OSyncError **error);

OSyncQueue *osync_queue_new(const char *name, OSyncError **error);
osync_bool osync_queue_new_pipes(OSyncQueue **read_queue, OSyncQueue **write_queue, OSyncError **error);
osync_bool osync_queue_create(OSyncQueue *queue, OSyncError **error);

void osync_queue_free(OSyncQueue *queue);
osync_bool osync_queue_remove(OSyncQueue *queue, OSyncError **error);
osync_bool osync_queue_exists(OSyncQueue *queue);

osync_bool osync_queue_connect(OSyncQueue *queue, OSyncQueueType type, OSyncError **error);
osync_bool osync_queue_disconnect(OSyncQueue *queue, OSyncError **error);
osync_bool osync_queue_is_connected(OSyncQueue *queue);

void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data);
osync_bool osync_queue_send_message(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, OSyncError **error);
osync_bool osync_queue_send_message_with_timeout(OSyncQueue *queue, OSyncQueue *replyqueue, OSyncMessage *message, int timeout, OSyncError **error);

void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context);
osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error);

OSyncQueueEvent osync_queue_poll(OSyncQueue *queue);

OSyncMessage *osync_queue_get_message(OSyncQueue *queue);

osync_bool osync_queue_is_alive(OSyncQueue *queue);

#endif
