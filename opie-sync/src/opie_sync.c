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
#include "opie_xml_utils.h"

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
	env->host = g_strdup("192.168.0.202");
	env->device_type = OPIE_SYNC_OPIE;
	env->conn_type = OPIE_CONN_FTP;
	env->device_port = 4242;
	env->use_qcop = TRUE;
	env->backupdir = NULL;
	env->localdir = g_strdup("/tmp");
	env->notes_type = NOTES_TYPE_BASIC;

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
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"hostname")) {
					g_free(env->host);
					env->host = g_strdup(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"url")) {
					osync_trace(TRACE_INTERNAL, "The 'url' configuration parameter is deprecated - please use 'hostname' instead");
					g_free(env->host);
					env->host = g_strdup(str);
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
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"localdir")) {
					g_free(env->localdir);
					env->localdir = g_strdup(str);
				} else if (!xmlStrcmp(cur->name, (const xmlChar *)"notestype")) {
					if ( (!strcasecmp(str, "opie-notes")) || (!strcasecmp(str, "opie_notes")) )
						env->notes_type = NOTES_TYPE_OPIE_NOTES;
					else if ( strcasecmp(str, "basic") == 0 )
						env->notes_type = NOTES_TYPE_BASIC;
					else {
						osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid value \"%s\" for configuration option \"%s\"", str, cur->name);
						goto error_free_doc;
					}
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


/**
 * If QCop is enabled, connect to the the QCop bridge on the remote device and
 * signal that a sync is starting
 */
static osync_bool device_connect(OpiePluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	
	if (env->qcopconn) {
		osync_trace(TRACE_EXIT, "%s: Already connected", __func__);
		return TRUE;
	}

	if ( env->use_qcop ) {
		/* Connect to QCopBridgeServer to lock GUI and get root path */
		osync_trace(TRACE_INTERNAL, "qcop_connect");
		env->qcopconn = qcop_connect(env->host,
		                             env->username,
		                             env->password);
		if(!env->qcopconn->result) {
			char *errmsg = g_strdup_printf("qcop_connect failed: %s", env->qcopconn->resultmsg);
			osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
			g_free(errmsg);
			goto error;
		}
		
		qcop_start_sync(env->qcopconn, &sync_cancelled);
		if(!env->qcopconn->result) {
			char *errmsg = g_strdup_printf("qcop_start_sync_failed: %s", env->qcopconn->resultmsg);
			osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
			g_free(errmsg);
			qcop_stop_sync(env->qcopconn);
			goto error;
		}
		
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	if(env->qcopconn) {
		qcop_freeqconn(env->qcopconn);
		env->qcopconn = NULL;
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/**
 * If QCop is enabled & connected, signal that syncing has finished
 */
static osync_bool device_disconnect(OpiePluginEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	
	if(env->qcopconn) {
		qcop_stop_sync(env->qcopconn);
		if (!env->qcopconn->result) {
			char *errmsg = g_strdup_printf("qcop_stop_sync_failed: %s", env->qcopconn->resultmsg);
			osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
			g_free(errmsg);
			qcop_disconnect(env->qcopconn); /* frees qcopconn */
			env->qcopconn = NULL;
			goto error;
		}
		else {
			qcop_disconnect(env->qcopconn); /* frees qcopconn */
			env->qcopconn = NULL;
		}
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static void connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	OpieSinkEnv *env = osync_objtype_sink_get_userdata(sink);

	OSyncError *error = NULL;

	g_mutex_lock(env->plugin_env->plugin_mutex);
	
	if(!env->plugin_env->connected) {
		/* We only want to connect once per session */
		
		if (!device_connect(env->plugin_env, &error)) {
			g_mutex_unlock(env->plugin_env->plugin_mutex);
			goto error;
		}
		env->plugin_env->connected = TRUE;
	}
	
	if(!env->plugin_env->categories_doc) {
		/* Fetch categories */
		opie_fetch_file(env->plugin_env, OPIE_OBJECT_TYPE_CATEGORY, OPIE_CATEGORY_FILE, &env->plugin_env->categories_doc, NULL);
	}
	
	g_mutex_unlock(env->plugin_env->plugin_mutex);
	
	/* pull the required data back */
	if(!opie_fetch_sink(env))
	{
		/* failed */
		char *errmsg;
		device_disconnect(env->plugin_env, &error);
		errmsg = g_strdup_printf("Failed to load data from device %s", env->plugin_env->host);
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

	if(env->objtype == OPIE_OBJECT_TYPE_NOTE) {
		/* Check if the notestype config option has changed since the last sync */
		char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
		char *notes_type_str = g_strdup_printf("%d", env->plugin_env->notes_type);
		if (!osync_anchor_compare(anchorpath, "notestype", notes_type_str))
			osync_objtype_sink_set_slowsync(sink, TRUE);
		g_free(notes_type_str);
		g_free(anchorpath);
	}
	
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
		char *uid = opie_xml_get_tagged_uid(item_node);
		unsigned char *hash = hash_xml_node(env->doc, item_node);
		
		/* Convert category IDs to names that other systems can use */
		g_mutex_lock(env->plugin_env->plugin_mutex);
		char *categories_bkup = opie_xml_get_categories(item_node);
		if(env->plugin_env->categories_doc && categories_bkup)
			opie_xml_category_ids_to_names(env->plugin_env->categories_doc, item_node);
		g_mutex_unlock(env->plugin_env->plugin_mutex);
		
		char *data = xml_node_to_text(env->doc, item_node);

		/* Restore old categories value as we don't want to save this back to our XML file */
		if(categories_bkup) {
			opie_xml_set_categories(item_node, categories_bkup);
			g_free(categories_bkup);
		}
		
		// Report every entry .. every unreported entry got deleted.
		osync_hashtable_report(env->hashtable, uid);

		OSyncChangeType changetype = osync_hashtable_get_changetype(env->hashtable, uid, hash);

		if (changetype != OSYNC_CHANGE_TYPE_UNMODIFIED) {
			//Set the hash of the object (optional, only required if you use hashtabled)
			osync_hashtable_update_hash(env->hashtable, changetype, uid, hash);
	
			//Make the new change to report
			OSyncChange *change = osync_change_new(&error);
			if (!change) {
				osync_context_report_osyncwarning(ctx, error);
				osync_error_unref(&error);
			}
			else {
	
				//Now set the uid of the object
				osync_change_set_uid(change, uid);
				osync_change_set_hash(change, hash);
				osync_change_set_changetype(change, changetype);
		
				OSyncData *odata = osync_data_new(data, strlen(data) + 1, env->objformat, &error);
				if (!odata) {
					osync_change_unref(change);
					osync_context_report_osyncwarning(ctx, error);
					osync_error_unref(&error);
				}
				else {
					osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
			
					//Now you can set the data for the object
					osync_change_set_data(change, odata);
					osync_data_unref(odata);
			
					// just report the change via
					osync_context_report_change(ctx, change);
			
					osync_change_unref(change);
				}
			}
		}

		g_free(data);
		g_free(hash);
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
		if(change_str) {
			/* Parse the string as XML */
			change_doc = opie_xml_change_parse(change_str, &change_node);
			if(!change_doc) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to retrieve XML from change");
				goto error;
			}
			
			/* Get UID */
			if(!strcmp(env->itemelement,"note"))
				opie_uid = g_strdup(ext_uid);
			else
				opie_uid = opie_xml_set_ext_uid(change_node, env->doc, env->listelement, env->itemelement, ext_uid);
			
			/* Convert category names to category IDs that Opie can use */
			g_mutex_lock(env->plugin_env->plugin_mutex);
			if(env->plugin_env->categories_doc)
				opie_xml_category_names_to_ids(env->plugin_env->categories_doc, change_node);
			g_mutex_unlock(env->plugin_env->plugin_mutex);
			
			/* Get hash */
			hash = hash_xml_node(env->doc, change_node);
		}
	}
	
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if(!opie_uid) {
				if(!strcmp(env->itemelement, "note"))
					opie_uid = g_strdup(ext_uid);
				else
					opie_uid = opie_xml_strip_uid(ext_uid);
			}
			if(opie_uid)
				opie_xml_remove_by_uid(env->doc, env->listelement, env->itemelement, opie_uid);
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

	/* Flag document as changed */
	env->doc->_private = 0;

	if(change_doc)
		xmlFreeDoc(change_doc);

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
		char *errmsg = g_strdup_printf( "Failed to send data to device %s", env->plugin_env->host ); /* FIXME specify which data */
		osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
		g_free(errmsg);
		goto error;
	}
	
	if(env->objtype == OPIE_OBJECT_TYPE_NOTE) {
		char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
		char *notes_type_str = g_strdup_printf("%d", env->plugin_env->notes_type);
 		osync_anchor_update(anchorpath, "notestype", notes_type_str);
		g_free(notes_type_str);
		g_free(anchorpath);
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
	env->plugin_mutex = g_mutex_new();
	
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
	
	OSyncError *error = NULL;
	device_disconnect(env, &error);
	
	comms_shutdown();
	
	g_mutex_free(env->plugin_mutex);
	
	g_free(env->contact_env);
	g_free(env->todo_env);
	g_free(env->event_env);
	g_free(env->note_env);
	g_free(env->username);
	g_free(env->password);
	g_free(env->host);
	g_free(env->localdir);
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static osync_bool opie_sync_discover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	OpiePluginEnv *env = (OpiePluginEnv *)data;
	
	osync_objtype_sink_set_available(env->contact_env->sink, TRUE);
	osync_objtype_sink_set_available(env->todo_env->sink, TRUE);
	osync_objtype_sink_set_available(env->event_env->sink, TRUE);
	osync_objtype_sink_set_available(env->note_env->sink, TRUE);
	
	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "opie-sync");
	//osync_version_set_modelversion(version, "version");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	/* FIXME define capabilities */

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
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
	osync_plugin_set_discover(plugin, opie_sync_discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);
	
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
