#include "opensync.h"
#include "opensync_internals.h"

#include "opensync_serializer.h"

osync_bool osync_marshal_changetype( OSyncQueue *queue, OSyncChangeType changetype, OSyncError **error )
{
  if ( !osync_queue_send_int( queue, (int)changetype, error ) )
    return FALSE;

  return TRUE;
}

osync_bool osync_demarshal_changetype( OSyncQueue *queue, OSyncChangeType *changetype, OSyncError **error )
{
  int change_type = 0;

  if ( !osync_queue_read_int( queue, &change_type, error ) )
    return FALSE;

  *changetype =(OSyncChangeType)change_type;

  return TRUE;
}

osync_bool osync_marshal_change( OSyncQueue *queue, OSyncChange *change, OSyncError **error )
{
  if ( !osync_queue_send_string( queue, change->uid, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->hash, error ) )
    return FALSE;

  if ( !osync_queue_send_int( queue, change->size, error ) )
    return FALSE;


  // TODO: check for plain/struct

  printf( "alive1\n" );
  if ( !osync_queue_send_data( queue, change->data, change->size, error ) )
    return FALSE;

  printf( "alive\n" );
  if ( !osync_queue_send_int( queue, change->has_data, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->objtype_name, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->format_name, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->initial_format_name, error ) )
    return FALSE;



  if ( !osync_marshal_changetype( queue, change->changetype, error ) )
    return FALSE;

  if ( !osync_queue_send_long_long_int( queue, change->id, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->destobjtype, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, change->sourceobjtype, error ) )
    return FALSE;

  if ( !osync_marshal_member( queue, change->sourcemember, error ) )
    return FALSE;

  return TRUE;
}

osync_bool osync_demarshal_change( OSyncQueue *queue, OSyncChange **change, OSyncError **error )
{
  OSyncChange *new_change = osync_change_new();

  if ( !osync_queue_read_string( queue, &( new_change->uid ), error ) )
    return FALSE;

  if ( !osync_queue_read_string( queue, &( new_change->hash ), error ) )
    return FALSE;

  if ( !osync_queue_read_int( queue, &( new_change->size ), error ) )
    return FALSE;

  new_change->data = (void*)malloc( new_change->size );
  if ( !osync_queue_read_data( queue, new_change->data, new_change->size, error ) )
    return FALSE;

  if ( !osync_queue_read_int( queue, &( new_change->has_data ), error ) )
    return FALSE;

  if ( !osync_queue_read_string( queue, &( new_change->objtype_name ), error ) )
    return FALSE;

  // TODO: find objtype in pool

  if ( !osync_queue_read_string( queue, &( new_change->format_name ), error ) )
    return FALSE;

  // TODO: find format in pool

  if ( !osync_queue_read_string( queue, &( new_change->initial_format_name ), error ) )
    return FALSE;

  // TODO: find initial_format in pool

  // TODO: set new_change->conv_env

  if ( !osync_demarshal_changetype( queue, &( new_change->changetype ), error ) )
    return FALSE;

  if ( !osync_queue_read_long_long_int( queue, &( new_change->id ), error ) )
    return FALSE;

  if ( !osync_queue_read_string( queue, &( new_change->destobjtype ), error ) )
    return FALSE;

  if ( !osync_queue_read_string( queue, &( new_change->sourceobjtype ), error ) )
    return FALSE;

  if ( !osync_demarshal_member( queue, &( new_change->sourcemember ), error ) )
    return FALSE;

  new_change->member = 0;
  new_change->engine_data = 0;
  new_change->mappingid = 0;
  new_change->changes_db = 0;

  *change = new_change;

  return TRUE;
}

void osync_print_change( OSyncChange *change )
{
  if ( !change )
    return;

  printf( "OSyncChange:\n" );
  printf( "\tuid:      %s\n", change->uid );
  printf( "\thash:     %s\n", change->hash );
  printf( "\tsize:     %d\n", change->size );
  printf( "\thasdata:  %d\n", change->has_data );
  printf( "\tobjtype:  %s\n", change->objtype_name );
  printf( "\tformat:   %s\n", change->format_name );
  printf( "\tiformat:  %s\n", change->initial_format_name );
  printf( "\tctype:    %d\n", change->changetype );
  printf( "\tid:       %lld\n", change->id );
  printf( "\tdobjtype: %s\n", change->destobjtype );
  printf( "\tsobjtype: %s\n", change->sourceobjtype );
}

osync_bool osync_marshal_member( OSyncQueue *queue, OSyncMember *member, OSyncError **error )
{
  if ( member ) {
    if ( !osync_queue_send_int( queue, member->id, error ) )
      return FALSE;
  } else {
    if ( !osync_queue_send_int( queue, -1, error ) )
      return FALSE;
  }

  return TRUE;
}

osync_bool osync_demarshal_member( OSyncQueue *queue, OSyncMember **member, OSyncError **error )
{
  int id;

  if ( !osync_queue_read_int( queue, &id, error ) )
    return FALSE;

  if ( id == -1 )
    *member = 0;
  else {
    // search in pool
  }

  return TRUE;
}

osync_bool osync_marshal_error( OSyncQueue *queue, OSyncError *error_object, OSyncError **error )
{
  if ( !osync_queue_send_int( queue, (int)error_object->type, error ) )
    return FALSE;

  if ( !osync_queue_send_string( queue, error_object->message, error ) )
    return FALSE;

  return TRUE;
}

osync_bool osync_demarshal_error( OSyncQueue *queue, OSyncError **error_object, OSyncError **error )
{
  char *message;
  int error_type;

  if ( !osync_queue_read_int( queue, &error_type, error ) )
    return FALSE;

  if ( !osync_queue_read_string( queue, &message, error ) )
    return FALSE;

  osync_error_set( error_object, (OSyncErrorType)error_type, message );

  return TRUE;
}

osync_bool osync_marshal_message( OSyncQueue *queue, OSyncMessage *message, OSyncError **error )
{
  if ( !osync_queue_send_int( queue, (int)message->cmd, error ) )
    return FALSE;

  if ( !osync_queue_send_long_long_int( queue, (int)message->id, error ) )
    return FALSE;

  if ( !osync_marshal_error( queue, message->error, error ) )
    return FALSE;

  return TRUE;
}

osync_bool osync_demarshal_message( OSyncQueue *queue, OSyncMessage *message, OSyncError **error )
{
  int message_command = 0;
  if ( !osync_queue_read_int( queue, &message_command, error ) )
    return FALSE;

  message->cmd = (OSyncMessageCommand)message_command;

  if ( !osync_queue_read_long_long_int( queue, &(message->id), error ) )
    return FALSE;

  if ( !osync_demarshal_error( queue, &(message->error), error ) )
    return FALSE;

  return TRUE;
}
