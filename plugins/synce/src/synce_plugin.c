/*
 * OpenSync SynCE plugin
 *
 * Copyright © 2005 by MirKuZ
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
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
 */
#include <opensync/opensync.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>

#include "synce_plugin.h"
#include "synce_file.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <glib.h>

static void *initialize(OSyncMember *member, OSyncError **error)
{
	char *configdata;
	int configsize;
	HRESULT hr;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	SyncePluginPtr *env = g_malloc0(sizeof(SyncePluginPtr));

	/* File sync needs a hash table */
	env->hashtable = osync_hashtable_new();

	//now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		free(env);
		return NULL;
	}
	
	if (!synce_parse_settings(env, configdata, configsize, error)) {
		g_free(env);
		return NULL;
	}

	//Process the configdata here and set the options on your environment
	free(configdata);
	env->member = member;
	
	//Initializing RAPI
	hr = CeRapiInit();
  	if (FAILED(hr))
  	{
		osync_error_update(NULL,"Unable to initialize RAPI");
		free(env);
		return NULL;
        }

	//Now your return your struct.
	return (void *)env;
}

static void connect(OSyncContext *ctx)
{
	RRA_Matchmaker* matchmaker = NULL;
	
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	 
	//1 - creating matchmaker
	matchmaker = rra_matchmaker_new();
	if (!matchmaker){
		osync_context_report_error(ctx, 1, "building matchmaker");
		return;
	}
	osync_debug("SynCE-SYNC", 4, "matchmaker built");

	//2 - setting partnership 
	if (!rra_matchmaker_set_current_partner(matchmaker, 1)){
		osync_context_report_error(ctx, 1, "set current partner");
		return;
	}
	osync_debug("SynCE-SYNC", 4, "partner set");

        //3 -setting timezone
        if (!rra_timezone_get(&(env->timezone))){
		osync_context_report_error(ctx, 1, "getting timezone");
		return;
    	}
       	osync_debug("SynCE-SYNC", 4, "timezone set");
	
	//4- creating syncmgr
	env->syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(env->syncmgr))
	{
		osync_context_report_error(ctx, 1, "can't connect");
		return;
	}
    	osync_debug("SynCE-SYNC", 4, "syncmgr created");
    	
	/*
	 * if (!osync_member_objtype_enabled(env->member, "contact"))
	 * 	env->config_contacts = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "todos"))
	 * 	env->config_todos = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "calendar"))
	 * 	env->config_calendar = FALSE;
	 */
	if (env->config_file) {
		/* Load the hash table */
		OSyncError *error = NULL;

		if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
			osync_context_report_osyncerror(ctx, &error);
			return;
		}
	}

	osync_context_report_success(ctx);
}

static bool callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	const char* event_str;
	ids_list* listids=(ids_list*)cookie;
	 
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
  
  	switch (event)
  	{
    	case SYNCMGR_TYPE_EVENT_UNCHANGED:
		event_str = "%i Unchanged";
		listids->unchanged_count=count;
		listids->unchanged_ids=(uint32_t*)malloc(sizeof(uint32_t)*count);
		memcpy(listids->unchanged_ids,ids,sizeof(uint32_t)*count);
      		break;
	case SYNCMGR_TYPE_EVENT_CHANGED:
		event_str = "%i Changed";
		listids->changed_count=count;
		listids->changed_ids=(uint32_t*)malloc(sizeof(uint32_t)*count);
		memcpy(listids->changed_ids,ids,sizeof(uint32_t)*count);
		break;
	case SYNCMGR_TYPE_EVENT_DELETED:
		event_str = "%i Deleted";
		listids->deleted_count=count;
		listids->deleted_ids=(uint32_t*)malloc(sizeof(uint32_t)*count);
		memcpy(listids->deleted_ids,ids,sizeof(uint32_t)*count);
		break;
	default:
      		event_str = "%i Unknown";
		break;
	}

	osync_debug("SynCE-SYNC", 4, event_str, count);
	return true;
}

/*
m_report_contact_changes: get datas for the ids changed, and report them to opensync engine
*/
bool m_report_contact_changes(OSyncContext *ctx,RRA_SyncMgrType *type,uint32_t *ids,uint32_t count, OSyncChangeType change_type)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	
	
	for (i=0;i<count;i++) {

		char* vcard = NULL;
		uint8_t* data = NULL;
		char strid[10];
		size_t data_size=0;
		
		if (!rra_syncmgr_get_single_object(env->syncmgr, type->id, ids[i], &data, &data_size)){
			osync_context_report_error(ctx, 1, "get_single_object fail. id=%i",ids[i]);			
			return FALSE;
		}
		
		sprintf(strid,"%08x",ids[i]);
	
		osync_debug("SynCE-SYNC", 4, "got object type: contact ids: %08x data_size: %i",ids[i],data_size);
	
		rra_contact_to_vcard(RRA_CONTACT_ID_UNKNOWN,data,data_size,&vcard,RRA_CONTACT_VERSION_3_0);
		
		OSyncChange *change = osync_change_new();
		osync_change_set_member(change, env->member);
		
		osync_change_set_uid(change, strid); // %08x
		osync_change_set_objformat_string(change, "vcard30");
		
		osync_change_set_data(change, vcard, strlen(vcard) + 1, TRUE);
		osync_change_set_changetype(change, change_type);
					
		osync_context_report_change(ctx, change);

	}	
	return TRUE;
}

/*
m_report_todo_changes: get datas for the ids changed, and report them to opensync engine
*/
bool m_report_todo_changes(OSyncContext *ctx,RRA_SyncMgrType *type,uint32_t *ids,uint32_t count, OSyncChangeType change_type)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	
	
	for (i=0;i<count;i++) {

		char* vtodo = NULL;
		uint8_t* data = NULL;
		char strid[10];
		size_t data_size=0;
		
		if (!rra_syncmgr_get_single_object(env->syncmgr, type->id, ids[i], &data, &data_size)){
			osync_context_report_error(ctx, 1, "get_single_object fail. id=%i",ids[i]);			
			return FALSE;
		}
		
		sprintf(strid,"%08x",ids[i]);
	
		osync_debug("SynCE-SYNC", 4, "got object type: todo ids: %08x data_size: %i",ids[i],data_size);
	
		rra_task_to_vtodo(RRA_TASK_ID_UNKNOWN,data,data_size,&vtodo,0,&env->timezone);
		/* Workaround since a single vevent component would be broken */
		char *newvtodo = g_strdup_printf("BEGIN:VCALENDAR\r\nVERSION:1.0\r\n%sEND:VCALENDAR\r\n", vtodo);
		g_free(vtodo);
		vtodo = newvtodo;
		osync_trace(TRACE_INTERNAL, "Generated vtodo: %s", osync_print_binary((unsigned char *)vtodo, strlen(vtodo)));
		
		OSyncChange *change = osync_change_new();
		osync_change_set_member(change, env->member);
		
		osync_change_set_uid(change, strid); // %08x
		osync_change_set_objformat_string(change, "vtodo10");
	
		osync_change_set_data(change, vtodo, strlen(vtodo) + 1, TRUE);
		osync_change_set_changetype(change, change_type);
	
		osync_context_report_change(ctx, change);
	}	
	return TRUE;
}

/*
m_report_cal_changes: get datas for the ids changed, and report them to opensync engine
*/
bool m_report_cal_changes(OSyncContext *ctx,RRA_SyncMgrType *type,uint32_t *ids,uint32_t count, OSyncChangeType change_type)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	
	
	for (i=0;i<count;i++) {

		char* vevent = NULL;
		uint8_t* data = NULL;
		char strid[10];
		size_t data_size=0;
		
		if (!rra_syncmgr_get_single_object(env->syncmgr, type->id, ids[i], &data, &data_size)){
			osync_context_report_error(ctx, 1, "get_single_object fail. id=%i",ids[i]);			
			return FALSE;
		}
		
		sprintf(strid,"%08x",ids[i]);
	
		osync_debug("SynCE-SYNC", 4, "got object type: cal ids: %08x data_size: %i",ids[i],data_size);
	
		rra_appointment_to_vevent(RRA_APPOINTMENT_ID_UNKNOWN,data,data_size,&vevent,0,&env->timezone);

		/* Workaround since a single vevent component would be broken */
		char *newvevent = g_strdup_printf("BEGIN:VCALENDAR\r\nVERSION:1.0\r\n%sEND:VCALENDAR\r\n", vevent);
		g_free(vevent);
		vevent = newvevent;
		osync_trace(TRACE_INTERNAL, "Generated vevent: %s", osync_print_binary((unsigned char *)vevent, strlen(vevent)));
		
		OSyncChange *change = osync_change_new();
		osync_change_set_member(change, env->member);
		
		osync_change_set_uid(change, strid); // %08x
		osync_change_set_objformat_string(change, "vevent10");
	
		osync_change_set_data(change, vevent, strlen(vevent) + 1, TRUE);
		osync_change_set_changetype(change, change_type);
	
		osync_context_report_change(ctx, change);
	}	
	return TRUE;
}

/*
 m_report_contact: get contacts changes from device, and report them via m_report_contact_changes
*/
bool m_report_contact(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	bool got_event = false;
	char *strType="Contact";
	RRA_SyncMgrType *type = NULL;		
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	

	//initialize list
	env->contact_ids=malloc(sizeof(ids_list));
	memset(env->contact_ids, 0, sizeof(ids_list));
	
	type = rra_syncmgr_type_from_name(env->syncmgr, strType);
	(env->contact_ids)->type=type;

	//subscribe event for contact
	rra_syncmgr_subscribe(env->syncmgr, type->id, callback, env->contact_ids);

	if (!rra_syncmgr_start_events(env->syncmgr))  {
		osync_context_report_error(ctx, 1, "can't start events");
		return FALSE;
	}
	rra_syncmgr_handle_all_pending_events(env->syncmgr);
	
	osync_debug("SynCE-SYNC", 4, "event started");

	//waiting for events
	while (rra_syncmgr_event_wait(env->syncmgr, 3, &got_event) && got_event) {
		osync_debug("SynCE-SYNC", 4, "*event received, processing");
		rra_syncmgr_handle_event(env->syncmgr);
	}
	osync_debug("SynCE-SYNC", 4, "finished receiving events");
	
	rra_syncmgr_unsubscribe(env->syncmgr,type->id);
	
	//getting results in ids
	osync_debug("SynCE-SYNC", 4, "%i changed",(env->contact_ids)->changed_count);
	osync_debug("SynCE-SYNC", 4, "%i unchanged",(env->contact_ids)->unchanged_count);
	osync_debug("SynCE-SYNC", 4, "%i deleted",(env->contact_ids)->deleted_count);

	//report changes
	osync_debug("SynCE-SYNC", 4, "report changes");
	
	if(!m_report_contact_changes(ctx,type,(env->contact_ids)->changed_ids,(env->contact_ids)->changed_count,CHANGE_MODIFIED)){
		osync_context_report_error(ctx, 1, "error reporting changes");
		return FALSE;
	}
	if(!m_report_contact_changes(ctx,type,(env->contact_ids)->deleted_ids,(env->contact_ids)->deleted_count,CHANGE_DELETED)){
		osync_context_report_error(ctx, 1, "error reporting contacts");
		return FALSE;
	};

	//unchanged only if fullsync
	if (osync_member_get_slow_sync(env->member, "contact")){
		if(!m_report_contact_changes(ctx,type,(env->contact_ids)->unchanged_ids,(env->contact_ids)->unchanged_count,CHANGE_ADDED)){
			osync_context_report_error(ctx, 1, "error reporting contacts");
			return FALSE;
		}
	}

	osync_debug("SynCE-SYNC", 4, "done reporting changes");
	
	return true;
}

/*
 m_report_todo: get todo changes from device, and report them via m_report_todo_changes
*/
bool m_report_todo(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	bool got_event = false;
	char *strType="Task";
	RRA_SyncMgrType *type = NULL;		
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	

	//initialize list
	env->todo_ids=malloc(sizeof(ids_list));
	memset(env->todo_ids, 0, sizeof(ids_list));
	
	type = rra_syncmgr_type_from_name(env->syncmgr, strType);
	(env->todo_ids)->type=type;

	//subscribe event for todos
	rra_syncmgr_subscribe(env->syncmgr, type->id, callback, env->todo_ids);

	if (!rra_syncmgr_start_events(env->syncmgr))  {
		osync_context_report_error(ctx, 1, "can't start events");
		return FALSE;
	}

	osync_debug("SynCE-SYNC", 4, "event started");

	//waiting for events
	while (rra_syncmgr_event_wait(env->syncmgr, 3, &got_event) && got_event) {
		osync_debug("SynCE-SYNC", 4, "*event received, processing");
		rra_syncmgr_handle_event(env->syncmgr);
	}
	
	rra_syncmgr_handle_all_pending_events(env->syncmgr);

	osync_debug("SynCE-SYNC", 4, "finished receiving events");
	
	rra_syncmgr_unsubscribe(env->syncmgr,type->id);
	
	//getting results in ids
	osync_debug("SynCE-SYNC", 4, "%i changed",(env->todo_ids)->changed_count);
	osync_debug("SynCE-SYNC", 4, "%i unchanged",(env->todo_ids)->unchanged_count);
	osync_debug("SynCE-SYNC", 4, "%i deleted",(env->todo_ids)->deleted_count);

	//report changes
	osync_debug("SynCE-SYNC", 4, "report changes");
	
	if(!m_report_todo_changes(ctx,type,(env->todo_ids)->changed_ids,(env->todo_ids)->changed_count,CHANGE_MODIFIED)){
		osync_context_report_error(ctx, 1, "error reporting changes");
		return FALSE;
	}
	if(!m_report_todo_changes(ctx,type,(env->todo_ids)->deleted_ids,(env->todo_ids)->deleted_count,CHANGE_DELETED)){
		osync_context_report_error(ctx, 1, "error reporting deleted");
		return FALSE;
	};

	//unchanged only if fullsync
	if (osync_member_get_slow_sync(env->member, "todo")){
		if(!m_report_todo_changes(ctx,type,(env->todo_ids)->unchanged_ids,(env->todo_ids)->unchanged_count,CHANGE_ADDED)){
			osync_context_report_error(ctx, 1, "error reporting todo");
			return FALSE;
		}
	}

	osync_debug("SynCE-SYNC", 4, "done reporting changes");
	
	return true;
}

/*
 m_report_cal: get cal changes from device, and report them via m_report_cal_changes
*/
bool m_report_cal(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	bool got_event = false;
	char *strType="appointment";
	RRA_SyncMgrType *type = NULL;		
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	

	//initialize list
	env->cal_ids=malloc(sizeof(ids_list));
	memset(env->cal_ids, 0, sizeof(ids_list));
	
	type = rra_syncmgr_type_from_name(env->syncmgr, strType);
	(env->cal_ids)->type=type;

	//subscribe event for cals
	rra_syncmgr_subscribe(env->syncmgr, type->id, callback, env->cal_ids);

	if (!rra_syncmgr_start_events(env->syncmgr))  {
		osync_context_report_error(ctx, 1, "can't start events");
		return FALSE;
	}

	osync_debug("SynCE-SYNC", 4, "event started");

	//waiting for events
	while (rra_syncmgr_event_wait(env->syncmgr, 3, &got_event) && got_event) {
		osync_debug("SynCE-SYNC", 4, "*event received, processing");
		rra_syncmgr_handle_event(env->syncmgr);
	}
	
	rra_syncmgr_handle_all_pending_events(env->syncmgr);

	osync_debug("SynCE-SYNC", 4, "finished receiving events");
	
	rra_syncmgr_unsubscribe(env->syncmgr,type->id);
	
	//getting results in ids
	osync_debug("SynCE-SYNC", 4, "%i changed",(env->cal_ids)->changed_count);
	osync_debug("SynCE-SYNC", 4, "%i unchanged",(env->cal_ids)->unchanged_count);
	osync_debug("SynCE-SYNC", 4, "%i deleted",(env->cal_ids)->deleted_count);

	//report changes
	osync_debug("SynCE-SYNC", 4, "report changes");
	
	if(!m_report_cal_changes(ctx,type,(env->cal_ids)->changed_ids,(env->cal_ids)->changed_count,CHANGE_MODIFIED)){
		osync_context_report_error(ctx, 1, "error reporting changes");
		return FALSE;
	}
	if(!m_report_cal_changes(ctx,type,(env->cal_ids)->deleted_ids,(env->cal_ids)->deleted_count,CHANGE_DELETED)){
		osync_context_report_error(ctx, 1, "error reporting deleted");
		return FALSE;
	};

	//unchanged only if fullsync
	if (osync_member_get_slow_sync(env->member, "event")){
		if(!m_report_cal_changes(ctx,type,(env->cal_ids)->unchanged_ids,(env->cal_ids)->unchanged_count,CHANGE_ADDED)){
			osync_context_report_error(ctx, 1, "error reporting cal");
			return FALSE;
		}
	}
	
	osync_debug("SynCE-SYNC", 4, "done reporting changes");
	
	return true;
}

/*
 get_changeinfo: report the changes to the opensync engine.
*/
static void get_changeinfo(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	osync_debug("SynCE-SYNC", 4,
			"Get_ChangeInfo(todos %d contacts %d calendar %d files(%s)\n",
			env->config_todos, env->config_contacts, env->config_calendar,
			env->config_file);

	//test RRA connection
	osync_debug("SynCE-SYNC", 4, "Testing connection");
	if (!env->syncmgr || !rra_syncmgr_is_connected(env->syncmgr)){
		//not connected, exit
		osync_context_report_error(ctx, 1, "not connected to device, exit.");
		return;
	}
	osync_debug("SynCE-SYNC", 4, "Testing connection -> ok");

	if (env->config_todos) {
		osync_debug("SynCE-SYNC", 4, "checking todos");
	
		if (!m_report_todo(ctx)){
			osync_context_report_error(ctx, 1, "Error while checking todos");
			return;
		}

		//need to reinit the connection
		rra_syncmgr_disconnect(env->syncmgr);
		
		if (!rra_syncmgr_connect(env->syncmgr))
		{
			osync_context_report_error(ctx, 1, "can't connect");
			return;
		}
		
	}
	
	if (env->config_contacts) {
		osync_debug("SynCE-SYNC", 4, "checking contacts");
		
		if (!m_report_contact(ctx)){
			osync_context_report_error(ctx, 1, "Error while checking contact");
			return;
		}
		
		//need to reinit the connection
		rra_syncmgr_disconnect(env->syncmgr);
		
		if (!rra_syncmgr_connect(env->syncmgr))
		{
			osync_context_report_error(ctx, 1, "can't connect");
			return;
		}
	}

	if (env->config_calendar) {
		osync_debug("SynCE-SYNC", 4, "checking calendar");
		
		if (!m_report_cal(ctx)){
			osync_context_report_error(ctx, 1, "Error while checking calendar");
			return;
		}
		
		//need to reinit the connection
		rra_syncmgr_disconnect(env->syncmgr);
		
		if (!rra_syncmgr_connect(env->syncmgr))
		{
			osync_context_report_error(ctx, 1, "can't connect");
			return;
		}
	}

	if (env->config_file) {
		osync_debug("SynCE-SYNC", 4, "checking files to synchronize");

		if (! synce_file_get_changeinfo(ctx)) {
			osync_context_report_error(ctx, 1, "Error while checking files");
			return;
		}
		
		//need to reinit the connection
		rra_syncmgr_disconnect(env->syncmgr);
	
		if (!rra_syncmgr_connect(env->syncmgr))
		{
			osync_context_report_error(ctx, 1, "can't connect");
			return;
		}
	}
	
	//All Right
	osync_context_report_success(ctx);
}

/*
commit_change: called when it's time to update device once a time for every update
*/
static osync_bool commit_contacts_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "contact");
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			
			id=strtol(osync_change_get_uid(change),NULL,16);

			osync_debug("SynCE-SYNC", 4, "deleting contact id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, 1, "Can't delete contact id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SynCE-SYNC", 4, "adding contact id %08x",id);

			rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't add contact id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SynCE-SYNC", 4, "updating contact id %08x",id);

			rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't update contact id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);

	return TRUE;
}

static osync_bool commit_todo_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "task");
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			id=strtol(osync_change_get_uid(change),NULL,16);

			osync_debug("SynCE-SYNC", 4, "deleting task id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, 1, "Can't delete task id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SynCE-SYNC", 4, "adding task id %08x",id);

			rra_task_from_vtodo(object,&dummy_id,&data,&data_size,0,&env->timezone);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't add task id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
						
			object=osync_change_get_data(change);
			
			id=strtol(osync_change_get_uid(change),NULL,16);
			
			osync_debug("SynCE-SYNC", 4, "updating task id %08x",id);

			rra_task_from_vtodo(object,&dummy_id,&data,&data_size,0,&env->timezone);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't update task id: %08x",id);
				return FALSE;
			}
		
			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	
	return TRUE;
}

static osync_bool commit_cal_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "appointment");
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			id=strtol(osync_change_get_uid(change),NULL,16);

			osync_debug("SynCE-SYNC", 4, "deleting cal id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, 1, "Can't delete cal id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SynCE-SYNC", 4, "adding cal id %08x",id);

			rra_appointment_from_vevent(object,&dummy_id,&data,&data_size,0,&env->timezone);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't add cal id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			
			id=strtol(osync_change_get_uid(change),NULL,16);

			osync_debug("SynCE-SYNC", 4, "updating cal id %08x",id);

			rra_appointment_from_vevent(object,&dummy_id,&data,&data_size,0,&env->timezone);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't update cal id: %08x",id);
				return FALSE;
			}
		
			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	
	return TRUE;
}

/*
 Sync_done: This function will only be called if the sync was successfull
*/
static void sync_done(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	if (env->config_contacts) {
		//commit any change done to forget contact changes
		for(i=0;i<(env->contact_ids)->changed_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->contact_ids)->type)->id,(env->contact_ids)->changed_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit changed contact id %08x",(env->contact_ids)->changed_ids[i]);
		}

		for(i=0;i<(env->contact_ids)->deleted_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->contact_ids)->type)->id,(env->contact_ids)->deleted_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit deleted contact id %08x",(env->contact_ids)->deleted_ids[i]);
		}
	}
	
	if (env->config_todos) {
		//commit any change done to forget task changes
		for(i=0;i<(env->todo_ids)->changed_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->todo_ids)->type)->id,(env->todo_ids)->changed_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit changed cal id %08x",(env->todo_ids)->changed_ids[i]);
		}

		for(i=0;i<(env->todo_ids)->deleted_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->todo_ids)->type)->id,(env->todo_ids)->deleted_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit deleted todo id %08x",(env->todo_ids)->deleted_ids[i]);
		}
	}
	
	if (env->config_calendar) {
		//commit any change done to forget calendar changes
		for(i=0;i<(env->cal_ids)->changed_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->cal_ids)->type)->id,(env->cal_ids)->changed_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit changed cal id %08x",(env->cal_ids)->changed_ids[i]);
		}

		for(i=0;i<(env->cal_ids)->deleted_count;i++){
			rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->cal_ids)->type)->id,(env->cal_ids)->deleted_ids[i]);
			osync_debug("SynCE-SYNC", 4, "commit deleted cal id %08x",(env->cal_ids)->deleted_ids[i]);
		}
	}
	
	if (env->config_file) {
		osync_hashtable_forget(env->hashtable);
	}

	osync_debug("SynCE-SYNC", 4, "Sync done.");	
	
	osync_context_report_success(ctx);
}

/*
 * Disconnect function: Called at the end of the process, should close the RRA connection.
 */
static void disconnect(OSyncContext *ctx)
{
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	
	if (env->syncmgr==NULL) {
		osync_context_report_error(ctx, 1, "ERRROR: no connection established");
		return;
	}

	if (env->config_file) {
		osync_hashtable_close(env->hashtable);
	}
	
	rra_syncmgr_disconnect(env->syncmgr);
	
	osync_debug("SynCE-SYNC", 4, "Connection closed.");	
		
	osync_context_report_success(ctx);
}

/*
 * Finalize function: Called to unalloc all the structs and libraries used.
 */
static void finalize(void *data)
{
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	SyncePluginPtr *env = (SyncePluginPtr *)data;

	if (env->config_file) {
		osync_hashtable_free(env->hashtable);
	}

	rra_syncmgr_destroy(env->syncmgr);
        env->syncmgr = NULL;

	CeRapiUninit();

	free(env);
}

void get_info(OSyncPluginInfo *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info((void*)env);

	info->name = "synce-plugin";
	info->longname = "Plugin to synchronize Windows CE devices";
	info->description = "by MirkuZ and Danny Backx";
	
	info->version = 1;
	
	info->is_threadsafe = FALSE;
	
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	info->functions.get_data = synce_file_getdata;

	info->timeouts.connect_timeout = 5;
	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", commit_contacts_change);
	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent10", NULL);
	osync_plugin_set_commit_objformat(info, "event", "vevent10", commit_cal_change);

	osync_plugin_accept_objtype(info, "todo");
    	osync_plugin_accept_objformat(info, "todo", "vtodo10", NULL);
    	osync_plugin_set_commit_objformat(info, "todo", "vtodo10", commit_todo_change);
	
	osync_plugin_accept_objtype(info, "data");
	osync_plugin_accept_objformat(info, "data", "file", NULL);
	osync_plugin_set_commit_objformat(info, "data", "file",
                                          synce_file_commit);
}
