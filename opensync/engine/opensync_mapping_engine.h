/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2006  Armin Bauer <armin.bauer@desscon.com>
 * Copyright (C) 2007       Daniel Gollub <dgollub@suse.de> 
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
 
#ifndef OPENSYNC_MAPPING_ENGINE_H_
#define OPENSYNC_MAPPING_ENGINE_H_

OSYNC_EXPORT int osync_mapping_engine_num_changes(OSyncMappingEngine *engine);
OSYNC_EXPORT OSyncChange *osync_mapping_engine_nth_change(OSyncMappingEngine *engine, int nth);
OSYNC_EXPORT OSyncChange *osync_mapping_engine_member_change(OSyncMappingEngine *engine, int memberid);
OSYNC_EXPORT OSyncMember *osync_mapping_engine_change_find_member(OSyncMappingEngine *engine, OSyncChange *change);

OSYNC_EXPORT osync_bool osync_mapping_engine_supports_ignore(OSyncMappingEngine *engine);
OSYNC_EXPORT osync_bool osync_mapping_engine_supports_use_latest(OSyncMappingEngine *engine);

OSYNC_EXPORT osync_bool osync_mapping_engine_solve(OSyncMappingEngine *engine, OSyncChange *change, OSyncError **error);
OSYNC_EXPORT osync_bool osync_mapping_engine_ignore(OSyncMappingEngine *engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_mapping_engine_use_latest(OSyncMappingEngine *engine, OSyncError **error);
OSYNC_EXPORT osync_bool osync_mapping_engine_duplicate(OSyncMappingEngine *existingMapping, OSyncError **error);

#endif /*OPENSYNC_MAPPING_ENGINE_H_*/
