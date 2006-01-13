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

void handle_read( OSyncQueue *queue )
{
  int request_type;
  OSyncChange *change;
  OSyncError *error = 0;

  while ( 1 ) {
    if ( !osync_queue_read_int( queue, &request_type, &error ) ) {
      printf( "can't read request type\n" );
      osync_error_free( &error );
      return;
    }

    switch ( request_type ) {
      case TYPE_OSYNC_CHANGE:

        if ( !osync_demarshal_change( queue, &change, &error ) ) {
          printf( "can't read request\n" );
          osync_error_free( &error );
          return;
        }
        osync_print_change( change );
        break;
    }
    break;
  }
}

void handle_write( OSyncQueue *queue )
{
  OSyncError *error = 0;

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

  if ( osync_queue_send_int( queue, TYPE_OSYNC_CHANGE, &error ) == FALSE )
    return;

  if ( osync_marshal_change( queue, change, &error ) == FALSE )
    return;
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
    OSyncQueue queue;
    queue.fd = pfd[ 0 ];
    handle_read( &queue );
    close( pfd[ 0 ] );

    exit( 0 );
  } else {
    close( pfd[ 0 ] );
    OSyncQueue queue;
    queue.fd = pfd[ 1 ];
    handle_write( &queue );
    close( pfd[ 1 ] );

    wait( NULL );
    exit( 0 );
  }

  return 0;
}
