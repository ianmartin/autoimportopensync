/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-support.h"
#include "opensync_support_internals.h"

GPrivate* current_tabs = NULL;
GPrivate* thread_id = NULL;
GPrivate* trace_disabled = NULL;
GPrivate* trace_sensitive = NULL;
GPrivate* print_stderr = NULL;
const char *trace = NULL;

#ifndef _WIN32
#include <pthread.h>
#endif

/**
 * @defgroup OSyncDebugAPI OpenSync Debug
 * @ingroup OSyncPublic
 * @brief Debug functions used by opensync
 * 
 */
/*@{*/

/*! @brief Reset the indentation of the trace function. 
 *
 * Use this function after when process got forked. It's up to the forked
 * process (child) to reset the indent.
 * 
 */
void osync_trace_reset_indent(void)
{
	g_private_set(current_tabs, GINT_TO_POINTER(0));
}

/*! @brief Disable tracing
 *
 */
void osync_trace_disable(void)
{
	if (!trace_disabled)
		trace_disabled = g_private_new (NULL);
	
	g_private_set(trace_disabled, GINT_TO_POINTER(1));
}

/*! @brief Enable tracing
 *
 */
void osync_trace_enable(void)
{
	if (!trace_disabled)
		trace_disabled = g_private_new (NULL);

	if (!trace)
		g_private_set(trace_disabled, GINT_TO_POINTER(1));
	else
		g_private_set(trace_disabled, GINT_TO_POINTER(0));

}

/*! @brief Initailize tracing 
 *
 */
static void _osync_trace_init()
{
	trace = g_getenv("OSYNC_TRACE");
	if (!trace)
		return;
	
	const char *noprivacy = g_getenv("OSYNC_NOPRIVACY");
	if (!trace_sensitive)
		trace_sensitive = g_private_new(NULL);

	if (noprivacy)
		g_private_set(trace_sensitive, GINT_TO_POINTER(1));
	else
		g_private_set(trace_sensitive, GINT_TO_POINTER(0));

	const char *error = g_getenv("OSYNC_PRINTERROR");
	if (!print_stderr)
		print_stderr = g_private_new(NULL);

	if (error)
		g_private_set(print_stderr, GINT_TO_POINTER(1));
	else
		g_private_set(print_stderr, GINT_TO_POINTER(0));
	
	if (!g_file_test(trace, G_FILE_TEST_IS_DIR)) {
		printf("OSYNC_TRACE argument is no directory\n");
		return;
	}

}
	
/*! @brief Used for tracing the application
 * 
 * use this function to trace calls. The call graph will be saved into
 * the file that is given in the OSYNC_TRACE environment variable
 * 
 * @param type The type of the trace
 * @param message The message to save
 * 
 */
 
void osync_trace(OSyncTraceType type, const char *message, ...)
{
#ifdef OPENSYNC_TRACE
	va_list arglist;
	char *buffer = NULL;
	int tabs = 0;
	unsigned long int id = 0;
#ifdef _WIN32
	int pid = 0;
	char tmp_buf[1024];
#else
	pid_t pid = 0;
#endif
	char *logfile = NULL;
	GString *tabstr = NULL;
	int i = 0;
	GTimeVal curtime;
	char *logmessage = NULL;
	GError *error = NULL;
	GIOChannel *chan = NULL;
	gsize writen;
	const char *endline = NULL;
	
	if (!g_thread_supported ()) g_thread_init (NULL);
	
	if (!trace_disabled || !g_private_get(trace_disabled)) {
		_osync_trace_init();
		osync_trace_enable();
	}
	
	if (GPOINTER_TO_INT(g_private_get(trace_disabled)))
		return;
	
	if (!current_tabs)
		current_tabs = g_private_new (NULL);
	else
		tabs = GPOINTER_TO_INT(g_private_get(current_tabs));
	
#ifdef _WIN32
	if (!thread_id)
		thread_id = g_private_new (NULL);
	id = GPOINTER_TO_INT(thread_id);
	pid = _getpid();
	endline = "\r\n";
#else
	id = (unsigned long int)pthread_self();
	pid = getpid();
	endline = "\n";
#endif
	logfile = g_strdup_printf("%s%cThread%lu-%i.log", trace, G_DIR_SEPARATOR, id, pid);
	
	va_start(arglist, message);
	
#ifdef _WIN32
	vsnprintf(tmp_buf, 1024, message, arglist);
	buffer = g_strdup(tmp_buf);
#else
	buffer = g_strdup_vprintf(message, arglist);
#endif
	
	tabstr = g_string_new("");
	for (i = 0; i < tabs; i++) {
		tabstr = g_string_append(tabstr, "\t");
	}

	g_get_current_time(&curtime);
	switch (type) {
		case TRACE_ENTRY:
			logmessage = g_strdup_printf("[%li.%06li]\t%s>>>>>>>  %s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);
			tabs++;
			break;
		case TRACE_INTERNAL:
			logmessage = g_strdup_printf("[%li.%06li]\t%s%s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);
			break;
		case TRACE_SENSITIVE:
			if (GPOINTER_TO_INT(g_private_get(trace_sensitive)))
				logmessage = g_strdup_printf("[%li.%06li]\t%s[SENSITIVE] %s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);
			else
				logmessage = g_strdup_printf("[%li.%06li]\t%s[SENSITIVE CONTENT HIDDEN]%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, endline);
			break;
		case TRACE_EXIT:
			logmessage = g_strdup_printf("[%li.%06li]%s<<<<<<<  %s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);
			tabs--;
			if (tabs < 0)
				tabs = 0;
			break;
		case TRACE_EXIT_ERROR:
			logmessage = g_strdup_printf("[%li.%06li]%s<--- ERROR --- %s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);
			tabs--;
			if (tabs < 0)
				tabs = 0;

			if (print_stderr)
				fprintf(stderr, "EXIT_ERROR: %s\n", buffer);
			break;
		case TRACE_ERROR:
			logmessage = g_strdup_printf("[%li.%06li]%sERROR: %s%s", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer, endline);

			if (print_stderr)
				fprintf(stderr, "ERROR: %s\n", buffer);

			break;
	}
	g_free(buffer);
	g_private_set(current_tabs, GINT_TO_POINTER(tabs));
	va_end(arglist);
	
	g_string_free(tabstr, TRUE);
	
	chan = g_io_channel_new_file(logfile, "a", &error);
	if (!chan) {
		printf("unable to open %s for writing: %s\n", logfile, error->message);
		return;
	}
	
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, logmessage, strlen(logmessage), &writen, NULL) != G_IO_STATUS_NORMAL) {
		printf("unable to write trace to %s\n", logfile);
	} else
		g_io_channel_flush(chan, NULL);

	g_io_channel_shutdown(chan, TRUE, NULL);
	g_io_channel_unref(chan);
	g_free(logmessage);
	g_free(logfile);
	
#endif /* OPENSYNC_TRACE */
}

/*! @brief Used for printing binary data
 * 
 * Unprintable character will be printed in hex, printable are just printed
 * 
 * @param data The data to print
 * @param len The length to print
 * 
 */
char *osync_print_binary(const unsigned char *data, int len)
{
  int t;
  GString *str = g_string_new("");
  for (t = 0; t < len; t++) {
    if (data[t] >= ' ' && data[t] <= 'z')
      g_string_append_c(str, data[t]);
    else
      g_string_append_printf(str, " %02x ", data[t]);
  }
  return g_string_free(str, FALSE);
}

/*@}*/

/**
 * @defgroup OSyncEnvAPIMisc OpenSync Misc
 * @ingroup OSyncPrivate
 * @brief Some helper functions
 * 
 */
/*@{*/

/*! @brief Stop callback function to stop thread mainloop
 *
 * @param data Pointer to passed callback data
 * @returns Always FALSE
 */
static gboolean osyncThreadStopCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_main_loop_quit(thread->loop);
	
	return FALSE;
}

/*! @brief Start callback function to emit signal when thread's mainloop got started
 *
 * @param data Pointer to passed callback data
 * @returns Always FALSE
 */
static gboolean osyncThreadStartCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_mutex_lock(thread->started_mutex);
	g_cond_signal(thread->started);
	g_mutex_unlock(thread->started_mutex);
	
	return FALSE;
}

/*@}*/

/**
 * @defgroup OSyncEnvAPIMisc OpenSync Misc
 * @ingroup OSyncPublic
 * @brief Some helper functions
 * 
 */
/*@{*/

/*! @brief Writes data to a file
 * 
 * Writes data to a file
 * 
 * @param filename Where to save the data
 * @param data Pointer to the data
 * @param size Size of the data
 * @param mode The mode to set on the file
 * @param oserror Pointer to a error struct
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_file_write(const char *filename, const char *data, unsigned int size, int mode, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	gsize writen;
	
	GIOChannel *chan = g_io_channel_new_file(filename, "w", &error);
	if (!chan) {
		osync_trace(TRACE_INTERNAL, "Unable to open file %s for writing: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for writing: %s", filename, error->message);
		return FALSE;
	}
	if (mode) {
		if (g_chmod(filename, mode)) {
			osync_trace(TRACE_INTERNAL, "Unable to set file permissions %i for file %s", mode, filename);
			osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to set file permissions %i for file %s", mode, filename);
			return FALSE;
		}
	}
	
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, data, size, &writen, &error) != G_IO_STATUS_NORMAL) {
		osync_trace(TRACE_INTERNAL, "Unable to write contents of file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to write contents of file %s: %s", filename, error->message);
	} else {
		g_io_channel_flush(chan, NULL);
		ret = TRUE;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}

/*! @brief Reads a file
 * 
 * Reads a file
 * 
 * @param filename Where to read the data from
 * @param data Pointer to the data
 * @param size Size of the data
 * @param oserror Pointer to a error struct
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_file_read(const char *filename, char **data, unsigned int *size, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	gsize sz = 0;
	GIOChannel *chan = NULL;
	
	if (!filename) {
		osync_trace(TRACE_INTERNAL, "No file open specified");
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "No file to open specified");
		return FALSE;
	}
	
	chan = g_io_channel_new_file(filename, "r", &error);
	if (!chan) {
		osync_trace(TRACE_INTERNAL, "Unable to read file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for reading: %s", filename, error->message);
		return FALSE;
	}
	
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_read_to_end(chan, data, &sz, &error) != G_IO_STATUS_NORMAL) {
		osync_trace(TRACE_INTERNAL, "Unable to read contents of file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to read contents of file %s: %s", filename, error->message);
	} else {
		ret = TRUE;
		if (size)
			*size = (int)sz;
	}
	g_io_channel_shutdown(chan, FALSE, NULL);
	g_io_channel_unref(chan);
	return ret;
}

/*! @brief Returns the version of opensync
 * 
 * Returns a string identifying the major and minor version
 * of opensync (something like "0.11")
 * 
 * @returns String with version
 * 
 */
const char *osync_get_version(void)
{
	return OPENSYNC_VERSION;
}

/*! @brief Safely tries to malloc memory
 * 
 * Tries to malloc memory but returns an error in an OOM situation instead
 * of aborting
 * 
 * @param size The size in bytes to malloc
 * @param error The error which will hold the info in case of an error
 * @returns A pointer to the new memory or NULL in case of error
 * 
 */
void *osync_try_malloc0(unsigned int size, OSyncError **error)
{
	void *result = NULL;
	
#ifdef OPENSYNC_UNITTESTS 	
	if (!g_getenv("OSYNC_NOMEMORY"))
		result = g_try_malloc(size);
#else		
	result = g_try_malloc(size);
#endif /*OPENSYNC_UNITTESTS*/

	if (!result) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left");
		return NULL;
	}
	memset(result, 0, size);
	return result;
}

/*! @brief Frees memory
 * 
 * Frees memory allocated by osync_try_malloc0().
 * 
 * @param ptr Pointer to allocated memory which should get freed
 * 
 */
void osync_free(void *ptr)
{
	if (!ptr)
		return;

	g_free(ptr);
}

/*! @brief Allocates a new thread with a g_mainloop 
 * 
 * @param context Pointer to GMainContext 
 * @param error The error which will hold the info in case of an error
 * @returns A pointer to the new allocated OSyncThread with inactive thread and mainloop 
 * 
 */
OSyncThread *osync_thread_new(GMainContext *context, OSyncError **error)
{
	OSyncThread *thread = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, context, error);
	
	thread = osync_try_malloc0(sizeof(OSyncThread), error);
	if (!thread)
		goto error;

	if (!g_thread_supported ()) g_thread_init (NULL);
	
	thread->started_mutex = g_mutex_new();
	thread->started = g_cond_new();
	thread->context = context;
	if (thread->context)
		g_main_context_ref(thread->context);
	thread->loop = g_main_loop_new(thread->context, FALSE);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, thread);
	return thread;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*! @brief Frees thread object 
 * 
 * @param thread Pointer to OSyncThread
 * 
 */
void osync_thread_free(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	if (thread->started_mutex)
		g_mutex_free(thread->started_mutex);

	if (thread->started)
		g_cond_free(thread->started);
	
	if (thread->loop)
		g_main_loop_unref(thread->loop);
	
	if (thread->context)
		g_main_context_unref(thread->context);
		
	g_free(thread);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*static gpointer osyncThreadStartCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_mutex_lock(thread->started_mutex);
	g_cond_signal(thread->started);
	g_mutex_unlock(thread->started_mutex);
	
	g_main_loop_run(thread->loop);
	
	return NULL;
}*/

/*void osync_thread_start(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	//Start the thread
	g_mutex_lock(thread->started_mutex);
	thread->thread = g_thread_create (osyncThreadStartCallback, thread, TRUE, NULL);
	g_cond_wait(thread->started, thread->started_mutex);
	g_mutex_unlock(thread->started_mutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}*/


/*! @brief Start thread and it's mainloop 
 * 
 * @param func GThreadFunc Pointer  
 * @param userdata Custom data poitner which get supplied to thread function
 * @param error Pointer to error struct
 * @return Newly allocate OSyncThread object with inactive mainloop
 * 
 */
OSyncThread *osync_thread_create(GThreadFunc func, void *userdata, OSyncError **error)
{
	osync_assert(func);
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, func, userdata, error);

	GError *gerror;
	
	OSyncThread *thread = osync_try_malloc0(sizeof(OSyncThread), error);
	if (!thread)
		goto error;

	if (!g_thread_supported ())
		g_thread_init (NULL);

	thread->thread = g_thread_create(func, userdata, TRUE, &gerror);

	if (!thread->thread) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", gerror->message);
		g_error_free(gerror);
		goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return thread;

error:	
	osync_trace(TRACE_EXIT_ERROR, "%s", __func__, osync_error_print(error));
	return NULL;
}


/*! @brief Start thread and it's mainloop 
 * 
 * @param thread Thread object 
 * 
 */
void osync_thread_start(OSyncThread *thread)
{
	GSource *idle = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	
	g_mutex_lock(thread->started_mutex);
	idle = g_idle_source_new();
	g_source_set_callback(idle, osyncThreadStartCallback, thread, NULL);
	g_source_attach(idle, thread->context);
	thread->thread = g_thread_create ((GThreadFunc)g_main_loop_run, thread->loop, TRUE, NULL);
	g_cond_wait(thread->started, thread->started_mutex);
	g_mutex_unlock(thread->started_mutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}	

/*! @brief Stops thread's mainloop and joins the thread
 * 
 * @param thread Thread object 
 * 
 */
void osync_thread_stop(OSyncThread *thread)
{
	GSource *source = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);

        source = g_idle_source_new();
	g_source_set_callback(source, osyncThreadStopCallback, thread, NULL);
        g_source_attach(source, thread->context);

	g_thread_join(thread->thread);
	thread->thread = NULL;
	
	g_source_unref(source);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Exit thread 
 * 
 * @param thread Thread object 
 * @param retval Return value of thread while exiting
 * 
 */
void osync_thread_exit(OSyncThread *thread, int retval)
{
	GSource *source = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, thread, retval);
	osync_assert(thread);

        source = g_idle_source_new();
	g_source_set_callback(source, osyncThreadStopCallback, thread, NULL);
        g_source_attach(source, thread->context);
	g_source_unref(source);
	thread->thread = NULL;

	g_thread_exit(GINT_TO_POINTER(retval));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}


/*! @brief String replace
 * 
 * @param input Input string to work on
 * @param delimiter Delimiter
 * @param replacement Replacement
 * @returns Replaced/Modified string result 
 * 
 */
char *osync_strreplace(const char *input, const char *delimiter, const char *replacement)
{
	osync_return_val_if_fail(input != NULL, NULL);
	osync_return_val_if_fail(delimiter != NULL, NULL);
	osync_return_val_if_fail(replacement != NULL, NULL);

	gchar **array = g_strsplit(input, delimiter, 0);
	gchar *ret = g_strjoinv(replacement, array);
	g_strfreev(array);

	return ret;
}

/*! @brief Bit counting
 * 
 * MIT HAKMEM Count, Bit counting in constant time and memory. 
 * 
 * @param u unsigned integer value to count bits
 * @returns The bit counting result 
 * 
 */
int osync_bitcount(unsigned int u)
{
	unsigned int uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
	return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

/*! @brief Creates a random string
 * 
 * Creates a random string of given length or less
 * 
 * @param maxlength The maximum length of the string
 * @returns The random string
 * 
 */
char *osync_rand_str(int maxlength)
{
	char *randchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIKLMNOPQRSTUVWXYZ1234567890";
	
	int length;
	char *retchar;
	int i = 0;

	length = g_random_int_range(1, maxlength + 1);
	retchar = malloc(length * sizeof(char) + 1);
	retchar[0] = 0;

	for (i = 0; i < length; i++) {
		retchar[i] = randchars[g_random_int_range(0, strlen(randchars))];
		retchar[i + 1] = 0;
	}

	return retchar;
}

/*! @brief Duplicates a string 
 * 
 * Duplicates a string, ending with terminating-zero: \0
 * 
 * @param str The pointer of the string to duplicate
 * @returns The duplicate string, caller is responsible for freeing.
 * 
 */
char *osync_strdup(char *str)
{
	return g_strdup(str);
}

/*! @brief Duplicates a formated string 
 * 
 * Duplicates a formated string, ending with terminating-zero: \0
 * 
 * @param str The pointer of the string to duplicate
 * @param args 
 * @returns The duplicate string, caller is responsible for freeing.
 * 
 */
char *osync_strdup_printf(const char *format, ...)
{
	va_list ap;
	char *str;

	va_start(ap, format);
	str = g_strdup_vprintf(format, ap); 
	va_end(ap);

	return str;
}


/*! @brief Duplicates a string 
 * 
 * Duplicates a string, ending with terminating-zero: \0
 * 
 * @param str The pointer of the string to duplicate
 * @returns The duplicate string, caller is responsible for freeing.
 * 
 */
char *osync_strdup(const char *str)
{
	return g_strdup(str);
}

/*! @brief Duplicates a formated string 
 * 
 * Duplicates a formated string, ending with terminating-zero: \0
 * 
 * @param str The pointer of the string to duplicate
 * @param args 
 * @returns The duplicate string, caller is responsible for freeing.
 * 
 */
char *osync_strdup_printf(const char *format, ...)
{
	va_list ap;
	char *str;

	va_start(ap, format);
	str = g_strdup_vprintf(format, ap); 
	va_end(ap);

	return str;
}

/*@}*/

