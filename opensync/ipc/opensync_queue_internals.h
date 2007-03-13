#ifndef _OPENSYNC_QUEUE_INTERNALS_H
#define _OPENSYNC_QUEUE_INTERNALS_H

#include <fcntl.h>
#include <sys/poll.h>

#include <sys/time.h>
#include <signal.h>

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

typedef struct OSyncPendingMessage {
	long long int id;
	/** Where should the reply be received? */
	OSyncMessageHandler callback;
	/** The user data */
	gpointer user_data;
} OSyncPendingMessage;

/*@}*/

int _osync_queue_write_data(OSyncQueue *queue, const void *vptr, size_t n, OSyncError **error);
osync_bool _osync_queue_write_long_long_int(OSyncQueue *queue, const long long int message, OSyncError **error);
osync_bool _osync_queue_write_int(OSyncQueue *queue, const int message, OSyncError **error);

#endif //_OPENSYNC_QUEUE_INTERNALS_H
