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
 
#include "opensync.h"
#include "opensync_internals.h"
#include <pthread.h>

GPrivate* current_tabs = NULL;

/**
 * @defgroup OSyncDebugAPI OpenSync Debug
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * Miscanellous functions
 * 
 */
/*@{*/

void osync_trace(OSyncTraceType type, const char *message, ...)
{
#if defined ENABLE_TRACE
	va_list arglist;
	char *buffer;
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
		current_tabs = g_private_new (g_free);
	else
		tabs = (int)g_private_get(current_tabs);
	
	unsigned long int id = (unsigned long int)pthread_self();
	char *logfile = g_strdup_printf("%s/Thread%lu.log", trace, id);
	
	va_start(arglist, message);
	g_vasprintf(&buffer, message, arglist);
	
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
	}
	g_private_set(current_tabs, (void *)tabs);
	va_end(arglist);
	g_free(buffer);
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
 * Used for debugging. Severity ranges from 0=Error to 5=Full Debug
 * 
 * @param subpart String to identify the subpart (and filter on it)
 * @param level The severity of the message
 * @param message The message to display
 * 
 */
void osync_debug(const char *subpart, int level, const char *message, ...)
{
#if defined ENABLE_DEBUG
		osync_assert(level <= 4 && level >= 0, "The debug level must be between 0 and 4.");
		va_list arglist;
		char *buffer;
		int debug = -1;

		va_start(arglist, message);
		g_vasprintf(&buffer, message, arglist);
		
		
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
		g_free(buffer);
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
void osync_print_binary(const unsigned char *data, int len)
{
  int t;
  for (t = 0; t < len; t++) {
    if (data[t] >= ' ' && data[t] <= 'z')
      printf("%c", data[t]);
    else
      printf(" %02x ", data[t]);
  }
  printf("\n");
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

/*@}*/
