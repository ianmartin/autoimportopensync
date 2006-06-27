/*
 * libosengine - A synchronization engine for the opensync framework
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

#include "opensync-client.h"
#include "opensync-engine.h"
#include "opensync-group.h"
#include "opensync-data.h"

#include "opensync_engine_internals.h"

/* Implementation of g_mkdir_with_parents()
 *
 * This function overwrite the contents of the 'dir' parameter
 */
static int __mkdir_with_parents(char *dir, int mode)
{
	if (g_file_test(dir, G_FILE_TEST_IS_DIR))
		return 0;

	char *slash = strrchr(dir, '/');
	if (slash && slash != dir) {
		/* Create parent directory if needed */

		/* This is a trick: I don't want to allocate a new string
		 * for the parent directory. So, just put a NUL char
		 * in the last slash, and restore it after creating the
		 * parent directory
		 */
		*slash = '\0';
		if (__mkdir_with_parents(dir, mode) < 0)
			return -1;
		*slash = '/';
	}

	if (mkdir(dir, mode) < 0)
		return -1;

	return 0;
}

static int mkdir_with_parents(const char *dir, int mode)
{
	int r;
	char *mydir = strdup(dir);
	if (!mydir)
		return -1;

	r = __mkdir_with_parents(mydir, mode);
	free(mydir);
	return r;
}

osync_bool waiting = TRUE;
static void _finalize_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	waiting = FALSE;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

		
/*! @brief This will create a new engine for the given group
 * 
 * This will create a new engine for the given group
 * 
 * @param group A pointer to the group, for which you want to create a new engine
 * @param error A pointer to a error struct
 * @returns Pointer to a newly allocated OSyncEngine on success, NULL otherwise
 * 
 */
OSyncEngine *osync_engine_new(OSyncGroup *group, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, error);
	g_assert(group);
	
	OSyncEngine *engine = osync_try_malloc0(sizeof(OSyncEngine), error);
	if (!engine)
		goto error;
	engine->ref_count = 1;
	
	if (!g_thread_supported ())
		g_thread_init (NULL);
	
	engine->context = g_main_context_new();
	engine->thread = osync_thread_new(engine->context, error);
	if (!engine->thread)
		goto error_free_engine;
	
	engine->group = group;
	osync_group_ref(group);

	char *enginesdir = g_strdup_printf("%s/.opensync/engines", g_get_home_dir());
	engine->engine_path = g_strdup_printf("%s/enginepipe", enginesdir);

	if (mkdir_with_parents(enginesdir, 0755) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't create engines directory: %s", strerror(errno));
		g_free(enginesdir);
		goto error_free_engine;
	}
	g_free(enginesdir);
	
	engine->syncing_mutex = g_mutex_new();
	engine->syncing = g_cond_new();
	
	engine->started_mutex = g_mutex_new();
	engine->started = g_cond_new();
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, engine);
	return engine;

error_free_engine:
	osync_engine_unref(engine);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

void osync_engine_ref(OSyncEngine *engine)
{
	osync_assert(engine);
	
	g_atomic_int_inc(&(engine->ref_count));
}

void osync_engine_unref(OSyncEngine *engine)
{
	osync_assert(engine);
		
	if (g_atomic_int_dec_and_test(&(engine->ref_count))) {
		if (engine->group)
			osync_group_unref(engine->group);
		
		if (engine->engine_path)
			g_free(engine->engine_path);
			
		if (engine->plugin_dir)
			g_free(engine->plugin_dir);
		
		if (engine->thread)
			osync_thread_free(engine->thread);
			
		if (engine->context)
			g_main_context_unref(engine->context);
			
		if (engine->syncing)
			g_cond_free(engine->syncing);
			
		if (engine->syncing_mutex)
			g_mutex_free(engine->syncing_mutex);
			
		if (engine->started)
			g_cond_free(engine->started);
			
		if (engine->started_mutex)
			g_mutex_free(engine->started_mutex);
	
		g_free(engine);
	}
}

void osync_engine_set_plugindir(OSyncEngine *engine, const char *dir)
{
	osync_assert(engine);
	if (engine->plugin_dir)
		g_free(engine->plugin_dir);
	engine->plugin_dir = g_strdup(dir);
}

osync_bool osync_engine_initialize(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	if (engine->state != OSYNC_ENGINE_STATE_UNINITIALIZED) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "This engine was not uninitialized: %i", engine->state);
		goto error;
	}
	
	OSyncGroup *group = engine->group;
	
	if (osync_group_num_members(group) < 2) {
		//Not enough members!
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "You only configured %i members, but at least 2 are needed", osync_group_num_members(group));
		goto error;
	}
	
	switch (osync_group_lock(group)) {
		case OSYNC_LOCKED:
			osync_error_set(error, OSYNC_ERROR_LOCKED, "Group is locked");
			goto error;
		case OSYNC_LOCK_STALE:
			osync_trace(TRACE_INTERNAL, "Detected stale lock file. Slow-syncing");
			//osync_status_update_engine(engine, ENG_PREV_UNCLEAN, NULL);
			//osync_group_set_slow_sync(engine->group, "data", TRUE);
			break;
		case OSYNC_LOCK_OK:
			break;
	}
	
	engine->state = OSYNC_ENGINE_STATE_INITIALIZED;
	
	osync_trace(TRACE_INTERNAL, "Running the main loop");
	osync_thread_start(engine->thread);
	
	osync_trace(TRACE_INTERNAL, "Spawning clients");
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		
		OSyncClientProxy *proxy = osync_client_proxy_new(error);
		if (!proxy)
			goto error_finalize;
			
		osync_client_proxy_set_context(proxy, engine->context);
	
		if (!osync_client_proxy_spawn(proxy, osync_member_get_start_type(member), osync_member_get_configdir(member), error))
			goto error_finalize;
		waiting = TRUE;
		
		if (!osync_client_proxy_initialize(proxy, _finalize_callback, engine, NULL, engine->plugin_dir, osync_member_get_pluginname(member), error))
			goto error_finalize;
		
		//FIXME
		while (waiting) { usleep(100); }
		
		engine->proxies = g_list_append(engine->proxies, proxy);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_finalize:
	osync_engine_finalize(engine, NULL);
	osync_group_unlock(engine->group);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_engine_finalize(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	
	if (engine->state != OSYNC_ENGINE_STATE_INITIALIZED) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "This engine was not in state initialized: %i", engine->state);
		goto error;
	}
	
	engine->state = OSYNC_ENGINE_STATE_UNINITIALIZED;
	
	OSyncClientProxy *proxy = NULL;
	while (engine->proxies) {
		proxy = engine->proxies->data;
		
		waiting = TRUE;
		
		if (!osync_client_proxy_finalize(proxy, _finalize_callback, engine, error))
			goto error;
		
		//FIXME
		while (waiting) { usleep(100); }
		
		if (!osync_client_proxy_shutdown(proxy, error))
			goto error;
		
		osync_client_proxy_unref(proxy);
	
		engine->proxies = g_list_remove(engine->proxies, proxy);
	}
	
	osync_thread_stop(engine->thread);
	
	osync_group_unlock(engine->group);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/* 
	*/
