#include <opensync-1.0/opensync/opensync.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>

#include "synce_plugin.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

static void *initialize(OSyncMember *member, OSyncError **error)
{
	char *configdata;
	int configsize;
	HRESULT hr;

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	plugin_environment *env = malloc(sizeof(plugin_environment));
	assert(env != NULL);
	memset(env, 0, sizeof(plugin_environment));
	
	//now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		free(env);
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

	//If you need a hashtable you make it here
	//env->hashtable = osync_hashtable_new();
	
	//Now your return your struct.
	return (void *)env;
}

static void connect(OSyncContext *ctx)
{
	RRA_Matchmaker* matchmaker = NULL;
	
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);
	//Each time you get passed a context (which is used to track
	//calls to your plugin) you can get the data your returned in
	//initialize via this call:
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);

	/*
	 * Now connect to your devices and report
	 * 
	 * an error via:
	 * osync_context_report_error(ctx, ERROR_CODE, "Some message");
	 * 
	 * or success via:
	 * osync_context_report_success(ctx);
	 * 
	 * You have to use one of these 2 somewhere to answer the context.
	 * 
	 */
	 
	//1 - creating matchmaker
	matchmaker = rra_matchmaker_new();
	if (!matchmaker){
		osync_context_report_error(ctx, 1, "building matchmaker");
		return;
	}
	osync_debug("SYNCE-SYNC", 4, "matchmaker built");

	//2 - setting partnership 
	if (!rra_matchmaker_set_current_partner(matchmaker, 1)){
		osync_context_report_error(ctx, 1, "set current partner");
		return;
	}
	osync_debug("SYNCE-SYNC", 4, "partner set");

        //3 -setting timezone
        if (!rra_timezone_get(&(env->timezone))){
		osync_context_report_error(ctx, 1, "getting timezone");
		return;
    	}
       	osync_debug("SYNCE-SYNC", 4, "timezone set");
	
	
	//4- creating syncmgr
	env->syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(env->syncmgr))
	{
		osync_context_report_error(ctx, 1, "can't connect");
		return;
	}
    	osync_debug("SYNCE-SYNC", 4, "syncmgr created");
    	
    	/*//5- subscribe changes
	if (!synce_subscribe(env-syncmgr)){
		osync_context_report_error(ctx, 1, "can't subscribe");
		return;
  	}
  	osync_debug("SYNCE-SYNC", 4, "5- subscribed");*/

	osync_context_report_success(ctx);
	
	//If you are using a hashtable you have to load it here
	/*OSyncError *error = NULL;
	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_osyncerror(ctx, &error);
		return;
	}*/
	
	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	//char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	//if (!osync_anchor_compare(env->member, "lanchor", lanchor))
	//	osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
}

static bool callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	const char* event_str;
	ids_list* listids=(ids_list*)cookie;
	 
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);

//  printf("event=%i, type=%08x, count=%08x",event, type, count);
  
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

	osync_debug("SYNCE-SYNC", 4, event_str, count);
	return true;
}

/*
m_report_contact_changes: get datas for the ids changed, and report them to opensync engine
*/
bool m_report_contact_changes(OSyncContext *ctx,RRA_SyncMgrType *type,uint32_t *ids,uint32_t count, OSyncChangeType change_type)
{
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);
	
	
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
	
		osync_debug("SYNCE-SYNC", 4, "got object type: contact ids: %08x data_size: %i",ids[i],data_size);
	
		rra_contact_to_vcard(RRA_CONTACT_ID_UNKNOWN,data,data_size,&vcard,RRA_CONTACT_VERSION_3_0);
		
		/*RRA_CONTACT_ID_UNKNOWN*/		
		/*strncpi(vcard,"BEGIN:VCARD",11);
		strncpi(vcard[strlen(-9],"END:VCARD");
		osync_debug("SYNCE-SYNC", 4, "vcard data: %s",vcard);*/
	

		OSyncChange *change = osync_change_new();
		osync_change_set_member(change, env->member);
		
		osync_change_set_uid(change, strid); // %08x
		osync_change_set_objformat_string(change, "vcard30");
		
		/*osync_debug("SYNCE-SYNC", 4, "vcard size: %i",strlen(vcard)*sizeof(char));
		osync_debug("SYNCE-SYNC", 4, "vcard content:\n%s",vcard);*/

		osync_change_set_data(change, vcard, strlen(vcard)*sizeof(char), TRUE);
		osync_change_set_changetype(change, change_type);
		
		//osync_change_set_hash(change, m_get_hash(vcard));

		//if (osync_hashtable_detect_change(env->hashtable, change)) {
			osync_context_report_change(ctx, change);
		//	osync_hashtable_update_hash(env->hashtable, change);
		//}	

	}	
	return TRUE;
}

/*
 m_report_contact: get contacts changes from device, and report them via m_report_contact_changes
*/
bool m_report_contact(OSyncContext *ctx)
{
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	bool got_event = false;
	char *strType="Contact";
	RRA_SyncMgrType *type = NULL;		
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	

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

	osync_debug("SYNCE-SYNC", 4, "event started");

	//waiting for events
	while (rra_syncmgr_event_wait(env->syncmgr, 3, &got_event) && got_event) {
		osync_debug("SYNCE-SYNC", 4, "*event received, processing");
		rra_syncmgr_handle_event(env->syncmgr);
	}
	osync_debug("SYNCE-SYNC", 4, "finished receiving events");
	
	rra_syncmgr_unsubscribe(env->syncmgr,type->id);
	
	//getting results in ids
	osync_debug("SYNCE-SYNC", 4, "%i changed",(env->contact_ids)->changed_count);
	osync_debug("SYNCE-SYNC", 4, "%i unchanged",(env->contact_ids)->unchanged_count);
	osync_debug("SYNCE-SYNC", 4, "%i deleted",(env->contact_ids)->deleted_count);

	//report changes
	osync_debug("SYNCE-SYNC", 4, "report changes");
	
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
		if(!m_report_contact_changes(ctx,type,(env->contact_ids)->unchanged_ids,(env->contact_ids)->unchanged_count,CHANGE_UNMODIFIED)){
			osync_context_report_error(ctx, 1, "error reporting contacts");
			return FALSE;
		}
	}

	//osync_hashtable_report_deleted(env->hashtable, ctx, "contact");

	osync_debug("SYNCE-SYNC", 4, "done reporting changes");
	
	return true;
}

/*
 get_changeinfo: report the changes to the opensync engine.
*/
static void get_changeinfo(OSyncContext *ctx)
{
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);
	
	//If you use opensync hashtables you can detect if you need
	//to do a slow-sync and set this on the hastable directly
	//otherwise you have to make 2 function like "get_changes" and
	//"get_all" and decide which to use using
	//osync_member_get_slow_sync
	/*if (osync_member_get_slow_sync(env->member, "contact"))
		osync_hashtable_set_slow_sync(env->hashtable, "contact");*/

	//test RRA connection
	osync_debug("SYNCE-SYNC", 4, "Testing connection");
	if (!env->syncmgr || !rra_syncmgr_is_connected(env->syncmgr)){
		//not connected, exit
		osync_context_report_error(ctx, 1, "not connected to device, exit.");
		return;
	}

	osync_debug("SYNCE-SYNC", 4, "checking contacts");
	
	if (!m_report_contact(ctx)){
		osync_context_report_error(ctx, 1, "Error while checking contact");
		return;
	}

	/*
	 * Now you can get the changes.
	 * Loop over all changes you get and do the following:
	 *
		char *data = NULL;
		//Now get the data of this change
		
		//Make the new change to report
		OSyncChange *change = osync_change_new();
		//Set the member
		osync_change_set_member(change, env->member);
		//Now set the uid of the object
		osync_change_set_uid(change, "<some uid>");
		//Set the object format
		osync_change_set_objformat_string(change, "<the format of the object>");
		//Set the hash of the object (optional, only required if you use hashtabled)
		//osync_change_set_hash(change, "the calculated hash of the object");
		//Now you can set the data for the object
		//Set the last argument to FALSE if the real data
		//should be queried later in a "get_data" function
		
		osync_change_set_data(change, data, sizeof(data), TRUE);			

		//If you use hashtables use these functions:
		if (osync_hashtable_detect_change(env->hashtable, change)) {
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}	
		//otherwise just report the change via
		//osync_context_report_change(ctx, change);
	*/
	
	//When you are done looping and if you are using hashtables	
	//osync_hashtable_report_deleted(env->hashtable, ctx, "data");
	
	//Now we need to answer the call

	//All Right with the contacts
	osync_context_report_success(ctx);
}

/*
commit_change: called when it's time to update device once a time for every update
*/
static osync_bool commit_contacts_change(OSyncContext *ctx, OSyncChange *change)
{		
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "contact");
	/*
	 * Here you have to add, modify or delete a object
	 * 
	 */
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			//Delete the change
			//Dont forget to answer the call on error
			
			id=strtol(osync_change_get_uid(change),NULL,16);

			osync_debug("SYNCE-SYNC", 4, "deleting contact id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, 1, "Can't delete contact id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			//Add the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the hash here:
			//osync_change_set_hash(change, "new hash")
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SYNCE-SYNC", 4, "adding contact id %08x",id);

			rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't add contact id: %08x",id);
				return FALSE;
			}
			
			//osync_change_set_hash(change, dummy_id);

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			//Modify the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the new hash here:
			//osync_change_set_hash(change, "new hash");
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id,id;
			
			object=osync_change_get_data(change);
			id=strtol(osync_change_get_uid(change),NULL,16);			

			osync_debug("SYNCE-SYNC", 4, "adding contact id %08x",id);

			rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0);

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, 1, "Can't add contact id: %08x",id);
				return FALSE;
			}

			//osync_change_set_hash(change, "new hash");

			osync_debug("SYNCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SYNCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	//if you use hashtable, update the hash now.
	//osync_hashtable_update_hash(env->hashtable, change);
	return TRUE;
}

/*
 Sync_done: This function will only be called if the sync was successfull
*/
static void sync_done(OSyncContext *ctx)
{
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
	
	//commit any change done to forget
	for(i=0;i<(env->contact_ids)->changed_count;i++){
		rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->contact_ids)->type)->id,(env->contact_ids)->changed_ids[i]);
		osync_debug("SYNCE-SYNC", 4, "commit changed contact id %08x",(env->contact_ids)->changed_ids[i]);
	}

	for(i=0;i<(env->contact_ids)->deleted_count;i++){
		rra_syncmgr_mark_object_unchanged(env->syncmgr,((env->contact_ids)->type)->id,(env->contact_ids)->deleted_ids[i]);
		osync_debug("SYNCE-SYNC", 4, "commit deleted contact id %08x",(env->contact_ids)->deleted_ids[i]);
	}

	//If we have a hashtable we can now forget the already reported changes
	//osync_hashtable_forget(env->hashtable);
	//If we use anchors we have to update it now.
	//char *lanchor = NULL;
	//Now you get/calculate the current anchor of the device
	//osync_anchor_update(env->member, "lanchor", lanchor);

	osync_debug("SYNCE-SYNC", 4, "Sync done.");	
	
	//Answer the call
	osync_context_report_success(ctx);
}

/*
Disconnect function: Called at the end of the process, should close the RRA connection.
*/
static void disconnect(OSyncContext *ctx)
{
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
	
	plugin_environment *env = (plugin_environment *)osync_context_get_plugin_data(ctx);
	
	if (env->syncmgr==NULL) {
		osync_context_report_error(ctx, 1, "ERRROR: no connection established");
		return;
	}
	
	rra_syncmgr_destroy(env->syncmgr);
        env->syncmgr = NULL;
        
       	osync_debug("SYNCE-SYNC", 4, "Connection closed.");	
        
	//Close all stuff you need to close
	//Close the hashtable
	//osync_hashtable_close(env->hashtable);
	//Answer the call
	osync_context_report_success(ctx);
}

/*
Finalize function: Called to unalloc all the structs and libraries used.
*/
static void finalize(void *data)
{
	osync_debug("SYNCE-SYNC", 4, "start: %s", __func__);	
	
	plugin_environment *env = (plugin_environment *)data;
	//Free all stuff that you have allocated here.
	
	CeRapiUninit();

	//osync_hashtable_free(env->hashtable);

	free(env);
}

void get_info(OSyncPluginInfo *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info((void*)env);

	//Tell opensync something about your plugin
	info->name = "synce-plugin";
	info->longname = "";
	info->description = "by mirkuz";
	//the version of the api we are using, (1 at the moment)
	info->version = 1;
	//If you plugin is threadsafe.
	info->is_threadsafe = TRUE;
	
	//Now set the function we made earlier
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;

	//If you like, you can overwrite the default timeouts of your plugin
	//The default is set to 60 sec. Note that this MUST NOT be used to
	//wait for expected timeouts (Lets say while waiting for a webserver).
	//you should wait for the normal timeout and return a error.
	info->timeouts.connect_timeout = 5;
	//There are more timeouts for the other functions
	
	//Now you have to tell opensync all the object types that your are accepting.
	//This can be more than one
	osync_plugin_accept_objtype(info, "contact");
	//which format do you accept for this objtype
	//osync_plugin_accept_objformat(info, "Contact", "vCard", NULL);
	//set the commit function for this format
	//osync_plugin_set_commit_objformat(info, "Contact", "vCard", commit_change);
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", commit_contacts_change);
	//osync_plugin_set_access_objformat(info, "contact", "vcard30", commit_contact_change);
}






