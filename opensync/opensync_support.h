#ifndef _OPENSYNC_SUPPORT_H
#define _OPENSYNC_SUPPORT_H

typedef struct OSyncThread {
	GThread *thread;
	GCond *started;
	GMutex *started_mutex;
	GMainContext *context;
	GMainLoop *loop;
} OSyncThread;

OSyncThread *osync_thread_new(GMainContext *context, OSyncError **error);
void osync_thread_free(OSyncThread *thread);
void osync_thread_start(OSyncThread *thread);
void osync_thread_stop(OSyncThread *thread);

#endif
