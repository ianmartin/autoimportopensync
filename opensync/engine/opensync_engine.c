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
#include "opensync-format.h"
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

static gboolean _command_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

static gboolean _command_check(GSource *source)
{
	OSyncEngine *engine = *((OSyncEngine **)(source + 1));
	if (g_async_queue_length(engine->command_queue) > 0)
		return TRUE;
	
	return FALSE;
}

/* This function is called from the master thread. The function dispatched incoming data from
 * the remote end */
static gboolean _command_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	OSyncEngine *engine = user_data;
	void *command = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, user_data);
	
	while ((command = g_async_queue_try_pop(engine->command_queue))) {
		/* We check if the message is a reply to something */
		osync_trace(TRACE_INTERNAL, "Dispatching %p", command);
		
		osync_engine_command(engine, GPOINTER_TO_INT(command));
	}
	
	osync_trace(TRACE_EXIT, "%s: Done dispatching", __func__);
	return TRUE;
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

	engine->command_queue = g_async_queue_new();

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
			
		if (engine->format_dir)
			g_free(engine->format_dir);
		
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
		
		if (engine->command_queue)
			g_async_queue_unref(engine->command_queue);
	
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

void osync_engine_set_formatdir(OSyncEngine *engine, const char *dir)
{
	osync_assert(engine);
	if (engine->format_dir)
		g_free(engine->format_dir);
	engine->format_dir = g_strdup(dir);
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
	
	engine->formatenv = osync_format_env_new(error);
	if (!engine->formatenv)
		goto error;
		
	engine->state = OSYNC_ENGINE_STATE_INITIALIZED;
	
	if (!osync_format_env_load_plugins(engine->formatenv, engine->format_dir, error))
		goto error_finalize;
	
	osync_trace(TRACE_INTERNAL, "Running the main loop");
	/* Now we attach a queue to the engine which handles our commands */
	engine->command_functions = g_malloc0(sizeof(GSourceFuncs));
	engine->command_functions->prepare = _command_prepare;
	engine->command_functions->check = _command_check;
	engine->command_functions->dispatch = _command_dispatch;
	engine->command_functions->finalize = NULL;

	engine->command_source = g_source_new(engine->command_functions, sizeof(GSource) + sizeof(OSyncEngine *));
	OSyncEngine **engineptr = (OSyncEngine **)(engine->command_source + 1);
	*engineptr = engine;
	g_source_set_callback(engine->command_source, NULL, engine, NULL);
	g_source_attach(engine->command_source, engine->context);
	g_main_context_ref(engine->context);
	
	osync_thread_start(engine->thread);
	
	osync_trace(TRACE_INTERNAL, "Spawning clients");
	int i;
	for (i = 0; i < osync_group_num_members(group); i++) {
		OSyncMember *member = osync_group_nth_member(group, i);
		
		OSyncClientProxy *proxy = osync_client_proxy_new(engine->formatenv, error);
		if (!proxy)
			goto error_finalize;
			
		osync_client_proxy_set_context(proxy, engine->context);
	
		if (!osync_client_proxy_spawn(proxy, osync_member_get_start_type(member), osync_member_get_configdir(member), error))
			goto error_finalize;
		waiting = TRUE;
		
		if (!osync_client_proxy_initialize(proxy, _finalize_callback, engine, engine->format_dir, engine->plugin_dir, osync_member_get_pluginname(member), error))
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
	
	g_source_unref(engine->command_source);
	
	g_free(engine->command_functions);
	engine->command_functions = NULL;
	
	osync_format_env_free(engine->formatenv);
		
	engine->state = OSYNC_ENGINE_STATE_INITIALIZED;
	
	
	osync_group_unlock(engine->group);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static int BitCount(unsigned int u)                          
{
	unsigned int uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
	return ((uCount + (uCount >> 3)) & 030707070707) % 63;
}

static void _generate_connected_event(OSyncEngine *engine)
{
	if (BitCount(engine->proxy_errors | engine->proxy_connects) != g_list_length(engine->proxies))
		return;
	
	if (BitCount(engine->obj_errors | engine->obj_connects) == g_list_length(engine->object_engines)) {
		osync_engine_event(engine, OSYNC_ENGINE_EVENT_CONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->obj_errors | engine->obj_connects));
}

static void _generate_get_changes_event(OSyncEngine *engine)
{
	if (BitCount(engine->proxy_errors | engine->proxy_get_changes) != g_list_length(engine->proxies))
		return;
	
	if (BitCount(engine->obj_errors | engine->obj_get_changes) == g_list_length(engine->object_engines)) {
		osync_engine_event(engine, OSYNC_ENGINE_EVENT_READ);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->obj_errors | engine->obj_get_changes));
}

static void _generate_written_event(OSyncEngine *engine)
{
	if (BitCount(engine->proxy_errors | engine->proxy_written) != g_list_length(engine->proxies))
		return;
	
	if (BitCount(engine->obj_errors | engine->obj_written) == g_list_length(engine->object_engines)) {
		osync_engine_event(engine, OSYNC_ENGINE_EVENT_WRITTEN);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->obj_errors | engine->obj_written));
}

static void _generate_sync_done_event(OSyncEngine *engine)
{
	if (BitCount(engine->proxy_errors | engine->proxy_sync_done) != g_list_length(engine->proxies))
		return;
	
	if (BitCount(engine->obj_errors | engine->obj_sync_done) == g_list_length(engine->object_engines)) {
		osync_engine_event(engine, OSYNC_ENGINE_EVENT_SYNC_DONE);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->obj_errors | engine->obj_sync_done));
}

static void _generate_disconnected_event(OSyncEngine *engine)
{
	if (BitCount(engine->proxy_errors | engine->proxy_disconnects) != g_list_length(engine->proxies))
		return;
	
	if (BitCount(engine->obj_errors | engine->obj_disconnects) == g_list_length(engine->object_engines)) {
		osync_engine_event(engine, OSYNC_ENGINE_EVENT_DISCONNECTED);
	} else
		osync_trace(TRACE_INTERNAL, "Not yet: %i", BitCount(engine->obj_errors | engine->obj_disconnects));
}

static void _engine_connect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->proxies; e; e = e->next) {
		if (e->data == proxy)
			break;
		i++;
	}
	
	if (error) {
		engine->proxy_errors = engine->proxy_errors | (0x1 << i);
		engine->error = error;
		osync_error_ref(&error);
	} else {
		engine->proxy_connects = engine->proxy_connects | (0x1 << i);
	}
	
	_generate_connected_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _engine_disconnect_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->proxies; e; e = e->next) {
		if (e->data == proxy)
			break;
		i++;
	}
	
	if (error) {
		engine->proxy_errors = engine->proxy_errors | (0x1 << i);
		engine->error = error;
		osync_error_ref(&error);
	} else {
		engine->proxy_disconnects = engine->proxy_disconnects | (0x1 << i);
	}
	
	_generate_disconnected_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _engine_get_changes_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->proxies; e; e = e->next) {
		if (e->data == proxy)
			break;
		i++;
	}
	
	if (error) {
		engine->proxy_errors = engine->proxy_errors | (0x1 << i);
		engine->error = error;
		osync_error_ref(&error);
	} else {
		engine->proxy_get_changes = engine->proxy_get_changes | (0x1 << i);
	}
	
	_generate_get_changes_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _engine_written_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->proxies; e; e = e->next) {
		if (e->data == proxy)
			break;
		i++;
	}
	
	if (error) {
		engine->proxy_errors = engine->proxy_errors | (0x1 << i);
		engine->error = error;
		osync_error_ref(&error);
	} else {
		engine->proxy_written = engine->proxy_written | (0x1 << i);
	}
	
	_generate_written_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _engine_sync_done_callback(OSyncClientProxy *proxy, void *userdata, OSyncError *error)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, proxy, userdata, error);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->proxies; e; e = e->next) {
		if (e->data == proxy)
			break;
		i++;
	}
	
	if (error) {
		engine->proxy_errors = engine->proxy_errors | (0x1 << i);
		engine->error = error;
		osync_error_ref(&error);
	} else {
		engine->proxy_sync_done = engine->proxy_sync_done | (0x1 << i);
	}
	
	_generate_sync_done_event(engine);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void _engine_event_callback(OSyncObjEngine *objengine, OSyncEngineEvent event, OSyncError *error, void *userdata)
{
	OSyncEngine *engine = userdata;
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p)", __func__, objengine, event, error, userdata);
	
	int i = 0;
	GList *e = NULL;
	for (e = engine->object_engines; e; e = e->next) {
		if (e->data == objengine)
			break;
		i++;
	}
	
	switch (event) {
		case OSYNC_ENGINE_EVENT_CONNECTED:
			engine->obj_connects = engine->obj_connects | (0x1 << i);
			_generate_connected_event(engine);
			break;
		case OSYNC_ENGINE_EVENT_ERROR:
			engine->obj_errors = engine->obj_errors | (0x1 << i);
			break;
		case OSYNC_ENGINE_EVENT_READ:
			engine->obj_get_changes = engine->obj_get_changes | (0x1 << i);
			_generate_get_changes_event(engine);
			break;
		case OSYNC_ENGINE_EVENT_WRITTEN:
			engine->obj_written = engine->obj_written | (0x1 << i);
			_generate_written_event(engine);
			break;
		case OSYNC_ENGINE_EVENT_SYNC_DONE:
			engine->obj_sync_done = engine->obj_sync_done | (0x1 << i);
			_generate_sync_done_event(engine);
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECTED:
			engine->obj_disconnects = engine->obj_disconnects | (0x1 << i);
			_generate_disconnected_event(engine);
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void osync_engine_command(OSyncEngine *engine, OSyncEngineCommand command)
{
	GList *o = NULL;
	int num = 0;
	int i;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, engine, command);
	osync_assert(engine);
	
	switch (command) {
		case OSYNC_ENGINE_COMMAND_CONNECT:
			/* Lets see which objtypes are synchronizable in this group */
			num = osync_group_num_objtypes(engine->group);
			if (num == 0) {
				osync_error_set(&engine->error, OSYNC_ERROR_GENERIC, "No synchronizable objtype");
				goto error;
			}
			
			for (i = 0; i < num; i++) {
				const char *objtype = osync_group_nth_objtype(engine->group, i);
				OSyncObjEngine *objengine = osync_obj_engine_new(engine, objtype, &engine->error);
				if (!objengine)
					goto error;
				osync_obj_engine_set_callback(objengine, _engine_event_callback, engine);
				engine->object_engines = g_list_append(engine->object_engines, objengine);
			}
		
			/* We first tell all object engines to connect */
			for (o = engine->object_engines; o; o = o->next) {
				OSyncObjEngine *objengine = o->data;
				if (!osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_CONNECT, &engine->error))
					goto error;
			}
			
			/* Then we connect ourselves */
			for (o = engine->proxies; o; o = o->next) {
				OSyncClientProxy *proxy = o->data;
				if (!osync_client_proxy_connect(proxy, _engine_connect_callback, engine, NULL, &engine->error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_COMMAND_READ:
		case OSYNC_ENGINE_COMMAND_WRITE:
		case OSYNC_ENGINE_COMMAND_DISCONNECT:
		case OSYNC_ENGINE_COMMAND_SYNC_DONE:
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_mutex_lock(engine->syncing_mutex);
	g_cond_signal(engine->syncing);
	g_mutex_unlock(engine->syncing_mutex);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&engine->error));
	osync_error_ref(&engine->error);
}

void osync_engine_event(OSyncEngine *engine, OSyncEngineEvent event)
{
	GList *o = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, engine, event);
	osync_assert(engine);
	
	switch (event) {
		case OSYNC_ENGINE_EVENT_CONNECTED:
			/* Now that we are connected, we read the changes */
			for (o = engine->object_engines; o; o = o->next) {
				OSyncObjEngine *objengine = o->data;
				if (!osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_READ, &engine->error))
					goto error;
			}
			
			/* Now we read the main sink */
			for (o = engine->proxies; o; o = o->next) {
				OSyncClientProxy *proxy = o->data;
				if (!osync_client_proxy_get_changes(proxy, _engine_get_changes_callback, NULL, engine, NULL, &engine->error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_EVENT_READ:
			/* Now that we are connected, we read the changes */
			for (o = engine->object_engines; o; o = o->next) {
				OSyncObjEngine *objengine = o->data;
				if (!osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_WRITE, &engine->error))
					goto error;
			}
			
			/* Now we write the main sink */
			for (o = engine->proxies; o; o = o->next) {
				OSyncClientProxy *proxy = o->data;
				if (!osync_client_proxy_committed_all(proxy, _engine_written_callback, engine, NULL, &engine->error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_EVENT_WRITTEN:
			/* Lets call sync done */
			for (o = engine->object_engines; o; o = o->next) {
				OSyncObjEngine *objengine = o->data;
				if (!osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_SYNC_DONE, &engine->error))
					goto error;
			}
			
			/* Now we call sync done on the main sink */
			for (o = engine->proxies; o; o = o->next) {
				OSyncClientProxy *proxy = o->data;
				if (!osync_client_proxy_sync_done(proxy, _engine_sync_done_callback, engine, NULL, &engine->error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_EVENT_SYNC_DONE:
			/* Lets disconnect */
			for (o = engine->object_engines; o; o = o->next) {
				OSyncObjEngine *objengine = o->data;
				if (!osync_obj_engine_command(objengine, OSYNC_ENGINE_COMMAND_DISCONNECT, &engine->error))
					goto error;
			}
			
			/* Now we disconnect the main sink */
			for (o = engine->proxies; o; o = o->next) {
				OSyncClientProxy *proxy = o->data;
				if (!osync_client_proxy_disconnect(proxy, _engine_disconnect_callback, engine, NULL, &engine->error))
					goto error;
			}
			break;
		case OSYNC_ENGINE_EVENT_DISCONNECTED:
			
			while (engine->object_engines) {
				OSyncObjEngine *objengine = engine->object_engines->data;
				osync_obj_engine_unref(objengine);
				engine->object_engines = g_list_remove(engine->object_engines, engine->object_engines->data);
			}
			
			engine->proxy_connects = 0;
			engine->proxy_disconnects = 0;
			engine->proxy_get_changes = 0;
			engine->proxy_written = 0;
			engine->proxy_errors = 0;
			engine->proxy_sync_done = 0;
			
			engine->obj_errors = 0;
			engine->obj_connects = 0;
			engine->obj_disconnects = 0;
			engine->obj_get_changes = 0;
			engine->obj_written = 0;
			engine->obj_sync_done = 0;
			
			g_mutex_lock(engine->syncing_mutex);
			g_cond_signal(engine->syncing);
			g_mutex_unlock(engine->syncing_mutex);
			break;
		case OSYNC_ENGINE_EVENT_ERROR:
			break;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_mutex_lock(engine->syncing_mutex);
	g_cond_signal(engine->syncing);
	g_mutex_unlock(engine->syncing_mutex);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&engine->error));
	osync_error_ref(&engine->error);
}

/*! @brief Starts to synchronize the given OSyncEngine
 *
 * This function synchronizes a given engine. The Engine has to be created
 * from a OSyncGroup before by using osengine_new(). This function will not block
 * 
 * @param engine A pointer to the engine, which will be used to sync
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise. Check the error on FALSE. Note that this just says if the sync has been started successfully, not if the sync itself was successful
 * 
 */
osync_bool osync_engine_synchronize(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, engine);
	osync_assert(engine);
	
	if (engine->state != OSYNC_ENGINE_STATE_INITIALIZED) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "This engine was not in state initialized: %i", engine->state);
		goto error;
	}
	
	g_async_queue_push(engine->command_queue, GINT_TO_POINTER(OSYNC_ENGINE_COMMAND_CONNECT));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*! @brief This function will synchronize once and block until the sync has finished
 *
 * This can be used to sync a group and wait for the synchronization end. DO NOT USE
 * osengine_wait_sync_end for this as this might introduce a race condition.
 * 
 * @param engine A pointer to the engine, which to sync and wait for the sync end
 * @param error A pointer to a error struct
 * @returns TRUE on success, FALSE otherwise.
 * 
 */
osync_bool osync_engine_synchronize_and_block(OSyncEngine *engine, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, engine, error);
	
	g_mutex_lock(engine->syncing_mutex);
	
	if (!osync_engine_synchronize(engine, error)) {
		g_mutex_unlock(engine->syncing_mutex);
		goto error;
	}
	
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
	
	if (engine->error) {
		osync_error_duplicate(error, &(engine->error));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/*! @brief This function will block until a synchronization has ended
 *
 * This can be used to wait until the synchronization has ended. Note that this function will always
 * block until 1 sync has ended. It can be used before the sync has started, to wait for one auto-sync
 * to end
 * 
 * @param engine A pointer to the engine, for which to wait for the sync end
 * @param error Return location for the error if the sync was not successful
 * @returns TRUE on success, FALSE otherwise.
 */
osync_bool osync_engine_wait_sync_end(OSyncEngine *engine, OSyncError **error)
{
	g_mutex_lock(engine->syncing_mutex);
	g_cond_wait(engine->syncing, engine->syncing_mutex);
	g_mutex_unlock(engine->syncing_mutex);
	
	if (engine->error) {
		osync_error_duplicate(error, &(engine->error));
		return FALSE;
	}
	return TRUE;
}

int osync_engine_num_proxies(OSyncEngine *engine)
{
	osync_assert(engine);
	return g_list_length(engine->proxies);
}

OSyncClientProxy *osync_engine_nth_proxy(OSyncEngine *engine, int nth)
{
	osync_assert(engine);
	return g_list_nth_data(engine->proxies, nth);
}
