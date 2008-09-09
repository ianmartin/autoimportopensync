/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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

#include "syncml_common.h"
#include "syncml_callbacks.h"
#include "syncml_devinf.h"
#include "syncml_vformat.h"

#include <opensync/plugin/opensync_sink.h>

GList *g_list_add(GList *databases, void *database)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);

    // if we find the item in the list
    // then we only return and do nothing

    if (g_list_find(databases, database) != NULL)
    {
        osync_trace(TRACE_EXIT, "%s - the item is an element of the list", __func__);
        return databases;
    }

    // add the item to the list

    GList *result = g_list_append(databases, database);
    osync_trace(TRACE_EXIT, "%s - add a new list item %p", __func__, database);
    return result;
}

void syncml_free_database(SmlDatabase *database)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, database);

	// cleanup configuration stuff

	if (database->remoteNext)
		safe_cfree(&(database->remoteNext));
	if (database->localNext)
		safe_cfree(&(database->localNext));
	if (database->objformat) {
		osync_objformat_unref(database->objformat);
		database->objformat = NULL;
	}
	if (database->sink) {
		osync_objtype_sink_unref(database->sink);
		database->sink = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - configuration cleaned", __func__);

	// cleanup contexts
	// if something is present here then it must be failed
	// because this is a software bug

	if (database->syncChanges != NULL) {
		osync_trace(TRACE_ERROR, "%s: detected old change array", __func__);
		int i;
		for (i = 0; database->syncChanges[i] != NULL; i++)
		{ 
			osync_trace(TRACE_ERROR, "%s: detected old change", __func__);
			osync_change_unref(database->syncChanges[i]);
			database->syncChanges[i] = NULL;
		}
		safe_free((void **) &(database->syncChanges));
	}
	if (database->syncContexts != NULL) {
		osync_trace(TRACE_ERROR, "%s: detected old change context array", __func__);
		int i;
		for (i = 0; database->syncContexts[i] != NULL; i++)
		{ 
			OSyncError *error = NULL;
			osync_error_set(&error, OSYNC_ERROR_GENERIC,
				"%s - context discovered on finalize", __func__);
			report_error_on_context(&(database->syncContexts[i]), &error, TRUE);
		}
		safe_free((void **) &(database->syncContexts));
	}

	if (database->syncModeCtx) {
		OSyncError *error = NULL;
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s - syncModeCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->syncModeCtx), &error, TRUE);
	}
	if (database->getChangesCtx) {
		OSyncError *error = NULL;
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s - getChangesCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->getChangesCtx), &error, TRUE);
	}
	if (database->commitCtx) {
		OSyncError *error = NULL;
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s - commitCtx context discovered on finalize", __func__);
		report_error_on_context(&(database->commitCtx), &error, TRUE);
	}
	osync_trace(TRACE_INTERNAL, "%s - contexts cleaned", __func__);

	// free the database struct itself

	safe_free((gpointer *)&database);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

SmlChangeType _get_changetype(OSyncChange *change)
{
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_ADDED:
			return SML_CHANGE_ADD;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			return SML_CHANGE_REPLACE;
		case OSYNC_CHANGE_TYPE_DELETED:
			return SML_CHANGE_DELETE;
		default:
			;
	}
	return SML_CHANGE_UNKNOWN;
}

SmlDatabase *syncml_config_parse_database(SmlPluginEnv *env, OSyncPluginResource *res, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, res, error);
	g_assert(env);
	g_assert(res);

	SmlDatabase *database = osync_try_malloc0(sizeof(SmlDatabase), error);
	if (!database)
		goto error;

	database->env = env;
	database->syncChanges = NULL;
	database->syncContexts = NULL;

	database->url = osync_plugin_resource_get_name(res);
        if (!database->url) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Database name not set");
                goto error_free_database;
        }

	database->objtype = osync_plugin_resource_get_objtype(res);
        if (!database->objtype) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "\"objtype\" of a database not set");
                goto error_free_database;
        }

	osync_trace(TRACE_EXIT, "%s: %p", __func__, database);
	return database;

error_free_database:
	syncml_free_database(database);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

OSyncChangeType _to_osync_changetype(SmlChangeType type)
{
	switch (type) {
		case SML_CHANGE_ADD:
			return OSYNC_CHANGE_TYPE_ADDED;
		case SML_CHANGE_REPLACE:
			return OSYNC_CHANGE_TYPE_MODIFIED;
		case SML_CHANGE_DELETE:
			return OSYNC_CHANGE_TYPE_DELETED;
		default:
			;	
	}
	return OSYNC_CHANGE_TYPE_UNKNOWN;
}

void sync_done(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_context_report_success(ctx);
}

void disconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	g_assert(env);

	OSyncError *oserror = NULL;
	SmlError *error = NULL;
	
	if ((env->state1 >= SML_DATA_SYNC_EVENT_DISCONNECT &&
	     (!env->dsObject2 ||
	      env->state2 >= SML_DATA_SYNC_EVENT_DISCONNECT ||
	      env->state2 < SML_DATA_SYNC_EVENT_CONNECT)
	    ) ||
	    env->state1 < SML_DATA_SYNC_EVENT_CONNECT)
	{
		/* The disconnect already happened or
		 * the session never existed (connect error). */
		report_success_on_context(&ctx);
	} else {
		env->disconnectCtx = ctx;
		osync_context_ref(env->disconnectCtx);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	if (env->disconnectCtx)
		report_error_on_context(&(env->disconnectCtx), &oserror, TRUE);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
}

void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	SmlPluginEnv *env = (SmlPluginEnv *)data;
	SmlError *error = NULL;

	/* glib stuff */
	/* It is necessary to stop the glib dispatching or
	 * otherwise it can happen that the manager is called
	 * when it is already cleaned up.
	 */

	if (env->source) {
		g_source_destroy(env->source);
		g_source_unref(env->source);
	}
	if (env->source_functions)
		safe_free((gpointer *)&(env->source_functions));
	if (env->context) {
		g_main_context_unref(env->context);
		env->context = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - glib cleaned", __func__);

	/* Cleanup the libsyncml library */

	if (env->dsObject1)
		smlDataSyncObjectUnref(&(env->dsObject1));
	if (env->dsObject2)
		smlDataSyncObjectUnref(&(env->dsObject2));
	while (env->databases) {
		SmlDatabase *db = env->databases->data;
		syncml_free_database(db);
		env->databases = g_list_remove(env->databases, db);
	}
	osync_trace(TRACE_INTERNAL, "%s - libsyncml cleaned", __func__);

	/* plugin config */

	if (env->anchor_path)
		safe_cfree(&(env->anchor_path));
	if (env->devinf_path)
		safe_cfree(&(env->devinf_path));
	osync_trace(TRACE_INTERNAL, "%s - plugin configuration cleaned", __func__);

	/* Signal forgotten contexts */

	if (env->connectCtx) {
		OSyncError *error = NULL;
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s - detected forgotten connect context", __func__);
		report_error_on_context(&(env->connectCtx), &error, TRUE);
	}
	if (env->disconnectCtx) {
		OSyncError *error = NULL;
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "%s - detected forgotten disconnect context", __func__);
		report_error_on_context(&(env->disconnectCtx), &error, TRUE);
	}
	osync_trace(TRACE_INTERNAL, "%s - contexts cleaned", __func__);

	/* cleanup OpenSync stuff */

	if (env->pluginInfo) {
		osync_plugin_info_unref(env->pluginInfo);
		env->pluginInfo = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - plugin info cleaned", __func__);

	/* final cleanup */

	safe_free((gpointer *) &env);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

unsigned int get_num_changes(OSyncChange **changes)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);

    if (changes == NULL || changes[0] == NULL)
    {
        osync_trace(TRACE_EXIT, "%s - no changes present", __func__);
        return 0;
    }

    unsigned int num = 0;
    unsigned int i;
    for (i = 0; changes[i]; i++) {
        num++;
    }

    osync_trace(TRACE_EXIT, "%s (%d)", __func__, num);
    return num;
}

SmlDatabase *get_database_from_plugin_info(OSyncPluginInfo *info)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
    SmlDatabase *database = osync_objtype_sink_get_userdata(sink);
    osync_trace(TRACE_EXIT, "%s - %s", __func__, database->url);
    return database;
} 

void safe_cfree(char **address)
{
    safe_free((gpointer *)address);
}

void safe_free(gpointer *address)
{
    g_assert(address);
    g_assert(*address);
    g_free(*address);
    *address = NULL;
}

void report_success_on_context(OSyncContext **ctx)
{
    osync_trace(TRACE_INTERNAL, "%s: report success on osync_context %p.", __func__, *ctx);
    g_assert(ctx);
    g_assert(*ctx);
    osync_context_report_success(*ctx);
    osync_context_unref(*ctx);
    *ctx = NULL;
}

void report_error_on_context(OSyncContext **ctx, OSyncError **error, osync_bool cleanupError)
{
    osync_trace(
        TRACE_INTERNAL, "%s: report error on osync_context %p (%s).", __func__,
        *ctx, osync_error_print(error));
    g_assert(ctx);
    g_assert(*ctx);
    g_assert(error);
    osync_context_report_osyncerror(*ctx, *error);
    osync_context_unref(*ctx);
    *ctx = NULL;
    if (cleanupError)
    {
        osync_error_unref(error);
        *error = NULL;
    }
}

SmlDatabase *get_database_from_source(
			SmlPluginEnv *env,
			const char *source,
			SmlError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, source, error);

	/* find database */
	SmlDatabase *database = NULL;
	GList *o = env->databases;
	while (o) {
		database = o->data;
		if (!strcmp(database->url, source)) {
			/* abort the scan */
			o = NULL;
		} else {
			database = NULL;
		}
		if (o) o = o->next;
	}
	if (!database) {
		smlErrorSet(error, SML_ERROR_GENERIC,
			"Cannot found datastore %s.",
			source);
		goto error;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return database;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s -%s", __func__, smlErrorPrint(error));
	return NULL;
}

void syncml_connect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

	SmlPluginEnv *env = (SmlPluginEnv *)data;
	SmlError *error = NULL;
	OSyncError *oserror = NULL;

	/* The context is preserved in the environment.
	 * If the event callback receives the got all alerts event
	 * then the connect context is signalled.
	 */

	env->connectCtx = ctx;
	osync_context_ref(env->connectCtx);

	if (!smlDataSyncInit(env->dsObject1, &error))
		goto error;
	if (!smlDataSyncRun(env->dsObject1, &error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
error:
	osync_error_set(&oserror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&oserror));
	report_error_on_context(&(env->connectCtx), &oserror, TRUE);
}

osync_bool discover(const char *name, void *data, OSyncPluginInfo *info, OSyncError **error)
{
        osync_trace(TRACE_ENTRY, "%s(%s, %p, %p, %p)", __func__, name, data, info, error);
        
        SmlPluginEnv *env = (SmlPluginEnv *)data;
        GList *o = env->databases;
        for (; o; o = o->next) {
                SmlDatabase *database = o->data;
                osync_objtype_sink_set_available(database->sink, TRUE);
        }
        
        OSyncVersion *version = osync_version_new(error);
        osync_version_set_plugin(version, name);
        //osync_version_set_modelversion(version, "version");
        //osync_version_set_firmwareversion(version, "firmwareversion");
        //osync_version_set_softwareversion(version, "softwareversion");
        //osync_version_set_hardwareversion(version, "hardwareversion");
        osync_plugin_info_set_version(info, version);
        osync_version_unref(version);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

osync_bool parse_config(
		SmlTransportType tsp,
		SmlDataSyncObject *dsObject,
		OSyncPluginConfig *config,
		OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%d, %p, %p, %p)",
		__func__, tsp, dsObject, config, oerror);
	OSyncPluginConnection *conn;
	char *url = NULL;
	SmlError *error = NULL;

	if (!smlDataSyncSetOption(
			dsObject,
			SML_DATA_SYNC_CONFIG_USE_TIMESTAMP_ANCHOR,
			"1", &error))
		goto error;
	if (!smlDataSyncSetOption(
			dsObject,
			SML_DATA_SYNC_CONFIG_MAX_MSG_SIZE,
			g_strdup_printf("%d", OSYNC_PLUGIN_SYNCML_MAX_MSG_SIZE),
			&error))
		goto error;
	if (!smlDataSyncSetOption(
			dsObject,
			SML_DATA_SYNC_CONFIG_MAX_OBJ_SIZE,
			g_strdup_printf("%d",OSYNC_PLUGIN_SYNCML_MAX_OBJ_SIZE),
			&error))
		goto error;

	conn = osync_plugin_config_get_connection(config);
	if (!conn) {
		osync_error_set(oerror, OSYNC_ERROR_MISCONFIGURATION, "No configuration of connection available.");
		goto oerror;
	}

	switch(osync_plugin_connection_get_type(conn)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:
			if (!smlDataSyncSetOption(
					dsObject,
					SML_DATA_SYNC_CONFIG_CONNECTION_TYPE,
					SML_DATA_SYNC_CONFIG_CONNECTION_BLUETOOTH,
					&error))
				goto error;
			if (!smlDataSyncSetOption(
					dsObject,
					SML_TRANSPORT_CONFIG_BLUETOOTH_ADDRESS,
					osync_plugin_connection_bt_get_addr(conn),
					&error))
				goto error;
			if (osync_plugin_connection_bt_get_channel(conn))
			{
				char *channel = g_strdup_printf("%u", osync_plugin_connection_bt_get_channel(conn));
				if (!smlDataSyncSetOption(
						dsObject,
						SML_TRANSPORT_CONFIG_BLUETOOTH_CHANNEL,
						channel, &error))
				{
					smlSafeCFree(&channel);
					goto error;
				}
				smlSafeCFree(&channel);
			}
			break;
		case OSYNC_PLUGIN_CONNECTION_USB:
			/* TODO: osync_plugin_connection_usb_get_interface(conn); */
			if (!smlDataSyncSetOption(
					dsObject,
					SML_DATA_SYNC_CONFIG_CONNECTION_TYPE,
					SML_DATA_SYNC_CONFIG_CONNECTION_USB,
					&error))
				goto error;
			if (osync_plugin_connection_usb_get_interface(conn))
			{
				char *port = g_strdup_printf("%u", osync_plugin_connection_usb_get_interface(conn));
				if (!smlDataSyncSetOption(
						dsObject,
						SML_TRANSPORT_CONFIG_PORT,
						port, &error))
				{
					smlSafeCFree(&port);
					goto error;
				}
				smlSafeCFree(&port);
			}
			break;
		case OSYNC_PLUGIN_CONNECTION_SERIAL:
			/* TODO serial */
			if (!smlDataSyncSetOption(
					dsObject,
					SML_DATA_SYNC_CONFIG_CONNECTION_TYPE,
					SML_DATA_SYNC_CONFIG_CONNECTION_SERIAL,
					&error))
				goto error;
			break;
		case OSYNC_PLUGIN_CONNECTION_IRDA:
			/* TODO IRDA */
			if (!smlDataSyncSetOption(
					dsObject,
					SML_DATA_SYNC_CONFIG_CONNECTION_TYPE,
					SML_DATA_SYNC_CONFIG_CONNECTION_IRDA,
					&error))
				goto error;
			break;
		case OSYNC_PLUGIN_CONNECTION_NETWORK:
			/* TODO Network */
			if (tsp == SML_TRANSPORT_OBEX_CLIENT &&
			    !smlDataSyncSetOption(
					dsObject,
					SML_DATA_SYNC_CONFIG_CONNECTION_TYPE,
					SML_DATA_SYNC_CONFIG_CONNECTION_NET,
					&error))
				goto error;
			if (tsp == SML_TRANSPORT_HTTP_CLIENT)
			{
				url = g_strdup_printf("%s://%s:%d",
						osync_plugin_connection_net_get_protocol(conn),
						osync_plugin_connection_net_get_address(conn),
						osync_plugin_connection_net_get_port(conn));
			} else {
				char *port = g_strdup_printf("%u", osync_plugin_connection_net_get_port(conn));
				if (!smlDataSyncSetOption(
						dsObject,
						SML_TRANSPORT_CONFIG_PORT,
						port, &error))
				{
					smlSafeCFree(&port);
					goto error;
				}
				smlSafeCFree(&port);
			}
			break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
		default:
			osync_error_set(oerror, OSYNC_ERROR_MISCONFIGURATION, "Unsupported connection type configured.");
			goto oerror;
			break;
	}

	OSyncPluginAuthentication *auth = osync_plugin_config_get_authentication(config);
	if (auth) {
		const char *value = NULL;
		value = osync_plugin_authentication_get_username(auth);
		if (value != NULL && strlen(value) == 0)
			value = NULL;
		if (value &&
		    !smlDataSyncSetOption(
				dsObject,
				SML_DATA_SYNC_CONFIG_AUTH_USERNAME,
				value, &error))
			goto error;
		value = osync_plugin_authentication_get_password(auth);
		if (value != NULL && strlen(value) == 0)
			value = NULL;
		if (value &&
		    !smlDataSyncSetOption(
				dsObject,
				SML_DATA_SYNC_CONFIG_AUTH_PASSWORD,
				value, &error))
			goto error;
	}

	OSyncList *optslist = osync_plugin_config_get_advancedoptions(config);
	for (; optslist; optslist = optslist->next) {
		OSyncPluginAdvancedOption *option = optslist->data;

		const char *val = osync_plugin_advancedoption_get_value(option);
		const char *name = osync_plugin_advancedoption_get_name(option);
		g_assert(name);
		g_assert(val);
		const char *key = NULL;

		if (!strcmp(SYNCML_PLUGIN_CONFIG_SANVERSION, name)) {
			key = SML_DATA_SYNC_CONFIG_VERSION;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_WBXML, name)) {
			key = SML_DATA_SYNC_CONFIG_USE_WBXML;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATCOMMAND, name)) {
			key = SML_TRANSPORT_CONFIG_AT_COMMAND;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATMANUFACTURER, name)) {
			key = SML_TRANSPORT_CONFIG_AT_MANUFACTURER;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ATMODEL, name)) {
			key = SML_TRANSPORT_CONFIG_AT_MODEL;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_IDENTIFIER, name)) {
			key = SML_DATA_SYNC_CONFIG_IDENTIFIER;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_USESTRINGTABLE, name)) {
			key = SML_DATA_SYNC_CONFIG_USE_STRING_TABLE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_USETIMEANCHOR, name)) {
			key = SML_DATA_SYNC_CONFIG_USE_TIMESTAMP_ANCHOR;
		/* TODO: Dead option? */
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ONLYREPLACE, name)) {
			key = SML_DATA_SYNC_CONFIG_ONLY_REPLACE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_MAXMSGSIZE, name)) {
			if (atoi(val))
				key = SML_DATA_SYNC_CONFIG_MAX_MSG_SIZE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_MAXOBJSIZE, name)) {
			if (atoi(val))
				key = SML_DATA_SYNC_CONFIG_MAX_OBJ_SIZE;
		/* XXX Workaround for mobiles which only handle localtime! */
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_ONLYLOCALTIME, name)) {
			key = SML_DATA_SYNC_CONFIG_USE_LOCALTIME;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_PROXY, name)) {
			key = SML_TRANSPORT_CONFIG_PROXY;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_PATH, name)) {
			/* build URL together with connection config */
			char *value = g_strdup_printf("%s%s", url, val);
			safe_cfree(&url);
			if (!smlDataSyncSetOption(
					dsObject,
					SML_TRANSPORT_CONFIG_URL,
					value, &error)) {
				safe_cfree(&value);
				goto error;
			}
			safe_cfree(&value);
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_CAFILE, name)) {
			key = SML_TRANSPORT_CONFIG_SSL_CA_FILE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_AUTH_TYPE, name)) {
			key = SML_DATA_SYNC_CONFIG_AUTH_TYPE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_FAKE_DEVICE, name)) {
			key = SML_DATA_SYNC_CONFIG_FAKE_DEVICE;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_FAKE_MANUFACTURER, name)) {
			key = SML_DATA_SYNC_CONFIG_FAKE_MANUFACTURER;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_FAKE_MODEL, name)) {
			key = SML_DATA_SYNC_CONFIG_FAKE_MODEL;
		} else if (!strcmp(SYNCML_PLUGIN_CONFIG_FAKE_SOFTWARE_VERSION, name)) {
			key = SML_DATA_SYNC_CONFIG_FAKE_SOFTWARE_VERSION;
		}

		if (key &&
		    !smlDataSyncSetOption(dsObject, key, val, &error))
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s - TRUE", __func__);
	return TRUE;
error:
	osync_error_set(oerror, OSYNC_ERROR_MISCONFIGURATION, "%s", smlErrorPrint(&error));
	smlErrorDeref(&error);
oerror:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(oerror));
	return FALSE;
}

void *syncml_init(
		SmlSessionType sessionType,
		SmlTransportType tspType,
		OSyncPlugin *plugin,
		OSyncPluginInfo *info,
		OSyncError **oerror)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, oerror);
	SmlError *error = NULL;
	
	SmlPluginEnv *env = osync_try_malloc0(sizeof(SmlPluginEnv), oerror);
	if (!env)
		goto error;
	env->sessionType = sessionType;
	env->pluginInfo = info;
	env->gotDatabaseCommits = 0;
	osync_plugin_info_ref(env->pluginInfo);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
        osync_trace(TRACE_INTERNAL, "The config: %p", config);

	/* create data sync object */
	env->dsObject1 = smlDataSyncNew(sessionType, tspType, &error);
	if (!env->dsObject1)
		goto error_free_env;
	if (sessionType == SML_SESSION_TYPE_CLIENT)
	{
		env->dsObject2 = smlDataSyncNew(sessionType, tspType, &error);
		if (!env->dsObject2)
			goto error_free_env;
	}

	/* configure the instance */
	if (!parse_config(tspType, env->dsObject1, config, oerror))
		goto error_free_env;
	if (sessionType == SML_SESSION_TYPE_CLIENT)
	{
		if (!parse_config(tspType, env->dsObject2, config, oerror))
			goto error_free_env;
	}

	/* prepare the function list for OpenSync */
	OSyncObjTypeSinkFunctions main_functions;
	memset(&main_functions, 0, sizeof(main_functions));
	main_functions.connect = syncml_connect;
	main_functions.disconnect = disconnect;

	/* Register main sink for connect and disconnect functions */
	OSyncObjTypeSink *mainsink = osync_objtype_main_sink_new(oerror);
	if (!mainsink)
		goto error_free_env;

	osync_objtype_sink_set_functions(mainsink, main_functions, env);
	osync_plugin_info_set_main_sink(info, mainsink);
	osync_objtype_sink_unref(mainsink);

	/* prepare paths for callbacks */
	env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	env->devinf_path = g_strdup_printf("%s/devinf.db", osync_plugin_info_get_configdir(info));

	/* set callbacks */
	smlDataSyncRegisterEventCallback(env->dsObject1, _recv_event, env);
	smlDataSyncRegisterGetAlertTypeCallback(env->dsObject1, _get_alert_type, env);
	smlDataSyncRegisterGetAnchorCallback(env->dsObject1, _get_anchor, env);
	smlDataSyncRegisterSetAnchorCallback(env->dsObject1, _set_anchor, env);
	smlDataSyncRegisterWriteDevInfCallback(env->dsObject1, _write_devinf, env);
	smlDataSyncRegisterReadDevInfCallback(env->dsObject1, _read_devinf, env);
	smlDataSyncRegisterHandleRemoteDevInfCallback(env->dsObject1, _handle_remote_devinf, env);
	smlDataSyncRegisterChangeStatusCallback(env->dsObject1, _recv_change_status);
	if (sessionType == SML_SESSION_TYPE_CLIENT)
	{
		smlDataSyncRegisterEventCallback(env->dsObject2, _recv_event, env);
		smlDataSyncRegisterGetAlertTypeCallback(env->dsObject2, _get_alert_type, env);
		smlDataSyncRegisterGetAnchorCallback(env->dsObject2, _get_anchor, env);
		smlDataSyncRegisterSetAnchorCallback(env->dsObject2, _set_anchor, env);
		smlDataSyncRegisterWriteDevInfCallback(env->dsObject2, _write_devinf, env);
		smlDataSyncRegisterReadDevInfCallback(env->dsObject2, _read_devinf, env);
		smlDataSyncRegisterHandleRemoteDevInfCallback(env->dsObject2, _handle_remote_devinf, env);
		smlDataSyncRegisterChangeStatusCallback(env->dsObject2, _recv_change_status);
	}
	osync_trace(TRACE_INTERNAL, "%s: config loaded", __func__);

	/* configure databases */
	if (sessionType == SML_SESSION_TYPE_SERVER &&
	    !ds_server_init_databases(env, info, oerror))
		goto error_free_env;
	if (sessionType == SML_SESSION_TYPE_CLIENT &&
	    !ds_client_init_databases(env, info, oerror))
		goto error_free_env;

	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	finalize(env);
error:
	if (error) {
		osync_error_set(oerror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
		smlErrorDeref(&error);
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(oerror));
	return NULL;
}

