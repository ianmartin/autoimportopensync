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

#define TYPE_OSYNC_CHANGE 1
#define TYPE_OSYNC_MEMBER 2

int osync_marshal_get_size_changetype( OSyncChangeType changetype );
void osync_marshal_changetype( OSyncMessage *message, OSyncChangeType changetype );
void osync_demarshal_changetype( OSyncMessage *message, OSyncChangeType *changetype );

int osync_marshal_get_size_change( OSyncChange *change );
void osync_marshal_change( OSyncMessage *message, OSyncChange *change );
void osync_demarshal_change( OSyncMessage *message, OSyncFormatEnv *conv_env, OSyncChange **change );

int osync_marshal_get_size_member( OSyncMember *member );
void osync_marshal_member( OSyncMessage *message, OSyncMember *member );
void osync_demarshal_member( OSyncMessage *message, OSyncMember **member );

int osync_marshal_get_size_error( OSyncError **error );
void osync_marshal_error( OSyncMessage *message, OSyncError *error );
void osync_demarshal_error( OSyncMessage *message, OSyncError **error );

int osync_marshal_get_size_message( OSyncMessage *message );

#endif
