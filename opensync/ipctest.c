#include <sys/wait.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <opensync/opensync.h>

// HACK
#include "opensync_internals.h"

#include "opensync_serializer.h"
#include "easyipc.h"

void handle_read( int fd )
{
  int request_type;
  OSyncChange *change;

  while ( 1 ) {
    eipc_read_int( fd, &request_type );
    switch ( request_type ) {
      case TYPE_OSYNC_CHANGE:

        osync_demarshal_change( fd, &change );
        osync_print_change( change );
        break;
    }
    break;
  }
}

void handle_write( int fd )
{
  OSyncChange *change = osync_change_new();
  change->uid = g_strdup( "myuid" );
  change->hash = g_strdup( "jshdfkahsdkfh" );
  change->size = 15;
  change->data = g_strdup( "that are my data" );
  change->has_data = 1;
  change->objtype_name = g_strdup( "contact" );
  change->format_name = g_strdup( "vcard" );
  change->initial_format_name = g_strdup( "vcard1.1" );
  change->changetype = 2;
  change->id = 123456789;
  change->destobjtype = g_strdup( "destobjtypex" );
  change->sourceobjtype = g_strdup( "sourceobjtypex" );

  osync_print_change( change );

  eipc_write_int( fd, TYPE_OSYNC_CHANGE );
  osync_marshal_change( fd, change );
}

int main()
{
  int pfd[ 2 ];
  pid_t cpid;


  if ( pipe( pfd ) == -1 ) {
    perror( "pipe" );
    return 1;
  }

  cpid = fork();
  if ( cpid == -1 ) {
    perror( "fork" );
    return 1;
  }

  if ( cpid == 0 ) { // child
    close( pfd[ 1 ] );
    handle_read( pfd[ 0 ] );
    close( pfd[ 0 ] );

    exit( 0 );
  } else {
    close( pfd[ 0 ] );
    handle_write( pfd[ 1 ] );
    close( pfd[ 1 ] );

    wait( NULL );
    exit( 0 );
  }

  return 0;
}
