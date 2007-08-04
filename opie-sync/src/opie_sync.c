/*

   Copyright 2005 Paul Eggleton & Holger Hans Peter Freyther

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

#include "opie_sync.h"
#include "opie_comms.h"
#include "opie_xml.h"

GTree *uidmap_read(const char *uidmap_file);
void uidmap_write(GTree *uidmap, const char *uidmap_file);
void uidmap_free(GTree **uidmap);
void uidmap_addmapping(GTree *uidmap, const char *opie_uid, const char *ext_uid);
char *uidmap_set_node_uid(GTree *uidmap, xmlNode *node, xmlDoc *doc, 
												 const char *listelement, const char *itemelement, const char *ext_uid);
const char *uidmap_get_mapped_uid(GTree *uidmap, const char *uid);
void uidmap_removemapping(GTree *uidmap, const char *uid1);
		
/* sync_cancelled()
 * 
 * Callback from the opie monitor thread.
 */
void sync_cancelled(void)
{
	/* FIXME handle cancelling from the Opie end */
	/*user_cancelled_sync = TRUE;*/
}

static osync_bool opie_sync_settings_parse(OpiePluginEnv *env, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	/* Set defaults */
	env->username = g_strdup("root");
	env->password = g_strdup("rootme");
	env->url = g_strdup("192.168.0.202");
	env->device_type = OPIE_SYNC_OPIE;
	env->conn_type = OPIE_CONN_FTP;
	env->device_port = 4242;
	env->use_qcop = TRUE;
	env->backupdir = NULL;

	doc = xmlParseMemory(config, strlen(config));

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		goto error;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get configuration XML root element");
		goto error_free_doc;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Configuration file root node name is invalid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if(cur->type == XML_ELEMENT_NODE) {
			char *str = (char *)xmlNodeGetContent(cur);
			if (str) {
				if (!xmlStrcmp(cur->name, (const xmlChar *)"username")) {
					g_free(env->username);
					env->username = g_strdup(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"password")) {
					g_free(env->password);
					env->password = g_strdup(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"url")) {
					g_free(env->url);
					env->url = g_strdup(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"port")) {
					env->device_port = atoi(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"device")) {
					if (!strcasecmp(str, "qtopia2"))
						env->device_type = OPIE_SYNC_QTOPIA_2;
					else
						env->device_type = OPIE_SYNC_OPIE;
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"conntype")) {
					if (!strcasecmp(str, "scp"))
						env->conn_type = OPIE_CONN_SCP;
					else if ( strcasecmp(str, "none") == 0 )
						env->conn_type = OPIE_CONN_NONE;
					else
						env->conn_type = OPIE_CONN_FTP;
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"use_qcop")) {
					if ( strcasecmp(str, "false") == 0 )
						env->use_qcop = FALSE;
					else 
						env->use_qcop = TRUE;
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"backupdir")) {
					if(strlen(str) > 0)
						env->backupdir = g_strdup(str);
				} else {
					osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid configuration file option \"%s\"", cur->name);
					goto error_free_doc;
				}
				xmlFree(str);
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error_free_doc:
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _connectDevice(OpiePluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	char* errmsg = NULL;
	
	if (env->qcopconn)
	{
		osync_trace(TRACE_EXIT, "%s: Already connected", __func__);
		return TRUE;
	}

	/* Connect to QCopBridgeServer to lock GUI and get root path */
	if ( env->use_qcop ) 
	{
		osync_trace(TRACE_INTERNAL, "qcop_connect");
		env->qcopconn = qcop_connect(env->url,
		                             env->username,
		                             env->password);
		if (env->qcopconn->result)
		{
			qcop_start_sync(env->qcopconn, &sync_cancelled);
			if (!env->qcopconn->result)
			{
				osync_trace(TRACE_INTERNAL, "qcop_start_sync_failed");
				errmsg = g_strdup(env->qcopconn->resultmsg);
				qcop_stop_sync(env->qcopconn);
				qcop_freeqconn(env->qcopconn);
				env->qcopconn = NULL;
				osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
				goto error;
			}
		}
		else
		{
			osync_trace(TRACE_INTERNAL, "QCop connection failed");
			errmsg = g_strdup(env->qcopconn->resultmsg);
			qcop_freeqconn(env->qcopconn);
			env->qcopconn = NULL;
			osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
			goto error;
		}
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

void _disconnectDevice(OpiePluginEnv *env)
{
	if(env->qcopconn) 
	{
		qcop_stop_sync(env->qcopconn);
		if (!env->qcopconn->result)
		{
			osync_trace(TRACE_INTERNAL, env->qcopconn->resultmsg);
		}
		qcop_disconnect(env->qcopconn); /* frees qcopconn */
		env->qcopconn = NULL;
	}
}

static void connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);

	OSyncError *error = NULL;

	if(!env->plugin_env->connected) {
		/* We only want to connect once per session */
		if (!_connectDevice(env->plugin_env, &error))
			goto error;
		env->plugin_env->connected = TRUE;
	}
	
	if(!env->plugin_env->categories_doc) {
		/* Fetch categories */
		opie_fetch_file(env->plugin_env, OPIE_OBJECT_TYPE_CATEGORY, OPIE_CATEGORY_FILE, &env->plugin_env->categories_doc, NULL);
	}
	
	/* pull the required data back */
	if(!opie_fetch_sink(env))
	{
		/* failed */
		char *errmsg;
		if(env->plugin_env->qcopconn)
		{
			qcop_stop_sync(env->plugin_env->qcopconn);
			if(!env->plugin_env->qcopconn->result)
			{
				osync_trace(TRACE_INTERNAL, "qcop_stop_sync_failed");
				char *errmsg = g_strdup(env->plugin_env->qcopconn->resultmsg);
				qcop_freeqconn(env->plugin_env->qcopconn);
				env->plugin_env->qcopconn = NULL;
				osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
				goto error;
			} 
			qcop_disconnect(env->plugin_env->qcopconn);
			env->plugin_env->qcopconn = NULL;
		}
		errmsg = g_strdup_printf("Failed to load data from device %s", env->plugin_env->url);
		osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
		g_free(errmsg);
		goto error;
	}

	/* Get hashtable */
	const char *configdir = osync_plugin_info_get_configdir(info);
	char *tablepath = g_strdup_printf("%s/hashtable.db", configdir);
	env->hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), &error);
	g_free(tablepath);
	if (!env->hashtable)
		goto error;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void get_changes(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);

	OSyncError *error = NULL;

	if (osync_objtype_sink_get_slowsync(sink)) {
		osync_trace(TRACE_INTERNAL, "Slow sync requested");
		osync_hashtable_reset(env->hashtable);
	}

	xmlNode *item_node = opie_xml_get_first(env->doc, env->listelement, env->itemelement);
	while(item_node)
	{
		/* Convert category IDs to names that other systems can use */
		char *categories_bkup = opie_xml_get_categories(item_node);
		if(env->plugin_env->categories_doc && categories_bkup)
			opie_xml_category_ids_to_names(env->plugin_env->categories_doc, item_node);
		
		char *opie_uid = opie_xml_get_uid(item_node);
		char *uid = NULL;
		if(opie_uid) {
			const char *uidentry = uidmap_get_mapped_uid(env->plugin_env->uidmap, opie_uid);
			if(uidentry)
				uid = g_strdup(uidentry);
			else if(!strcasecmp(item_node->name, "note"))
				uid = g_strdup(opie_uid);
			else {
				uid = opie_xml_get_tagged_uid(item_node);
				if(opie_uid) {
					uidmap_addmapping(env->plugin_env->uidmap, opie_uid, uid);
					uidmap_addmapping(env->plugin_env->uidmap, uid, opie_uid);
				}
			}
			g_free(opie_uid);
		}
		
		char *data = xml_node_to_text(env->doc, item_node);
		printf("OPIE: uid %s\n", uid);
		printf("OPIE: change xml = %s\n", data);
		
		unsigned char *hash = hash_xml_node(env->doc, item_node);

		/* Restore old categories value as we don't want to save this back to our XML file */
		if(categories_bkup) {
			opie_xml_set_categories(item_node, categories_bkup);
			g_free(categories_bkup);
		}
		
		// Report every entry .. every unreported entry got deleted.
		osync_hashtable_report(env->hashtable, uid);

		OSyncChangeType changetype = osync_hashtable_get_changetype(env->hashtable, uid, hash);

		if (changetype == OSYNC_CHANGE_TYPE_UNMODIFIED) {
			g_free(hash);
			g_free(uid);
			g_free(data);
			continue;
		}

		//Set the hash of the object (optional, only required if you use hashtabled)
		osync_hashtable_update_hash(env->hashtable, changetype, uid, hash);

		//Make the new change to report
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		//Now set the uid of the object
		osync_change_set_uid(change, uid);
		osync_change_set_hash(change, hash);
		osync_change_set_changetype(change, changetype);

		g_free(hash);

		OSyncData *odata = osync_data_new(data, strlen(data) + 1, env->objformat, &error);
		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

		//Now you can set the data for the object
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		// just report the change via
		osync_context_report_change(ctx, change);

		osync_change_unref(change);

		g_free(uid);
		
		item_node = opie_xml_get_next(item_node);
	}

	//When you are done looping and if you are using hashtables
	//check for deleted entries ... via hashtable
	int i;
	char **uids = osync_hashtable_get_deleted(env->hashtable);
	for (i=0; uids[i]; i++) {
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			g_free(uids[i]);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_change_set_uid(change, uids[i]);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

		OSyncData *odata = osync_data_new(NULL, 0, env->objformat, &error);
		if (!odata) {
			g_free(uids[i]);
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		osync_context_report_change(ctx, change);

		osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), NULL);

		osync_change_unref(change);
		g_free(uids[i]);
	}
	g_free(uids);

	//Now we need to answer the call
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void commit_change(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);

	OSyncError *error = NULL;
	const char *ext_uid = osync_change_get_uid(change);
	char *opie_uid = NULL;
	unsigned char *hash = NULL;
	
	xmlNode *change_node = NULL;
	xmlDoc *change_doc = NULL;
	OSyncData *change_data = osync_change_get_data(change);
	if(change_data) {
		/* Make data into a null-terminated string */
		unsigned int change_size;
		char *change_str = NULL;
		osync_data_get_data(change_data, &change_str, &change_size);
		/* Parse the string as XML */
		change_doc = opie_xml_change_parse(change_str, &change_node);
		if(!change_doc) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to retrieve XML from change");
			goto error;
		}
		opie_uid = uidmap_set_node_uid(env->plugin_env->uidmap, change_node, env->doc, env->listelement, env->itemelement, ext_uid);
		/* Convert categories into names that other systems can use */
		if(env->plugin_env->categories_doc)
			opie_xml_category_names_to_ids(env->plugin_env->categories_doc, change_node);
		
		/* Get hash */
		hash = hash_xml_node(env->doc, change_node);
	}
	
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if(!opie_uid) {
				const char *uidentry = uidmap_get_mapped_uid(env->plugin_env->uidmap, ext_uid);
				if(uidentry)
					opie_uid = g_strdup(uidentry);
				else if(!strcmp(env->itemelement, "note"))
					opie_uid = g_strdup(ext_uid);
				else
					opie_uid = opie_xml_strip_uid(ext_uid, env->itemelement);
			}
			opie_xml_remove_by_uid(env->doc, env->listelement, env->itemelement, opie_uid);
			uidmap_removemapping(env->plugin_env->uidmap, ext_uid);
			break;
		
		case OSYNC_CHANGE_TYPE_ADDED:
			if(change_node) {
				opie_xml_add_node(env->doc, env->listelement, change_node);
			}
			else
			{
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Change data expected, none passed");
				goto error;
			}
			osync_change_set_hash(change, hash);
			break;
		
		case OSYNC_CHANGE_TYPE_MODIFIED:
			if(change_node) {
				opie_xml_update_node(env->doc, env->listelement, change_node);
			}
			else
			{
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Change data expected, none passed");
				goto error;
			}
			osync_change_set_hash(change, hash);
			break;
		
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unknown change type");
			goto error;
	}

	osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), hash);

	//Answer the call
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);
	OSyncError *error = NULL;
	
	if ( !opie_put_sink(env) ) {
		osync_trace( TRACE_INTERNAL, "opie_connect_and_put failed" );
		char *errmsg = g_strdup_printf( "Failed to send data to device %s", env->plugin_env->url ); /* FIXME specify which data */
		osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
		g_free(errmsg);
		goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);
	
	/* Close the hashtable */
	osync_hashtable_free(env->hashtable);
	env->hashtable = NULL;

	//Answer the call
	osync_context_report_success(ctx);
}

OpieSinkEnv *opie_sync_create_sink_env(OpiePluginEnv *env, OSyncPluginInfo *info, const char *objtype, const char *objformat, OPIE_OBJECT_TYPE opie_objtype, const char *remotefile, const char *listelement, const char *itemelement, OSyncError **error)
{
	OSyncObjTypeSink *sink = osync_objtype_sink_new(objtype, error);
	if (!sink)
		return NULL;
	
	OpieSinkEnv *sink_env = osync_try_malloc0(sizeof(OpieSinkEnv), error);
	if (!sink_env)
		return NULL;
	sink_env->plugin_env = env; /* back-pointer */
	sink_env->sink = sink;
	sink_env->listelement = listelement;
	sink_env->itemelement = itemelement;
	sink_env->remotefile = remotefile;
	sink_env->objtype = opie_objtype;
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	sink_env->objformat = osync_format_env_find_objformat(formatenv, objformat);

	osync_objtype_sink_add_objformat(sink, objformat);

	/* Every sink can have different functions ... */
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.connect = connect;
	functions.disconnect = disconnect;
	functions.get_changes = get_changes;
	functions.commit = commit_change;
	functions.sync_done = sync_done;

	/* We pass the sink_env object to the sink, so we dont have to look it up
		* again once the functions are called */
	osync_objtype_sink_set_functions(sink, functions, sink_env);
	osync_plugin_info_add_objtype(info, sink);
	
	return sink_env;
}

static void* opie_sync_initialize( OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error )
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);

	OpiePluginEnv *env = osync_try_malloc0(sizeof(OpiePluginEnv), error);
	if (!env)
		goto error;
	
	const char *configdata = osync_plugin_info_get_config(info);
	if (!configdata) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get config data.");
		goto error_free_env;
	}

	osync_trace(TRACE_INTERNAL, "The config: %s", osync_plugin_info_get_config(info));
	
	if (!opie_sync_settings_parse(env, configdata, error))
		goto error_free_env;
	
	env->backuppath = NULL;
	
	/* Contacts sink */
	env->contact_env = opie_sync_create_sink_env(env, info, "contact", OPIE_FORMAT_XML_CONTACT, OPIE_OBJECT_TYPE_CONTACT, OPIE_ADDRESS_FILE, "Contacts", "Contact", error);
	if(!env->contact_env)
		goto error_free_env;
	
	/* Todos sink */
	env->todo_env = opie_sync_create_sink_env(env, info, "todo", OPIE_FORMAT_XML_TODO, OPIE_OBJECT_TYPE_TODO, OPIE_TODO_FILE, "Tasks", "Task", error);
	if(!env->todo_env)
		goto error_free_env;
	
	/* Events sink */
	env->event_env = opie_sync_create_sink_env(env, info, "event", OPIE_FORMAT_XML_EVENT, OPIE_OBJECT_TYPE_EVENT, OPIE_CALENDAR_FILE, "events", "event", error);
	if(!env->event_env)
		goto error_free_env;
	
	/* Notes sink */
	env->note_env = opie_sync_create_sink_env(env, info, "note", OPIE_FORMAT_XML_NOTE, OPIE_OBJECT_TYPE_NOTE, NULL, "notes", "note", error);
	if(!env->note_env)
		goto error_free_env;
	
	env->qcopconn = NULL;
	env->connected = FALSE;
	
	const char *configdir = osync_plugin_info_get_configdir(info);
	char *uidmap_path = g_strdup_printf("%s/opie_uidmap.xml", configdir);
	env->uidmap = uidmap_read(uidmap_path);
	g_free(uidmap_path);
	
	comms_init();
	
	osync_trace(TRACE_EXIT, "%s, %p", __func__, env);
	return (void *)env;

error_free_env:
	g_free(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void opie_sync_finalize( void* userdata )
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, userdata);
	OpiePluginEnv *env = (OpiePluginEnv *)userdata;

	/* Put categories */
	if(env->categories_doc && env->categories_doc->_private == 0) {
		opie_put_file(env, OPIE_OBJECT_TYPE_CATEGORY, OPIE_CATEGORY_FILE, env->categories_doc);
	}
	
	_disconnectDevice(env);

	comms_shutdown();
	
	uidmap_write(env->uidmap, env->uidmap_file);
	uidmap_free(&env->uidmap);
	g_free(env->uidmap_file);
	
	g_free(env->contact_env);
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}


void uidmap_addmapping(GTree *uidmap, const char *uid1, const char *uid2) {
	char *key = g_strdup(uid1);
	char *value = g_strdup(uid2);
	g_tree_insert(uidmap, key, value);
}

void uidmap_removemapping(GTree *uidmap, const char *uid1) {
	const char *uid2 = uidmap_get_mapped_uid(uidmap, uid1);
	if(uid2) {
		char *uid2_copy = g_strdup(uid2);
		g_tree_remove(uidmap, uid1);
		g_tree_remove(uidmap, uid2_copy);
		g_free(uid2_copy);
	}
}

char *uidmap_set_node_uid(GTree *uidmap, xmlNode *node, xmlDoc *doc, 
												 const char *listelement, const char *itemelement, const char *ext_uid) {
	const char *uidentry = uidmap_get_mapped_uid(uidmap, ext_uid);
	char *opie_uid;
	if(uidentry) {
		opie_xml_set_uid(node, uidentry);
		opie_uid = g_strdup(uidentry);
	}
	else {
		if(!strcmp(node->name,"note")) {
			opie_uid = g_strdup(ext_uid);
		}
		else {
			opie_uid = opie_xml_set_ext_uid(node, doc, listelement, itemelement, ext_uid);
			uidmap_addmapping(uidmap, opie_uid, ext_uid);
			uidmap_addmapping(uidmap, ext_uid, opie_uid);
		}
	}
	return opie_uid;
}

const char *uidmap_get_mapped_uid(GTree *uidmap, const char *uid) {
	const char *uidentry = (const char *)g_tree_lookup(uidmap, uid);
	return uidentry;
}

gint uidmap_compare(gconstpointer a, gconstpointer b, gpointer user_data) {
	return strcmp((const char *)a, (const char *)b);
}

GTree *uidmap_read(const char *uidmap_file) {
	GTree *uidmap = g_tree_new_full(uidmap_compare, NULL, g_free, g_free);
	xmlDoc *doc = opie_xml_file_open(uidmap_file);
	if(doc) {
		xmlNode *node = opie_xml_get_first(doc, "mappinglist", "mapping");
		while(node) {
			char *uid1 = xmlGetProp(node, "uid1");
			if(uid1) {
				char *uid2 = xmlGetProp(node, "uid2");
				if(uid2) {
					uidmap_addmapping(uidmap, uid1, uid2);
					xmlFree(uid2);
				}
				xmlFree(uid1);
			}
			node = opie_xml_get_next(node);
		}
	}
	return uidmap;
}

gboolean uidmap_write_entry(gpointer key, gpointer value, gpointer data) {
	xmlNode *node = xmlNewNode(NULL, "mapping");
	xmlSetProp(node, "uid1", (const char *)key);
	xmlSetProp(node, "uid2", (const char *)value);
	xmlAddChild((xmlNode *)data, node);
	return FALSE;
}

void uidmap_write(GTree *uidmap, const char *uidmap_file) {
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(doc) {
		xmlNode *root = xmlNewNode(NULL, "uidmap");
		xmlDocSetRootElement(doc, root);
		xmlNode *listnode = xmlNewNode(NULL, "mappinglist");
		xmlAddChild(root, listnode);
		
		g_tree_foreach(uidmap, uidmap_write_entry, listnode); 
		
		xmlSaveFormatFile(uidmap_file, doc, 1);
	}
}

void uidmap_free(GTree **uidmap) {
	g_tree_destroy(*uidmap);
	uidmap = NULL;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	/*
		* Initial Names
		*/
	osync_plugin_set_name(plugin, "opie-sync");
	osync_plugin_set_longname(plugin, "Opie Synchronization Plugin");
	osync_plugin_set_description(plugin, "Synchronize with Opie/Qtopia based devices");

	/* Now set the function we made earlier */
	osync_plugin_set_initialize(plugin, opie_sync_initialize);
	osync_plugin_set_finalize(plugin, opie_sync_finalize);
/*	osync_plugin_set_discover(plugin, discover);*/

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);
	
  /*
		* Function pointers
		*/
/*	info->functions.is_available   = opie_sync_is_available;
	info->functions.initialize     = opie_sync_initialize;
	info->functions.finalize       = opie_sync_finalize;
	info->functions.connect        = opie_sync_connect;
	info->functions.disconnect     = opie_sync_disconnect;
	info->functions.sync_done      = opie_sync_sync_done;
	info->functions.get_changeinfo = opie_sync_get_changeinfo;
*/

	/*
		* Object types
		*/
/*
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "opie-xml-contact", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "opie-xml-contact", opie_sync_contact_commit);
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "opie-xml-todo", NULL);
	osync_plugin_set_commit_objformat(info, "todo",    "opie-xml-todo",    opie_sync_todo_commit);
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "opie-xml-event", NULL);
	osync_plugin_set_commit_objformat(info, "event",   "opie-xml-event",   opie_sync_event_commit);
	osync_plugin_accept_objtype(info, "note");
	osync_plugin_accept_objformat(info, "note", "opie-xml-note", NULL);
	osync_plugin_set_commit_objformat(info, "note",   "opie-xml-note",     opie_sync_note_commit);
*/
  
	return TRUE;
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
