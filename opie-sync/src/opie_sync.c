/*

   Copyright 2005 Holger Hans Peter Freyther

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


/* sync_cancelled()
 * 
 * Callback from the opie monitor thread.
 */
void sync_cancelled(void)
{
  /*user_cancelled_sync = TRUE;*/
}



static osync_bool opie_sync_settings_parse(OpieSyncEnv *env, const char *config, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;

	//set defaults
	env->username = g_strdup("root");
	env->password = g_strdup("Qtopia");
	env->url = g_strdup("192.168.10.123");
	env->device_type = OPIE_SYNC_OPIE;
	env->conn_type = OPIE_CONN_FTP;
	env->device_port = 4242;
	env->use_qcop = TRUE;

	doc = xmlParseMemory(config, size);

	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
		goto error;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get settings root element");
		goto error_free_doc;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Configuration file is invalid");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
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
			}
			xmlFree(str);
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
	opie_object_type object_types = OPIE_OBJECT_TYPE_PHONEBOOK; // FIXME
	
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

	/*
	for (li = env->contacts; li != NULL; li = g_list_next(li))
	{
		contact = (contact_data*)li->data;
		errmsg = g_strdup_printf("Contact %s %s", contact->first_name, contact->last_name);
		osync_trace(TRACE_INTERNAL, errmsg);
	}
	*/

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
	/*	
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;
	opie_object_type object_types = OPIE_OBJECT_TYPE_PHONEBOOK; // FIXME
	*/
        
	/*
	osync_trace( TRACE_INTERNAL, "number of contacts = %d", 
		     g_list_length( env->contacts ) );
	*/

	/*	if ( !opie_connect_and_put(env, 
                                   object_types, 
                                   env->calendar, 
                                   env->contacts, 
                                   env->todos,
                                   env->categories ) )
        { 
            osync_trace( TRACE_INTERNAL, "opie_connect_and_put failed" );
            char *errmsg = g_strdup_printf( "Failed to send data to device %s", env->url );
            osync_error_set(&error, OSYNC_ERROR_GENERIC, errmsg);
            goto error;
        }

	*/
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	/*
error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	*/
}

static osync_bool opie_sync_contact_commit(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	//OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	OSyncError *error = NULL;
	//unsigned char *uid = osync_change_get_uid(change);

	char *change_str = (char *)osync_change_get_data(change);
	xmlDoc *change_doc = xmlRecoverMemory(change_str, strlen(change_str));
	if(!change_doc) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to retrieve XML from change");
		goto error;
	}
	
	// FIXME should create some functions in opie_xml.c to handle these operations
	// now that we are dealing with XML nodes/documents
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			break;
		case CHANGE_ADDED:
//	    env->contacts = g_list_append( env->contacts, contact );
			break;
		case CHANGE_DELETED:
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	xmlFreeDoc(change_doc);
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
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
			g_free(env->qcopconn->resultmsg);
		}
		qcop_disconnect(env->qcopconn);
		qcop_freeqconn(env->qcopconn);
		env->qcopconn = NULL;
	}	

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}


static OSyncChange *opie_sync_contact_change_create(xmlDoc *doc, xmlNode *node, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, doc, node, error);

	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	char *uid = xmlGetProp(node, "Uid");
	char *full_uid = g_strdup_printf("uid-contact-%s", uid);
	osync_change_set_uid(change, full_uid);
	g_free(uid);
	g_free(full_uid);
	
	osync_change_set_objformat_string(change, "opie-xml-contact");
	
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

static osync_bool opie_sync_contact_get_changeinfo(OSyncContext *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);
	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);
	
	if (osync_member_get_slow_sync(env->member, "contact") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		osync_hashtable_set_slow_sync(env->hashtable, "contact");
		printf("OPIE: slow sync\n");
	}

	xmlNode *contact_node;
	xmlDoc *doc = opie_xml_contact_open(env->contacts_file, &contact_node);
	
	while(contact_node)
	{
		printf("OPIE: creating change\n");
		OSyncChange *change = opie_sync_contact_change_create(doc, contact_node, error);
		if (!change)
			goto error;
		
		/* Use the hash table to check if the object
		needs to be reported */
//		osync_change_set_member(change, env->member);
		if (osync_hashtable_detect_change(env->hashtable, change)) {
			printf("OPIE: reporting change\n");
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}
		
		contact_node = opie_xml_contact_next(contact_node);
	}
	
	/* Use the hashtable to report deletions */
	osync_hashtable_report_deleted(env->hashtable, ctx, "contact");
	
	xmlFreeDoc(doc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}



static void opie_sync_get_changeinfo( OSyncContext* ctx )
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
/*	OpieSyncEnv *env = (OpieSyncEnv *)osync_context_get_plugin_data(ctx);*/
	OSyncError *error = NULL;

	if (!opie_sync_contact_get_changeinfo(ctx, &error))
		goto error;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
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
}
