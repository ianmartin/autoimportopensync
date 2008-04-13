/*
 * osyncplugin - swiss-knife for testing OpenSync synchronization plugins 
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>

#include <opensync/opensync.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-ipc.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-format.h>

char *pluginpath = NULL;
char *formatpath = NULL;
char *pluginname = NULL;
char *configfile = NULL;
char *configdir = NULL;
GList *sinks = NULL;
GList *cmdlist = NULL;
GList *changesList = NULL;
OSyncPlugin *plugin = NULL;
OSyncPluginEnv *plugin_env = NULL;
OSyncFormatEnv *format_env = NULL;
OSyncPluginInfo *plugin_info = NULL;

typedef enum {
	EMPTY,
	INITIALIZE,
	FINALIZE,
	CONNECT,
	DISCONNECT,
	SYNC,
	SLOWSYNC,
	FASTSYNC,
	COMMIT,
	BATCHCOMMIT,
	COMMITTEDALL,
	READ,
	WRITE,
	SYNCDONE,
	DISCOVER
} Cmd;

typedef struct _Command {
	Cmd cmd;
	char *arg;
} Command;



/*
 * Argument handling 
 */
Command *new_command(Cmd cmd, const char *arg) {

	Command *newcommand = malloc(sizeof(Command));
	if (!newcommand) {
		perror("Can't allocate new command");
		exit(-errno);
	}

	memset(newcommand, 0, sizeof(Command));

	newcommand->cmd = cmd;
	if (arg)
		newcommand->arg = strdup(arg);

	cmdlist = g_list_append(cmdlist, newcommand);

	return newcommand;
}

void free_command(Command **cmd) {
	assert(*cmd);

	if ((*cmd)->arg)
		free((*cmd)->arg);
	
	free(*cmd);
	*cmd = NULL;
}

void usage(const char *name)
{
	fprintf(stderr, "Usage: %s\n", name);
	/* TODO: write usage output */

	exit(1);
}

void parse_args(int argc, char **argv) {

	int i;
	char *arg;

	for (i=1; i < argc; i++) {

		arg = argv[i];

		if (!strcmp(arg, "--config") || !strcmp(arg, "-c")) {
			if (!configfile)
				configfile = strdup(argv[i+1]);

			i++;
			continue;
		} else if (!strcmp(arg, "--configdir") || !strcmp(arg, "-C")) {
			if (!configdir)
				configdir = strdup(argv[i+1]);

			i++;
			continue;
		} else if (!strcmp(arg, "--plugin") || !strcmp(arg, "-p")) {
			if (!pluginname)
				pluginname = strdup(argv[i+1]);

			i++;
			continue;
		} else if (!strcmp(arg, "--pluginpath") || !strcmp(arg, "-P")) {
			if (!pluginpath)
				pluginpath = strdup(argv[i+1]);

			i++;
			continue;
		} else if (!strcmp(arg, "--formatpath") || !strcmp(arg, "-F")) {
			if (!formatpath)
				formatpath = strdup(argv[i+1]);

			i++;
			continue;
		} else if (!strcmp(arg, "--initialize")) {
			new_command(INITIALIZE, NULL);
			continue;
		} else if (!strcmp(arg, "--connect")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CONNECT, NULL);
			else
				new_command(CONNECT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--disconnect")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(DISCONNECT, NULL);
			else
				new_command(DISCONNECT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--finalize")) {
			new_command(FINALIZE, NULL);
			continue;
		} else if (!strcmp(arg, "--slowsync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(SLOWSYNC, NULL);
			else
				new_command(SLOWSYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--sync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(SYNC, NULL);
			else
				new_command(SYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--fastsync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(FASTSYNC, NULL);
			else
				new_command(FASTSYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--syncdone")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(SYNCDONE, NULL);
			else
				new_command(SYNCDONE, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--committedall")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(COMMITTEDALL, NULL);
			else
				new_command(COMMITTEDALL, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--commit")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(COMMIT, NULL);
			else
				new_command(COMMIT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--batchcommit")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(BATCHCOMMIT, NULL);
			else
				new_command(BATCHCOMMIT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--write")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(WRITE, NULL);
			else
				new_command(WRITE, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--read")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(READ, NULL);
			else
				new_command(READ, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--discover")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(DISCOVER, NULL);
			else
				new_command(DISCOVER, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--empty")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(EMPTY, NULL);
			else
				new_command(EMPTY, argv[++i]);

			continue;
		} else {
			fprintf(stderr, "Unknown argument: %s\n", argv[i]);
			usage(argv[0]);
		}
	}

	if (!pluginname || !cmdlist || !configdir)
		usage(argv[0]);
}

/*
 * Plugin Commands
 */

osync_bool init(OSyncError **error) {
	assert(!plugin);
	assert(!plugin_env);

	char *config;
	unsigned int size;
	void *plugin_data;
	
	if (!(plugin_env = osync_plugin_env_new(error)))
		goto error;

	if (!(format_env = osync_format_env_new(error)))
		goto error_free_pluginenv;

	if (!osync_format_env_load_plugins(format_env, formatpath, error))
		goto error_free_formatenv;

	if (!osync_plugin_env_load(plugin_env, pluginpath, error))
		goto error_free_pluginenv;

	if (!(plugin = osync_plugin_env_find_plugin(plugin_env, pluginname))) {
		osync_error_set(error, OSYNC_ERROR_PLUGIN_NOT_FOUND, "Plugin not found: \"%s\"", pluginname);
		goto error_free_pluginenv;
	}

	if (!(plugin_info = osync_plugin_info_new(error)))
		goto error_free_pluginenv;

        if (osync_plugin_get_config_type(plugin) != OSYNC_PLUGIN_NO_CONFIGURATION && configfile) {
                if (!osync_file_read(configfile, &config, &size, error))
			goto error_free_plugininfo;

		osync_plugin_info_set_config(plugin_info, config);
		g_free(config);
	}

	if (!configfile && osync_plugin_get_config_type(plugin) == OSYNC_PLUGIN_NEEDS_CONFIGURATION) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Plugin \"%s\" requires configuration!", pluginname); 
		goto error_free_plugininfo;
	}
	GMainLoop *loop = g_main_loop_new(NULL, TRUE);
	if (!loop) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't allocate loop object.");
		goto error_free_plugininfo;
	}

	osync_plugin_info_set_configdir(plugin_info, configdir);
	osync_plugin_info_set_loop(plugin_info, loop);
	osync_plugin_info_set_format_env(plugin_info, format_env);
	
	return TRUE;

error_free_loop:
	g_main_loop_unref(loop);
error_free_plugininfo:
	osync_plugin_info_unref(plugin_info);
error_free_formatenv:
	osync_format_env_free(format_env);
	format_env = NULL;
error_free_pluginenv:
	osync_plugin_env_free(plugin_env);
	plugin_env = NULL;
error:	
	return FALSE;
}

void *plugin_initialize(OSyncError **error)
{
	void *plugin_data = osync_plugin_initialize(plugin, plugin_info, error);
	
	if (!plugin_data)
		return NULL;

	return plugin_data;
}

void finalize_plugin(void **plugin_data)
{

	if (!*plugin_data)
		return;

	osync_plugin_finalize(plugin, *plugin_data);
	*plugin_data = NULL;
}


OSyncObjTypeSink *find_sink(const char *objtype, OSyncError **error)
{
	assert(objtype);

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(plugin_info, objtype);
	if (!sink) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find sink for %s", objtype);
		return NULL;
	}

	return sink;
}

OSyncObjTypeSink *get_main_sink()
{
	return osync_plugin_info_get_main_sink(plugin_info);
}

const char *_osyncplugin_changetype_str(OSyncChange *change)
{
	assert(change);

	const char *type;

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_UNKNOWN:
			type = "UNKNOWN";
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			type = "ADDED";
			break;
		case OSYNC_CHANGE_TYPE_UNMODIFIED:
			type = "UNMODIFIED";
			break;
		case OSYNC_CHANGE_TYPE_DELETED:
			type = "DELETED";
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			type = "MODIFIED";
			break;
	}

	return type;
}

//typedef void (* OSyncContextChangeFn) (OSyncChange *, void *);
void _osyncplugin_ctx_change_callback(OSyncChange *change, void *user_data)
{
	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;
	printf("%s\t%s\t%s", 
			osync_objtype_sink_get_name(sink),  
			_osyncplugin_changetype_str(change), 
			osync_change_get_uid(change));

	if (osync_change_get_objformat(change))
		printf("\t%s", osync_objformat_get_name(osync_change_get_objformat(change)));

	printf("\n");

	changesList = g_list_append(changesList, change);
}

//typedef void (* OSyncContextCallbackFn)(void *, OSyncError *);
void _osyncplugin_ctx_callback_getchanges(void *user_data, OSyncError *error)
{
	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error)
		fprintf(stderr, "Sink \"%s\": %s\n", osync_objtype_sink_get_name(sink), osync_error_print(&error));
}

osync_bool get_changes_sink(OSyncObjTypeSink *sink, int slowsync, void *plugin_data, OSyncError **error)
{
	assert(sink);
	
	if (slowsync)
		osync_objtype_sink_set_slowsync(sink, TRUE);
	else
		osync_objtype_sink_set_slowsync(sink, FALSE);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_changes_callback(context, _osyncplugin_ctx_change_callback);
	osync_context_set_callback(context, _osyncplugin_ctx_callback_getchanges, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	osync_objtype_sink_get_changes(sink, plugin_data, plugin_info, context);

	osync_context_unref(context);


	return TRUE;

error:
	return FALSE;
}

osync_bool get_changes(const char *objtype, int slowsync, void *plugin_data, OSyncError **error)
{
	assert(objtype);

	int num, i;

	OSyncObjTypeSink *sink = find_sink(objtype, error);
	if (!sink)
		goto error;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!get_changes_sink(sink, slowsync, plugin_data, error))
			goto error;

	} else {
	/* all available objtypes */
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!get_changes_sink(sink, slowsync, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!get_changes_sink(get_main_sink(), slowsync, plugin_data, error))
				goto error;
	}

error:
	return FALSE;
}

osync_bool synchronization_sink(OSyncObjTypeSink *sink, void *plugin_data, osync_bool forcesync, osync_bool forcedstatus, OSyncError **error)
{
	assert(sink);
	
	osync_bool slowsync = forcedstatus;

	if (!forcesync) {
		/* Update REAL SlowSync status */
		slowsync = osync_objtype_sink_get_slowsync(sink);
	}

	if (!get_changes_sink(sink, slowsync, plugin_data, error))
		return FALSE;

	return TRUE;

}

osync_bool synchronization(const char *objtype, void *plugin_data, osync_bool force, osync_bool slowsync, OSyncError **error)
{
	/* objtype can be NULL - this means sync ALL objtypes */
		
	OSyncObjTypeSink *sink = NULL;
	int num, i;


	/** TODO get the slowsync statu form the connect function! */

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!synchronization_sink(sink, plugin_data, force, slowsync, error))
			goto error;

	} else {
	/* all available objtypes */
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!synchronization_sink(sink, plugin_data, force, slowsync, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!synchronization_sink(get_main_sink(), plugin_data, force, slowsync, error))
				goto error;
	}

	return TRUE;

error:
	return FALSE;
}

void _osyncplugin_ctx_callback_connect(void *user_data, OSyncError *error)
{
	assert(user_data);

	OSyncError *locerror = NULL;

	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error) {
		osync_error_set_from_error(&locerror, &error);
		goto error;
	}

	if (osync_objtype_sink_get_slowsync(sink)) {
		printf("SlowSync got requested by CONNECT function");
		if (osync_objtype_sink_get_name(sink))
			printf(" for ObjType: \"%s\"", osync_objtype_sink_get_name(sink));

		printf(".\n");
	}

	return;
error:
	fprintf(stderr, "ERROR: %s\n", osync_error_print(&locerror));
	return;
}

osync_bool connect_sink(OSyncObjTypeSink *sink, void *plugin_data, OSyncError **error) {

	assert(sink);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_callback(context, _osyncplugin_ctx_callback_connect, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	osync_objtype_sink_connect(sink, plugin_data, plugin_info, context);

	osync_context_unref(context);

	return TRUE;

error:	
	return FALSE;
}

static osync_bool connect(const char *objtype, void *plugin_data, OSyncError **error)
{
	
	int i, num;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!connect_sink(sink, plugin_data, error))
			goto error;
	} else {
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!connect_sink(sink, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!connect_sink(get_main_sink(), plugin_data, error))
				goto error;
	}

	return TRUE;
error:
	return FALSE;
}

void _osyncplugin_ctx_callback_disconnect(void *user_data, OSyncError *error)
{
	assert(user_data);

	OSyncError *locerror = NULL;

	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error) {
		osync_error_set_from_error(&locerror, &error);
		goto error;
	}

	if (!osync_objtype_sink_get_slowsync(sink)) {
		if (osync_objtype_sink_get_name(sink))
			printf("Sink \"%s\"", osync_objtype_sink_get_name(sink));
		else
			printf("Main Sink");

		printf(" disconnected.\n");
	}

	return;
error:
	fprintf(stderr, "ERROR: %s\n", osync_error_print(&locerror));
	return;
}

osync_bool disconnect_sink(OSyncObjTypeSink *sink, void *plugin_data, OSyncError **error) {

	assert(sink);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_callback(context, _osyncplugin_ctx_callback_disconnect, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	osync_objtype_sink_disconnect(sink, plugin_data, plugin_info, context);

	osync_context_unref(context);

	return TRUE;

error:	
	return FALSE;
}

osync_bool disconnect(const char *objtype, void *plugin_data, OSyncError **error)
{
	
	int i, num;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!disconnect_sink(sink, plugin_data, error))
			goto error;
	} else {
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!disconnect_sink(sink, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!disconnect_sink(get_main_sink(), plugin_data, error))
				goto error;
	}

	return TRUE;
error:
	return FALSE;
}

osync_bool empty(const char *objtype, void *plugin_data, OSyncError **error) {

	
	/* Perform slowync */
	if (!get_changes(objtype, TRUE, plugin_data, error))
		goto error;

	return TRUE;

error:
	return FALSE;
}

void _osyncplugin_ctx_callback_syncdone(void *user_data, OSyncError *error)
{
	assert(user_data);

	OSyncError *locerror = NULL;

	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error) {
		osync_error_set_from_error(&locerror, &error);
		goto error;
	}

	return;
error:
	fprintf(stderr, "ERROR for sink \"%s\": %s\n", osync_objtype_sink_get_name(sink), osync_error_print(&locerror));
	return;
}

osync_bool syncdone_sink(OSyncObjTypeSink *sink, void *plugin_data, OSyncError **error) {

	assert(sink);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_callback(context, _osyncplugin_ctx_callback_syncdone, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	osync_objtype_sink_sync_done(sink, plugin_data, plugin_info, context);

	osync_context_unref(context);

	return TRUE;

error:	
	return FALSE;
}

osync_bool syncdone(const char *objtype, void *plugin_data, OSyncError **error)
{
	
	int i, num;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!syncdone_sink(sink, plugin_data, error))
			goto error;
	} else {
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!syncdone_sink(sink, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!syncdone_sink(get_main_sink(), plugin_data, error))
				goto error;
	}

	return TRUE;
error:
	return FALSE;
}

void _osyncplugin_ctx_callback_committedall(void *user_data, OSyncError *error)
{
	assert(user_data);

	OSyncError *locerror = NULL;
	//char *objtype = NULL;
	//OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error) {
		osync_error_set_from_error(&locerror, &error);
		goto error;
	}

	return;
error:
	fprintf(stderr, "ERROR: %s\n", osync_error_print(&locerror));
	return;
}

osync_bool committedall_sink(OSyncObjTypeSink *sink, void *plugin_data, OSyncError **error) {

	assert(sink);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_callback(context, _osyncplugin_ctx_callback_committedall, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	osync_objtype_sink_sync_done(sink, plugin_data, plugin_info, context);

	osync_context_unref(context);

	return TRUE;

error:	
	return FALSE;
}

osync_bool committedall(const char *objtype, void *plugin_data, OSyncError **error)
{
	int i, num;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!committedall_sink(sink, plugin_data, error))
			goto error;
	} else {
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!committedall_sink(sink, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!committedall_sink(get_main_sink(), plugin_data, error))
				goto error;
	}

	return TRUE;
error:
	return FALSE;
}

/*
 * Sync Flow
 */
osync_bool run_command(Command *cmd, void **plugin_data, OSyncError **error) {

	assert(cmd);

	if (cmd->cmd != INITIALIZE && *plugin_data == NULL)
		fprintf(stderr, "WARNING: Got Plugin initialized? plugin_data is NULL.\n");

	switch (cmd->cmd) {
		case EMPTY:
			if (!empty(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case INITIALIZE:
			if (!(*plugin_data = plugin_initialize(error)))
				goto error;
			break;
		case FINALIZE:
			finalize_plugin(plugin_data);
			break;
		case CONNECT:
			if (!connect(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case DISCONNECT:
			if (!disconnect(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case SLOWSYNC:
			if (!synchronization(cmd->arg, *plugin_data, TRUE, TRUE, error))
				goto error;

		case FASTSYNC:
			if (!synchronization(cmd->arg, *plugin_data, TRUE, FALSE, error))
				goto error;
			break;
		case SYNC:
			if (!synchronization(cmd->arg, *plugin_data, FALSE, FALSE, error))
				goto error;
			break;
		case COMMIT:
			fprintf(stderr, "COMMIT not yet implemented\n");
			break;
		case BATCHCOMMIT:
			fprintf(stderr, "BATCHCOMMIT not yet implemented\n");
			break;
		case COMMITTEDALL:
			if (!committedall(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case READ:
			fprintf(stderr, "READ not yet implemented\n");
			break;
		case WRITE:
			fprintf(stderr, "WRITE not yet implemented\n");
			break;
		case SYNCDONE:
			if (!syncdone(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case DISCOVER:
			fprintf(stderr, "DISCOVER not yet implemented\n");
			break;
	}

	return TRUE;

error:
	return FALSE;
}

int main(int argc, char **argv) {

	GList *o;
	void *plugin_data = NULL;
	OSyncError *error = NULL;

	parse_args(argc, argv);

	if (!init(&error))
		goto error;

	for (o=cmdlist; o; o = o->next)
		if (!run_command((Command *) o->data, &plugin_data, &error))
			goto error_disconnect_and_finalize;


	/* TODO: free command list - for easier memory leak checking */

	if (plugin_env)
		osync_plugin_env_free(plugin_env);

	return TRUE;

error_disconnect_and_finalize:
	if (plugin_data)
		disconnect(NULL, plugin_data, NULL);
//error_finalize:
	finalize_plugin(&plugin_data);
//error_free_plugin_env:
	if (plugin_env)
		osync_plugin_env_free(plugin_env);
error:	
	fprintf(stderr, "%s\n", osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

