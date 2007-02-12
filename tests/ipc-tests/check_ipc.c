#include "support.h"
#include <sys/wait.h>

#include <opensync/opensync-ipc.h>

void _remove_pipe(const char *name)
{
	char *cmd = g_strdup_printf("rm %s &> /dev/null", name);
	system(cmd);
	g_free(cmd);
}

START_TEST (ipc_new)
{
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *queue1 = osync_queue_new("/tmp/testpipe", &error);
	fail_unless(queue1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_queue_free(queue1);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_create)
{
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *queue1 = osync_queue_new("/tmp/testpipe", &error);
	fail_unless(queue1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_queue_create(queue1, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(queue1, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);
	
	osync_queue_free(queue1);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_connect)
{
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *queue = osync_queue_new("/tmp/testpipe", &error);
	
	osync_queue_create(queue, &error);
	fail_unless(error == NULL, NULL);
		
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		fail_unless(osync_queue_connect(queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		if (osync_queue_disconnect(queue, &error) != TRUE || error != NULL)
			exit(1);
		
		osync_queue_free(queue);
	
		g_free(testbed);
		exit(0);
	} else {
		fail_unless(osync_queue_connect(queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		osync_queue_disconnect(queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
		
	}
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
		
	fail_unless(osync_queue_remove(queue, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);

	osync_queue_free(queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_payload)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(client_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
			exit (1);
		}
		
		int int1;
		long long int longint1;
		char *string;
		void *databuf;
		
		osync_message_read_int(message, &int1);
		osync_message_read_const_string(message, &string);
		osync_message_read_long_long_int(message, &longint1);
		osync_message_read_const_data(message, &databuf, strlen(data) + 1);
		
		fail_unless(int1 == 4000000, NULL);
		fail_unless(!strcmp(string, "this is a test string"), NULL);
		fail_unless(longint1 == 400000000, NULL);
		fail_unless(!strcmp(databuf, "this is another test string"), NULL);
		
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		
		osync_message_unref(message);
		
		if (osync_queue_disconnect(client_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(client_queue);
		
		osync_queue_send_message(server_queue, NULL, reply, &error);
		osync_message_unref(reply);
		
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
	
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		while (!(message = osync_queue_get_message(server_queue))) {
			usleep(100000);
		}
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		while (!(message = osync_queue_get_message(client_queue))) {
			usleep(10000);
		}
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_payload_wait)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		sleep(1);
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		while (!(message = osync_queue_get_message(client_queue))) {
			usleep(10000);
		}
		
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
		
		fail_unless(int1 == 4000000, NULL);
		fail_unless(!strcmp(string, "this is a test string"), NULL);
		fail_unless(longint1 == 400000000, NULL);
		fail_unless(!strcmp(databuf, "this is another test string"), NULL);
		
		sleep(1);
		
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		
		osync_message_unref(message);
		
		osync_queue_send_message(server_queue, NULL, reply, &error);
		
		osync_message_unref(reply);
		
		sleep(1);
		
		if (osync_queue_disconnect(client_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(client_queue);
	
		while (!(message = osync_queue_get_message(server_queue))) {
			usleep(10000);
		}
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
	
		osync_message_unref(message);
		sleep(1);
		
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		while (!(message = osync_queue_get_message(server_queue))) {
			usleep(100000);
		}
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		while (!(message = osync_queue_get_message(client_queue))) {
			usleep(10000);
		}
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_payload_stress)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	int num_mess = 1000;
	int size = 100;
	
	char *data = malloc(size);
	memset(data, 42, size);
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		while (num_mess > 0) {
			osync_trace(TRACE_INTERNAL, "Waiting for message");
			message = osync_queue_get_message(client_queue);
			
			if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
				exit (1);
			}
			
			osync_trace(TRACE_INTERNAL, "Parsing message");
			char databuf[size];
				
			osync_message_read_data(message, databuf, size);
			
			fail_unless(!memcmp(databuf, data, size), NULL);
			
			osync_trace(TRACE_INTERNAL, "Creating new reply");
			OSyncMessage *reply = osync_message_new_reply(message, &error);
			
			osync_message_unref(message);
			
			osync_trace(TRACE_INTERNAL, "Sending reply");
			osync_queue_send_message(server_queue, NULL, reply, &error);
			
			osync_message_unref(reply);
			
			num_mess--;
		}
		
		if (osync_queue_disconnect(client_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(client_queue);
	
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
	
		osync_message_unref(message);
			
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		g_free(data);
		g_free(testbed);
		
		exit(0);
	} else {
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		while (num_mess > 0) {
			osync_trace(TRACE_INTERNAL, "Creating new message");
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_data(message, data, size);
			
			osync_trace(TRACE_INTERNAL, "Sending message");
			fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_unref(message);
			
			osync_trace(TRACE_INTERNAL, "Waiting for message");
			message = osync_queue_get_message(server_queue);
			
			fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
			
			osync_message_unref(message);
			
			num_mess--;
		}
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(client_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
			
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	g_free(data);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_payload_stress2)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	int i = 0;
	
	int num_mess = 1000;
	int size = 100;
	
	char *data = malloc(size);
	memset(data, 42, size);
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		for (i = 0; i < num_mess; i++) {
			message = osync_queue_get_message(client_queue);
			
			if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
				exit (1);
			}
			
			char databuf[size];
				
			osync_message_read_data(message, databuf, size);
			
			fail_unless(!memcmp(databuf, data, size), NULL);
			
			osync_message_unref(message);
		}
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			
		for (i = 0; i < num_mess; i++) {
			OSyncMessage *reply = osync_message_new_reply(message, &error);
			
			osync_queue_send_message(server_queue, NULL, reply, &error);
			
			osync_message_unref(reply);
		}
		
		osync_message_unref(message);
		
		if (osync_queue_disconnect(client_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(client_queue);
	
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
	
		osync_message_unref(message);
			
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		g_free(data);
		g_free(testbed);
		
		exit(0);
	} else {
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		for (i = 0; i < num_mess; i++) {
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_data(message, data, size);
			
			fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_unref(message);
		}
		
		for (i = 0; i < num_mess; i++) {
			message = osync_queue_get_message(server_queue);
			
			fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
			
			osync_message_unref(message);
		}
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(client_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	g_free(data);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_large_payload)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	int i = 0;
	
	int num_mess = 10;
	int size = 1024 * 1024 * 20; //20mbyte
	
	char *data = malloc(size);
	memset(data, 42, size);
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	OSyncQueue *client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		for (i = 0; i < num_mess; i++) {
			message = osync_queue_get_message(client_queue);
			
			if (osync_message_get_command(message) != OSYNC_MESSAGE_INITIALIZE) {
				exit (1);
			}
			
			void *databuf = NULL;
			osync_message_read_const_data(message, &databuf, size);
		
			if (memcmp(databuf, data, size))
				exit(1);
			
			OSyncMessage *reply = osync_message_new_reply(message, &error);
			
			osync_message_unref(message);
			
			osync_queue_send_message(server_queue, NULL, reply, &error);
			
			osync_message_unref(reply);
		}
		
		if (osync_queue_disconnect(client_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(client_queue);
	
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
			
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		g_free(data);
		
		g_free(testbed);
		
		exit(0);
	} else {
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		for (i = 0; i < num_mess; i++) {
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_data(message, data, size);
			
			fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);

			osync_message_unref(message);

			message = osync_queue_get_message(server_queue);
			
			fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
			
			osync_message_unref(message);
		}
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(client_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
			
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	g_free(data);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_error_no_pipe)
{
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *queue1 = osync_queue_new("/tmp/testpipe", &error);
	fail_unless(queue1 != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(!osync_queue_connect(queue1, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	osync_queue_free(queue1);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_error_perm)
{
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *queue = osync_queue_new("/tmp/testpipe", &error);
	
	osync_queue_create(queue, &error);
	fail_unless(error == NULL, NULL);
	
	if (system("chmod 000 /tmp/testpipe"))
		abort();
	
	fail_unless(!osync_queue_connect(queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error != NULL, NULL);
	osync_error_unref(&error);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
		
	fail_unless(osync_queue_remove(queue, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);

	osync_queue_free(queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_error_rem)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		osync_assert(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error));
		osync_assert(error == NULL);
		
		g_free(testbed);
		exit(0);
	} else {
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		osync_message_unref(message);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);

	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_error_rem2)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe");
	
	OSyncError *error = NULL;
	OSyncQueue *server_queue = osync_queue_new("/tmp/testpipe", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		osync_assert(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error));
		osync_assert(error == NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		
		fail_unless(osync_queue_send_message(server_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		sleep(2);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_queue_free(server_queue);
		
		g_free(testbed);
		exit(0);
	} else {
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_ERROR);
		osync_message_unref(message);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);

	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

OSyncQueue *server_queue = NULL;
OSyncQueue *client_queue = NULL;

void server_handler1(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	
	fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	osync_queue_disconnect(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void client_handler1(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	int int1;
	long long int longint1;
	char *string;
	void *databuf;
	
	osync_message_read_int(message, &int1);
	osync_message_read_const_string(message, &string);
	osync_message_read_long_long_int(message, &longint1);
	osync_message_read_const_data(message, &databuf, strlen("this is another test string") + 1);
	
	fail_unless(int1 == 4000000, NULL);
	fail_unless(!strcmp(string, "this is a test string"), NULL);
	fail_unless(longint1 == 400000000, NULL);
	fail_unless(!strcmp(databuf, "this is another test string"), NULL);
	
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
	
	osync_queue_send_message(server_queue, NULL, reply, &error);
	
	osync_message_unref(reply);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

START_TEST (ipc_loop_payload)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	GMainContext *context = g_main_context_new();
	OSyncThread *thread = osync_thread_new(context, &error);
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		osync_queue_set_message_handler(client_queue, client_handler1, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
	
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		osync_assert(osync_queue_disconnect(client_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		osync_queue_set_message_handler(server_queue, server_handler1, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		message = osync_queue_get_message(client_queue);
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

int num_msgs = 0;
int req_msgs = 1000;

void server_handler2(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	char *data = "this is another test string";
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	
	num_msgs++;
	fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
		
	if (num_msgs >= req_msgs) {
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
	} else {
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void client_handler2(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	int int1;
	long long int longint1;
	char *string;
	void *databuf;
	
	osync_message_read_int(message, &int1);
	osync_message_read_const_string(message, &string);
	osync_message_read_long_long_int(message, &longint1);
	osync_message_read_const_data(message, &databuf, strlen("this is another test string") + 1);
	
	fail_unless(int1 == 4000000, NULL);
	fail_unless(!strcmp(string, "this is a test string"), NULL);
	fail_unless(longint1 == 400000000, NULL);
	fail_unless(!strcmp(databuf, "this is another test string"), NULL);
	
	OSyncMessage *reply = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
	
	osync_queue_send_message(server_queue, NULL, reply, &error);
	
	osync_message_unref(reply);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

START_TEST (ipc_loop_stress)
{	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
	
		osync_queue_set_message_handler(client_queue, client_handler2, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
		
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
	
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		osync_assert(osync_queue_disconnect(client_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
		
		osync_queue_set_message_handler(server_queue, server_handler2, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		message = osync_queue_get_message(client_queue);
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

void callback_handler(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) == 1);
	
	num_msgs++;
	fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
	
	if (num_msgs >= req_msgs) {
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void server_handler3(OSyncMessage *message, void *user_data)
{
	abort();
}

void client_handler3(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	int int1;
	long long int longint1;
	char *string;
	void *databuf;
	
	osync_message_read_int(message, &int1);
	osync_message_read_const_string(message, &string);
	osync_message_read_long_long_int(message, &longint1);
	osync_message_read_const_data(message, &databuf, strlen("this is another test string") + 1);
	
	fail_unless(int1 == 4000000, NULL);
	fail_unless(!strcmp(string, "this is a test string"), NULL);
	fail_unless(longint1 == 400000000, NULL);
	fail_unless(!strcmp(databuf, "this is another test string"), NULL);
	
	OSyncMessage *reply = osync_message_new_reply(message, &error);
	
	osync_queue_send_message(server_queue, NULL, reply, &error);
	
	osync_message_unref(reply);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

START_TEST (ipc_loop_callback)
{	
	num_msgs = 0;
	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
	
		osync_queue_set_message_handler(client_queue, client_handler3, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
		
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		
		osync_message_unref(message);
	
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		osync_assert(osync_queue_disconnect(client_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
		
		osync_queue_set_message_handler(server_queue, server_handler3, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		int i = 0;
		for (i = 0; i < req_msgs; i++) {
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_int(message, 4000000);
			osync_message_write_string(message, "this is a test string");
			osync_message_write_long_long_int(message, 400000000);
			osync_message_write_data(message, data, strlen(data) + 1);
			
			osync_message_set_handler(message, callback_handler, GINT_TO_POINTER(1));
			
			fail_unless(osync_queue_send_message(client_queue, server_queue, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_unref(message);
		}
		
		message = osync_queue_get_message(client_queue);
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

int stop_after = 500;

void callback_handler2(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	
	osync_assert(GPOINTER_TO_INT(user_data) == 1);
	
	if (num_msgs >= stop_after) {
		osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_ERRORREPLY);
	} else {
		osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_REPLY);
	}
	
	num_msgs++;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

int num_msgs2 = 0;

void server_handler4(OSyncMessage *message, void *user_data)
{
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP || osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_ERROR);
}

void client_handler4(OSyncMessage *message, void *user_data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, message, user_data);
	OSyncError *error = NULL;
	
	osync_assert(GPOINTER_TO_INT(user_data) ==1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	int int1;
	long long int longint1;
	char *string;
	void *databuf;
	
	
	osync_message_read_int(message, &int1);
	osync_message_read_const_string(message, &string);
	osync_message_read_long_long_int(message, &longint1);
	osync_message_read_const_data(message, &databuf, strlen("this is another test string") + 1);
	
	fail_unless(int1 == 4000000, NULL);
	fail_unless(!strcmp(string, "this is a test string"), NULL);
	fail_unless(longint1 == 400000000, NULL);
	fail_unless(!strcmp(databuf, "this is another test string"), NULL);
	
	if (num_msgs2 >= stop_after) {
		osync_assert(osync_queue_disconnect(client_queue, &error));
		osync_assert(error == NULL);
	} else {
		OSyncMessage *reply = osync_message_new_reply(message, &error);
		
		osync_queue_send_message(server_queue, NULL, reply, &error);
		
		osync_message_unref(reply);
	}
	
	num_msgs2++;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

START_TEST (ipc_callback_break)
{	
	num_msgs = 0;
	
	char *testbed = setup_testbed(NULL);
	_remove_pipe("/tmp/testpipe-server");
	_remove_pipe("/tmp/testpipe-client");
	
	OSyncError *error = NULL;
	server_queue = osync_queue_new("/tmp/testpipe-server", &error);
	client_queue = osync_queue_new("/tmp/testpipe-client", &error);
	OSyncMessage *message = NULL;
	
	osync_queue_create(server_queue, &error);
	fail_unless(error == NULL, NULL);
	
	osync_queue_create(client_queue, &error);
	fail_unless(error == NULL, NULL);
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
	
		osync_queue_set_message_handler(client_queue, client_handler4, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
		
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		while (osync_queue_is_connected(client_queue)) { usleep(100); }
		
		osync_assert(osync_queue_disconnect(server_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		osync_queue_free(server_queue);
		
		g_free(testbed);
		exit(0);
	} else {
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
		
		osync_queue_set_message_handler(server_queue, server_handler4, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		int i = 0;
		for (i = 0; i < req_msgs; i++) {
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_int(message, 4000000);
			osync_message_write_string(message, "this is a test string");
			osync_message_write_long_long_int(message, 400000000);
			osync_message_write_data(message, data, strlen(data) + 1);
			
			osync_message_set_handler(message, callback_handler2, GINT_TO_POINTER(1));
			
			fail_unless(osync_queue_send_message(client_queue, server_queue, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_unref(message);
		}
		
		message = osync_queue_get_message(client_queue);
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		while (num_msgs < req_msgs) { usleep(100); };
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_queue_disconnect(server_queue, &error);
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") == 0, NULL);
	
	fail_unless(osync_queue_remove(client_queue, &error), NULL);
	fail_unless(osync_queue_remove(server_queue, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(system("ls /tmp/testpipe-client &> /dev/null") != 0, NULL);

	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST


START_TEST (ipc_pipes)
{	
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncQueue *read1 = NULL;
	OSyncQueue *write1 = NULL;
	char *data = "this is another test string";
	
	osync_assert(osync_queue_new_pipes(&read1, &write1, &error));
	osync_assert(error == NULL);
		
	fail_unless(osync_queue_connect(read1, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	fail_unless(osync_queue_connect(write1, OSYNC_QUEUE_SENDER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	osync_message_write_int(message, 4000000);
	osync_message_write_string(message, "this is a test string");
	osync_message_write_long_long_int(message, 400000000);
	osync_message_write_data(message, data, strlen(data) + 1);
	
	fail_unless(osync_queue_send_message(write1, NULL, message, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	osync_message_unref(message);
		
	message = osync_queue_get_message(read1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_INITIALIZE);
	
	int int1;
	long long int longint1;
	char *string;
	void *databuf;
	
	osync_message_read_int(message, &int1);
	osync_message_read_const_string(message, &string);
	osync_message_read_long_long_int(message, &longint1);
	osync_message_read_const_data(message, &databuf, strlen("this is another test string") + 1);
	
	fail_unless(int1 == 4000000, NULL);
	fail_unless(!strcmp(string, "this is a test string"), NULL);
	fail_unless(longint1 == 400000000, NULL);
	fail_unless(!strcmp(databuf, "this is another test string"), NULL);
		
	osync_message_unref(message);
		
	osync_assert(osync_queue_disconnect(read1, &error));
	osync_assert(error == NULL);
	
	message = osync_queue_get_message(write1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
	osync_message_unref(message);

	osync_assert(osync_queue_disconnect(write1, &error));
	osync_assert(error == NULL);
	
	
	osync_queue_free(read1);
	osync_queue_free(write1);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_pipes_stress)
{	
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncQueue *read1 = NULL;
	OSyncQueue *read2 = NULL;
	OSyncQueue *write1 = NULL;
	OSyncQueue *write2 = NULL;
	
	
	// First the pipe from the parent to the child
	osync_assert(osync_queue_new_pipes(&read1, &write1, &error));
	osync_assert(error == NULL);
	
	// Then the pipe from the child to the parent
	osync_assert(osync_queue_new_pipes(&read2, &write2, &error));
	osync_assert(error == NULL);
	
	OSyncMessage *message = NULL;
	
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		
		osync_assert(osync_queue_disconnect(write1, &error));
		osync_queue_free(write1);
		
		osync_assert(osync_queue_disconnect(read2, &error));
		osync_queue_free(read2);
		
		client_queue = read1;
		server_queue = write2;
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
	
		osync_queue_set_message_handler(client_queue, client_handler2, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
		
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		if (osync_message_get_command(message) != OSYNC_MESSAGE_QUEUE_HUP) {
			exit (1);
		}
		osync_message_unref(message);
	
		
		if (osync_queue_disconnect(server_queue, &error) != TRUE || error != NULL)
			exit(1);
		osync_queue_free(server_queue);
		
		osync_assert(osync_queue_disconnect(client_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		
		g_free(testbed);
		
		exit(0);
	} else {
		
		osync_assert(osync_queue_disconnect(write2, &error));
		osync_queue_free(write2);
		
		osync_assert(osync_queue_disconnect(read1, &error));
		osync_queue_free(read1);
		
		client_queue = write1;
		server_queue = read2;
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
		
		osync_queue_set_message_handler(server_queue, server_handler2, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
		fail_unless(message != NULL, NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_write_int(message, 4000000);
		osync_message_write_string(message, "this is a test string");
		osync_message_write_long_long_int(message, 400000000);
		osync_message_write_data(message, data, strlen(data) + 1);
		
		fail_unless(osync_queue_send_message(client_queue, NULL, message, &error), NULL);
		fail_unless(!osync_error_is_set(&error), NULL);
		
		osync_message_unref(message);
		
		message = osync_queue_get_message(client_queue);
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (ipc_callback_break_pipes)
{	
	num_msgs = 0;
	
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncQueue *read1 = NULL;
	OSyncQueue *read2 = NULL;
	OSyncQueue *write1 = NULL;
	OSyncQueue *write2 = NULL;
	OSyncMessage *message = NULL;
	
	// First the pipe from the parent to the child
	osync_assert(osync_queue_new_pipes(&read1, &write1, &error));
	osync_assert(error == NULL);
	
	// Then the pipe from the child to the parent
	osync_assert(osync_queue_new_pipes(&read2, &write2, &error));
	osync_assert(error == NULL);
	
	char *data = "this is another test string";
	
	pid_t cpid = fork();
	if (cpid == 0) { //Child
		
		osync_assert(osync_queue_disconnect(write1, &error));
		osync_queue_free(write1);
		
		osync_assert(osync_queue_disconnect(read2, &error));
		osync_queue_free(read2);
		
		client_queue = read1;
		server_queue = write2;
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
	
		osync_queue_set_message_handler(client_queue, client_handler4, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(client_queue, context);
		
		osync_thread_start(thread);
		
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		while (osync_queue_is_connected(client_queue)) { usleep(100); }
		
		osync_assert(osync_queue_disconnect(server_queue, &error));
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		osync_queue_free(client_queue);
		osync_queue_free(server_queue);
		
		g_free(testbed);
		exit(0);
	} else {
		
		osync_assert(osync_queue_disconnect(write2, &error));
		osync_queue_free(write2);
		
		osync_assert(osync_queue_disconnect(read1, &error));
		osync_queue_free(read1);
		
		client_queue = write1;
		server_queue = read2;
		
		GMainContext *context = g_main_context_new();
		OSyncThread *thread = osync_thread_new(context, &error);
		
		osync_queue_set_message_handler(server_queue, server_handler4, GINT_TO_POINTER(1));
		
		osync_queue_setup_with_gmainloop(server_queue, context);
		
		osync_thread_start(thread);
	
		fail_unless(osync_queue_connect(client_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		int i = 0;
		for (i = 0; i < req_msgs; i++) {
			message = osync_message_new(OSYNC_MESSAGE_INITIALIZE, 0, &error);
			fail_unless(message != NULL, NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_write_int(message, 4000000);
			osync_message_write_string(message, "this is a test string");
			osync_message_write_long_long_int(message, 400000000);
			osync_message_write_data(message, data, strlen(data) + 1);
			
			osync_message_set_handler(message, callback_handler2, GINT_TO_POINTER(1));
			
			fail_unless(osync_queue_send_message(client_queue, server_queue, message, &error), NULL);
			fail_unless(!osync_error_is_set(&error), NULL);
			
			osync_message_unref(message);
		}
		
		message = osync_queue_get_message(client_queue);
		
		fail_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
		
		osync_message_unref(message);
		
		while (num_msgs < req_msgs) { usleep(100); };
		
		osync_queue_disconnect(client_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_queue_disconnect(server_queue, &error);
		osync_assert(error == NULL);
		
		osync_thread_stop(thread);
		osync_thread_free(thread);
		
		int status = 0;
		wait(&status);
		fail_unless(WEXITSTATUS(status) == 0, NULL);
	}
	
	osync_queue_free(client_queue);
	osync_queue_free(server_queue);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *ipc_suite(void)
{
	Suite *s = suite_create("IPC");
	//Suite *s2 = suite_create("IPC");
	
	create_case(s, "ipc_new", ipc_new);
	create_case(s, "ipc_create", ipc_create);
	create_case(s, "ipc_connect", ipc_connect);
	create_case(s, "ipc_payload", ipc_payload);
	create_case(s, "ipc_payload_wait", ipc_payload_wait);
	create_case(s, "ipc_payload_stress", ipc_payload_stress);
	create_case(s, "ipc_payload_stress2", ipc_payload_stress2);
	create_case(s, "ipc_large_payload", ipc_large_payload);
	
	create_case(s, "ipc_error_no_pipe", ipc_error_no_pipe);
	create_case(s, "ipc_error_perm", ipc_error_perm);
	create_case(s, "ipc_error_rem", ipc_error_rem);
	create_case(s, "ipc_error_rem2", ipc_error_rem2);
	
	create_case(s, "ipc_loop_payload", ipc_loop_payload);
	create_case(s, "ipc_loop_stress", ipc_loop_stress);
	create_case(s, "ipc_loop_callback", ipc_loop_callback);
	create_case(s, "ipc_callback_break", ipc_callback_break);
	
	create_case(s, "ipc_pipes", ipc_pipes);
	create_case(s, "ipc_pipes_stress", ipc_pipes_stress);
	create_case(s, "ipc_callback_break_pipes", ipc_callback_break_pipes);
	
	return s;
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
