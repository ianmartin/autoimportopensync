/** Google Calendar plugin
 *
 * Copyright (c) 2006 Eduardo Pereira Habkost <ehabkost@raisama.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 *
 */

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-version.h>

#include <glib.h>

#include <libxml/tree.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

struct gc_plgdata
{
	char *url;
	char *username;
	char *password;
	OSyncHashTable *hashtable;
	OSyncObjTypeSink *sink;
	OSyncObjFormat *objformat;
};

static void free_plg(struct gc_plgdata *plgdata)
{
	if (plgdata->url)
		xmlFree(plgdata->url);
	if (plgdata->username)
		xmlFree(plgdata->username);
	if (plgdata->password)
		xmlFree(plgdata->password);
	if (plgdata->hashtable)
		osync_hashtable_free(plgdata->hashtable);
	if (plgdata->sink)
		osync_objtype_sink_unref(plgdata->sink);
	if (plgdata->objformat)
		osync_objformat_unref(plgdata->objformat);
	g_free(plgdata);
}

/** Run gchelper and return the file descriptors for its stdin/stdout
 *
 */
osync_bool run_helper(struct gc_plgdata *plgdata, const char *operation,
               const char *arg, int *in, int *out, pid_t *ppid,
               OSyncError **error)
{
	int fdin[2];
	int fdout[2];

	/* pipe for sending the password */
	int fdpwd[2];
	pid_t pid;

	if (pipe(fdin) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "pipe() failed");
		goto error;
	}

	if (pipe(fdout) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "pipe() failed");
		goto error_fdout;
	}

	if (pipe(fdpwd) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "pipe() failed");
		goto error_fdpwd;
	}

	pid = fork();
	if (pid < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "fork() failed");
		goto error_fork;
	}

	if (!pid) {
		/* child process */
		close(fdin[1]);
		close(fdpwd[1]);
		close(fdout[0]);
		close(0);
		close(1);
		dup2(fdin[0], 0);
		dup2(fdout[1], 1);

		char *pwdstr = g_strdup_printf("%d", fdpwd[0]);
		char *const argv[] = {
			GCAL_HELPER,
			plgdata->url,
			plgdata->username,
			pwdstr,
			strdup(operation),
			arg ? strdup(arg) : NULL,
			NULL
		};

		execvp(argv[0], argv);
		
		/* execvp() error */
		fprintf(stderr, "Cannot exec plugin helper (%s)\n", GCAL_HELPER);
		exit(1);
	}

	/* parent process */
	close(fdin[0]);
	close(fdpwd[0]);
	close(fdout[1]);

	/* send the password to gchelper */
	if (write(fdpwd[1],
	             plgdata->password, strlen(plgdata->password)
	         ) < strlen(plgdata->password)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't write password to gchelper");
		goto error_sendpass;
	}

	close(fdpwd[1]);

	if (in)
		*in = fdin[1];
	else
		close(fdin[1]);

	if (out)
		*out = fdout[0];
	else
		close(fdout[0]);

	if (ppid)
		*ppid = pid;

	return TRUE;

error_sendpass:
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
error_fork:
	close(fdpwd[0]);
	close(fdpwd[1]);
error_fdpwd:
	close(fdout[0]);
	close(fdout[1]);
error_fdout:
	close(fdin[0]);
	close(fdin[1]);
error:
	return FALSE;
}

char *gc_get_cfgvalue(xmlNode *cfg, const char *name)
{
	xmlNode *c;
	for (c = cfg->xmlChildrenNode; c; c = c->next) {
		if (!xmlStrcmp(c->name, (const xmlChar*)name))
			return (char*)xmlNodeGetContent(c);
	}
	return NULL;
}

osync_bool gc_parse_config(struct gc_plgdata *plgdata, const char *cfg, OSyncError **error)
{
	xmlNode *node;
	xmlDoc *doc;
	osync_bool ret = FALSE;

	doc = xmlParseMemory(cfg, strlen(cfg) + 1);
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't parse configuration");
		goto out;
	}

	node = xmlDocGetRootElement(doc);
	if (!node || xmlStrcmp(node->name, (const xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid configuration");
		goto out_freedoc;
	}

	/*TODO: Make this more user-friendly: allow the URL to be omitted
	 *      by the user and build it automatically from the username
	 */
	plgdata->url = gc_get_cfgvalue(node, "url");
	plgdata->username = gc_get_cfgvalue(node, "username");
	/*FIXME: We need an opensync API for getting info from the user,
	 *       such as passwords
	 */
	plgdata->password = gc_get_cfgvalue(node, "password");

	if (!plgdata->url || !plgdata->username || !plgdata->password) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid configuration");
		goto error_freedata;
	}

	ret = TRUE;

out_freedoc:
	xmlFreeDoc(doc);
out:
	return ret;

error_freedata:
	xmlFree(plgdata->url);
	xmlFree(plgdata->username);
	xmlFree(plgdata->password);
	goto out_freedoc;
}

static void gc_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	struct gc_plgdata *plgdata = data;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncError *error = NULL;

	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));

	plgdata->hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), &error);
	g_free(tablepath);

	if (!plgdata->hashtable) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void gc_get_changes(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	struct gc_plgdata *plgdata = data;
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OSyncError *error = NULL;

	int output;
	int pid;
	FILE *out;
	char sizeline[1024];
	int status;

	/* Flush internal reports of hashtable to determin deleted entries. */
	osync_hashtable_reset_reports(plgdata->hashtable);

	if (osync_objtype_sink_get_slowsync(sink)) {
		if (!osync_hashtable_slowsync(plgdata->hashtable, &error)) {
			osync_context_report_osyncerror(ctx, &error);
			goto error;
		}
	}

	if (!run_helper(plgdata, "get_all", NULL,
	                NULL, &output, &pid, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		goto error;
	}

	out = fdopen(output, "r");
	if (!out) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't open helper output");
		close(output);
		goto error_fdopen;
	}

	while (fgets(sizeline, sizeof(sizeline), out)) {
		int size, uidsize, hashsize;
		char *xmldata, *uid, *hash;

		if (sscanf(sizeline, "%d %d %d", &size, &uidsize, &hashsize) < 3) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Invalid size line from helper");
			goto error_size;
		}

		xmldata = malloc(size);
		if (!xmldata) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			goto error_xmldata;
		}

		uid = malloc(uidsize + 1);
		if (!uid) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			goto error_uid;
		}

		hash = malloc(hashsize + 1);
		if (!hash) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			goto error_hash;
		}

		if (fread(xmldata, size, 1, out) < 1) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
			goto error_xml;
		}

		if (fread(uid, uidsize, 1, out) < 1) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
			goto error_xml;
		}
		uid[uidsize] = '\0';

		if (fread(hash, hashsize, 1, out) < 1) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
			goto error_xml;
		}
		hash[hashsize] = '\0';

		OSyncXMLFormat *doc = osync_xmlformat_parse(xmldata, size, &error);
		if (!doc) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Invalid XML data from helper");
			osync_error_unref(&error);
			goto error_xml;
		}
		/* osync_merge_merge() seems to like its input sorted... */
		osync_xmlformat_sort(doc);

		OSyncData *odata = osync_data_new((char *)doc, osync_xmlformat_size(), plgdata->objformat, &error);
		if (!odata) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			osync_error_unref(&error);
			/* osync_data_new() does not increase the reference count of
			   its 'data' member, but osync_data_unref() will decrease it,
			   so this is the only case where 'doc' has to be unreferenced
			   manually */
			osync_xmlformat_unref(doc);
			goto error_xml;
		}

		OSyncChange *chg = osync_change_new(&error);
		if (!chg) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			osync_error_unref(&error);
			goto error_chg;
		}

		osync_change_set_uid(chg, uid);
		osync_change_set_data(chg, odata);
		osync_data_unref(odata);
		osync_change_set_objtype(chg, osync_objtype_sink_get_name(sink));
		osync_change_set_hash(chg, hash);

		osync_change_set_changetype(chg, osync_hashtable_get_changetype(plgdata->hashtable, uid, hash));
		osync_hashtable_report(plgdata->hashtable, uid);

		if (osync_change_get_changetype(chg) != OSYNC_CHANGE_TYPE_UNMODIFIED) {
			osync_context_report_change(ctx, chg);
			osync_hashtable_update_hash(plgdata->hashtable, osync_change_get_changetype(chg), uid, hash);
		}

		osync_change_unref(chg);
		free(hash);
		free(uid);
		free(xmldata);

		/* end of loop */
		continue;

		/* error handling in the loop */
	error_chg:
		osync_data_unref(odata);
	error_xml:
		free(hash);
	error_hash:
		free(uid);
	error_uid:
		free(xmldata);
		goto error_xmldata;
	}

	char **uids = osync_hashtable_get_deleted(plgdata->hashtable);
	int i;
	for (i = 0; uids[i]; i++) {
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "ERROR is: %s", osync_error_print(&error));
			goto error_fdopen;
		}
		OSyncData *data = osync_data_new(NULL, 0, plgdata->objformat, &error);
		if (!data) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "ERROR is: %s", osync_error_print(&error));
			goto error_fdopen;
		}
		osync_data_set_objtype(data, "event");

		osync_change_set_data(change, data);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
		osync_change_set_uid(change, uids[i]);
		osync_context_report_change(ctx, change);
		osync_hashtable_update_hash(plgdata->hashtable, OSYNC_CHANGE_TYPE_DELETED, uids[i], NULL);
		osync_change_unref(change);
		osync_data_unref(data);
		g_free(uids[i]);
	}
	g_free(uids);

	fclose(out);
	waitpid(pid, &status, 0);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Helper exited abnormally");
		goto error;
	}

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_xmldata:
error_size:
	fclose(out);
error_fdopen:
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
error:
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

static void gc_commit_change(void *data, OSyncPluginInfo *info,
			     OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx, change);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	struct gc_plgdata *plgdata = data;

	pid_t pid;
	int input, output;
	OSyncError *error = NULL;
	const char *cmd;
	const char *arg;
	FILE *out;
	int status;
	char sizeline[1024];
	char *hash;

	hash = osync_hashtable_get_hash(plgdata->hashtable, osync_change_get_uid(change));

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_ADDED:
			cmd = "add";
			arg = NULL;
		break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			cmd = "edit";
			arg = hash;
		break;
		case OSYNC_CHANGE_TYPE_DELETED:
			cmd = "delete";
			arg = hash;
		break;
		default:
			osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unknown change type");
			goto error;
		break;
	}

	if (!run_helper(plgdata, cmd, arg,
			&input, &output, &pid, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		goto error;
	}

	g_free(hash);

	out = fdopen(output, "r");
	if (!out) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't open helper output");
		close(output);
		goto error_fdopen;
	}

	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_ADDED:
		case OSYNC_CHANGE_TYPE_MODIFIED:
		{
			OSyncXMLFormat *doc = (OSyncXMLFormat *)osync_data_get_data_ptr(osync_change_get_data(change));
			int size;
			char *buffer;

			osync_xmlformat_assemble(doc, &buffer, &size);
			osync_trace(TRACE_INTERNAL, "input to helper:\n%s", buffer);
			if (write(input, buffer, size) < size) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't write data to helper");
				close(input);
				g_free(buffer);
				goto error_write;
			}

			close(input);
			g_free(buffer);

			int xmlsize, uidsize, hashsize;
			char *xmldata, *uid, *hash;

			if (!fgets(sizeline, sizeof(sizeline), out)) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't read from helper");
				goto error_read;
			}

			if (sscanf(sizeline, "%d %d %d", &xmlsize, &uidsize, &hashsize) < 3) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Invalid size line from helper");
				goto error_size;
			}

			xmldata = malloc(xmlsize);
			if (!xmldata) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
				goto error_xmldata;
			}

			uid = malloc(uidsize + 1);
			if (!uid) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
				goto error_uid;
			}

			hash = malloc(hashsize + 1);
			if (!hash) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
				goto error_hash;
			}

			if (fread(xmldata, xmlsize, 1, out) < 1) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
				goto error_readxml;

			}

			if (fread(uid, uidsize, 1, out) < 1) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
				goto error_readxml;
			}
			uid[uidsize] = '\0';

			if (fread(hash, hashsize, 1, out) < 1) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading xml data from helper");
				goto error_readxml;
			}
			hash[hashsize] = '\0';

			/* Done writing. Update UID and hash */
			osync_change_set_uid(change, uid);
			osync_change_set_hash(change, hash);

			free(hash);
			free(uid);
			free(xmldata);

		break;

		/* Error handling */
			error_readxml:
				free(hash);
			error_hash:
				free(uid);
			error_uid:
				free(xmldata);
			error_xmldata:
			error_size:
		break;
		}
		break;
		case OSYNC_CHANGE_TYPE_DELETED:
			close(input);
		break;
		default:
			g_assert_not_reached();
	}

	osync_hashtable_update_hash(plgdata->hashtable,
				    osync_change_get_changetype(change),
				    osync_change_get_uid(change),
				    osync_change_get_hash(change));

	fclose(out);

	waitpid(pid, &status, 0);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Helper exited abnormally");
		goto error;
	}

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_read:
error_write:
	fclose(out);
error_fdopen:
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return;
}

static void gc_disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	struct gc_plgdata *plgdata = data;

	osync_hashtable_free(plgdata->hashtable);
	plgdata->hashtable = NULL;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void gc_finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	struct gc_plgdata *plgdata = data;

	free_plg(plgdata);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void *gc_initialize(OSyncPlugin *plugin,
			   OSyncPluginInfo *info,
			   OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);
	struct gc_plgdata *plgdata;
	const char *cfg;

	plgdata = osync_try_malloc0(sizeof(struct gc_plgdata), error);
	if (!plgdata)
		goto out;

	cfg = osync_plugin_info_get_config(info);
	if (!cfg) {
		osync_error_set(error, OSYNC_ERROR_GENERIC,
				"Unable to get config data.");
		goto error_freeplg;
	}

	if (!gc_parse_config(plgdata, cfg, error))
		goto error_freeplg;

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	plgdata->objformat = osync_format_env_find_objformat(formatenv, "xmlformat-event");
	if (!plgdata->objformat)
		goto error_freeplg;
	osync_objformat_ref(plgdata->objformat);

	plgdata->sink = osync_objtype_sink_new("event", error);
	if (!plgdata->sink)
		goto error_freeplg;

	osync_objtype_sink_add_objformat(plgdata->sink, "xmlformat-event");

	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = gc_connect;
	functions.disconnect = gc_disconnect;
	functions.get_changes = gc_get_changes;
	functions.commit = gc_commit_change;

	osync_objtype_sink_set_functions(plgdata->sink, functions, plgdata);
	osync_plugin_info_add_objtype(info, plgdata->sink);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return plgdata;

error_freeplg:
	free_plg(plgdata);
out:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool gc_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);

	struct gc_plgdata *plgdata = data;

	osync_objtype_sink_set_available(plgdata->sink, TRUE);

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "google-calendar");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;

	osync_plugin_set_name(plugin, "google-calendar");
	osync_plugin_set_longname(plugin, "Google Calendar");
	osync_plugin_set_description(plugin, "Google Calendar plugin");
	osync_plugin_set_config_type(plugin, OSYNC_PLUGIN_NEEDS_CONFIGURATION);

	osync_plugin_set_initialize(plugin, gc_initialize);
	osync_plugin_set_finalize(plugin, gc_finalize);
	osync_plugin_set_discover(plugin, gc_discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
