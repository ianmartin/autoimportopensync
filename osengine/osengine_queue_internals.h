/**
 * @defgroup OSEngineQueue OpenSync Message Queues Internals
 * @ingroup OSEnginePrivate
 * @brief A Queue used for asynchronous communication between thread
 * 
 */

/*@{*/

/*! @brief Represents a Queue which can be used to receive messages
 */
struct ITMQueue {
	/** The real asynchronous queue from glib **/
	GAsyncQueue *queue;
	/** The message handler for this queue **/
	ITMessageHandler message_handler;
	/** The user_data associated with this queue **/
	gpointer user_data;
	GSource *source;
	GMainContext *context;
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct timeout_info {
	ITMQueue *sendingqueue;
	ITMessage *message;
	void *replysender;
	int timeout;
	gboolean (*timeoutfunc)(gpointer);
};
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*@}*/

ITMQueue *itm_queue_new(void);
void itm_queue_set_message_handler(ITMQueue *queue, ITMessageHandler handler, gpointer user_data);
void itm_queue_send(ITMQueue *queue, ITMessage *message);
void itm_queue_send_with_timeout(ITMQueue *queue, ITMessage *message, int timeout, void *replysender);
void itm_queue_setup_with_gmainloop(ITMQueue *queue, GMainContext *context);
void itm_queue_flush(ITMQueue *queue);
void itm_queue_free(ITMQueue *queue);
void itm_queue_dispatch(ITMQueue *queue);
