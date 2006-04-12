#include "support.h"

#include <sys/wait.h>

START_TEST (ipc_new)
{
	OSyncError *error = NULL;
	OSyncQueue *queue1 = osync_queue_new("/tmp/testpipe", TRUE, &error);
	fail_unless(queue1 != NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_queue_free(queue1);
}
END_TEST

START_TEST (ipc_create)
{
	OSyncError *error = NULL;
	OSyncQueue *queue1 = osync_queue_new("/tmp/testpipe", TRUE, &error);
	fail_unless(queue1 != NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(osync_queue_create(queue1, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe") == 0, NULL);
	
	fail_unless(osync_queue_remove(queue1, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe") != 0, NULL);
	
	osync_queue_free(queue1);
}
END_TEST


START_TEST (ipc_connect)
{
	pid_t cpid = fork();
	
	OSyncError *error = NULL;
	OSyncQueue *queue = osync_queue_new("/tmp/testpipe", TRUE, &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(queue, NULL);
	
	if ( cpid == 0 ) { //Child
		while (!osync_queue_connect( queue, O_RDWR, 0 )) {
			usleep( 10000 );
		}
		
		//sleep(1);
		while (!(message = osync_queue_get_message(queue)))
			usleep(10000);
			
		//handle_write( queue );
		if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE)
			exit (1);
		
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		osync_queue_send_message(queue, NULL, reply, &error);
		
		sleep(1);
		exit( 0 );
	} else {
		fail_unless(osync_queue_connect( queue, O_RDWR, NULL ), NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		fail_unless(osync_queue_send_message(queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		while (!(message = osync_queue_get_message(queue))) {
			usleep(100000);
		}
		
		//handle_read( queue );
		wait( NULL );
	}
	
	
	
	fail_unless(system("ls /tmp/testpipe") == 0, NULL);
	
	fail_unless(osync_queue_remove(queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe") != 0, NULL);

	osync_queue_free(queue);
}
END_TEST

START_TEST (ipc_payload)
{
	pid_t cpid = fork();
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", TRUE, &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", TRUE, &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, NULL);
	osync_queue_create(client_queue, NULL);
	char *data = "this is another test string";
	
	if ( cpid == 0 ) { //Child
		while (!osync_queue_connect( server_queue, O_WRONLY, 0 )) {
			usleep( 10000 );
		}
		
		while (!osync_queue_connect( client_queue, O_RDONLY, 0 )) {
			usleep( 10000 );
		}
		
		//sleep(1);
		while (!(message = osync_queue_get_message(client_queue))) {
			usleep(10000);
		}	
		//handle_write( queue );
		if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
			exit (1);
		}
		
		int int1;
		long long int longint1;
		char *string;
		char databuf[strlen(data) + 1];
			
		osync_message_read_int(message, &int1);
		osync_message_read_string(message, &string);
		osync_message_read_long_long_int(message, &longint1);
		osync_message_read_data(message, databuf, strlen(data) + 1);
		
		
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		osync_queue_send_message(server_queue, NULL, reply, &error);
		
		sleep(1);
		exit( 0 );
	} else {
		while (!osync_queue_connect( server_queue, O_RDONLY, 0 )) {
			usleep( 10000 );
		}
		
		while (!osync_queue_connect( client_queue, O_WRONLY, 0 )) {
			usleep( 10000 );
		}
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		while (!(message = osync_queue_get_message(server_queue))) {
			usleep(100000);
		}
		
		//handle_read( queue );
		wait( NULL );
	}
	
	
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
}
END_TEST


START_TEST (ipc_large_payload)
{
	int bytes = 1024 * 1024 * 20; //5mbyte
	
	char *data = malloc(bytes);
	memset(data, 42, sizeof(data));
	char *databuf2 = malloc(bytes);
	memset(databuf2, 0, sizeof(databuf2));
	
	pid_t cpid = fork();
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", TRUE, &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", TRUE, &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, NULL);
	osync_queue_create(client_queue, NULL);
	
	if ( cpid == 0 ) { //Child
		while (!osync_queue_connect( server_queue, O_WRONLY, 0 )) {
			usleep( 10000 );
		}
		
		while (!osync_queue_connect( client_queue, O_RDONLY, 0 )) {
			usleep( 10000 );
		}
		
		//sleep(1);
		while (!(message = osync_queue_get_message(client_queue))) {
			usleep(10000);
		}	
		//handle_write( queue );
		if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
			exit (1);
		}
		
		
		osync_message_read_data(message, databuf2, bytes);
		
		long long int i = 0;
		for (i = 0; i < bytes; i++) {
			if (databuf2[i] != data[i]) {
				exit(1);
			}
		}
		
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		osync_queue_send_message(server_queue, NULL, reply, &error);
		
		sleep(1);
		exit( 0 );
	} else {
		while (!osync_queue_connect( server_queue, O_RDONLY, 0 )) {
			usleep( 10000 );
		}
		
		while (!osync_queue_connect( client_queue, O_WRONLY, 0 )) {
			usleep( 10000 );
		}
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_data(message, data, bytes);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		while (!(message = osync_queue_get_message(server_queue))) {
			usleep(100000);
		}
		
		//handle_read( queue );
		wait( NULL );
	}
	
	
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
}
END_TEST

Suite *ipc_suite(void)
{
	Suite *s = suite_create("IPC");
	Suite *s2 = suite_create("IPC");
	create_case(s, "ipc_new", ipc_new);
	create_case(s, "ipc_create", ipc_create);
	create_case(s, "ipc_connect", ipc_connect);
	create_case(s, "ipc_payload", ipc_payload);
	create_case(s2, "ipc_large_payload", ipc_large_payload);
	
	return s2;
}

int main(void)
{
	int nf;

	Suite *s = ipc_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
