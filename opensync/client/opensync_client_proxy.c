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

#include "opensync-data.h"
#include "opensync-ipc.h"
#include "opensync-serializer.h"
#include "opensync-client.h"
#include "opensync-group.h"
#include "opensync-plugin.h"
#include "opensync-merger.h"
#include "opensync-version.h"
 
#include "opensync_client_proxy.h"
#include "opensync_client_proxy_internals.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef _WIN32
/* Historical signals specified by POSIX. */
#define SIGKILL 9   /* Kill (cannot be blocked, caught, or ignored).  */
#define SIGTERM 15	/* can be caught and interpreted or ignored by the process */
#endif //_WIN32

typedef struct callContext {
	OSyncClientProxy *proxy;
	
	initialize_cb init_callback;
	void *init_callback_data;
	
	finalize_cb fin_callback;
	void *fin_callback_data;
	
	discover_cb discover_callback;
	void *discover_callback_data;
	
	connect_cb connect_callback;
	void *connect_callback_data;
	
	disconnect_cb disconnect_callback;
	void *disconnect_callback_data;

	read_cb read_callback;
	void *read_callback_data;
	
	get_changes_cb get_changes_callback;
	void *get_changes_callback_data;
	
	commit_change_cb commit_change_callback;
	void *commit_change_callback_data;
	
	committed_all_cb committed_all_callback;
	void *committed_all_callback_data;
	
	sync_done_cb sync_done_callback;
	void *sync_done_callback_data;
} callContext;

//portable kill pid helper
/*static int _osync_kill(pid_t pid, int sig) 
{
#ifndef _WIN32
	return kill(pid, sig);
#else //_WIN32
	int ret = -1;
	DWORD dwExitCode = 0;

	HANDLE hProc = OpenProcess(1, 0, pid);
	GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid);
	WaitForSingleObject(hProc, 3000);
	GetExitCodeProcess(hProc, &dwExitCode);
	
	if(dwExitCode != STILL_ACTIVE) {
		ret = 0;
		goto end;
	}
	if (sig == SIGKILL) {
		if (TerminateProcess(hProc, 0))
			ret = 0;
		else
			ret = -1;
	}
end:
	CloseHandle(hProc);
	return (ret);
#endif //_WIN32
}*/

/*static char *_osync_client_pid_filename(OSyncClientProxy *proxy)
{
	return g_strdup_printf("%s/osplugin.pid", proxy->path);
}*/

/*static osync_bool osync_client_remove_pidfile(OSyncClientProxy *proxy, OSyncError **error)
{
	char *pidpath = _osync_client_pid_filename(proxy);

	if (unlink(pidpath) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't remove pid file: %s", g_strerror(errno));
		g_free(pidpath);
		return FALSE;
	}
	
	g_free(pidpath);
	return TRUE;
}*/

/*static osync_bool _osync_client_create_pidfile(OSyncClientProxy *proxy, OSyncError **error)
{
	char *pidpath = _osync_client_pid_filename(proxy);
	char *pidstr = g_strdup_printf("%ld", (long)proxy->child_pid);

	if (!osync_file_write(pidpath, pidstr, strlen(pidstr), 0644, error)) {
		g_free(pidstr);
		g_free(pidpath);
		return FALSE;
	}

	g_free(pidstr);
	g_free(pidpath);
	return TRUE;
}*/

/*static osync_bool _osync_client_kill_old_osplugin(OSyncClientProxy *proxy, OSyncError **error)
{
	osync_bool ret = FALSE;

	char *pidstr;
	unsigned int pidlen;
	pid_t pid;

	char *pidpath = _osync_client_pid_filename(proxy);

	// Simply returns if there is no PID file
	if (!g_file_test(pidpath, G_FILE_TEST_EXISTS)) {
		ret = TRUE;
		goto out_free_path;
	}

	if (!osync_file_read(pidpath, &pidstr, &pidlen, error))
		goto out_free_path;

	pid = atol(pidstr);
	if (!pid)
		goto out_free_str;

	osync_trace(TRACE_INTERNAL, "Killing old osplugin process. PID: %ld", (long)pid);

	if (_osync_kill(pid, SIGTERM) < 0) {
		osync_trace(TRACE_INTERNAL, "Error killing old osplugin: %s. Stale pid file?", g_strerror(errno));
		// Don't return failure if kill() failed, because it may be a stale pid file
	}

	int count = 0;
	while (osync_queue_is_alive(proxy->outgoing)) {
		if (count++ > 10) {
			osync_trace(TRACE_INTERNAL, "Killing old osplugin process with SIGKILL");
			_osync_kill(pid, SIGKILL);
			break;
		}
		osync_trace(TRACE_INTERNAL, "Waiting for other side to terminate");
		// FIXME: Magic numbers are evil
		g_usleep(500000);
	}

	if (unlink(pidpath) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't erase PID file: %s", g_strerror(errno));
		goto out_free_str;
	}

	// Success
	ret = TRUE;

out_free_str:
	g_free(pidstr);
out_free_path:
	g_free(pidpath);
//out:
	return ret;
}*/


/** This function takes care of the messages received on the outgoing (sending)
 * queue. The only messages we can receive there, are HUPs or ERRORs. */
static void _osync_client_proxy_hup_handler(OSyncMessage *message, void *user_data)
{
	OSyncClientProxy *proxy = user_data;
	OSyncError *error = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);

	osync_trace(TRACE_INTERNAL, "client received command %i on sending queue", osync_message_get_command(message));

	if (osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_ERROR) {
		/* Houston, we have a problem */
	} else if (osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP) {
		/* The remote side disconnected. So we can now disconnect as well and then
		 * shutdown */
		if (!osync_queue_disconnect(proxy->outgoing, &error))
			osync_error_unref(&error);
		
		if (!osync_queue_disconnect(proxy->incoming, &error))
			osync_error_unref(&error);
		
	} else {
		/* This should never ever happen */
		osync_trace(TRACE_ERROR, "received neither a hup, nor a error on a sending queue...");
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void _osync_client_proxy_init_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->init_callback(proxy, ctx->init_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->init_callback(proxy, ctx->init_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_fin_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->fin_callback(proxy, ctx->fin_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->fin_callback(proxy, ctx->fin_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_discover_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	OSyncVersion *version = NULL;
	OSyncCapabilities *capabilities = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		
		osync_message_read_int(message, &proxy->has_main_sink);
		
		int num_sinks = 0;
		osync_message_read_int(message, &num_sinks);
		
		osync_trace(TRACE_INTERNAL, "main sink?: %i, num objs?: %i", proxy->has_main_sink, num_sinks);
		
		int i = 0;
		for (i = 0; i < num_sinks; i++) {
			OSyncObjTypeSink *sink = NULL;
			if (!osync_demarshal_objtype_sink(message, &sink, &locerror))
				goto error;

			proxy->objtypes = g_list_append(proxy->objtypes, sink);

			if (proxy->member)
				osync_member_add_objtype_sink(proxy->member, sink);
		}
		
		/* Merger - Set the capabilities */
		int sent_version;
		osync_message_read_int(message, &sent_version);
		if (sent_version) {
			char* str;
			version = osync_version_new(&locerror);
			if(!version) {
				goto error;
			}
			
			osync_message_read_string(message, &str);
			osync_version_set_plugin(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_priority(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_vendor(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_modelversion(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_firmwareversion(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_softwareversion(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_hardwareversion(version, str);
			g_free(str);
			osync_message_read_string(message, &str);
			osync_version_set_identifier(version, str);
			g_free(str);	
		}
				
		int sent_capabilities;
		osync_message_read_int(message, &sent_capabilities);
		if (sent_capabilities) {
			char* str;
			osync_message_read_string(message, &str);
			capabilities = osync_capabilities_parse(str, strlen(str), &locerror);
			g_free(str);
			if(!capabilities) {
				goto error_free_version;
			}
		}
		
		/* we set the capabilities for the member only if they are not set yet */
 		OSyncMember *member = osync_client_proxy_get_member(proxy);
 		if (member && osync_member_get_capabilities(member) == NULL)
 		{
			osync_trace(TRACE_INTERNAL, "No capabilities set for the member right now. version: %p capabilities: %p\n", version, capabilities);

			/* we take our own capabilities rather then from the client */ 
			OSyncCapabilities *version_cap = NULL; 
			if (version)
				version_cap = osync_version_find_capabilities(version, &locerror);

			if (locerror)
				goto error_free_capabilities;

			if (version_cap) {
				if (capabilities)
					osync_capabilities_unref(capabilities);
				capabilities = version_cap;
			}

 			if (capabilities) {
				if (!osync_member_set_capabilities(member, capabilities, &locerror))
					goto error_free_capabilities; 
 				if (!osync_member_save(member, &locerror)) /* TODO: Merger we have to save? */
 					goto error_free_capabilities;

				osync_capabilities_unref(capabilities);
 			}
 		}
		if (version)
			osync_version_unref(version);
		
		ctx->discover_callback(proxy, ctx->discover_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->discover_callback(proxy, ctx->discover_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_free_capabilities:
	if (capabilities)
		osync_capabilities_unref(capabilities);
error_free_version:
	if (version)
		osync_version_unref(version);
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_connect_handler(OSyncMessage *message, void *user_data)
{
	int slowsync;
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		osync_message_read_int(message, &slowsync);
		ctx->connect_callback(proxy, ctx->connect_callback_data, slowsync, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->connect_callback(proxy, ctx->connect_callback_data, FALSE, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_disconnect_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->disconnect_callback(proxy, ctx->disconnect_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->disconnect_callback(proxy, ctx->disconnect_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_read_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->read_callback(proxy, ctx->read_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->read_callback(proxy, ctx->read_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_get_changes_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->get_changes_callback(proxy, ctx->get_changes_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->get_changes_callback(proxy, ctx->get_changes_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_commit_change_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		char *uid = NULL;
		osync_message_read_string(message, &uid);
		ctx->commit_change_callback(proxy, ctx->commit_change_callback_data, uid, NULL);
		g_free(uid);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->commit_change_callback(proxy, ctx->commit_change_callback_data, NULL, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_committed_all_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->committed_all_callback(proxy, ctx->committed_all_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->committed_all_callback(proxy, ctx->committed_all_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_sync_done_handler(OSyncMessage *message, void *user_data)
{
	callContext *ctx = user_data;
	OSyncClientProxy *proxy = ctx->proxy;
	OSyncError *error = NULL;
	OSyncError *locerror = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	if (osync_message_get_cmd(message) == OSYNC_MESSAGE_REPLY) {
		ctx->sync_done_callback(proxy, ctx->sync_done_callback_data, NULL);
	} else if (osync_message_get_cmd(message) == OSYNC_MESSAGE_ERRORREPLY) {
		osync_demarshal_error(message, &error);
		ctx->sync_done_callback(proxy, ctx->sync_done_callback_data, error);
		osync_error_unref(&error);
	} else {
		osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "Unexpected reply");
		goto error;
	}
	
	g_free(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	g_free(ctx);
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&locerror));
	osync_error_unref(&locerror);
	return;
}

static void _osync_client_proxy_message_handler(OSyncMessage *message, void *user_data)
{
	OSyncClientProxy *proxy = user_data;
	OSyncError *error = NULL;

	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	osync_trace(TRACE_INTERNAL, "proxy received command %i", osync_message_get_command(message));
	switch (osync_message_get_command(message)) {
		case OSYNC_MESSAGE_NEW_CHANGE:
		case OSYNC_MESSAGE_READ_CHANGE:

			osync_assert(proxy->change_callback);
			OSyncChange *change = NULL;
			
			if (!osync_demarshal_change(message, &change, proxy->formatenv, &error))
				goto error;
			
			proxy->change_callback(proxy, proxy->change_callback_data, change);
			
			osync_change_unref(change);
			break;

		default:
			break;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

OSyncClientProxy *osync_client_proxy_new(OSyncFormatEnv *formatenv, OSyncMember *member, OSyncError **error)
{
	OSyncClientProxy *proxy = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, formatenv, member, error);
	
	proxy = osync_try_malloc0(sizeof(OSyncClientProxy), error);
	if (!proxy)
		goto error;
	proxy->ref_count = 1;
	proxy->type = OSYNC_START_TYPE_UNKNOWN;
	proxy->formatenv = formatenv;
	
	if (member) {
		proxy->member = member;
		osync_member_ref(member);
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, proxy);
	return proxy;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %p", __func__, osync_error_print(error));
	return NULL;
}

OSyncClientProxy *osync_client_proxy_ref(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	
	g_atomic_int_inc(&(proxy->ref_count));

	return proxy;
}

void osync_client_proxy_unref(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	
	if (g_atomic_int_dec_and_test(&(proxy->ref_count))) {
		if (proxy->path)
			g_free(proxy->path);
	
		if (proxy->member)
			osync_member_unref(proxy->member);
		
		while (proxy->objtypes) {
			OSyncObjTypeSink *sink = proxy->objtypes->data;
			osync_objtype_sink_unref(sink);
			proxy->objtypes = g_list_remove(proxy->objtypes, sink);
		}
		
		if (proxy->context)
			g_main_context_unref(proxy->context);
		
		g_free(proxy);
	}
}

void osync_client_proxy_set_context(OSyncClientProxy *proxy, GMainContext *ctx)
{
	osync_assert(proxy);
	proxy->context = ctx;
	if (ctx)
		g_main_context_ref(ctx);
}


void osync_client_proxy_set_change_callback(OSyncClientProxy *proxy, change_cb cb, void *userdata)
{
	osync_assert(proxy);
	
	proxy->change_callback = cb;
	proxy->change_callback_data = userdata;
}

OSyncMember *osync_client_proxy_get_member(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	return proxy->member;
}

osync_bool osync_client_proxy_spawn(OSyncClientProxy *proxy, OSyncStartType type, const char *path, OSyncError **error)
{
	OSyncQueue *read1 = NULL;
	OSyncQueue *read2 = NULL;
	OSyncQueue *write1 = NULL;
	OSyncQueue *write2 = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p)", __func__, proxy, type, path, error);
	osync_assert(proxy);
	osync_assert(type != OSYNC_START_TYPE_UNKNOWN);
		
	proxy->type = type;
	
	if (type != OSYNC_START_TYPE_EXTERNAL) {
		// First, create the pipe from the engine to the client
		if (!osync_queue_new_pipes(&read1, &write1, error))
			goto error;
			
		// Then the pipe from the client to the engine
		if (!osync_queue_new_pipes(&read2, &write2, error))
			goto error_free_pipe1;
		
		proxy->outgoing = write1;
		proxy->incoming = read2;
		
		/* Now we either spawn a new process, or we create a new thread */
		if (type == OSYNC_START_TYPE_THREAD) {
			proxy->client = osync_client_new(error);
			if (!proxy->client)
				goto error_free_pipe2;
			
			/* We now connect to our incoming queue */
			if (!osync_queue_connect(read1, OSYNC_QUEUE_RECEIVER, error))
				goto error_free_pipe2;
			
			/* and the to the outgoing queue */
			if (!osync_queue_connect(write2, OSYNC_QUEUE_SENDER, error))
				goto error_free_pipe2;
			
			
			osync_client_set_incoming_queue(proxy->client, read1);
			osync_client_set_outgoing_queue(proxy->client, write2);
			
			if (!osync_client_run(proxy->client, error))
				goto error_free_pipe2;
		} else {
			/* First lets see if the old plugin exists, and kill it if it does */
			//if (!_osync_client_kill_old_osplugin(proxy, error))
			//	goto error;

			//if (!osync_queue_exists(proxy->outgoing) || !osync_queue_is_alive(proxy->outgoing)) {
			if (!proxy->outgoing || !osync_queue_exists(proxy->outgoing) || !osync_queue_is_alive(proxy->outgoing)) {
				pid_t cpid = fork();
				if (cpid == 0) {
					osync_trace_reset_indent();
					
					/* close the read and write ends of the pipes */
					osync_queue_disconnect(write1, error);
					osync_queue_disconnect(read2, error);
						
					osync_trace(TRACE_INTERNAL, "About to exec osplugin");
					//char *memberstring = g_strdup_printf("%lli", osync_member_get_id(proxy->member));
					//execlp("osplugin", "osplugin", osync_group_get_configdir(osync_member_get_group(osync_proxy_get_member(proxy)), memberstring, NULL);
					char *readfd = g_strdup_printf("%i", osync_queue_get_fd(read1));
					char *writefd = g_strdup_printf("%i", osync_queue_get_fd(write2));
					execlp(OSPLUGIN, "osplugin", "-f", readfd, writefd, NULL);

					if (errno == ENOENT) {
						osync_trace(TRACE_INTERNAL, "Unable to find osplugin. Trying local path.");
						//execlp("osplugin", "osplugin", osync_group_get_configdir(osync_member_get_group(osync_proxy_get_member(proxy)), memberstring, NULL);
						execlp("./osplugin", "osplugin", "-f", readfd, writefd, NULL);
					}
											
					osync_trace(TRACE_INTERNAL, strerror(errno));
					osync_trace(TRACE_INTERNAL, "Unable to execute osplugin.");
					exit(1);
				} else {
					/* close the read and write ends of the pipes */
					osync_queue_disconnect(write2, error);
					osync_queue_disconnect(read1, error);
				}
	
				proxy->child_pid = cpid;
			
				//while (!osync_queue_exists(proxy->outgoing)) {
				//	osync_trace(TRACE_INTERNAL, "Waiting for other side to create fifo");
				//	g_usleep(500000);
				//}
			
				osync_trace(TRACE_INTERNAL, "Queue was created");
			}
	
			//if (proxy->child_pid) {
			//	if (!_osync_client_create_pidfile(proxy, error))
			//		goto error;
			//}
		}
		
		/* We now connect to our incoming queue */
		if (!osync_queue_connect(proxy->incoming, OSYNC_QUEUE_RECEIVER, error))
			goto error;
			
		/* and the to the outgoing queue */
		if (!osync_queue_connect(proxy->outgoing, OSYNC_QUEUE_SENDER, error))
			goto error;
	} else {
		char *name = g_strdup_printf("%s/pluginpipe", path);
		proxy->outgoing = osync_queue_new(name, error);
		g_free(name);
		if (!proxy->outgoing)
			goto error;
		
		name = g_strdup_printf("%s/enginepipe", path);
		proxy->incoming = osync_queue_new(name, error);
		g_free(name);
		if (!proxy->incoming)
			goto error;
			
		if (!osync_queue_create(proxy->outgoing, error))
			goto error;
			
		if (!osync_queue_create(proxy->incoming, error))
			goto error;
			
		/* and the to the outgoing queue */
		if (!osync_queue_connect(proxy->outgoing, OSYNC_QUEUE_SENDER, error))
			goto error;
	}
	
	osync_queue_set_message_handler(proxy->incoming, _osync_client_proxy_message_handler, proxy);
	osync_queue_setup_with_gmainloop(proxy->incoming, proxy->context);
	
	osync_queue_set_message_handler(proxy->outgoing, _osync_client_proxy_hup_handler, proxy);
	osync_queue_setup_with_gmainloop(proxy->outgoing, proxy->context);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_pipe2:
	osync_queue_free(read2);
	osync_queue_free(write2);
error_free_pipe1:
	osync_queue_free(read1);
	osync_queue_free(write1);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_shutdown(OSyncClientProxy *proxy, OSyncError **error)
{
	OSyncMessage *message = NULL;
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, proxy, error);
	
	/* We first disconnect our reading queue. This will generate a HUP
	 * on the remote side */
	if (!osync_queue_disconnect(proxy->incoming, error))
		goto error;
	
	/* We now wait for the HUP on our sending queue */
	message = osync_queue_get_message(proxy->outgoing);
	if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Disconnected, but received no HUP");
		osync_message_unref(message);
		goto error;
	}
	osync_message_unref(message);
	
	/* After we received the HUP, we can disconnect */
	if (!osync_queue_disconnect(proxy->outgoing, error))
		goto error;
			
	if (proxy->type == OSYNC_START_TYPE_THREAD) {
		osync_client_shutdown(proxy->client);
		
		osync_client_unref(proxy->client);
	} else if (proxy->type == OSYNC_START_TYPE_PROCESS) {
		if (proxy->child_pid) {
			int status;
			if (waitpid(proxy->child_pid, &status, 0) == -1) {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Error waiting for osplugin process: %s", g_strerror(errno));
				goto error;
			}

			if (!WIFEXITED(status))
				osync_trace(TRACE_INTERNAL, "Child has exited abnormally");
			else if (WEXITSTATUS(status) != 0)
				osync_trace(TRACE_INTERNAL, "Child has returned non-zero exit status (%d)", WEXITSTATUS(status));

			//if (!osync_client_remove_pidfile(client, error))
			//	goto error;
		}
		
		/* First lets see if the old plugin exists, and kill it if it does */
		//if (!_osync_client_kill_old_osplugin(proxy, error))
		//	goto error;
	}
			
	osync_queue_free(proxy->incoming);
	osync_queue_free(proxy->outgoing);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_initialize(OSyncClientProxy *proxy, initialize_cb callback, void *userdata, const char *formatdir, const char *plugindir, const char *plugin, const char *groupname, const char *configdir, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %s, %s, %s, %p, %p)", __func__, proxy, callback, userdata, formatdir, plugindir, plugin, groupname, configdir, config, error);
	osync_assert(proxy);
	
	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	ctx->proxy = proxy;
	ctx->init_callback = callback;
	ctx->init_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, error);
	if (!message)
		goto error;
	
	osync_message_write_string(message, osync_queue_get_path(proxy->incoming));
	osync_message_write_string(message, formatdir);
	osync_message_write_string(message, plugindir);
	osync_message_write_string(message, plugin);
	osync_message_write_string(message, groupname);
	osync_message_write_string(message, configdir);
	osync_message_write_string(message, config);

#ifdef OPENSYNC_UNITTESTS
	// Introduced (only) for testing/debugging purpose (mock-sync)
	long long int memberid = 0; 

	if (proxy->member)
		memberid = osync_member_get_id(proxy->member);

	osync_message_write_long_long_int(message, memberid);
#endif	
	
	osync_message_set_handler(message, _osync_client_proxy_init_handler, ctx);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, proxy->timeout.initialize, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	if (proxy->type == OSYNC_START_TYPE_EXTERNAL) {
		/* We now connect to our incoming queue */
		if (!osync_queue_connect(proxy->incoming, OSYNC_QUEUE_RECEIVER, error))
			goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

unsigned int osync_client_proxy_get_initialize_timeout(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	return proxy->timeout.initialize;
}

void osync_client_proxy_set_initialize_timeout(OSyncClientProxy *proxy, unsigned int timeout)
{
	osync_assert(proxy);
	proxy->timeout.initialize = timeout;
}

osync_bool osync_client_proxy_finalize(OSyncClientProxy *proxy, finalize_cb callback, void *userdata, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, proxy, callback, userdata, error);
	
	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	ctx->proxy = proxy;
	ctx->fin_callback = callback;
	ctx->fin_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_FINALIZE, 0, error);
	if (!message)
		goto error;

	osync_message_set_handler(message, _osync_client_proxy_fin_handler, ctx);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, proxy->timeout.finalize, error))
		goto error_free_message;

	osync_message_unref(message);

	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

unsigned int osync_client_proxy_get_finalize_timeout(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	return proxy->timeout.finalize;
}

void osync_client_proxy_set_finalize_timeout(OSyncClientProxy *proxy, unsigned int timeout)
{
	osync_assert(proxy);
	proxy->timeout.finalize = timeout;
}

osync_bool osync_client_proxy_discover(OSyncClientProxy *proxy, discover_cb callback, void *userdata, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, proxy, callback, userdata, error);
	
	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	ctx->proxy = proxy;
	ctx->discover_callback = callback;
	ctx->discover_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_DISCOVER, 0, error);
	if (!message)
		goto error;

	osync_message_set_handler(message, _osync_client_proxy_discover_handler, ctx);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, proxy->timeout.discover, error))
		goto error_free_message;
	
	osync_message_unref(message);

	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

unsigned int osync_client_proxy_get_discover_timeout(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	return proxy->timeout.discover;
}

void osync_client_proxy_set_discover_timeout(OSyncClientProxy *proxy, unsigned int timeout)
{
	osync_assert(proxy);
	proxy->timeout.discover = timeout;
}

int osync_client_proxy_num_objtypes(OSyncClientProxy *proxy)
{
	osync_assert(proxy);
	return g_list_length(proxy->objtypes);
}

OSyncObjTypeSink *osync_client_proxy_nth_objtype(OSyncClientProxy *proxy, int nth)
{
	osync_assert(proxy);
	return g_list_nth_data(proxy->objtypes, nth);
}

OSyncObjTypeSink *osync_client_proxy_find_objtype_sink(OSyncClientProxy *proxy, const char *objtype)
{
	osync_assert(proxy);
	GList *o;
	for (o = proxy->objtypes; o; o = o->next) {
		OSyncObjTypeSink *sink = o->data;
		if (!objtype && !osync_objtype_sink_get_name(sink))
			return sink;

		if (objtype && !strcmp(osync_objtype_sink_get_name(sink), objtype))
			return sink;
	}

	if (objtype && proxy->member)
		return osync_member_find_objtype_sink(proxy->member, objtype);
	/*
	else if (!objtype && proxy->member)
		return osync_member_get_main_sink(proxy->member)
	*/

	return NULL;
}

osync_bool osync_client_proxy_connect(OSyncClientProxy *proxy, connect_cb callback, void *userdata, const char *objtype, osync_bool slowsync, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %i, %p)", __func__, proxy, callback, userdata, objtype, slowsync, error);
	
	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_CONNECT;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	ctx->proxy = proxy;
	ctx->connect_callback = callback;
	ctx->connect_callback_data = userdata;
	
	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, objtype);
	if (sink)
		timeout = osync_objtype_sink_get_connect_timeout(sink); 

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_CONNECT, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_connect_handler, ctx);

	osync_message_write_string(message, objtype);
	osync_message_write_int(message, slowsync);

	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error_free_message:
	osync_message_unref(message);
error_free_context:
	g_free(ctx);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_disconnect(OSyncClientProxy *proxy, disconnect_cb callback, void *userdata, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, proxy, callback, userdata, objtype, error);

	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_DISCONNECT;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, objtype);
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 

	ctx->proxy = proxy;
	ctx->disconnect_callback = callback;
	ctx->disconnect_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_DISCONNECT, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_disconnect_handler, ctx);

	osync_message_write_string(message, objtype);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_read(OSyncClientProxy *proxy, read_cb callback, void *userdata, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, proxy, callback, userdata, change, error);
	
	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_READ;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, osync_change_get_objtype(change));
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 

	ctx->proxy = proxy;
	ctx->read_callback = callback;
	ctx->read_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_READ_CHANGE, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_read_handler, ctx);

	if (!osync_marshal_change(message, change, error))
		goto error_free_message;

	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;

	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
    	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_get_changes(OSyncClientProxy *proxy, get_changes_cb callback, void *userdata, const char *objtype, osync_bool slowsync, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %i, %p)", __func__, proxy, callback, userdata, objtype, slowsync, error);
	
	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_GETCHANGES;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, objtype);
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 

	ctx->proxy = proxy;
	ctx->get_changes_callback = callback;
	ctx->get_changes_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_GET_CHANGES, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_get_changes_handler, ctx);

	osync_message_write_string(message, objtype);
	osync_message_write_int(message, slowsync);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_commit_change(OSyncClientProxy *proxy, commit_change_cb callback, void *userdata, OSyncChange *change, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, proxy, callback, userdata, change, error);
	osync_assert(proxy);
	osync_assert(change);

	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_COMMIT;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;

	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, osync_change_get_objtype(change));
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 
	
	ctx->proxy = proxy;
	ctx->commit_change_callback = callback;
	ctx->commit_change_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_COMMIT_CHANGE, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_commit_change_handler, ctx);

	if (!osync_marshal_change(message, change, error))
		goto error_free_message;
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_committed_all(OSyncClientProxy *proxy, committed_all_cb callback, void *userdata, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, proxy, callback, userdata, objtype, error);
	osync_assert(proxy);

	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_COMMITTEDALL;
	
	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;
	
	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, objtype);
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 

	ctx->proxy = proxy;
	ctx->committed_all_callback = callback;
	ctx->committed_all_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_COMMITTED_ALL, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_committed_all_handler, ctx);

	osync_message_write_string(message, objtype);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_client_proxy_sync_done(OSyncClientProxy *proxy, sync_done_cb callback, void *userdata, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, proxy, callback, userdata, objtype, error);
	osync_assert(proxy);

	int timeout = OSYNC_CLIENT_PROXY_TIMEOUT_SYNCDONE;

	callContext *ctx = osync_try_malloc0(sizeof(callContext), error);
	if (!ctx)
		goto error;

	OSyncObjTypeSink *sink = osync_client_proxy_find_objtype_sink(proxy, objtype);
	if (sink)
		timeout = osync_objtype_sink_get_disconnect_timeout(sink); 

	ctx->proxy = proxy;
	ctx->sync_done_callback = callback;
	ctx->sync_done_callback_data = userdata;
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_SYNC_DONE, 0, error);
	if (!message)
		goto error_free_context;
	
	osync_message_set_handler(message, _osync_client_proxy_sync_done_handler, ctx);

	osync_message_write_string(message, objtype);
	
	if (!osync_queue_send_message_with_timeout(proxy->outgoing, proxy->incoming, message, timeout, error))
		goto error_free_message;
	
	osync_message_unref(message);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_context:
	g_free(ctx);
error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}
