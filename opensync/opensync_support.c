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

#include <pthread.h>
GPrivate* current_tabs = NULL;
/**
 * @defgroup OSyncDebugAPI OpenSync Debug
 * @ingroup OSyncPublic
 * @brief Debug functions used by opensync
 * 
 */
/*@{*/

/** This function will reset the indentation of the trace function. use this
 * after you forked your process. the new process should call this function */
void osync_trace_reset_indent(void)
{
	g_private_set(current_tabs, GINT_TO_POINTER(0));
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
#if defined ENABLE_TRACE

	va_list arglist;
	char *buffer = NULL;
	
	const char *trace = g_getenv("OSYNC_TRACE");
	if (!trace)
		return;
	
	if (!g_file_test(trace, G_FILE_TEST_IS_DIR)) {
		printf("OSYNC_TRACE argument is no directory\n");
		return;
	}
	
	if (!g_thread_supported ()) g_thread_init (NULL);
	int tabs = 0;
	
	if (!current_tabs)
		current_tabs = g_private_new (NULL);
	else
		tabs = GPOINTER_TO_INT(g_private_get(current_tabs));
	
	unsigned long int id = (unsigned long int)pthread_self();
	pid_t pid = getpid();
	char *logfile = g_strdup_printf("%s/Thread%lu-%i.log", trace, id, pid);
	
	va_start(arglist, message);
	buffer = g_strdup_vprintf(message, arglist);
		
	GString *tabstr = g_string_new("");
	int i = 0;
	for (i = 0; i < tabs; i++) {
		tabstr = g_string_append(tabstr, "\t");
	}

	GTimeVal curtime;
	g_get_current_time(&curtime);
	char *logmessage = NULL;
	switch (type) {
		case TRACE_ENTRY:
			logmessage = g_strdup_printf("[%li.%li]\t%s>>>>>>>  %s\n", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer);
			tabs++;
			break;
		case TRACE_INTERNAL:
			logmessage = g_strdup_printf("[%li.%li]\t%s%s\n", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer);
			break;
		case TRACE_EXIT:
			logmessage = g_strdup_printf("[%li.%li]%s<<<<<<<  %s\n", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer);
			tabs--;
			if (tabs < 0)
				tabs = 0;
			break;
		case TRACE_EXIT_ERROR:
			logmessage = g_strdup_printf("[%li.%li]%s<--- ERROR --- %s\n", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer);
			tabs--;
			if (tabs < 0)
				tabs = 0;
			break;
		case TRACE_ERROR:
			logmessage = g_strdup_printf("[%li.%li]%sERROR: %s\n", curtime.tv_sec, curtime.tv_usec, tabstr->str, buffer);
			break;
	}
	g_free(buffer);
	g_private_set(current_tabs, GINT_TO_POINTER(tabs));
	va_end(arglist);
	
	g_string_free(tabstr, TRUE);
	
	GError *error = NULL;
	GIOChannel *chan = g_io_channel_new_file(logfile, "a", &error);
	if (!chan) {
		printf("unable to open %s for writing: %s\n", logfile, error->message);
		return;
	}
	
	gsize writen;
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, logmessage, strlen(logmessage), &writen, NULL) != G_IO_STATUS_NORMAL) {
		printf("unable to write trace to %s\n", logfile);
	} else
		g_io_channel_flush(chan, NULL);

	g_io_channel_shutdown(chan, TRUE, NULL);
	g_io_channel_unref(chan);
	g_free(logmessage);
	g_free(logfile);
	
#endif
}

/*! @brief Used for debugging
 * 
 * Used for debugging. Severity ranges from 0=Error to 4=Full Debug
 * 
 * @param subpart String to identify the subpart (and filter on it)
 * @param level The severity of the message
 * @param message The message to display
 * 
 */
void osync_debug(const char *subpart, int level, const char *message, ...)
{
#if defined ENABLE_DEBUG
		osync_assert_msg(level <= 4 && level >= 0, "The debug level must be between 0 and 4.");
		va_list arglist;
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		int debug = -1;

		va_start(arglist, message);
		g_vsnprintf(buffer, 1024, message, arglist);
		
		char *debugstr = NULL;
		switch (level) {
			case 0:
				//Error
				debugstr = g_strdup_printf("[%s] ERROR: %s", subpart, buffer);
				break;
			case 1:
				// Warning
				debugstr = g_strdup_printf("[%s] WARNING: %s", subpart, buffer);
				break;
			case 2:
				//Information
				debugstr = g_strdup_printf("[%s] INFORMATION: %s", subpart, buffer);
				break;
			case 3:
				//debug
				debugstr = g_strdup_printf("[%s] DEBUG: %s", subpart, buffer);
				break;
			case 4:
				//fulldebug
				debugstr = g_strdup_printf("[%s] FULL DEBUG: %s", subpart, buffer);
				break;
		}
		g_assert(debugstr);
		va_end(arglist);
		
		osync_trace(TRACE_INTERNAL, debugstr);
		
		const char *dbgstr = g_getenv("OSYNC_DEBUG");
		if (dbgstr) {
			debug = atoi(dbgstr);
			if (debug >= level)
				printf("%s\n", debugstr);
		}
		
		g_free(debugstr);
#endif
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

/**
 * @defgroup OSyncEnvAPIMisc OpenSync Misc
 * @ingroup OSyncPublic
 * @brief Some helper functions
 * 
 */
/*@{*/

/*! @brief Opens a xml document
 * 
 * Opens a xml document
 * 
 * @param doc Pointer to a xmldoc
 * @param cur The pointer to the first node
 * @param path The path of the document
 * @param topentry the name of the top node
 * @param error Pointer to a error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool _osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error)
{
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		osync_debug("OSXML", 1, "File %s does not exist", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "File %s does not exist", path);
		return FALSE;
	}
	
	*doc = xmlParseFile(path);

	if (!*doc) {
		osync_debug("OSXML", 1, "Could not open: %s", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Could not open: %s", path);
		return FALSE;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		osync_debug("OSXML", 0, "%s seems to be empty", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems to be empty", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		osync_debug("OSXML", 0, "%s seems not to be a valid configfile.\n", path);
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems not to be a valid configfile.\n", path);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	*cur = (*cur)->xmlChildrenNode;
	return TRUE;
}

/*! @brief Writes data to a file
 * 
 * Writes data to a file
 * 
 * @param filename Where to save the data
 * @param data Pointer to the data
 * @param size Size of the data
 * @param mode The mode to set on the file
 * @param oserror Pointer to a error struct
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_file_write(const char *filename, const char *data, int size, int mode, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	GIOChannel *chan = g_io_channel_new_file(filename, "w", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to open file %s for writing: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for writing: %s", filename, error->message);
		return FALSE;
	}
	if (mode) {
		int fd = g_io_channel_unix_get_fd(chan);
		if (fchmod(fd, mode)) {
			osync_debug("OSYNC", 3, "Unable to set file permissions %i for file %s", mode, filename);
			osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to set file permissions %i for file %s", mode, filename);
			return FALSE;
		}
	}
	gsize writen;
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_write_chars(chan, data, size, &writen, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to write contents of file %s: %s", filename, error->message);
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
 * @returns TRUE if successfull, FALSE otherwise
 * 
 */
osync_bool osync_file_read(const char *filename, char **data, int *size, OSyncError **oserror)
{
	osync_bool ret = FALSE;
	GError *error = NULL;
	gsize sz = 0;
	
	if (!filename) {
		osync_debug("OSYNC", 3, "No file open specified");
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "No file to open specified");
		return FALSE;
	}
	GIOChannel *chan = g_io_channel_new_file(filename, "r", &error);
	if (!chan) {
		osync_debug("OSYNC", 3, "Unable to read file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to open file %s for reading: %s", filename, error->message);
		return FALSE;
	}
	g_io_channel_set_encoding(chan, NULL, NULL);
	if (g_io_channel_read_to_end(chan, data, &sz, &error) != G_IO_STATUS_NORMAL) {
		osync_debug("OSYNC", 3, "Unable to read contents of file %s: %s", filename, error->message);
		osync_error_set(oserror, OSYNC_ERROR_IO_ERROR, "Unable to read contents of file %s: %s", filename, error->message);
	} else {
		ret = TRUE;
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
	return VERSION;
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
	void *result = g_try_malloc(size);
	if (!result) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left");
		return NULL;
	}
	memset(result, 0, size);
	return result;
}

/*@}*/

OSyncThread *osync_thread_new(GMainContext *context, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, context, error);
	
	OSyncThread *thread = osync_try_malloc0(sizeof(OSyncThread), error);
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

static gboolean osyncThreadStopCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_main_loop_quit(thread->loop);
	
	return FALSE;
}

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

static gboolean osyncThreadStartCallback(gpointer data)
{
	OSyncThread *thread = data;
	
	g_mutex_lock(thread->started_mutex);
	g_cond_signal(thread->started);
	g_mutex_unlock(thread->started_mutex);
	return FALSE;
}

void osync_thread_start(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	
	g_mutex_lock(thread->started_mutex);
	GSource *idle = g_idle_source_new();
	g_source_set_callback(idle, osyncThreadStartCallback, thread, NULL);
	g_source_attach(idle, thread->context);
	thread->thread = g_thread_create ((GThreadFunc)g_main_loop_run, thread->loop, TRUE, NULL);
	g_cond_wait(thread->started, thread->started_mutex);
	g_mutex_unlock(thread->started_mutex);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}	

void osync_thread_stop(OSyncThread *thread)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, thread);
	osync_assert(thread);
	
	GSource *source = g_idle_source_new();
	g_source_set_callback(source, osyncThreadStopCallback, thread, NULL);
	g_source_attach(source, thread->context);

	g_thread_join(thread->thread);
	thread->thread = NULL;
	
	g_source_unref(source);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}
