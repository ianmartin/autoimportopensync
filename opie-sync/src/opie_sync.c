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


#include "opie_sync.h"
#include "opie_comms.h"
#include "opie_xml.h"


void uidmap_read(OpieSyncEnv *env);
void uidmap_write(OpieSyncEnv *env);
void uidmap_free(OpieSyncEnv *env);
void uidmap_addmapping(OpieSyncEnv *env, const char *opie_uid, const char *ext_uid);
char *uidmap_set_node_uid(OpieSyncEnv *env, xmlNode *node, xmlDoc *doc, 
												 const char *listelement, const char *itemelement, const char *ext_uid);
const char *uidmap_get_mapped_uid(OpieSyncEnv *env, const char *uid);
void uidmap_removemapping(OpieSyncEnv *env, const char *uid1);
		
/* sync_cancelled()
 * 
 * Callback from the opie monitor thread.
 */
void sync_cancelled(void)
{
	/* FIXME handle cancelling from the Opie end */
	/*user_cancelled_sync = TRUE;*/
}



static osync_bool opie_sync_settings_parse(OpieSyncEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	/* Set defaults */
	env->username = g_strdup("root");
	env->password = g_strdup("Qtopia");
	env->url = g_strdup("192.168.10.123");
	env->device_type = OPIE_SYNC_OPIE;
	env->conn_type = OPIE_CONN_FTP;
	env->device_port = 4242;
	env->use_qcop = TRUE;
	env->backupdir = NULL;

	doc = xmlParseMemory(config, size);

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

static osync_bool _connectDevice(OpieSyncEnv *env, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, error);
	char* errmsg = NULL;
	opie_object_type object_types = OPIE_OBJECT_TYPE_ANY;
	
	if (env->qcopconn)
	{
		osync_trace(TRACE_EXIT, "%s: Already connected", __func__);
		return TRUE;
	}

	if (!osync_hashtable_load(env->hashtable, env->member, error)) {
		goto error;
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

	/* connect to the device and pull the required data back */
	if(!opie_connect_and_fetch(env, object_types))
	{
		/* failed */
		if(env->qcopconn)
		{
			qcop_stop_sync(env->qcopconn);
			if(!env->qcopconn->result)
			{
				osync_trace(TRACE_INTERNAL, "qcop_stop_sync_failed");
				errmsg = g_strdup(env->qcopconn->resultmsg);
				qcop_freeqconn(env->qcopconn);
				env->qcopconn = NULL;
				osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
				goto error;
			} 
			qcop_disconnect(env->qcopconn);
			env->qcopconn = NULL;
		}
		errmsg = g_strdup_printf("Failed to load data from device %s", env->url);
		osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
		goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}



static osync_bool opie_sync_is_available( OSyncError** error)
{
	return TRUE;
}

static void* opie_sync_initialize( OSyncMember* member, OSyncError** error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);

	OpieSyncEnv *env = osync_try_malloc0(sizeof(OpieSyncEnv), error);
	if (!env)
		goto error;
	
	char *configdata = NULL;
	int configsize = 0;
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		goto error_free_env;
	}
	
	if (!opie_sync_settings_parse(env, configdata, configsize, error))
		goto error_free_config;
	
	env->member = member;
	g_free(configdata);
	
	env->hashtable = osync_hashtable_new();
	
	env->qcopconn = NULL;
	
	uidmap_read(env);
	
	osync_trace(TRACE_EXIT, "%s, %p", __func__, env);
	return (void *)env;

error_free_config:
	g_free(configdata);
error_free_env:
	g_free(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void opie_sync_finalize( void* data )
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	OpieSyncEnv *env = (OpieSyncEnv *)data;

	if (env->hashtable) {
		osync_hashtable_free(env->hashtable);
		env->hashtable = 0;
	}
	
	uidmap_write(env);
	uidmap_free(env);
	
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void opie_sync_connect( OSyncContext* ctx )
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;

	/* now connect with the device */
	if (!_connectDevice(env, &error))
		goto error;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static void opie_sync_sync_done( OSyncContext* ctx )
{
	osync_trace(TRACE_ENTRY, "%s", __func__ );
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;
	opie_object_type object_types = OPIE_OBJECT_TYPE_ANY;
	
	if ( !opie_connect_and_put(env, object_types) ) { 
		osync_trace( TRACE_INTERNAL, "opie_connect_and_put failed" );
		char *errmsg = g_strdup_printf( "Failed to send data to device %s", env->url );
		osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
		goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

static osync_bool opie_sync_item_commit(OSyncContext *ctx, OSyncChange *change, 
																				xmlDoc *doc, const char *listelement, 
																				const char *itemelement) {
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, \"%s\", \"%s\")", __func__, ctx, change, doc, listelement, itemelement);
	OSyncError *error = NULL;
	const char *ext_uid = osync_change_get_uid(change);
	char *opie_uid = NULL;
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	
	xmlNode *change_node = NULL;
	xmlDoc *change_doc = NULL;
	char *change_data = (char *)osync_change_get_data(change);
	if(change_data) {
		/* Make data into a null-terminated string */
		char *change_str = g_strndup(change_data, osync_change_get_datasize(change));
		/* Parse the string as XML */
		change_doc = opie_xml_change_parse(change_str, &change_node);
		if(!change_doc) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to retrieve XML from change");
			goto error;
		}
		opie_uid = uidmap_set_node_uid(env, change_node, doc, listelement, itemelement, ext_uid);
		/* Convert categories into names that other systems can use */
		if(env->categories_doc)
			opie_xml_category_names_to_ids(env->categories_doc, change_node);
	}
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			if(change_node) {
				opie_xml_update_node(doc, listelement, change_node);
			}
			else
			{
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Change data expected, none passed");
				goto error;
			}
			break;
		case CHANGE_ADDED:
			if(change_node) {
				opie_xml_add_node(doc, listelement, change_node);
			}
			else
			{
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Change data expected, none passed");
				goto error;
			}
			break;
		case CHANGE_DELETED:
			if(!opie_uid) {
				const char *uidentry = uidmap_get_mapped_uid(env, ext_uid);
				if(uidentry)
					opie_uid = g_strdup(uidentry);
				else
					opie_uid = opie_xml_strip_uid(ext_uid, itemelement);
			}
			opie_xml_remove_by_uid(doc, listelement, itemelement, opie_uid);
			uidmap_removemapping(env, ext_uid);
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	/* Flag document as changed */
	doc->_private = 0;
	
	if(change_doc)
		xmlFreeDoc(change_doc);
	
	g_free(opie_uid);
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

static osync_bool opie_sync_contact_commit(OSyncContext *ctx, OSyncChange *change)
{
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	return opie_sync_item_commit(ctx, change, env->contacts_doc, "Contacts", "Contact");
}

static osync_bool opie_sync_todo_commit(OSyncContext *ctx, OSyncChange *change)
{
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	return opie_sync_item_commit(ctx, change, env->todos_doc, "Tasks", "Task");
}

static osync_bool opie_sync_event_commit(OSyncContext *ctx, OSyncChange *change)
{
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	return opie_sync_item_commit(ctx, change, env->calendar_doc, "events", "event");
}


static void opie_sync_disconnect( OSyncContext* ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);

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

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}


static OSyncChange *opie_sync_item_change_create(OpieSyncEnv *env, xmlDoc *doc, xmlNode *node, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, doc, node, error);

	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	const char *uidentry = NULL;
	char *opie_uid = opie_xml_get_uid(node);
	if(opie_uid)
		uidentry = uidmap_get_mapped_uid(env, opie_uid);
	if(uidentry)
		osync_change_set_uid(change, uidentry);
	else {
		char *full_uid = opie_xml_get_tagged_uid(node);
		if(opie_uid) {
			uidmap_addmapping(env, opie_uid, full_uid);
			uidmap_addmapping(env, full_uid, opie_uid);
		}
		osync_change_set_uid(change, full_uid);
		g_free(full_uid);
	}
	g_free(opie_uid);
	
	char *nodetext = xml_node_to_text(doc, node);
	printf("OPIE: change xml = %s\n", nodetext); 
	
	unsigned char *hash = hash_str(nodetext);
	osync_change_set_hash(change, hash);
	g_free(hash);
	
	osync_change_set_data(change, nodetext, strlen(nodetext) + 1, TRUE);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;

/*error_free_change:
	osync_change_free(change);*/
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool opie_sync_item_get_changeinfo(OSyncContext *ctx, OSyncError **error, 
		const char *objtype, const char *objformat, 
		xmlDoc *doc, const char *listelement, const char *itemelement)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, \"%s\", \"%s\", %p, \"%s\", \"%s\")", 
							__func__, ctx, error, objtype, objformat, doc, listelement, itemelement);
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	
	if (osync_member_get_slow_sync(env->member, objtype) == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		osync_hashtable_set_slow_sync(env->hashtable, objtype);
		printf("OPIE: slow sync\n");
	}

	xmlNode *item_node = opie_xml_get_first(doc, listelement, itemelement);
	while(item_node)
	{
		printf("OPIE: creating change\n");
		/* Convert category IDs to names that other systems can use */
		char *categories_bkup = opie_xml_get_categories(item_node);
		if(env->categories_doc && categories_bkup)
			opie_xml_category_ids_to_names(env->categories_doc, item_node);
		/* Create the change */
		OSyncChange *change = opie_sync_item_change_create(env, doc, item_node, error);
		/* Restore old categories value as we don't want to save this back to our XML file */
		if(categories_bkup) {
			opie_xml_set_categories(item_node, categories_bkup);
			g_free(categories_bkup);
		}
		
		if (!change)
			goto error;
		
		osync_change_set_objformat_string(change, objformat);
		
		/* Use the hash table to check if the object
		needs to be reported */
		if (osync_hashtable_detect_change(env->hashtable, change)) {
			printf("OPIE: reporting change\n");
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}
		
		item_node = opie_xml_get_next(item_node);
	}
	
	/* Use the hashtable to report deletions */
	osync_hashtable_report_deleted(env->hashtable, ctx, "contact");
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static void opie_sync_get_changeinfo( OSyncContext* ctx )
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;

	if (!opie_sync_item_get_changeinfo(ctx, &error, "contact", "opie-xml-contact", env->contacts_doc, "Contacts", "Contact"))
		goto error;
	if (!opie_sync_item_get_changeinfo(ctx, &error, "todo",    "opie-xml-todo",    env->todos_doc,    "Tasks",    "Task"))
		goto error;
	if (!opie_sync_item_get_changeinfo(ctx, &error, "event",   "opie-xml-event",   env->calendar_doc, "events",   "event"))
		goto error;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

void uidmap_addmapping(OpieSyncEnv *env, const char *uid1, const char *uid2) {
	char *key = g_strdup(uid1);
	char *value = g_strdup(uid2);
	g_tree_insert(env->uid_map, key, value);
}

void uidmap_removemapping(OpieSyncEnv *env, const char *uid1) {
	const char *uid2 = uidmap_get_mapped_uid(env, uid1);
	if(uid2) {
		char *uid2_copy = g_strdup(uid2);
		g_tree_remove(env->uid_map, uid1);
		g_tree_remove(env->uid_map, uid2_copy);
		g_free(uid2_copy);
	}
}

char *uidmap_set_node_uid(OpieSyncEnv *env, xmlNode *node, xmlDoc *doc, 
												 const char *listelement, const char *itemelement, const char *ext_uid) {
	const char *uidentry = uidmap_get_mapped_uid(env, ext_uid);
	char *opie_uid;
	if(uidentry) {
		opie_xml_set_uid(node, uidentry);
		opie_uid = g_strdup(uidentry);
	}
	else {
		opie_uid = opie_xml_set_ext_uid(node, doc, listelement, itemelement, ext_uid);
		uidmap_addmapping(env, opie_uid, ext_uid);
		uidmap_addmapping(env, ext_uid, opie_uid);
	}
	return opie_uid;
}

const char *uidmap_get_mapped_uid(OpieSyncEnv *env, const char *uid) {
	const char *uidentry = (const char *)g_tree_lookup(env->uid_map, uid);
	return uidentry;
}

gint uidmap_compare(gconstpointer a, gconstpointer b, gpointer user_data) {
	return strcmp((const char *)a, (const char *)b);
}

void uidmap_read(OpieSyncEnv *env) {
	env->uid_map = g_tree_new_full(uidmap_compare, NULL, g_free, g_free);
	
	const char *configdir = osync_member_get_configdir(env->member);
	char *mapfile = g_build_filename(configdir, "opie_uidmap.xml", NULL);
	xmlDoc *doc = opie_xml_file_open(mapfile);
	if(doc) {
		xmlNode *node = opie_xml_get_first(doc, "mappinglist", "mapping");
		while(node) {
			char *uid1 = xmlGetProp(node, "uid1");
			if(uid1) {
				char *uid2 = xmlGetProp(node, "uid2");
				if(uid2) {
					uidmap_addmapping(env, uid1, uid2);
					xmlFree(uid2);
				}
				xmlFree(uid1);
			}
			node = opie_xml_get_next(node);
		}
	}
	g_free(mapfile);
}

gboolean uidmap_write_entry(gpointer key, gpointer value, gpointer data) {
	xmlNode *node = xmlNewNode(NULL, "mapping");
	xmlSetProp(node, "uid1", (const char *)key);
	xmlSetProp(node, "uid2", (const char *)value);
	xmlAddChild((xmlNode *)data, node);
	return FALSE;
}

void uidmap_write(OpieSyncEnv *env) {
	const char *configdir = osync_member_get_configdir(env->member);
	char *mapfile = g_build_filename(configdir, "opie_uidmap.xml", NULL);
	
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(doc) {
		xmlNode *root = xmlNewNode(NULL, "uidmap");
		xmlDocSetRootElement(doc, root);
		xmlNode *listnode = xmlNewNode(NULL, "mappinglist");
		xmlAddChild(root, listnode);
		
		g_tree_foreach(env->uid_map, uidmap_write_entry, listnode); 
		
		xmlSaveFormatFile(mapfile, doc, 1);
	}
	g_free(mapfile);
	
}

void uidmap_free(OpieSyncEnv *env) {
	g_tree_destroy(env->uid_map);
	env->uid_map = NULL;
}

void get_info(OSyncEnv* env )
{
	OSyncPluginInfo* info = osync_plugin_new_info(env);

	/*
		* Initial Names
		*/
	info->name          = "opie-sync";
	info->longname      = "Opie Synchronization Plugin";
	info->description   = "Synchronize with Opie/Qtopia based devices";
	info->version       = 1;
	info->is_threadsafe = TRUE;
	info->config_type   = NEEDS_CONFIGURATION;


	/*
		* Function pointers
		*/
	info->functions.is_available   = opie_sync_is_available;
	info->functions.initialize     = opie_sync_initialize;
	info->functions.finalize       = opie_sync_finalize;
	info->functions.connect        = opie_sync_connect;
	info->functions.disconnect     = opie_sync_disconnect;
	info->functions.sync_done      = opie_sync_sync_done;
	info->functions.get_changeinfo = opie_sync_get_changeinfo;


	/*
		* Object types
		*/
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "opie-xml-contact", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "opie-xml-contact", opie_sync_contact_commit);
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "opie-xml-todo", NULL);
	osync_plugin_set_commit_objformat(info, "todo",    "opie-xml-todo",    opie_sync_todo_commit);
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "opie-xml-event", NULL);
	osync_plugin_set_commit_objformat(info, "event",   "opie-xml-event",   opie_sync_event_commit);
}
