/*
 * libopensync - A synchronization engine for the opensync framework
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
 
#ifndef OPENSYNC_ENGINE_INTERNALS_H_
#define OPENSYNC_ENGINE_INTERNALS_H_

osync_bool osync_engine_check_get_changes(OSyncEngine *engine);

void osync_engine_event(OSyncEngine *engine, OSyncEngineEvent event);

OSyncClientProxy *osync_engine_find_proxy(OSyncEngine *engine, OSyncMember *member);

OSyncArchive *osync_engine_get_archive(OSyncEngine *engine);
OSyncGroup *osync_engine_get_group(OSyncEngine *engine);

OSyncObjEngine *osync_engine_nth_objengine(OSyncEngine *engine, int nth);
int osync_engine_num_objengine(OSyncEngine *engine);

OSyncClientProxy *osync_engine_nth_proxy(OSyncEngine *engine, int nth);
int osync_engine_num_proxies(OSyncEngine *engine);

void osync_engine_set_formatdir(OSyncEngine *engine, const char *dir);
void osync_engine_set_plugindir(OSyncEngine *engine, const char *dir);

#ifdef OPENSYNC_UNITTESTS
void osync_engine_set_schemadir(OSyncEngine *engine, const char *schema_dir);
#endif /* OPENSYNC_UNITTESTS */

#endif /*OPENSYNC_ENGINE_INTERNALS_H_*/
