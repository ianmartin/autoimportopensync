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
  OSyncError *error = 0;
  OSyncQueue *queue;

  pid_t cpid = fork();
  if ( cpid == -1 ) {
    perror( "fork" );
    return 1;
  }

  queue = osync_queue_new( "/tmp/testpipe", &error );
  osync_queue_create( queue, &error );
  if ( osync_error_is_set( &error ) )
    osync_error_free( &error );

  while ( !osync_queue_connect( queue, 0 ) ) {
    printf( "i'm sleeping (%x)\n", cpid );
    usleep( 10000 );
  }

  if ( cpid == 0 ) { // child
    sleep( 1 );
    handle_read( queue );
    printf( "done read\n" );
    exit( 0 );
  } else {
    handle_write( queue );
    printf( "done write\n" );
    wait( NULL );
    exit( 0 );
  }

  return 0;
}
