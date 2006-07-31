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
#include <opensync/opensync-xml.h>

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
	OSyncMember *member;
	OSyncHashTable *hashtable;
};

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

osync_bool gc_parse_config(struct gc_plgdata *plgdata, char *cfg, int cfgsize, OSyncError **error)
{
	xmlNode *node;
	xmlDoc *doc;
	osync_bool ret = FALSE;

	doc = xmlParseMemory(cfg, cfgsize);
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

static void *gc_initialize(OSyncMember *member, OSyncError **error)
{
	struct gc_plgdata *plgdata = NULL;
	char *cfg;
	int cfgsize;

	if (!osync_member_get_config(member, &cfg, &cfgsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto out;
	}

	plgdata = osync_try_malloc0(sizeof(struct gc_plgdata), error);
	if (!plgdata)
		goto out_freecfg;

	if (!gc_parse_config(plgdata, cfg, cfgsize, error))
		goto error_freedata;

	plgdata->member = member;
	plgdata->hashtable = osync_hashtable_new();

out_freecfg:
	g_free(cfg);
out:
	return plgdata;


error_freedata:
	g_free(plgdata);
	plgdata = NULL;
	goto out_freecfg;
}

static void gc_connect(OSyncContext *ctx)
{
	struct gc_plgdata *plgdata = (struct gc_plgdata*)osync_context_get_plugin_data(ctx);

	OSyncError *error = NULL;
	if (!osync_hashtable_load(plgdata->hashtable, plgdata->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}

	osync_context_report_success(ctx);
}

static void gc_get_changeinfo(OSyncContext *ctx)
{
	struct gc_plgdata *plgdata = (struct gc_plgdata*)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;

	int output;
	int pid;
	FILE *out;
	char sizeline[1024];
	int status;

	if (osync_member_get_slow_sync(plgdata->member, "event")) {
		osync_hashtable_set_slow_sync(plgdata->hashtable, "event");
	}

	if (!run_helper(plgdata, "get_all", NULL,
	                NULL, &output, &pid, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		goto error;
	}

	out = fdopen(output, "r");
	if (!out) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,"Couldn't open helper output");
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

		xmlDoc *doc = xmlParseMemory(xmldata, size);
		if (!doc) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Invalid XML data from helper");
			goto error_invalidxml;
		}

		OSyncChange *chg = osync_change_new();
		if (!chg) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory");
			goto error_chg;
		}

		osync_change_set_uid(chg, uid);
		osync_change_set_member(chg, plgdata->member);
		osync_change_set_objtype_string(chg, "event");
		osync_change_set_objformat_string(chg, "xml-event");
		osync_change_set_data(chg, (char*)doc, sizeof(doc), 1);

		osync_change_set_hash(chg, hash);
		if (osync_hashtable_detect_change(plgdata->hashtable, chg)) {
			osync_context_report_change(ctx, chg);
			osync_hashtable_update_hash(plgdata->hashtable, chg);
		}

		xmlFreeDoc(doc);
		free(hash);
		free(uid);
		free(xmldata);

	/* end of loop */
	continue;

	/* error handling in the loop */
	error_chg:
		xmlFreeDoc(doc);
	error_invalidxml:
	error_readxml:
		free(hash);
	error_hash:
		free(uid);
	error_uid:
		free(xmldata);
		goto error_xmldata;
	}

	osync_hashtable_report_deleted(plgdata->hashtable, ctx, "event");

	fclose(out);
	waitpid(pid, &status, 0);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Helper exited abnormally");
		goto error;
	}

	osync_context_report_success(ctx);

	return;

error_xmldata:
error_size:
	fclose(out);
error_fdopen:
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
error:
	return;
}

static osync_bool gc_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	struct gc_plgdata *plgdata = (struct gc_plgdata*)osync_context_get_plugin_data(ctx);

	pid_t pid;
	int input, output;
	OSyncError *error = NULL;
	const char *cmd;
	const char *arg;
	FILE *out;
	int status;
	char sizeline[1024];

	osync_hashtable_get_hash(plgdata->hashtable, change);

	switch (osync_change_get_changetype(change)) {
		case CHANGE_ADDED:
			cmd = "add";
			arg = NULL;
		break;
		case CHANGE_MODIFIED:
			cmd = "edit";
			arg = osync_change_get_hash(change);
		break;
		case CHANGE_DELETED:
			cmd = "delete";
			arg = osync_change_get_hash(change);
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

	out = fdopen(output, "r");
	if (!out) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't open helper output");
		close(output);
		goto error_fdopen;
	}

	switch (osync_change_get_changetype(change)) {
		case CHANGE_ADDED:
		case CHANGE_MODIFIED:
		{
			xmlDoc *doc = (xmlDoc*)osync_change_get_data(change);
			xmlChar *data;
			int size;
			int xmlsize, uidsize, hashsize;
			char *xmldata, *uid, *hash;

			xmlDocDumpMemory(doc, &data, &size);
			if (write(input, data, size) < size) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't write data to helper");
				close(input);
				goto error_write;
			}

			close(input);

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
		case CHANGE_DELETED:
			close(input);
		break;
		default:
			g_assert_not_reached();
	}

	osync_hashtable_update_hash(plgdata->hashtable, change);

	fclose(out);

	waitpid(pid, &status, 0);

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Helper exited abnormally");
		goto error;
	}

	osync_context_report_success(ctx);

	return TRUE;

error_read:
error_write:
	fclose(out);
error_fdopen:
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
error:
	return FALSE;
}

static void gc_disconnect(OSyncContext *ctx)
{
	struct gc_plgdata *plgdata = (struct gc_plgdata*)osync_context_get_plugin_data(ctx);

	osync_hashtable_close(plgdata->hashtable);

	osync_context_report_success(ctx);
}

static void gc_finalize(void *data)
{
	struct gc_plgdata *plgdata = (struct gc_plgdata*)data;

	osync_hashtable_free(plgdata->hashtable);
	g_free(data);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->version = 1;
	info->name = "google-calendar";

	info->longname = "Google Calendar";
	info->description = "Google Calendar plugin";
	info->config_type = NEEDS_CONFIGURATION;

	info->functions.initialize = gc_initialize;
	info->functions.connect = gc_connect;
	info->functions.disconnect = gc_disconnect;
	info->functions.finalize = gc_finalize;
	info->functions.get_changeinfo = gc_get_changeinfo;

	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "xml-event", NULL);
	osync_plugin_set_commit_objformat(info, "event", "xml-event", gc_commit_change);
	osync_plugin_set_access_objformat(info, "event", "xml-event", gc_commit_change);
}
