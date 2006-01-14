#include <opensync/opensync.h>
#include <glib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "opensync/opensync_internals.h"

void message_handler(OSyncMessage*, void*);
void message_callback(OSyncMember*, ITMessage*, OSyncError**);

int main( int argc, char **argv )
{
  GMainLoop *syncloop;
  GMainContext *context;
  OSyncError *error = NULL;
  OSyncMember *member = NULL;

  assert(argc == 3);

  char *groupname = argv[ 1 ];
  int member_id = atoi( argv[ 2 ] );

  context = g_main_context_new();
  syncloop = g_main_loop_new(context, FALSE);

  /** Create environment **/
  OSyncEnv *env = osync_env_new();
  if (!osync_env_initialize(env, &error)) {
    printf("Unable to initialize environment: %s\n", osync_error_print(&error));
    osync_error_free(&error);
    return 1;
  }

  /** Find group **/
  OSyncGroup *group = osync_env_find_group(env, groupname);
  if (!group) {
    printf("Unable to find group with name %s\n", groupname);
    return 2;
  }

  /** Find member **/
  int i;
  for ( i = 0; i < osync_group_num_members(group); ++i ) {
    member = osync_group_nth_member(group, i);
    if (member_id == osync_member_get_id(member))
      break;
    else
      member = 0;
  }
  if ( !member ) {
    printf("Unable to find member with id %d\n", member_id);
    return 3;
  }

  /** Create connection pipes **/
  char *pipe_path = g_strdup_printf( "%s/pluginpipe", osync_member_get_configdir( member ) );
  OSyncQueue *queue = osync_queue_new( pipe_path, &error );
  g_free( pipe_path );

  osync_queue_create( queue, &error );
  if ( osync_error_is_set( &error ) )
    osync_error_free( &error );

  /** Idle until the syncengine connects to (and reads from) our pipe **/
  while ( !osync_queue_connect( queue, O_WRONLY | O_NONBLOCK, 0 ) )
    usleep( 10000 );

	/** Set callback functions **/
	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(member);
	functions->rf_change = osync_client_changes_sink;
	functions->rf_message = osync_client_message_sink;
	functions->rf_sync_alert = osync_client_sync_alert_sink;

	/** Start loop **/
  osync_queue_set_message_handler(queue, message_handler, member);
  osync_queue_setup_with_gmainloop(queue, context);
	osync_member_set_loop(member, context);

  /** Instanciate plugin **/
  if ( !osync_member_instance_default_plugin(member, &error) ) {
    printf("Unable to instanciate plugin %d\n", member_id);
    return 4;
  }

	/** Initialize plugin **/
	if (!osync_member_initialize(member, &error)) {
    printf("Unable to initialize plugin %d\n", member_id);
		return FALSE;
	}

  g_main_loop_run(syncloop);

  return 0;
}

void message_handler(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "message_handler(%p:%p)", message, user_data);

  OSyncError *error = 0;
  OSyncChange *change = 0;
  OSyncMember *member = (OSyncMember*)user_data;

  switch ( osync_message_get_command( message ) ) {
    case OSYNC_MESSAGE_CONNECT:
  		osync_member_connect(member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_GET_CHANGES:
  		osync_member_get_changeinfo(member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_COMMIT_CHANGE:
  		OSyncChange *change = itm_message_get_data(message, "change");
	  	osync_member_commit_change(member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_SYNC_DONE:
  		osync_member_sync_done(member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_DISCONNECT:
  		osync_member_disconnect(member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_REPLY:
      break;
  	case OSYNC_MESSAGE_ERRORREPLY:
      break;
  	case OSYNC_MESSAGE_GET_DATA:
      osync_demarshal_change( queue, &change, &error );
	  	osync_member_get_change_data(member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "message_handler");
      break;
  	case OSYNC_MESSAGE_COMMITTED_ALL:
  		osync_member_committed_all(member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
  	case OSYNC_MESSAGE_READ_CHANGE:
      osync_demarshal_change( queue, &change, &error );
	  	osync_member_read_change(client->member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "message_handler");
      break;
    case OSYNC_MESSAGE_CALL_PLUGIN:
/*
  		char *function = itm_message_get_data(message, "function");
	  	void *data = itm_message_get_data(message, "data");
		  OSyncError *error = NULL;
  		void *replydata = osync_member_call_plugin(client->member, function, data, &error);


  		if (itm_message_get_data(message, "want_reply")) {
	  		ITMessage *reply = NULL;
		  	if (!osync_error_is_set(&error)) {
			  	reply = itm_message_new_methodreply(client, message);
				  itm_message_set_data(message, "reply", replydata);
  			} else {
	  			reply = itm_message_new_errorreply(client, message);
		  		itm_message_set_error(reply, error);
  			}
		
	  		itm_message_send_reply(reply);
  		}
*/
	  	osync_trace(TRACE_EXIT, "message_handler");
      break;
    default:
    	osync_trace(TRACE_EXIT_ERROR, "message_handler: Unknown message");
    	g_assert_not_reached();
      break;
  }
}

void message_callback(OSyncMember *member, OSyncMessage *message, OSyncError **error)
{
	OSyncMessage *reply = NULL;

	if (osync_message_is_answered(message) == TRUE)
		return;

	if (!osync_error_is_set(error)) {
		reply = osync_message_new_reply(message, error);
		osync_debug("CLI", 4, "Member is replying with message %p to message %p:\"%s\" with no error", reply, message, message->msgname);
	} else {
		reply = osync_message_new_errorreply(message, error);
		osync_message_set_error(reply, *error);
		osync_debug("CLI", 1, "Member is replying with message %p to message %p:\"%s\" with error %i: %s", reply, message, message->msgname, osync_error_get_type(error), osync_error_print(error));
	}
	
	itm_message_move_data(message, reply);
	itm_message_send_reply(reply);
	osync_message_set_answered(message);
}

void *osync_client_message_sink(OSyncMember *member, const char *name, void *data, osync_bool synchronous)
{
/*
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	if (!synchronous) {
  
		ITMessage *message = itm_message_new_signal(client, "PLUGIN_MESSAGE");
		osync_debug("CLI", 3, "Sending message %p PLUGIN_MESSAGE for message %s", message, name);
		itm_message_set_data(message, "data", data);
		itm_message_set_data(message, "name", g_strdup(name));
		itm_queue_send(engine->incoming, message);
    
		return NULL;
	} else {
		return engine->plgmsg_callback(engine, client, name, data, engine->plgmsg_userdata);
	}	
*/
  return NULL;
}

void osync_client_changes_sink(OSyncMember *member, OSyncChange *change, void *user_data)
{
/*
	ITMessage *orig = (ITMessage *)user_data;
	
	if (itm_message_is_answered(orig))
		return; //FIXME How do we free the change here?
	
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "NEW_CHANGE");
	itm_message_set_data(message, "change", change);
	itm_message_reset_timeout(orig);
	itm_queue_send(engine->incoming, message);
*/
}

void osync_client_sync_alert_sink(OSyncMember *member)
{
/*
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, member);
	OSyncClient *client = osync_member_get_data(member);
	OSyncEngine *engine = client->engine;
	ITMessage *message = itm_message_new_signal(client, "SYNC_ALERT");
	itm_queue_send(engine->incoming, message);
	osync_trace(TRACE_EXIT, "%s", __func__);
*/
}
