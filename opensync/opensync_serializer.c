#include "easyipc.h"

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync_serializer.h"

static int osync_marshal_changetype( int fd, OSyncChangeType changetype )
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

  return eipc_write_int( fd, change_type );
}

static int osync_demarshal_changetype( int fd, OSyncChangeType *changetype )
{
  int change_type, result = 0;
  result |= eipc_read_int( fd, &change_type );

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

  return result;
}

int osync_marshal_change( int fd, OSyncChange *change )
{
  int result = 0;

  result |= eipc_write_string( fd, change->uid );
  result |= eipc_write_string( fd, change->hash );
  result |= eipc_write_int( fd, change->size );
  // TODO: check for plain/struct
  result |= eipc_writen( fd, change->data, change->size );
  result |= eipc_write_int( fd, change->has_data );
  result |= eipc_write_string( fd, change->objtype_name );
  result |= eipc_write_string( fd, change->format_name );
  result |= eipc_write_string( fd, change->initial_format_name );
  result |= osync_marshal_changetype( fd, change->changetype );
  result |= eipc_write_longlongint( fd, change->id );
  result |= eipc_write_string( fd, change->destobjtype );
  result |= eipc_write_string( fd, change->sourceobjtype );
  result |= osync_marshal_member( fd, change->sourcemember );

  return result;
}

int osync_demarshal_change( int fd, OSyncChange **change )
{
  OSyncChange *new_change = osync_change_new();
  int result = 0;

  result |= eipc_read_string_alloc( fd, &( new_change->uid ) );
  result |= eipc_read_string_alloc( fd, &( new_change->hash ) );
  new_change->size = 0;

  result |= eipc_read_int( fd, &( new_change->size ) );

  new_change->data = (void*)malloc( new_change->size );
  result |= eipc_readn( fd, new_change->data, new_change->size );
  result |= eipc_read_int( fd, &( new_change->has_data ) );
  result |= eipc_read_string_alloc( fd, &( new_change->objtype_name ) );
  // TODO: find objtype in pool
  result |= eipc_read_string_alloc( fd, &( new_change->format_name ) );
  // TODO: find format in pool
  result |= eipc_read_string_alloc( fd, &( new_change->initial_format_name ) );
  // TODO: find initial_format in pool

  // TODO: set new_change->conv_env

  result |= osync_demarshal_changetype( fd, &( new_change->changetype ) );
  result |= eipc_read_longlongint( fd, &( new_change->id ) );
  result |= eipc_read_string_alloc( fd, &( new_change->destobjtype ) );
  result |= eipc_read_string_alloc( fd, &( new_change->sourceobjtype ) );
  result |= osync_demarshal_member( fd, &( new_change->sourcemember ) );

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

int osync_marshal_member( int fd, OSyncMember *member )
{
  int result = 0;

  if ( member ) {
    result |= eipc_write_int( fd, member->id );
  } else {
    result |= eipc_write_int( fd, -1 );
  }

  return result;
}

int osync_demarshal_member( int fd, OSyncMember **member )
{
  int id, result = 0;
  result |= eipc_read_int( fd, &id );
  if ( id == -1 ) {
    *member = 0;
  } else {
    // search in pool
  }

  return result;
}
