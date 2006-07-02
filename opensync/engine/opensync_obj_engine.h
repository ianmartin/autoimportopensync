/*
 * libosync_engine - A synchronization engine for the opensync framework
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
 
#ifndef OPENSYNC_OBJ_ENGINE_H_
#define OPENSYNC_OBJ_ENGINE_H_

typedef struct OSyncObjEngine OSyncObjEngine;

typedef void (* OSyncObjEngineEventCallback) (OSyncObjEngine *engine, OSyncEngineEvent event, OSyncError *error, void *userdata);

OSyncObjEngine *osync_obj_engine_new(OSyncEngine *engine, const char *objtype, OSyncError **error);
void osync_obj_engine_ref(OSyncObjEngine *engine);
void osync_obj_engine_unref(OSyncObjEngine *engine);

void osync_obj_engine_event(OSyncObjEngine *objengine, OSyncEngineEvent event);
osync_bool osync_obj_engine_command(OSyncObjEngine *engine, OSyncEngineCommand cmd, OSyncError **error);
void osync_obj_engine_set_callback(OSyncObjEngine *engine, OSyncObjEngineEventCallback callback, void *userdata);
				
void osync_obj_engine_set_error(OSyncObjEngine *engine, OSyncError *error);

#endif /*OPENSYNC_OBJ_ENGINE_H_*/
