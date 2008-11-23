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

#ifndef OPENSYNC_OBJTYPE_SINK_INTERNALS_H_
#define OPENSYNC_OBJTYPE_SINK_INTERNALS_H_

osync_bool osync_objtype_sink_get_function_read(OSyncObjTypeSink *sink);
void osync_objtype_sink_set_function_read(OSyncObjTypeSink *sink, osync_bool read);

osync_bool osync_objtype_sink_get_function_getchanges(OSyncObjTypeSink *sink);
void osync_objtype_sink_set_function_getchanges(OSyncObjTypeSink *sink, osync_bool getchanges);

osync_bool osync_objtype_sink_get_function_write(OSyncObjTypeSink *sink);
void osync_objtype_sink_set_function_write(OSyncObjTypeSink *sink, osync_bool write);

unsigned int osync_objtype_sink_get_connect_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_connect_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_disconnect_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_disconnect_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_getchanges_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_getchanges_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_commit_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_commit_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_batchcommit_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_batchcommit_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_committedall_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_committedall_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_syncdone_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_syncdone_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_write_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_write_timeout(OSyncObjTypeSink *sink);

unsigned int osync_objtype_sink_get_read_timeout_or_default(OSyncObjTypeSink *sink);
unsigned int osync_objtype_sink_get_read_timeout(OSyncObjTypeSink *sink);

#endif /*OPENSYNC_SINK_INTERNALS_H_*/

