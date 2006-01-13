#include <opensync/opensync.h>
#include <glib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "opensync/opensync_internals.h"

void message_handler(OSyncMessage*, void*);

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

	/** Start loop **/
  osync_queue_set_message_handler(queue, message_handler, 0);
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
  OSyncError *error = 0;
  

  switch ( osync_message_get_command( message ) ) {
    case OSYNC_MESSAGE_CONNECT:
  		osync_member_connect(client->member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_GET_CHANGES:
  		osync_member_get_changeinfo(client->member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_COMMIT_CHANGE:
  		OSyncChange *change = itm_message_get_data(message, "change");
	  	osync_member_commit_change(client->member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_SYNC_DONE:
  		osync_member_sync_done(client->member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_DISCONNECT:
  		osync_member_disconnect(client->member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_REPLY:
      break;
  	case OSYNC_MESSAGE_ERRORREPLY:
      break;
  	case OSYNC_MESSAGE_GET_DATA:
  		OSyncChange *change = itm_message_get_data(message, "change");
	  	osync_member_get_change_data(client->member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "client_message_handler");
      break;
  	case OSYNC_MESSAGE_COMMITTED_ALL:
  		osync_member_committed_all(client->member, (OSyncEngCallback)message_callback, message);
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
  	case OSYNC_MESSAGE_READ_CHANGE:
  		OSyncChange *change = itm_message_get_data(message, "change");
	  	osync_member_read_change(client->member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "client_message_handler");
      break;
    case OSYNC_MESSAGE_CALL_PLUGIN:
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
	  	osync_trace(TRACE_EXIT, "client_message_handler");
      break;
  }
	
	osync_debug("CLI", 0, "Unknown message \"%s\"\n", itm_message_get_msgname(message));
	osync_trace(TRACE_EXIT_ERROR, "client_message_handler: Unknown message");
	g_assert_not_reached();
}
