/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008       Daniel Gollub <dgollub@suse.de>
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

#ifndef _OPENSYNC_OBJFORMAT_SINK_H_
#define _OPENSYNC_OBJFORMAT_SINK_H_

OSYNC_EXPORT OSyncObjFormatSink *osync_objformat_sink_new(const char *objformat, OSyncError **error);
OSYNC_EXPORT OSyncObjFormatSink *osync_objformat_sink_ref(OSyncObjFormatSink *format);
OSYNC_EXPORT void osync_objformat_sink_unref(OSyncObjFormatSink *format);

OSYNC_EXPORT const char *osync_objformat_sink_get_objformat(OSyncObjFormatSink *format);
OSYNC_EXPORT const char *osync_objformat_sink_get_config(OSyncObjFormatSink *format);
OSYNC_EXPORT void osync_objformat_sink_set_config(OSyncObjFormatSink *format, const char *format_config);

#endif //_OPENSYNC_OBJFORMAT_SINK_H_
