#include "opensync.h"
#include "opensync_internals.h"

int osync_marshal_get_size_changetype( OSyncChangeType changetype )
{
  return sizeof( int );
}

void osync_marshal_changetype( OSyncMessage *message, OSyncChangeType changetype )
{
  osync_message_write_int( message, (int)changetype );
}

void osync_demarshal_changetype( OSyncMessage *message, OSyncChangeType *changetype )
{
  int change_type = 0;

  osync_message_read_int( message, &change_type );
  *changetype =(OSyncChangeType)change_type;
}

int osync_marshal_get_size_change( OSyncChange *change )
{
  int size = 0;

  if ( !change )
    return size;

  size += strlen( change->uid );
  size += strlen( change->hash );
  size += sizeof( int ); // change->size
  size += change->size;  // change->data
  size += sizeof( int ); // change->has_data
  size += strlen( change->objtype_name );
  size += strlen( change->format_name );
  size += strlen( change->initial_format_name );
  size += osync_marshal_get_size_changetype( change->changetype );
  size += sizeof( long long int ); // change->id
  size += strlen( change->destobjtype );
  size += strlen( change->sourceobjtype );
  size += osync_marshal_get_size_member( change->sourcemember );

  return size;
}

static void osync_marshal_changedata(OSyncMessage *message, OSyncChange *change)
{
  // TODO: check for plain/struct
  osync_message_write_int( message, change->size );
  osync_message_write_data( message, change->data, change->size );
}

void osync_marshal_change( OSyncMessage *message, OSyncChange *change )
{
  osync_message_write_string( message, change->uid );
  osync_message_write_string( message, change->hash );

  osync_marshal_changedata(message, change);

  char *format_name = change->format ? change->format->name : change->format_name;
  char *objtype_name = change->objtype ? change->objtype->name : change->objtype_name;
  char *initial_format_name = change->initial_format ? change->initial_format->name : change->initial_format_name;
  osync_message_write_int( message, change->has_data );
  osync_message_write_string( message, objtype_name );
  osync_message_write_string( message, format_name );
  osync_message_write_string( message, initial_format_name );
  osync_marshal_changetype( message, change->changetype );
  osync_message_write_long_long_int( message, change->id );
  osync_message_write_string( message, change->destobjtype );
  osync_message_write_string( message, change->sourceobjtype );
  osync_marshal_member( message, change->sourcemember );
}

static void osync_demarshal_changedata(OSyncMessage *message, OSyncChange *change)
{
  // TOOD: check for plain/struct
  osync_message_read_int( message, &( change->size ) );

  change->data = malloc( change->size );
  osync_message_read_data( message, change->data, change->size );
}

void osync_demarshal_change( OSyncMessage *message, OSyncChange **change )
{
  OSyncChange *new_change = osync_change_new();

  osync_message_read_string( message, &( new_change->uid ) );
  osync_message_read_string( message, &( new_change->hash ) );

  osync_demarshal_changedata(message, new_change);

  osync_message_read_int( message, &( new_change->has_data ) );
  osync_message_read_string( message, &( new_change->objtype_name ) );
  // TODO: find objtype in pool

  osync_message_read_string( message, &( new_change->format_name ) );
  // TODO: find format in pool

  osync_message_read_string( message, &( new_change->initial_format_name ) );
  // TODO: find initial_format in pool

  // TODO: set new_change->conv_env
  osync_demarshal_changetype( message, &( new_change->changetype ) );
  osync_message_read_long_long_int( message, &( new_change->id ) );
  osync_message_read_string( message, &( new_change->destobjtype ) );
  osync_message_read_string( message, &( new_change->sourceobjtype ) );
  osync_demarshal_member( message, &( new_change->sourcemember ) );

  new_change->member = 0;
  new_change->engine_data = 0;
  new_change->mappingid = 0;
  new_change->changes_db = 0;

  *change = new_change;
}

int osync_marshal_get_size_member( OSyncMember *member )
{
  return sizeof( int );
}

void osync_marshal_member( OSyncMessage *message, OSyncMember *member )
{
  if ( member ) {
    osync_message_write_int( message, member->id );
  } else {
    osync_message_write_int( message, -1 );
  }
}

void osync_demarshal_member( OSyncMessage *message, OSyncMember **member )
{
  int id;

  osync_message_read_int( message, &id );

  if ( id == -1 )
    *member = 0;
  else {
    // search in pool
  }
}

int osync_marshal_get_size_error( OSyncError **error )
{
  int size = 0;

  if ( !osync_error_is_set(error) )
    return size;

  size += sizeof( int );
  size += strlen( (*error)->message );

  return size;
}

void osync_marshal_error( OSyncMessage *message, OSyncError *error )
{
	if (error) {
		osync_message_write_int( message, 1 );
		osync_message_write_int( message, (int)error->type );
		osync_message_write_string( message, error->message );
	} else {
		osync_message_write_int( message, 0 );
	}
}

void osync_demarshal_error( OSyncMessage *message, OSyncError **error )
{
	int hasError = 0;

	osync_message_read_int( message, &hasError );
	
	if (hasError) {
		char *msg;
		int error_type;
		
		osync_message_read_int( message, &error_type );
		osync_message_read_string( message, &msg );
		
		osync_error_set( error, (OSyncErrorType)error_type, msg );
	} else
		osync_error_free(error);
}

int osync_marshal_get_size_message( OSyncMessage *message )
{
  int size = 0;

  if ( !message )
    return size;

  size += sizeof( int ); // message->cmd
  size += sizeof( long long int ); // message->id
  size += sizeof( int ); // has error
  size += osync_marshal_get_size_error( &(message->error) );

  return 0;
}
