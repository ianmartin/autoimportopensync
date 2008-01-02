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

#ifndef OPENSYNC_SINK_INTERNALS_H_
#define OPENSYNC_SINK_INTERNALS_H_

#define OSYNC_SINK_TIMEOUT_TRANSPORT 	15000
#define OSYNC_SINK_TIMEOUT_SINGLEIO	10000
#define OSYNC_SINK_TIMEOUT_BATCHIO	180000

#define OSYNC_SINK_TIMEOUT_CONNECT 	OSYNC_SINK_TIMEOUT_TRANSPORT 
#define OSYNC_SINK_TIMEOUT_DISCONNECT 	OSYNC_SINK_TIMEOUT_TRANSPORT 
#define OSYNC_SINK_TIMEOUT_GETCHANGES	OSYNC_SINK_TIMEOUT_BATCHIO
#define OSYNC_SINK_TIMEOUT_COMMIT	OSYNC_SINK_TIMEOUT_SINGLEIO
#define OSYNC_SINK_TIMEOUT_BATCHCOMMIT	OSYNC_SINK_TIMEOUT_BATCHIO
#define OSYNC_SINK_TIMEOUT_COMMITTEDALL	OSYNC_SINK_TIMEOUT_BATCHIO 
#define OSYNC_SINK_TIMEOUT_SYNCDONE	OSYNC_SINK_TIMEOUT_SINGLEIO
#define OSYNC_SINK_TIMEOUT_READ		OSYNC_SINK_TIMEOUT_SINGLEIO
#define OSYNC_SINK_TIMEOUT_WRITE	OSYNC_SINK_TIMEOUT_SINGLEIO

typedef struct OSyncObjTypeSinkFunctionTimeouts {
	unsigned int connect;
	unsigned int disconnect;
	unsigned int get_changes;
	unsigned int commit;
	unsigned int batch_commit;
	unsigned int committed_all;
	unsigned int sync_done;
	unsigned int read;
	unsigned int write;
} OSyncObjTypeSinkFunctionTimeouts;

struct OSyncObjTypeSink {
	/** The format which can be synchronized by this sink */
	OSyncList *objformats;
	/** The functions to be called */
	OSyncObjTypeSinkFunctions functions;
	void *userdata;

	/** The timeout values of the sink functions */
	OSyncObjTypeSinkFunctionTimeouts timeout;
	
	/** The objtype type of this sink. In case of a main sink this is always NULL. */
	char *objtype;

	/** The status if this sink is allowed to write (commit) */
	osync_bool write;
	/** The status if this sink has a write function (commit) */
	osync_bool func_write;

	/** The status if this sink is allowed to read (single entries) */
	osync_bool read;
	/** The status if this sink has a read function (single entries) */
	osync_bool func_read;

	/** The status if this sink is allowed to get_changes (latest changed entries) */
	osync_bool getchanges; 
	/** The status if this sink has a get_changes function (latest changed entries) */
	osync_bool func_getchanges; 

	/** The status of this sink */
	osync_bool enabled;

	/** The request status of a slow-sync of this sink */
	osync_bool slowsync;

	int ref_count;
	/** List to pile up changes for batch commit */
	GList *commit_changes;
	GList *commit_contexts;
	osync_bool available;
};

#endif /*OPENSYNC_SINK_INTERNALS_H_*/
