#include "opensync.h"
#include "opensync_internals.h"

#include "opensync_serializer.h"

osync_bool osync_marshal_changetype( OSyncQueue *queue, OSyncChangeType changetype, OSyncError **error )
{
  int change_type = 0;

  switch ( changetype ) {
    case CHANGE_UNKNOWN:
      change_type = 0;
      break;
    case CHANGE_ADDED:
      change_type = 1;
      break;
    case CHANGE_UNMODIFIED:
      change_type = 2;
      break;
    case CHANGE_DELETED:
      change_type = 3;
      break;
    case CHANGE_MODIFIED:
      change_type = 4;
      break;
  }

  if ( osync_queue_send_int( queue, change_type, error ) == FALSE )
    return FALSE;
  else
    return TRUE;
}

osync_bool osync_demarshal_changetype( OSyncQueue *queue, OSyncChangeType *changetype, OSyncError **error )
{
  int change_type;

  if ( osync_queue_read_int( queue, &change_type, error ) == FALSE )
    return FALSE;

  switch ( change_type ) {
    case 0:
      *changetype = CHANGE_UNKNOWN;
      break;
    case 1:
      *changetype = CHANGE_ADDED;
      break;
    case 2:
      *changetype = CHANGE_UNMODIFIED;
      break;
    case 3:
      *changetype = CHANGE_DELETED;
      break;
    case 4:
      *changetype = CHANGE_MODIFIED;
      break;
  }

  return TRUE;
}

osync_bool osync_marshal_change( OSyncQueue *queue, OSyncChange *change, OSyncError **error );
{
  if ( osync_queue_send_string( queue, change->uid, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->hash, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_int( queue, change->size, error ) == FALSE )
    return FALSE;

  // TODO: check for plain/struct

  if ( eipc_writen( queue, change->data, change->size, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_int( queue, change->has_data, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->objtype_name, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->format_name, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->initial_format_name, error ) == FALSE )
    return FALSE;

  if ( osync_marshal_changetype( queue, change->changetype, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_longlongint( queue, change->id, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->destobjtype, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, change->sourceobjtype, error ) == FALSE )
    return FALSE;

  if ( osync_marshal_member( queue, change->sourcemember, error ) == FALSE )
    return FALSE;

  return TRUE;
}

osync_bool osync_demarshal_change( OSyncQueue *queue, OSyncChange **change, OSyncError **error );
{
  OSyncChange *new_change = osync_change_new();
  int result = 0;

  if ( osync_queue_read_string( queue, &( new_change->uid ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_string( queue, &( new_change->hash ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_int( queue, &( new_change->size ), error ) == FALSE )
    return FALSE;

  new_change->data = (void*)malloc( new_change->size );
  if ( osync_queue_read_data( queue, new_change->data, new_change->size, error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_int( queue, &( new_change->has_data ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_string( queue, &( new_change->objtype_name ), error ) == FALSE )
    return FALSE;

  // TODO: find objtype in pool

  if ( osync_queue_read_string( queue, &( new_change->format_name ), error ) == FALSE )
    return FALSE;

  // TODO: find format in pool

  if ( osync_queue_read_string( queue, &( new_change->initial_format_name ), error ) == FALSE )
    return FALSE;

  // TODO: find initial_format in pool

  // TODO: set new_change->conv_env

  if ( osync_demarshal_changetype( queue, &( new_change->changetype ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_longlongint( queue, &( new_change->id ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_string( queue, &( new_change->destobjtype ), error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_string( queue, &( new_change->sourceobjtype ), error ) == FALSE )
    return FALSE;

  if ( osync_demarshal_member( queue, &( new_change->sourcemember ), error ) == FALSE )
    return FALSE;

  new_change->member = 0;
  new_change->engine_data = 0;
  new_change->mappingid = 0;
  new_change->changes_db = 0;

  *change = new_change;

  return result;
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



osync_bool osync_marshal_member( OSyncQueue *queue, OSyncMember *member, OSyncError **error );
{
  if ( member ) {
    if ( osync_queue_send_int( queue, member->id, error ) == FALSE )
      return FALSE;
  } else {
    if ( osync_queue_send_int( queue, -1, error ) == FALSE )
      return FALSE;
  }

  return TRUE;
}

osync_bool osync_demarshal_member( OSyncQueue *queue, OSyncMember **member, OSyncError **error );
{
  int id;

  if ( osync_queue_read_int( queue, &id, error ) == FALSE )
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
  int error_type;

  switch ( error_object->type ) {
    case OSYNC_NO_ERROR:
      error_type = 0;
      break;
    case OSYNC_ERROR_GENERIC:
      error_type = 1;
      break;
    case OSYNC_ERROR_IO_ERROR:
      error_type = 2;
      break;
    case OSYNC_ERROR_NOT_SUPPORTED:
      error_type = 3;
      break;
    case OSYNC_ERROR_TIMEOUT:
      error_type = 4;
      break;
    case OSYNC_ERROR_DISCONNECTED:
      error_type = 5;
      break;
    case OSYNC_ERROR_FILE_NOT_FOUND:
      error_type = 6;
      break;
    case OSYNC_ERROR_EXISTS:
      error_type = 7;
      break;
    case OSYNC_ERROR_CONVERT:
      error_type = 8;
      break;
    case OSYNC_ERROR_MISCONFIGURATION:
      error_type = 9;
      break;
    case OSYNC_ERROR_INITIALIZATION:
      error_type = 10;
      break;
    case OSYNC_ERROR_PARAMETER:
      error_type = 11;
      break;
    case OSYNC_ERROR_EXPECTED:
      error_type = 12;
      break;
    case OSYNC_ERROR_NO_CONNECTION:
      error_type = 13;
      break;
    case OSYNC_ERROR_TEMPORARY:
      error_type = 14;
      break;
    case OSYNC_ERROR_LOCKED:
      error_type = 15;
      break;
    case OSYNC_ERROR_PLUGIN_NOT_FOUND:
      error_type = 16;
      break;
  }

  if ( osync_queue_send_int( queue, error_type, error ) == FALSE )
    return FALSE;

  if ( osync_queue_send_string( queue, error_object->message, error ) == FALSE )
    return FALSE;
}

osync_bool osync_demarshal_error( OSyncQueue *queue, OSyncError **error_object, OSyncError **error )
{
  OSyncErrorType error_type;
  char *message;

  int value;

  if ( osync_queue_read_int( queue, &value, error ) == FALSE )
    return FALSE;

  if ( osync_queue_read_string( queue, &message, error ) == FALSE )
    return FALSE;

  switch ( value ) {
    case 0:
      error_type = OSYNC_NO_ERROR;
      break;
    case 1:
      error_type = OSYNC_ERROR_GENERIC;
      break;
    case 2:
      error_type = OSYNC_ERROR_IO_ERROR;
      break;
    case 3:
      error_type = OSYNC_ERROR_NOT_SUPPORTED;
      break;
    case 4:
      error_type = OSYNC_ERROR_TIMEOUT;
      break;
    case 5:
      error_type = OSYNC_ERROR_DISCONNECTED;
      break;
    case 6:
      error_type = OSYNC_ERROR_FILE_NOT_FOUND;
      break;
    case 7:
      error_type = OSYNC_ERROR_EXISTS;
      break;
    case 8:
      error_type = OSYNC_ERROR_CONVERT;
      break;
    case 9:
      error_type = OSYNC_ERROR_MISCONFIGURATION;
      break;
    case 10:
      error_type = OSYNC_ERROR_INITIALIZATION;
      break;
    case 11:
      error_type = OSYNC_ERROR_PARAMETER;
      break;
    case 12:
      error_type = OSYNC_ERROR_EXPECTED;
      break;
    case 13:
      error_type = OSYNC_ERROR_NO_CONNECTION;
      break;
    case 14:
      error_type = OSYNC_ERROR_TEMPORARY;
      break;
    case 15:
      error_type = OSYNC_ERROR_LOCKED;
      break;
    case 16:
      error_type = OSYNC_ERROR_PLUGIN_NOT_FOUND;
      break;
  }

  osync_error_set( error_object, error_type, message );

  return TRUE;
}
