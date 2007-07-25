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

#include "opensync-ipc.h"
#include "opensync-client.h"

static void usage (int ecode)
{
  fprintf (stderr, "This is the opensync sync client.\n\n");
  fprintf (stderr, "The purpose of this tool is,to allow plugin to run in its own process\n");
  fprintf (stderr, "It is normally started by OpenSync. But you can start it yourself, for\n");
  fprintf (stderr, "debugging purposes (in gdb for example).\n\n");
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "osplugin <PipePath>\n\n");
  fprintf (stderr, "The pipe path tells the plugin, where to create the listing pipe.\n");
  fprintf (stderr, "The pipe is normally created in the member directory with the name\n");
  fprintf (stderr, "\"pluginpipe\"\n");
  exit (ecode);
}

/** The setup process for the client is as follows:
 * 
 * 3 cases:
 * a) client with anonymous pipes and threads
 * b) client with anonymous pipes and fork
 * c) client with named pipes from external
 * 
 * For a) and b), our pipes will already be connected. In the init message,
 * the engine will send a NULL string for its queue path.
 * 
 * For c), we will get the place where we are supposed to create our pipe. We will
 * listen there until the engine tells us its locations in the init message */
int main(int argc, char **argv)
{
	osync_trace(TRACE_ENTRY, "%s(%i, %p)", __func__, argc, argv);
	
	if (argc != 2 && argc != 4)
		usage(0);
	
	osync_bool usePipes = FALSE;
	OSyncError *error = NULL;
	char *pipe_path = NULL;
	int read_fd = 0;
	int write_fd = 0;
	OSyncQueue *incoming = NULL;
	OSyncQueue *outgoing = NULL;
	
	OSyncClient *client = osync_client_new(&error);
	if (!client)
		goto error;
	
	if (!strcmp (argv[0], "-f")) {
		usePipes = TRUE;
		/* We cannot preserve the knowledge about the fds through the execve call.
		 * Therefore, we pass the fd numbers through the args */
		read_fd = atoi(argv[1]);
		write_fd = atoi(argv[2]);
	} else
		pipe_path = argv[0];
	
	if (usePipes) {
		/* We are using anonymous pipes. */
		incoming = osync_queue_new_from_fd(read_fd, &error);
		if (!incoming)
			goto error;
		
		outgoing = osync_queue_new_from_fd(write_fd, &error);
		if (!outgoing)
			goto error;
		
		/* We now connect to our incoming queue */
		if (!osync_queue_connect(incoming, OSYNC_QUEUE_RECEIVER, &error))
			goto error;
		
		/* and the to the outgoing queue */
		if (!osync_queue_connect(outgoing, OSYNC_QUEUE_SENDER, &error))
			goto error;
		
		osync_client_set_incoming_queue(client, incoming);
		osync_client_set_outgoing_queue(client, outgoing);
	} else {
		/* Create connection pipes **/
		incoming = osync_queue_new(pipe_path, &error);
		if (!incoming)
			goto error;
		
		if (!osync_queue_create(incoming, &error))
			goto error;
		
		/* We now connect to our incoming queue */
		if (!osync_queue_connect(incoming, OSYNC_QUEUE_RECEIVER, &error))
			goto error;
		
		osync_client_set_incoming_queue(client, incoming);
	}

	osync_client_run_and_block(client);
	
	osync_client_unref(client);
	
	osync_queue_free(incoming);
	osync_queue_free(outgoing);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return 0;

error:
	osync_client_unref(client);
	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error) ? osync_error_print(&error) : "nil");
	fprintf(stderr, "Unable to initialize environment: %s\n", osync_error_print(&error));
	osync_error_unref(&error);
	return 1;
}
