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
GMainContext *ctx = NULL;
OSyncPlugin *plugin = NULL;
OSyncPluginEnv *plugin_env = NULL;
OSyncFormatEnv *format_env = NULL;
OSyncPluginInfo *plugin_info = NULL;

typedef enum {
	CMD_EMPTY,
	CMD_INITIALIZE,
	CMD_FINALIZE,
	CMD_CONNECT,
	CMD_DISCONNECT,
	CMD_SYNC,
	CMD_SLOWSYNC,
	CMD_FASTSYNC,
	CMD_COMMIT,
	CMD_BATCHCOMMIT,
	CMD_COMMITTEDALL,
	CMD_READ,
	CMD_WRITE,
	CMD_SYNCDONE,
	CMD_DISCOVER
} Cmd;

typedef struct _Command {
	Cmd cmd;
	char *arg;
} Command;

typedef enum {
	/* regular sync - plugin decides for slow or fast sync */
	SYNCTYPE_NORMAL,
	/* force fast sync */
	SYNCTYPE_FORCE_FASTSYNC,
	/* foce slow sync */
	SYNCTYPE_FORCE_SLOWSYNC
} SyncType;

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
			new_command(CMD_INITIALIZE, NULL);
			continue;
		} else if (!strcmp(arg, "--connect")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_CONNECT, NULL);
			else
				new_command(CMD_CONNECT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--disconnect")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_DISCONNECT, NULL);
			else
				new_command(CMD_DISCONNECT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--finalize")) {
			new_command(CMD_FINALIZE, NULL);
			continue;
		} else if (!strcmp(arg, "--slowsync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_SLOWSYNC, NULL);
			else
				new_command(CMD_SLOWSYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--sync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_SYNC, NULL);
			else
				new_command(CMD_SYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--fastsync")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_FASTSYNC, NULL);
			else
				new_command(CMD_FASTSYNC, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--syncdone")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_SYNCDONE, NULL);
			else
				new_command(CMD_SYNCDONE, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--committedall")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_COMMITTEDALL, NULL);
			else
				new_command(CMD_COMMITTEDALL, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--commit")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_COMMIT, NULL);
			else
				new_command(CMD_COMMIT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--batchcommit")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_BATCHCOMMIT, NULL);
			else
				new_command(CMD_BATCHCOMMIT, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--write")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_WRITE, NULL);
			else
				new_command(CMD_WRITE, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--read")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_READ, NULL);
			else
				new_command(CMD_READ, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--discover")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_DISCOVER, NULL);
			else
				new_command(CMD_DISCOVER, argv[++i]);

			continue;
		} else if (!strcmp(arg, "--empty")) {
			if (!argv[i+1] || *argv[i+1] == '-')
				new_command(CMD_EMPTY, NULL);
			else
				new_command(CMD_EMPTY, argv[++i]);

			continue;
		} else {
			fprintf(stderr, "Unknown argument: %s\n", argv[i]);
			usage(argv[0]);
		}
	}


	if (!cmdlist)
		fprintf(stderr, "No command set.\n");

	if (!pluginname)
		fprintf(stderr, "No plugin set.\n");

	if (!configdir)
		fprintf(stderr, "No working/configuraiton directory set.\n");

	if (!pluginname || !cmdlist || !configdir)
		usage(argv[0]);
}

/*
 * Plugin Commands
 */

osync_bool init(OSyncError **error) {
	assert(!plugin);
	assert(!plugin_env);

	OSyncPluginConfig *config;

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

	config = osync_plugin_config_new(error);
	if (!config)
		goto error_free_plugininfo;

        if (osync_plugin_get_config_type(plugin) != OSYNC_PLUGIN_NO_CONFIGURATION && configfile) {
		if (!osync_plugin_config_file_load(config, configfile, NULL, error))
			goto error_free_pluginconfig;

		osync_plugin_info_set_config(plugin_info, config);

		/** Redudant(aka. stolen) code from opensync/client/opensync_client.c */
		/* Enable active sinks */
		OSyncList *r = osync_plugin_config_get_ressources(config);
		for (; r; r = r->next) {
			OSyncPluginRessource *res = r->data;
			OSyncObjTypeSink *sink;

			OSyncList *o = osync_plugin_ressource_get_objformat_sinks(res);
			for (; o; o = o->next) {
				OSyncObjFormatSink *format_sink = (OSyncObjFormatSink *) o->data; 
				const char *objformat_str = osync_objformat_sink_get_objformat(format_sink);
				OSyncObjFormat *objformat = osync_format_env_find_objformat(format_env, objformat_str);

				if (!objformat) {
					osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Couldn't find object format \"%s\"!", objformat_str); 
					goto error_free_pluginconfig;
				}

				const char *objtype = osync_objformat_get_objtype(objformat);

				/* Check for ObjType sink */
				if (!(sink = osync_plugin_info_find_objtype(plugin_info, objtype))) {
					sink = osync_objtype_sink_new(objtype, error);
					if (!sink)
						goto error_free_pluginconfig;

					osync_plugin_info_add_objtype(plugin_info, sink);
				}

				osync_objtype_sink_add_objformat_sink(sink, format_sink);
			}
		}

		osync_plugin_config_unref(config);

	}

	if (!configfile && osync_plugin_get_config_type(plugin) == OSYNC_PLUGIN_NEEDS_CONFIGURATION) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Plugin \"%s\" requires configuration!", pluginname); 
		goto error_free_pluginconfig;
	}

	assert(!ctx);
	ctx = g_main_context_new();

	osync_plugin_info_set_configdir(plugin_info, configdir);
	osync_plugin_info_set_loop(plugin_info, ctx);
	osync_plugin_info_set_format_env(plugin_info, format_env);

	return TRUE;

/*
error_free_loop:
	g_main_context_unref(ctx);
*/
error_free_pluginconfig:
	osync_plugin_config_unref(config);
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
		case OSYNC_CHANGE_TYPE_UNKNOWN:
		default:
			type = "UNKNOWN";
			break;
	}

	return type;
}

//typedef void (* OSyncContextChangeFn) (OSyncChange *, void *);
void _osyncplugin_ctx_change_callback(OSyncChange *change, void *user_data)
{
	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;
	printf("GETCHANGES:\t%s\t%s\t%s", 
			_osyncplugin_changetype_str(change), 
			osync_objtype_sink_get_name(sink),  
			osync_change_get_uid(change));

	if (osync_change_get_objformat(change))
		printf("\t%s", osync_objformat_get_name(osync_change_get_objformat(change)));

	printf("\n");

	osync_change_ref(change);
	changesList = g_list_append(changesList, change);
}

//typedef void (* OSyncContextCallbackFn)(void *, OSyncError *);
void _osyncplugin_ctx_callback_getchanges(void *user_data, OSyncError *error)
{
	OSyncObjTypeSink *sink = (OSyncObjTypeSink *) user_data;

	if (error)
		fprintf(stderr, "Sink \"%s\": %s\n", osync_objtype_sink_get_name(sink), osync_error_print(&error));
}

osync_bool get_changes_sink(OSyncObjTypeSink *sink, SyncType type, void *plugin_data, OSyncError **error)
{
	assert(sink);

	switch (type) {
		case SYNCTYPE_NORMAL:
			break;
		case SYNCTYPE_FORCE_FASTSYNC:
			osync_objtype_sink_set_slowsync(sink, FALSE);
			break;
		case SYNCTYPE_FORCE_SLOWSYNC:
			osync_objtype_sink_set_slowsync(sink, TRUE);
			break;
	}

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

osync_bool get_changes(const char *objtype, SyncType type, void *plugin_data, OSyncError **error)
{
	int num, i;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!get_changes_sink(sink, type, plugin_data, error))
			goto error;

	} else {
		/* all available objtypes */
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!get_changes_sink(sink, type, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!get_changes_sink(get_main_sink(), type, plugin_data, error))
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

static void _osyncplugin_ctx_callback_commit_change(void *user_data, OSyncError *error)
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

osync_bool commit_sink(OSyncObjTypeSink *sink, OSyncChange *change, void *plugin_data, OSyncError **error) {

	assert(sink);
	assert(change);

	OSyncContext *context = osync_context_new(error);
	if (!context)
		goto error;

	osync_context_set_callback(context, _osyncplugin_ctx_callback_commit_change, sink);

	osync_plugin_info_set_sink(plugin_info, sink);

	printf("COMMIT:\t%s\t%s\t%s\n", 
				_osyncplugin_changetype_str(change), 
				osync_data_get_objtype(osync_change_get_data(change)), 
				osync_change_get_uid(change));

	osync_objtype_sink_commit_change(sink, plugin_data, plugin_info, change, context);

	osync_context_unref(context);

	return TRUE;

error:	
	return FALSE;
}

osync_bool commit(const char *objtype, OSyncChange *change, void *plugin_data, OSyncError **error)
{
	assert(change);

	int i, num;
	OSyncObjTypeSink *sink = NULL;

	if (objtype) {
		sink = find_sink(objtype, error);
		if (!sink)
			goto error;

		if (!commit_sink(sink, change, plugin_data, error))
			goto error;
	} else {
		num = osync_plugin_info_num_objtypes(plugin_info);
		for (i=0; i < num; i++) {
			sink = osync_plugin_info_nth_objtype(plugin_info, i);

			if (!commit_sink(sink, change, plugin_data, error))
				goto error;
		}

		/* last but not least - the main sink */
		if (get_main_sink())
			if (!commit_sink(get_main_sink(), change, plugin_data, error))
				goto error;
	}
	
	return TRUE;
error:
	return FALSE;
}

osync_bool empty(const char *objtype, void *plugin_data, OSyncError **error)
{
	int i;
	GList *c;
	
	/* Perform slowync - if objtype is set for this objtype, otherwise slowsync for ALL */
	if (!get_changes(objtype, SYNCTYPE_FORCE_SLOWSYNC, plugin_data, error))
		goto error;


	for (i=0, c = changesList; c; c = c->next, i++) {
		OSyncChange *change = c->data;
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

		if (!commit(objtype, change, plugin_data, error))
			goto error;

	}

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

	osync_objtype_sink_committed_all(sink, plugin_data, plugin_info, context);

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

	if (cmd->cmd != CMD_INITIALIZE && *plugin_data == NULL)
		fprintf(stderr, "WARNING: Got Plugin initialized? plugin_data is NULL.\n");

	switch (cmd->cmd) {
		case CMD_EMPTY:
			if (!empty(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case CMD_INITIALIZE:
			if (!(*plugin_data = plugin_initialize(error)))
				goto error;
			break;
		case CMD_FINALIZE:
			finalize_plugin(plugin_data);
			break;
		case CMD_CONNECT:
			if (!connect(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case CMD_DISCONNECT:
			if (!disconnect(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case CMD_SLOWSYNC:
			if (!get_changes(cmd->arg, SYNCTYPE_FORCE_SLOWSYNC, *plugin_data, error))
				goto error;
			break;
		case CMD_FASTSYNC:
			if (!get_changes(cmd->arg, SYNCTYPE_FORCE_FASTSYNC, *plugin_data, error))
				goto error;
			break;
		case CMD_SYNC:
			if (!get_changes(cmd->arg, SYNCTYPE_NORMAL, *plugin_data, error))
				goto error;
			break;
		case CMD_COMMIT:
			fprintf(stderr, "COMMIT not yet implemented\n");
			break;
		case CMD_BATCHCOMMIT:
			fprintf(stderr, "BATCHCOMMIT not yet implemented\n");
			break;
		case CMD_COMMITTEDALL:
			if (!committedall(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case CMD_READ:
			fprintf(stderr, "READ not yet implemented\n");
			break;
		case CMD_WRITE:
			fprintf(stderr, "WRITE not yet implemented\n");
			break;
		case CMD_SYNCDONE:
			if (!syncdone(cmd->arg, *plugin_data, error))
				goto error;
			break;
		case CMD_DISCOVER:
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
	fprintf(stderr, "Error: %s\n", osync_error_print(&error));
	osync_error_unref(&error);
	return FALSE;
}

