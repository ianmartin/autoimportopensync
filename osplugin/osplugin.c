#include <opensync/opensync.h>
#include <glib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "opensync/opensync_internals.h"

typedef struct PluginProcess {
	OSyncEnv *env;
	OSyncMember *member;
	OSyncQueue *incoming;
	OSyncQueue *outgoing;
} PluginProcess;

typedef struct context {
	PluginProcess *pp;
	OSyncMessage *message;
} context;

void message_handler(OSyncMessage*, void*);
void message_callback(OSyncMember*, context*, OSyncError**);

void process_free(PluginProcess *pp)
{
	if (pp->incoming) {
		osync_queue_disconnect(pp->incoming, NULL);
		osync_queue_remove(pp->incoming, NULL);
		osync_queue_free(pp->incoming);
	}

	if (pp->outgoing) {
		osync_queue_disconnect(pp->incoming, NULL);
		osync_queue_free(pp->outgoing);
	}
	
	if (pp->env)
		osync_env_free(pp->env);
	
	g_free(pp);
}

void process_error_shutdown(PluginProcess *pp, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, pp, error);
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_ERROR, 0, NULL);
	if (!message)
		goto error;
	
	osync_message_set_error(message, error);
	
	if (!osync_queue_send_message(pp->outgoing, NULL, message, NULL))
		goto error_free_message;
	
	sleep(1);
	
	process_free(pp);
	osync_trace(TRACE_EXIT, "%s", __func__);
	exit(1);

error_free_message:
	osync_message_unref(message);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	exit(2);
}

void osync_client_changes_sink(OSyncMember *member, OSyncChange *change, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, change, user_data);
	context *ctx = (context *)user_data;
	PluginProcess *pp = ctx->pp;
	OSyncMessage *orig = ctx->message;
	
	OSyncError *error = NULL;
	
	if (osync_message_is_answered(orig)) {
		osync_change_free(change);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}
	
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NEW_CHANGE, 0, &error);
	if (!message)
		process_error_shutdown(pp, &error);
	
	osync_marshal_change(message, change);
	
	osync_message_write_long_long_int(message, osync_member_get_id(member));
	
	if (!osync_queue_send_message(pp->outgoing, NULL, message, &error))
		process_error_shutdown(pp, &error);
		
	osync_trace(TRACE_EXIT, "%s", __func__);
}

int main( int argc, char **argv )
{
	osync_trace(TRACE_ENTRY, "%s(%i, %p)", __func__, argc, argv);
	GMainLoop *syncloop;
	GMainContext *context;
	OSyncError *error = NULL;
	PluginProcess pp;
	assert(argc == 3);

  char *groupname = argv[ 1 ];
  int member_id = atoi( argv[ 2 ] );

  context = g_main_context_new();
  syncloop = g_main_loop_new(context, TRUE);

  /** Create environment **/
  OSyncEnv *env = osync_env_new();
  if (!osync_env_initialize(env, &error)) {
    fprintf(stderr, "Unable to initialize environment: %s\n", osync_error_print(&error));
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
    osync_error_free(&error);
    return 1;
  }

  /** Find group **/
  OSyncGroup *group = osync_env_find_group(env, groupname);
  if (!group) {
    fprintf(stderr, "Unable to find group with name %s\n", groupname);
    osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find group with name %s", __func__, groupname);
    return 2;
  }

  /** Find member **/
  int i;
  for ( i = 0; i < osync_group_num_members(group); ++i ) {
    pp.member = osync_group_nth_member(group, i);
    if (member_id == osync_member_get_id(pp.member))
      break;
    else
      pp.member = NULL;
  }
  if ( !pp.member ) {
    fprintf(stderr, "Unable to find member with id %d\n", member_id);
    osync_trace(TRACE_EXIT_ERROR, "%s: Unable to find member with id %d", __func__, member_id);
    return 3;
  }

  /** Create connection pipes **/
  char *pipe_path = g_strdup_printf( "%s/pluginpipe", osync_member_get_configdir( pp.member ) );
  pp.incoming = osync_queue_new( pipe_path, TRUE, &error );
  pp.outgoing = NULL;
  g_free( pipe_path );

  osync_queue_create( pp.incoming, &error );
  if ( osync_error_is_set( &error ) )
    osync_error_free( &error );

  /** Idle until the syncengine connects to (and reads from) our pipe **/
  if (!osync_queue_connect( pp.incoming, O_RDONLY, 0 )) {
  	fprintf(stderr, "Unable to connect\n");
	osync_trace(TRACE_EXIT_ERROR, "%s: Unable to connect", __func__);
  	exit(1);
  }
	/** Set callback functions **/
	OSyncMemberFunctions *functions = osync_member_get_memberfunctions(pp.member);
	functions->rf_change = osync_client_changes_sink;
	//functions->rf_message = osync_client_message_sink;
	//functions->rf_sync_alert = osync_client_sync_alert_sink;

	/** Start loop **/
	osync_trace(TRACE_INTERNAL, "plugin setuping up mainloop");
  osync_queue_set_message_handler(pp.incoming, message_handler, &pp);
  osync_queue_setup_with_gmainloop(pp.incoming, context);
	osync_member_set_loop(pp.member, context);

	osync_trace(TRACE_INTERNAL, "running loop");
  g_main_loop_run(syncloop);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return 0;
}

void message_handler(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	PluginProcess *pp = user_data;

	OSyncMessage *reply = NULL;
	OSyncError *error = NULL;
	//OSyncChange *change = 0;
	OSyncMember *member = pp->member;
	char *enginepipe = NULL;
   	context *ctx = NULL;

	osync_trace(TRACE_INTERNAL, "plugin received command %i", osync_message_get_command( message ));

	switch ( osync_message_get_command( message ) ) {
		case OSYNC_MESSAGE_NOOP:
			break;
		case OSYNC_MESSAGE_INITIALIZE:
			osync_trace(TRACE_INTERNAL, "init.");
			osync_message_read_string(message, &enginepipe);
			
			osync_trace(TRACE_INTERNAL, "enginepipe %s", enginepipe);
			pp->outgoing = osync_queue_new(enginepipe, TRUE, NULL);
			if (!pp->outgoing) {
				fprintf(stderr, "Unable to make new queue\n");
				exit(1);
			}
			osync_trace(TRACE_INTERNAL, "connecting to engine");
			if (!osync_queue_connect(pp->outgoing, O_WRONLY, 0 )) {
				fprintf(stderr, "Unable to make new queue\n");
				exit(1);
			}
			
			osync_trace(TRACE_INTERNAL, "done connecting to engine");
			/** Instanciate plugin **/
			if (!osync_member_instance_default_plugin(pp->member, &error))
				goto error;

			/** Initialize plugin **/
			if (!osync_member_initialize(pp->member, &error))
				goto error;
			
			osync_trace(TRACE_INTERNAL, "sending reply to engine");
  			reply = osync_message_new_reply(message, NULL);
  			if (!reply) {
				fprintf(stderr, "Unable to make new reply\n");
  				exit(1);
  			}
  				
			if (!osync_queue_send_message(pp->outgoing, NULL, reply, NULL)) {
				fprintf(stderr, "Unable to make send reply\n");
				exit(1);
			}
			
			osync_trace(TRACE_INTERNAL, "done sending to engine");
			break;
	case OSYNC_MESSAGE_FINALIZE:
		osync_member_finalize(pp->member);
		
		reply = osync_message_new_reply(message, NULL);
		if (!reply) {
			fprintf(stderr, "Unable to make new reply\n");
			exit(1);
		}
			
		if (!osync_queue_send_message(pp->outgoing, NULL, reply, NULL)) {
			fprintf(stderr, "Unable to make send reply\n");
			exit(1);
		}

		/*FIXME: how to wait for a message to be sent? */

		exit(0);
	break;
    case OSYNC_MESSAGE_CONNECT:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
		osync_member_connect(member, (OSyncEngCallback)message_callback, ctx);
		break;
    case OSYNC_MESSAGE_GET_CHANGES:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
  		osync_member_get_changeinfo(member, (OSyncEngCallback)message_callback, ctx);
      break;
    case OSYNC_MESSAGE_COMMIT_CHANGE:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
		OSyncChange *change;
  		osync_demarshal_change(message, member->group->conv_env, &change);
		osync_change_set_member(change, member);
	  	osync_member_commit_change(member, change, (OSyncEngCallback)message_callback, ctx);
      break;
    case OSYNC_MESSAGE_SYNC_DONE:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
  		osync_member_sync_done(member, (OSyncEngCallback)message_callback, ctx);
      break;
    case OSYNC_MESSAGE_DISCONNECT:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
  		osync_member_disconnect(member, (OSyncEngCallback)message_callback, ctx);
      break;
    case OSYNC_MESSAGE_REPLY:
      break;
  	case OSYNC_MESSAGE_ERRORREPLY:
      break;
  	/*case OSYNC_MESSAGE_GET_DATA:
      osync_demarshal_change( queue, &change, &error );
	  	osync_member_get_change_data(member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "message_handler");
      break;*/
  	case OSYNC_MESSAGE_COMMITTED_ALL:
    	ctx = malloc(sizeof(context));
    	ctx->pp = pp;
    	ctx->message = message;
    	osync_message_ref(message);
  		osync_member_committed_all(member, (OSyncEngCallback)message_callback, ctx);
      break;
  	/*case OSYNC_MESSAGE_READ_CHANGE:
      osync_demarshal_change( queue, &change, &error );
	  	osync_member_read_change(client->member, change, (OSyncEngCallback)message_callback, message);
		  osync_trace(TRACE_EXIT, "message_handler");
      break;*/
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
      break;
    default:
    	osync_trace(TRACE_INTERNAL, "%s: ERROR: Unknown message", __func__);
    	g_assert_not_reached();
      break;
  }

	if (reply)
		osync_message_unref(reply);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
	
error:;
	OSyncMessage *errorreply = osync_message_new_errorreply(message, NULL);
	if (!errorreply) {
		fprintf(stderr, "Unable to make new reply\n");
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		exit(1);
	}
		
	osync_message_set_error(errorreply, &error);	
	
	if (!osync_queue_send_message(pp->outgoing, NULL, errorreply, NULL)) {
		fprintf(stderr, "Unable to send error\n");
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		exit(1);
	}
	
	osync_message_unref(errorreply);
	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
}

void message_callback(OSyncMember *member, context *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, member, ctx, error);
	
	OSyncMessage *message = ctx->message;
	PluginProcess *pp = ctx->pp;
	g_free(ctx);
	
	OSyncMessage *reply = NULL;

	if (osync_message_is_answered(message) == TRUE) {
    	osync_message_unref(message);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}
	
	if (!osync_error_is_set(error)) {
		reply = osync_message_new_reply(message, error);
		osync_debug("CLI", 4, "Member is replying with message %p to message %p:\"%lli-%i\" with no error", reply, message, message->id1, message->id2);
	} else {
		reply = osync_message_new_errorreply(message, error);
		osync_message_set_error(reply, error);
		osync_debug("CLI", 1, "Member is replying with message %p to message %p:\"%lli-%i\" with error %i: %s", reply, message, message->id1, message->id2, osync_error_get_type(error), osync_error_print(error));
	}
	
	osync_queue_send_message(pp->outgoing, NULL, reply, NULL);
	osync_message_set_answered(message);
	
	osync_message_unref(message);
	osync_message_unref(reply);
    	
	osync_trace(TRACE_EXIT, "%s", __func__);
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
