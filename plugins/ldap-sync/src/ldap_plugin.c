/** LDAP addressbook synchronization plugin
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

#include "ldap_plugin.h"
#include "ldap_connect.h"
#include "md5_crypt.h"
#include "xml_convert.h"

#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

/*****************************************************************************/
/*  Module function declarations                                             */

static osync_bool _ldap_parse_config (	ldap_plgdata *plgdata, char *cfg,
					int cfgsize, OSyncError **error);

/*****************************************************************************/
/*  Plugin API functions                                                     */

static void *pl_ldap_initialize(OSyncMember *member, OSyncError **error)
{
	ldap_plgdata *plgdata = NULL;
	char *cfg;
	int cfgsize;

	/* Allocate memory for plugin data */
	plgdata = (ldap_plgdata *)osync_try_malloc0(sizeof(ldap_plgdata), error);
	if (!plgdata)
		goto out;

	/* Read contents of configuration file */
	if (!osync_member_get_config(member, &cfg, &cfgsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto out;
	}

	/* Parse configuration data */
	if (!_ldap_parse_config(plgdata, cfg, cfgsize, error))
		goto error_freedata;

	/* Set values in plugin data structure */
	plgdata->member = member;
	plgdata->hashtable = osync_hashtable_new();

	/* Get mapped LDAP attribute list - for ldap_search */
	plgdata->attrs = get_map_attribute_list();

	/* Initialize gcrypt library */
	init_md5(0);

out_freecfg:
	g_free(cfg);
out:
	return (void*)plgdata;


error_freedata:
	g_free(plgdata);
	plgdata = NULL;
	goto out_freecfg;
}

static void pl_ldap_finalize(void *data)
{
	int i;
	ldap_plgdata *plgdata = (ldap_plgdata*)data;

	/* Free all stuff that have been allocated at initialization */

	for ( i = 0 ; plgdata->attrs[i] ; i++ )
		g_free(plgdata->attrs[i]);
	g_free(plgdata->attrs);

	if (plgdata->servername) g_free(plgdata->servername);
	if (plgdata->binddn) g_free(plgdata->binddn);
	if (plgdata->bindpwd) g_free(plgdata->bindpwd);
	if (plgdata->searchbase) g_free(plgdata->searchbase);
	if (plgdata->searchfilter) g_free(plgdata->searchfilter);
	if (plgdata->storebase) g_free(plgdata->storebase);
	if (plgdata->keyattr) g_free(plgdata->keyattr);
	if (plgdata->authmech) g_free(plgdata->authmech);

	osync_hashtable_free(plgdata->hashtable);
	g_free(plgdata);
}

static void pl_ldap_connect(OSyncContext *ctx)
{
	ldap_plgdata *plgdata = (ldap_plgdata*)osync_context_get_plugin_data(ctx);
	int *ldap_version = 0;

	OSyncError *error = NULL;
	if (!osync_hashtable_load(plgdata->hashtable, plgdata->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}

	/* Make connection to LDAP server */
	if (!os_ldap_connect(ctx, plgdata))
		return;

	/* Set LDAP version on created connection */
	os_ldap_set_version(ctx, plgdata);

	/* Set encryption, if requested */
	if (plgdata->encryption &&
	    !os_ldap_encrypt(ctx, plgdata))
	{
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Could not start encryption");
		return;
	}

	/* TODO login from: login_uid, login_basedn, login_scope */

	/* Bind to server */
	if (!os_ldap_makebind(ctx, plgdata))
		return;

	/* Check evolution support */
	if (!os_ldap_check_evolution(ctx, plgdata)) {
		plgdata->evolution_support = TRUE;
	}

	osync_context_report_success(ctx);
}

static void pl_ldap_disconnect(OSyncContext *ctx)
{
	ldap_plgdata *plgdata = (ldap_plgdata*)osync_context_get_plugin_data(ctx);

	os_ldap_disconnect(ctx, plgdata);

	osync_hashtable_close(plgdata->hashtable);

	osync_context_report_success(ctx);
}

static void pl_ldap_sync_done(OSyncContext *ctx)
{
	ldap_plgdata *plgdata = (ldap_plgdata*)osync_context_get_plugin_data(ctx);

	/* If we have a hashtable we can now forget the already reported changes */
	osync_hashtable_forget(plgdata->hashtable);

	osync_context_report_success(ctx);
}

static void pl_ldap_get_changeinfo(OSyncContext *ctx)
{
	ldap_plgdata *plgdata = (ldap_plgdata*)osync_context_get_plugin_data(ctx);
	GList *ldaplist = NULL;			/* List of ldap_entry objects */
	int i;
	ldap_entry *ldapdata;

	/* Check read permission */
	if (!(plgdata->ldap_read)) {
		osync_context_send_log(ctx, "Reading entries from LDAP disabled");
		osync_context_report_success(ctx);
		return;
	}

	if (osync_member_get_slow_sync(plgdata->member, "contact")) {
		osync_hashtable_set_slow_sync(plgdata->hashtable, "contact");
	}

	/* Get all entries from LDAP server */
	ldaplist = os_load_ldap_entries(ctx, plgdata);
	if (!ldaplist) {
		osync_context_send_log(ctx, "Got 0 entries from LDAP");
		goto exit_no_entries;
	}
	osync_context_send_log(ctx, "Got %d entries from LDAP", g_list_length(ldaplist));

	{
		OSyncChange *chg;
		xmlDoc *xmlcard;
		xmlChar *xmlbuff;
		gchar *md5;

		for (i = 0 ; i < g_list_length(ldaplist) ; i++)
		{
			ldapdata = (ldap_entry *)g_list_nth_data(ldaplist, i);

			/* Convert ldapdata to xml buffer */
			xmlcard = (xmlDoc*)convert_ldap2xml(ldapdata);

			/* Create new change object */
			if (!(chg = osync_change_new())) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "No memory to create change object");
				return;
			}

			/* Fill up changedata */
			osync_change_set_uid(chg, ldapdata->id);
			osync_change_set_member(chg, plgdata->member);
			osync_change_set_objtype_string(chg, "contact");
			osync_change_set_objformat_string(chg, "xml-contact");
			osync_change_set_data(chg, (char*)xmlcard, sizeof(xmlcard), TRUE);

			/* Get MD5 sum of XML buffer */
			xmlbuff = osxml_write_to_string(xmlcard);
			md5 = encrypt_md5((gchar*)xmlbuff);
			xmlFree(xmlbuff);

			/* Set change hash key */
			osync_change_set_hash(chg, md5);

			if (osync_hashtable_detect_change(plgdata->hashtable, chg)) {
				osync_context_report_change(ctx, chg);
				osync_hashtable_update_hash(plgdata->hashtable, chg);
			}

			g_free(md5);
			xmlFreeDoc(xmlcard);
		}
	}

	os_free_ldap_entries(ldaplist);

exit_no_entries:
	osync_hashtable_report_deleted(plgdata->hashtable, ctx, "contact");

	osync_context_report_success(ctx);
}

static osync_bool pl_ldap_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	ldap_plgdata *plgdata = (ldap_plgdata*)osync_context_get_plugin_data(ctx);
	OSyncChangeType chtype = osync_change_get_changetype(change);
	xmlDoc *xmlcard;
	ldap_entry *ldapdata = NULL;
	int mod_type, result;
	gchar *id, *dn;

	/* Check write permission */
	if (!(plgdata->ldap_write)) {
		osync_context_send_log(ctx, "Writing entries to LDAP disabled");
		osync_context_report_success(ctx);
		return TRUE;
	}

	osync_hashtable_get_hash(plgdata->hashtable, change);

	/* Contruct entry DN */
	id = g_strdup((gchar*)osync_change_get_uid(change));
	dn = g_strdup_printf("%s=%s,%s", plgdata->keyattr, id, plgdata->storebase);

	/* Perform delete (simplest) */
	if (chtype == CHANGE_DELETED)
	{
		result = ldap_delete_s (plgdata->ld, dn);
		goto commit_check_status;
	}

	/* Check changetype, if supported */
	if ((chtype != CHANGE_ADDED) && (chtype != CHANGE_MODIFIED)) {
		osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unknown change type");
		goto error;
	}

	/* Get XML change data */
	xmlcard = (xmlDoc*)osync_change_get_data(change);

	/* Map xmlDoc to LDAP data */
	ldapdata = convert_xml2ldap (xmlcard);
	if (!ldapdata) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Wrong entry: %s", id);
		goto error;
	}
	ldapdata->dn = dn;
	ldapdata->id = id;

	/* Change action */
	switch (chtype) {
		case CHANGE_ADDED:
			result = ldap_add_s (plgdata->ld, dn, ldapdata->attrs);
		break;
		case CHANGE_MODIFIED:
			/* Detect changes - leave only attributes to be changed in LDAP entry */
			os_detect_attribute_changes (plgdata, ldapdata);
			result = ldap_modify_s (plgdata->ld, dn, ldapdata->attrs);
		break;
		default:;
	}

commit_check_status:
	if (ldapdata) os_free_ldap_entry(ldapdata);		/* Also will free up *dn and *id */

	if (result != LDAP_SUCCESS) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "LDAP Error: %s", ldap_err2string(result));
		return FALSE;
	}

	if (chtype != CHANGE_DELETED) {
		/* Get MD5 sum of XML buffer */
		xmlChar *xmlbuff = osxml_write_to_string(xmlcard);
		gchar *md5 = encrypt_md5((gchar*)xmlbuff);
		xmlFree(xmlbuff);

		osync_change_set_hash(change, md5);

		g_free(md5);
	}

	osync_hashtable_update_hash(plgdata->hashtable, change);

	osync_context_report_success(ctx);
	return TRUE;

error:
	g_free(dn);
	g_free(id);
	return FALSE;
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	info->version = 1;
	info->name = "ldap-sync";
	info->longname = "LDAP addressbook synchronization plugin";
	info->description = "Plugin synchronizing addressbook against specific DN in LDAP";
	info->config_type = NEEDS_CONFIGURATION;

	info->functions.initialize = pl_ldap_initialize;
	info->functions.finalize = pl_ldap_finalize;
	info->functions.connect = pl_ldap_connect;
	info->functions.disconnect = pl_ldap_disconnect;
	info->functions.sync_done = pl_ldap_sync_done;
	info->functions.get_changeinfo = pl_ldap_get_changeinfo;

	info->timeouts.disconnect_timeout = 0;
	info->timeouts.connect_timeout = 0;
	info->timeouts.sync_done_timeout = 0;
	info->timeouts.get_changeinfo_timeout = 0;
	info->timeouts.get_data_timeout = 0;
	info->timeouts.commit_timeout = 0;

	/* Synchronize contacts (addressbook) */
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "xml-contact", "clean");
	osync_plugin_set_commit_objformat(info, "contact", "xml-contact", pl_ldap_commit_change);
}


/*****************************************************************************/
/*  Plugin inner functions                                                   */

osync_bool _is_on (const char *str)
{
	gchar *lowstr;
	osync_bool retval = FALSE;
	int i;

	/* Check if string is defined, and if it isn't null-string */
	if (!str || !strlen(str)) return FALSE;

	/* Check, if string is a numeric null */
	for (i=0; i<strlen(str); i++) {
		if (str[i] != '0') {
			retval = TRUE;
			break;
		}
	}
	if (!retval) return FALSE;
	/* At this point retval is TRUE */

	/* Check 'off' and 'false' */
	lowstr = g_ascii_strdown(str, -1);
	if (!xmlStrcmp((xmlChar*)lowstr, (xmlChar*)"off") || !xmlStrcmp((xmlChar*)lowstr, (xmlChar*)"false"))
		retval = FALSE;
	g_free(lowstr);

	return retval;
}

static osync_bool _ldap_parse_config (	ldap_plgdata *plgdata, char *cfg,
					int cfgsize, OSyncError **error)
{
	xmlNode *node;
	xmlDoc *doc;
	xmlNode *c;
	char *xmlstr;
	osync_bool retval = FALSE;

	/* Set defaults */
	plgdata->servername = NULL;
	plgdata->serverport = 389;
	plgdata->binddn = NULL;
	plgdata->bindpwd = NULL;
	plgdata->searchbase = NULL;
	plgdata->searchfilter = NULL;
	plgdata->storebase = NULL;
	plgdata->keyattr = NULL;
	plgdata->scope = 0;
	plgdata->authmech = NULL;
	plgdata->encryption = TRUE;
	plgdata->anonymous = FALSE;
	plgdata->ldap_read = TRUE;
	plgdata->ldap_write = FALSE;
	plgdata->ldap_version = LDAP_VERSION3;

	/* Parse configuration file */
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

	/* Read the configuration values */
	for (c = node->xmlChildrenNode; c; c = c->next)
	{
		if((xmlstr = (char*)xmlNodeGetContent(c)) == NULL)
			continue;

		/* char *servername */
		if (!xmlStrcmp(c->name, (const xmlChar*)"servername"))
			plgdata->servername = g_strdup(xmlstr);

		/* int serverport */
		if (!xmlStrcmp(c->name, (const xmlChar*)"serverport"))
			plgdata->serverport = atoi(xmlstr);

		/* char *binddn */
		if (!xmlStrcmp(c->name, (const xmlChar*)"binddn"))
			plgdata->binddn = g_strdup(xmlstr);

		/* char *bindpwd */
		if (!xmlStrcmp(c->name, (const xmlChar*)"password"))
			plgdata->bindpwd = g_strdup(xmlstr);

		/* int anonymous */
		if (!xmlStrcmp(c->name, (const xmlChar*)"anonymous"))
			plgdata->anonymous = _is_on(xmlstr);

		/* char *searchbase */
		if (!xmlStrcmp(c->name, (const xmlChar*)"searchbase"))
			plgdata->searchbase = g_strdup(xmlstr);

		/* char *searchfilter */
		if (!xmlStrcmp(c->name, (const xmlChar*)"searchfilter")) {
			plgdata->searchfilter = g_strdup_printf("%s%s%s", (xmlstr[0] == '(') ? "" : "(",
									  xmlstr,
									  (xmlstr[strlen(xmlstr)-1] == ')') ? "" : ")");
		}

		/* char *storebase */
		if (!xmlStrcmp(c->name, (const xmlChar*)"storebase"))
			plgdata->storebase = g_strdup(xmlstr);

		/* char *keyattr */
		if (!xmlStrcmp(c->name, (const xmlChar*)"keyattr"))
			plgdata->keyattr = g_strdup(xmlstr);

		/* int scope */
		if (!xmlStrcmp(c->name, (const xmlChar*)"scope")) {
			if (!xmlStrcmp((xmlChar*)xmlstr, (xmlChar*)"base")) {
				plgdata->scope = LDAP_SCOPE_BASE;
			}
			else if (!xmlStrcmp((xmlChar*)xmlstr, (xmlChar*)"one")) {
				plgdata->scope = LDAP_SCOPE_ONELEVEL;
			}
			else if (!xmlStrcmp((xmlChar*)xmlstr, (xmlChar*)"sub")) {
				plgdata->scope = LDAP_SCOPE_SUBTREE;
			}
			else {
				osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid scope option (should be 'base', 'one' or 'sub')");
				goto error_freedata;
			}
		}

		/* char *authmech */
		if (!xmlStrcmp(c->name, (const xmlChar*)"authmech"))
			plgdata->authmech = g_strdup(xmlstr);

		/* int encryption */
		if (!xmlStrcmp(c->name, (const xmlChar*)"encryption"))
			plgdata->encryption = _is_on(xmlstr);

		/* int ldap_read */
		if (!xmlStrcmp(c->name, (const xmlChar*)"ldap_read"))
			plgdata->ldap_read = _is_on(xmlstr);

		/* int ldap_write */
		if (!xmlStrcmp(c->name, (const xmlChar*)"ldap_write"))
			plgdata->ldap_write = _is_on(xmlstr);

		xmlFree(xmlstr);
	}

	/* Check mandatory settings */
	if (!plgdata->servername || !plgdata->binddn || !plgdata->bindpwd || !plgdata->keyattr ||
	    (plgdata->ldap_read && !plgdata->searchbase) ||
	    (plgdata->ldap_write && !plgdata->storebase && !plgdata->searchbase))
	{
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid configuration");
		goto error_freedata;
	}

	/* Set defaults, if no configuration values found */
	if (!plgdata->searchbase)
		plgdata->searchbase = g_strdup("");

	if (!plgdata->authmech)
		plgdata->authmech = g_strdup("SIMPLE");

	/* Set storebase to searchbase, if not defined */
	if (!(plgdata->storebase) || !strlen(plgdata->storebase)) {
		if(plgdata->storebase) g_free(plgdata->storebase);
		plgdata->storebase = g_strdup(plgdata->searchbase);
	}

	retval = TRUE;

out_freedoc:
	xmlFreeDoc(doc);
out:
	return retval;

error_freedata:
	if (plgdata->servername) g_free(plgdata->servername);
	if (plgdata->binddn) g_free(plgdata->binddn);
	if (plgdata->bindpwd) g_free(plgdata->bindpwd);
	if (plgdata->searchbase) g_free(plgdata->searchbase);
	if (plgdata->searchfilter) g_free(plgdata->searchfilter);
	if (plgdata->storebase) g_free(plgdata->storebase);
	if (plgdata->keyattr) g_free(plgdata->keyattr);
	if (plgdata->authmech) g_free(plgdata->authmech);
	goto out_freedoc;
}

