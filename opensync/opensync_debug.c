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
	
	unsigned long int id = (unsigned long int)pthread_self();
	char *logfile = g_strdup_printf("%s/Thread%lu.log", trace, id);
	
	va_start(arglist, message);
	g_vasprintf(&buffer, message, arglist);
	
	GTimeVal curtime;
	g_get_current_time(&curtime);
	char *logmessage = NULL;
	switch (type) {
		case TRACE_ENTRY:
			logmessage = g_strdup_printf("[%li.%li]\t------->  %s\n", curtime.tv_sec, curtime.tv_usec, buffer);
			break;
		case TRACE_INTERNAL:
			logmessage = g_strdup_printf("[%li.%li]\t%s\n", curtime.tv_sec, curtime.tv_usec, buffer);
			break;
		case TRACE_EXIT:
			logmessage = g_strdup_printf("[%li.%li]\t<-------  %s\n", curtime.tv_sec, curtime.tv_usec, buffer);
			break;
		case TRACE_EXIT_ERROR:
			logmessage = g_strdup_printf("[%li.%li]\t<--- ERROR --- %s\n", curtime.tv_sec, curtime.tv_usec, buffer);
			break;
	}
	va_end(arglist);
	g_free(buffer);
	
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

	g_io_channel_shutdown(chan, FALSE, NULL);
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
		va_list arglist;
		char *buffer;
		int debug = -1;

		va_start(arglist, message);
		g_vasprintf(&buffer, message, arglist);
		
		const char *dbgstr = g_getenv("OSYNC_DEBUG");
		if (!dbgstr)
			return;
		debug = atoi(dbgstr);
		if (debug < level)
			return;
		
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
		va_end(arglist);
		g_free(buffer);
		osync_trace(TRACE_INTERNAL, debugstr);
		printf("%s\n", debugstr);
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
