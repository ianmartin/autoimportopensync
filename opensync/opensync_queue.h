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
	/** The message handler for this queue **/
	OSyncMessageHandler message_handler;
	/** The user_data associated with this queue **/
	gpointer user_data;
	/** The source associated with this queue */
	GSource *source;
	/** The context in which this queue is dispatched */
	GMainContext *context;
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct timeout_info {
	OSyncQueue *sendingqueue;
	OSyncMessage *message;
	void *replysender;
	int timeout;
	gboolean (*timeoutfunc)(gpointer);
};
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*@}*/

OSyncQueue *osync_queue_new(const char *name);
void osync_queue_free(OSyncQueue *queue);
osync_bool osync_queue_exists(OSyncQueue *queue);
osync_bool osync_queue_connect(OSyncQueue *queue, OSyncError **error);
void osync_queue_set_message_handler(OSyncQueue *queue, OSyncMessageHandler handler, gpointer user_data);

osync_bool osync_queue_send_int(OSyncQueue *queue, int data, OSyncError **error);
osync_bool osync_queue_send_string(OSyncQueue *queue, const char *string, OSyncError **error);
osync_bool osync_queue_send_data(OSyncQueue *queue, void *data, unsigned int size, OSyncError **error);
osync_bool osync_queue_send_long_long_int(OSyncQueue *queue, long long int data, OSyncError **error);

osync_bool osync_queue_read_int(OSyncQueue *queue, int *data, OSyncError **error);
osync_bool osync_queue_read_string(OSyncQueue *queue, char **string, OSyncError **error);
osync_bool osync_queue_read_data(OSyncQueue *queue, void *data, unsigned int size, OSyncError **error);
osync_bool osync_queue_read_long_long_int(OSyncQueue *queue, long long int *data, OSyncError **error);

void osync_queue_setup_with_gmainloop(OSyncQueue *queue, GMainContext *context);
osync_bool osync_queue_dispatch(OSyncQueue *queue, OSyncError **error);
