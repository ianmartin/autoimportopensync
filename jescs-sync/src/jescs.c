/** Java Enterprise System Calendar Server plugin
 *
 * Copyright (c) 2006 Gergely Santa <gergely_santa@tempest.sk>
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

#define _GNU_SOURCE
#include <string.h>

#include <opensync/opensync.h>

#include <glib.h>

#include <libxml/tree.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

/*****************************************************************************/
/*  Global variables                                                         */

typedef struct jescs_plgdata
{
	OSyncMember *member;
	OSyncHashTable *hashtable;

	char *url;
	char *username;
	char *password;
	osync_bool del_notify;

} jescs_plgdata;

/*****************************************************************************/
/*  Function prototypes                                                      */

osync_bool get_entry_changeinfo (OSyncContext *ctx, char *entry);

static osync_bool commit_change (OSyncContext *ctx, OSyncChange *change, char *entry);

osync_bool run_wcaptool (	jescs_plgdata *plgdata, const char *operation, char *arg,
				int *in, int *out, pid_t *ppid, OSyncError **error );

osync_bool jescs_parse_config ( jescs_plgdata *plgdata, char *cfg,
				int cfgsize, OSyncError **error );

char *generate_random_number (short digits);

char *get_value_for_key (char *buffer, char *key);
char *change_value_for_key (char *buffer, char *key, char *new_value);
char *add_key_to_entry (char *buffer, char *entry_type, char *key, char *value);

/*****************************************************************************/
/*  Plugin API functions                                                     */

static void *jescs_initialize(OSyncMember *member, OSyncError **error)
{
	jescs_plgdata *plgdata = NULL;
	char *cfg;
	int cfgsize;

	/* Allocate memory for plugin data */
	plgdata = (jescs_plgdata *)osync_try_malloc0(sizeof(jescs_plgdata), error);
	if (!plgdata)
		goto out;

	/* Read contents of configuration file */
	if (!osync_member_get_config(member, &cfg, &cfgsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto out;
	}

	/* Parse configuration data */
	if (!jescs_parse_config(plgdata, cfg, cfgsize, error))
		goto error_freedata;

	/* Set values in plugin data structure */
	plgdata->member = member;
	plgdata->hashtable = osync_hashtable_new();

out_freecfg:
	g_free(cfg);
out:
	return (void*)plgdata;


error_freedata:
	g_free(plgdata);
	plgdata = NULL;
	goto out_freecfg;
}

static void jescs_finalize(void *data)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)data;

	/* Free all stuff that have been allocated at initialization */

	xmlFree(plgdata->url);
	xmlFree(plgdata->username);
	xmlFree(plgdata->password);

	osync_hashtable_free(plgdata->hashtable);
	g_free(plgdata);
}

static void jescs_connect(OSyncContext *ctx)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)osync_context_get_plugin_data(ctx);

	OSyncError *error = NULL;
	if (!osync_hashtable_load(plgdata->hashtable, plgdata->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}

	osync_context_report_success(ctx);
}

static void jescs_disconnect(OSyncContext *ctx)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)osync_context_get_plugin_data(ctx);

	osync_hashtable_close(plgdata->hashtable);

	osync_context_report_success(ctx);
}

static void jescs_sync_done(OSyncContext *ctx)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)osync_context_get_plugin_data(ctx);

	/* If we have a hashtable we can now forget the already reported changes */
	osync_hashtable_forget(plgdata->hashtable);

	osync_context_report_success(ctx);
}

static void jescs_get_changeinfo(OSyncContext *ctx)
{
	osync_bool  retval = get_entry_changeinfo (ctx, "event");
	if (retval) retval = get_entry_changeinfo (ctx, "todo");
	if (retval) osync_context_report_success(ctx);
	return;
}

static osync_bool jescs_commit_change_event(OSyncContext *ctx, OSyncChange *change)
{
	return commit_change(ctx, change, "event");
}

static osync_bool jescs_commit_change_task(OSyncContext *ctx, OSyncChange *change)
{
	return commit_change(ctx, change, "todo");
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->version = 1;
	info->name = "jescs-sync";
	info->longname = "Java Enterprise System Calendar Server";
	info->description = "JES Calendar Server plugin synchronizing events and tasks";
	info->config_type = NEEDS_CONFIGURATION;

	info->functions.initialize = jescs_initialize;
	info->functions.finalize = jescs_finalize;
	info->functions.connect = jescs_connect;
	info->functions.disconnect = jescs_disconnect;
	info->functions.sync_done = jescs_sync_done;
	info->functions.get_changeinfo = jescs_get_changeinfo;

	info->timeouts.disconnect_timeout = 0;
	info->timeouts.connect_timeout = 0;
	info->timeouts.sync_done_timeout = 0;
	info->timeouts.get_changeinfo_timeout = 0;
	info->timeouts.get_data_timeout = 0;
	info->timeouts.commit_timeout = 0;

	/* Synchronize events */
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent20", "clean");
	osync_plugin_set_commit_objformat(info, "event", "vevent20", jescs_commit_change_event);

	/* Synchronize tasks */
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo20", "clean");
	osync_plugin_set_commit_objformat(info, "todo", "vtodo20", jescs_commit_change_task);
}


/*****************************************************************************/
/*  Plugin inner functions                                                   */

/*
 * Get changeinfo for given entry
 * Entry should be 'event' or 'todo'
 */
osync_bool get_entry_changeinfo (OSyncContext *ctx, char *entry)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;

	int    output, bytes;
	int    pid, status;
	gchar *wcapout;
	gchar *command, *format;

	if (strcmp(entry, "event") == 0) {
		command = g_strdup("get-events");
		format  = g_strdup("vevent20");
	}
	else if (strcmp(entry, "todo") == 0) {
		command = g_strdup("get-tasks");
		format  = g_strdup("vtodo20");
	}
	else {
		format = g_strdup_printf("Requesting changes on unsupported entry: %s", entry);
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, format);
		g_free(format);
		return FALSE;
	}

	if (osync_member_get_slow_sync(plgdata->member, entry)) {
		osync_hashtable_set_slow_sync(plgdata->hashtable, entry);
	}

	/* Run wcaptool - get events from calendar */
	if (!run_wcaptool(plgdata, command, NULL,
			  NULL, &output, &pid, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		goto exit_command;
	}
	/* Wait, while process ends */
	waitpid(pid, &status, 0);

	/* Read output from process */
	{
		char line[256];
		GString *outstr = g_string_new("");

		/* Read output */
		while ((bytes = read(output, line, sizeof(line)-1)) > 0) {
			outstr = g_string_append_len(outstr, line, bytes);
		}
		close(output);

		/* Get the output buffer */
		wcapout = outstr->str;
		g_string_free(outstr, FALSE);

		/* Exit, if error at read occured */
		if (bytes < 0) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading wcaptool output");
			goto exit_wcapout;
		}
	}

	/* Check termination status of process */
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* wcapout contains error message in this case */
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, wcapout);
		goto exit_wcapout;
	}

	{
		OSyncChange *chg;
		gchar *chbuf, *ptr, *uid, *mod;
		chbuf = wcapout;
		while(*chbuf != '\0') {
			/* Read following buffer length */
			sscanf(chbuf, "%d", &bytes);
			/* Set buffer pointer to the beginning of next line */
			chbuf = strstr(chbuf, "\n");
			chbuf++;

			/* Create new change object */
			if (!(chg = osync_change_new())) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory to create change object");
				goto exit_wcapout;
			}

			/* Read the buffer*/
			ptr = g_strndup(chbuf, bytes);
			/* Jump over buffer - to the beginning of following line (next buffer length) */
			chbuf += bytes + 1;

			/* Get UID of change */
			uid = (gchar*)get_value_for_key (ptr, "UID");

			/* Get timestamp of last modification */
			mod = (gchar*)get_value_for_key (ptr, "LAST-MODIFIED");
			if (mod == NULL)
				mod = generate_random_number(16);

			/* Fill up changedata */
			osync_change_set_uid(chg, uid);
			osync_change_set_member(chg, plgdata->member);
			osync_change_set_objtype_string(chg, entry);
			osync_change_set_objformat_string(chg, format);
			osync_change_set_data(chg, ptr, bytes, TRUE);
			osync_change_set_hash(chg, mod);

			if (osync_hashtable_detect_change(plgdata->hashtable, chg)) {
				osync_context_report_change(ctx, chg);
				osync_hashtable_update_hash(plgdata->hashtable, chg);
			}

			g_free(mod);
			g_free(uid);
			g_free(ptr);
		}
	}

	osync_hashtable_report_deleted(plgdata->hashtable, ctx, entry);

	g_free(wcapout);
	return TRUE;

exit_wcapout:
	g_free(wcapout);
exit_command:
	g_free(command);
	g_free(format);
	return FALSE;
}

/*
 * Commit change of entry
 * Entry should be 'event' or 'todo'
 */
static osync_bool commit_change (OSyncContext *ctx, OSyncChange *change, char *entry)
{
	jescs_plgdata *plgdata = (jescs_plgdata*)osync_context_get_plugin_data(ctx);
	OSyncChangeType chtype = osync_change_get_changetype(change);

	char *cmd, *data, *uid, *new_data, *arg = NULL, *wcapout;
	pid_t pid;
	int input, output;
	OSyncError *error = NULL;
	int status;

	osync_hashtable_get_hash(plgdata->hashtable, change);

	data = osync_change_get_data(change);
	uid = (char*)osync_change_get_uid(change);

	/* Add/Change UID LINE in data buffer */
	new_data = change_value_for_key (data, "UID", uid);
	if (new_data == NULL)
		new_data = add_key_to_entry(data, entry, "UID", uid);
	if (new_data != NULL) {
		osync_change_set_data(change, new_data, strlen(new_data), TRUE);
		g_free(data);
		data = new_data;
	}

	switch (chtype) {
		case CHANGE_MODIFIED:
		case CHANGE_ADDED:
			cmd = "import";
		break;
		case CHANGE_DELETED:
			if (strcmp(entry, "event") == 0) {
				cmd = "delevent";
			} else if (strcmp(entry, "todo") == 0) {
				cmd = "deltask";
			} else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unknown entry type");
				goto error;
			}
			arg = uid;
		break;
		default:
			osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unknown change type");
			goto error;
		break;
	}

	if (!run_wcaptool(plgdata, cmd, arg,
			  &input, &output, &pid, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		if (arg != NULL) free(arg);
		goto error;
	}

	/* INPUT to wcaptool process */
	if((chtype == CHANGE_ADDED) || (chtype == CHANGE_MODIFIED)) 
	{
		/* Write data to wcaptool's input */
		if (write(input, data, strlen(data)) < 0) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to write data to wcaptool's input pipe");
			kill(pid, SIGTERM);
			waitpid(pid, NULL, 0);
			goto exit_pid;
		}
		if (write(input, "\0", 1) < 0) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to write data to wcaptool's input pipe");
			kill(pid, SIGTERM);
			waitpid(pid, NULL, 0);
			goto exit_pid;
		}
	}

	/* Wait, while process ends */
	waitpid(pid, &status, 0);

	/* Read output from wcaptool process */
	{
		char line[256];
		int bytes;
		GString *outstr = g_string_new("");

		/* Read output */
		while ((bytes = read(output, line, sizeof(line)-1)) > 0) {
			outstr = g_string_append_len(outstr, line, bytes);
		}

		/* Get the output buffer */
		wcapout = outstr->str;
		g_string_free(outstr, FALSE);

		/* Exit, if error at read occured */
		if (bytes < 0) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error reading wcaptool output");
			goto exit_wcapout;
		}
	}

	/* Check termination status of process */
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* wcapout contains error message in this case */
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, wcapout);
		goto exit_wcapout;
	}

	if((chtype == CHANGE_ADDED) || (chtype == CHANGE_MODIFIED)) 
	{
		/* Get timestamp of last modification */
		char *mod = get_value_for_key (data, "LAST-MODIFIED");
		if (mod == NULL)
			mod = generate_random_number(16);
		/* Done writing. Update hash */
		osync_change_set_hash(change, mod);
		g_free(mod);
	}

	osync_hashtable_update_hash(plgdata->hashtable, change);

	osync_context_report_success(ctx);

	g_free(wcapout);
	close(input);
	close(output);
	return TRUE;

exit_wcapout:
	g_free(wcapout);
exit_pid:
	close(input);
	close(output);
error:
	return FALSE;
}

/*
 * Run wcaptool and return the file descriptors for its stdin/stdout
 */
osync_bool run_wcaptool (	jescs_plgdata *plgdata, const char *operation, char *arg,
				int *in, int *out, pid_t *ppid, OSyncError **error )
{
	int fdin[2];		/* pipe for sending input to process */
	int fdout[2];		/* pipe for receiving output from server */
	pid_t pid;		/* process ID */

	if (pipe(fdin) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Input pipe creation failed");
		goto error;
	}

	if (pipe(fdout) < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Output pipe creation failed");
		goto error_fdout;
	}

	pid = fork();
	if (pid < 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Forking wcaptool process failed");
		goto error_fork;
	}

	if (!pid) {
		extern char **environ;
		/* Set environment variable for child process */
		gchar *envvar = g_strdup_printf("JESCS_OSYNC_PWD=%s", plgdata->password);
		putenv(envvar);
		/* child process */
		close(fdin[1]);
		close(fdout[0]);
		close(0);           /* Close STDIN  */
		close(1);           /* Close STDOUT */
		dup2(fdin[0], 0);   /* Dup pipe to STDIN  */
		dup2(fdout[1], 1);  /* Dup pipe to STDOUT */

		char *const argv[] = {	WCAPTOOL,
					"-s", plgdata->url,
					"-u", plgdata->username,
					strdup(operation),
					arg, NULL
				     };
		execvp(argv[0], argv);

		/* execvp() error */
		printf("Cannot exec plugin tool (%s)\n", WCAPTOOL);
		exit(1);
	}

	/* parent process */
	close(fdin[0]);
	close(fdout[1]);

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

error_fork:
	close(fdout[0]);
	close(fdout[1]);
error_fdout:
	close(fdin[0]);
	close(fdin[1]);
error:
	return FALSE;
}

char *generate_random_number (short digits)
{
	unsigned int i, seed = (unsigned int)time(NULL);
	char *tstamp = (char*)g_malloc(digits+1);

	for (i=0; i<digits; i++) {
		tstamp[i] = (rand_r(&seed) % 10) + '0';
	}
	tstamp[digits] = '\0';
	return tstamp;
}

/*
 * Get value of key from buffer
 * Awaited format of lines in buffer: "KEY:value"
 */
char *get_value_for_key (char *buffer, char *key)
{
	char *value;
	char *needle = (char*)g_strdup_printf("%s:", key);
	char *ptr = strstr(buffer, needle);
	size_t nsize = strlen(needle);
	g_free((gchar*)needle);

	if (ptr == NULL)
		return NULL;

	{
		char *ptrend = strstr(ptr, "\n");
		if (ptrend != NULL) {
			value = strndup(ptr + nsize, ptrend - ptr - nsize);
		} else {
			value = strdup(ptr + nsize);
		}
	}
	return (char*)g_strchomp(value);
}

/*
 * Changes value of key in buffer
 * Awaited format of lines in buffer: "KEY:value"
 */
char *change_value_for_key (char *buffer, char *key, char *new_value)
{
	char *needle = (char*)g_strdup_printf("%s:", key);
	char *ptr = strstr(buffer, needle);
	char *ptrend;
	GString *result;
	char *result_str;

	if (ptr == NULL) {
		g_free((gchar*)needle);
		return NULL;
	}

	ptr += strlen(needle);
	g_free((gchar*)needle);

	result = g_string_new_len(buffer, (gssize)(ptr - buffer));
	result = g_string_append(result, new_value);

	ptrend = strstr(ptr, "\n");
	if (ptrend != NULL)
		result = g_string_append(result, ptrend);

	result_str = result->str;
	g_string_free(result, FALSE);
	return result_str;
}

/*
 * Adds line "KEY:value" to buffer
 * Line will be added to block between lines: BEGIN:V<entry_type>
 *                                            END:V<entry_type>
 * entry_type should be "event" or "todo"
 */
char *add_key_to_entry (char *buffer, char *entry_type, char *key, char *value)
{
	char *needle = (char*)g_strdup_printf("BEGIN:V%s", g_ascii_strup(entry_type, -1));
        char *ptr, *result_str;
        GString *result;

        ptr = strstr(buffer, needle);

        if (ptr == NULL) {
                g_free((gchar*)needle);
                return NULL;
        }

        ptr += strlen(needle);
        g_free((gchar*)needle);

        result = g_string_new_len(buffer, (gssize)(ptr - buffer));
        g_string_append_printf(result, "\n%s:%s", g_ascii_strup(key, -1), value);
        result = g_string_append(result, ptr);

        result_str = result->str;
        g_string_free(result, FALSE);
        return result_str;
}

char *jescs_get_cfgvalue (xmlNode *cfg, const char *name)
{
	xmlNode *c;
	for (c = cfg->xmlChildrenNode; c; c = c->next) {
		if (!xmlStrcmp(c->name, (const xmlChar*)name))
			return (char*)xmlNodeGetContent(c);
	}
	return NULL;
}

osync_bool jescs_parse_config (	jescs_plgdata *plgdata, char *cfg,
				int cfgsize, OSyncError **error)
{
	xmlNode *node;
	xmlDoc *doc;
	osync_bool ret = FALSE;
	char *tmp;

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

	/* Server URL */
	plgdata->url = jescs_get_cfgvalue(node, "url");

	/* Username and password */
	plgdata->username = jescs_get_cfgvalue(node, "username");
	plgdata->password = jescs_get_cfgvalue(node, "password");

	/* Deletion notify */
	tmp = jescs_get_cfgvalue(node, "del_notify");
	plgdata->del_notify = ((tmp != NULL) && (strcmp(tmp, "0") != 0)) ? TRUE : FALSE;

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

