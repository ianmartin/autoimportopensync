/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006 Tobias Koenig <tokoe@kde.org>
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

#ifndef _OPENSYNC_SERIALIZER_H_
#define _OPENSYNC_SERIALIZER_H_

osync_bool osync_marshal_data(OSyncMessage *message, OSyncData *data, OSyncError **error);
osync_bool osync_demarshal_data(OSyncMessage *message, OSyncData **data, OSyncFormatEnv *env, OSyncError **error);

osync_bool osync_marshal_change(OSyncMessage *message, OSyncChange *change, OSyncError **error);
osync_bool osync_demarshal_change(OSyncMessage *message, OSyncChange **change, OSyncFormatEnv *env, OSyncError **error);

void osync_marshal_error(OSyncMessage *message, OSyncError *error);
void osync_demarshal_error(OSyncMessage *message, OSyncError **error);

osync_bool osync_marshal_objformat_sink(OSyncMessage *message, OSyncObjFormatSink *sink, OSyncError **error);
osync_bool osync_demarshal_objformat_sink(OSyncMessage *message, OSyncObjFormatSink **sink, OSyncError **error);

osync_bool osync_marshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink *sink, OSyncError **error);
osync_bool osync_demarshal_objtype_sink(OSyncMessage *message, OSyncObjTypeSink **sink, OSyncError **error);

osync_bool osync_marshal_pluginconfig(OSyncMessage *message, OSyncPluginConfig *config, OSyncError **error);
osync_bool osync_demarshal_pluginconfig(OSyncMessage *message, OSyncPluginConfig **config, OSyncError **error);

#endif
