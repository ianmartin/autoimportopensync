#include "support.h"
#include <sys/wait.h>

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
	osync_error_free(&error);
	
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
	osync_error_free(&error);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") == 0, NULL);
		
	fail_unless(osync_queue_remove(queue, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(system("ls /tmp/testpipe &> /dev/null") != 0, NULL);

	osync_queue_free(queue);
	
	destroy_testbed(testbed);
}
END_TEST

void abort_unless(int i)
{
	if (!i)
		abort();
}

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
		abort_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error));
		abort_unless(error == NULL);
		
		g_free(testbed);
		exit(0);
	} else {
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		abort_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
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
		abort_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_SENDER, &error));
		abort_unless(error == NULL);
		
		abort_unless(_osync_queue_write_int(server_queue, 10000, &error));
		abort_unless(error == NULL);
		
		abort_unless(_osync_queue_write_int(server_queue, 0, &error));
		abort_unless(error == NULL);

		abort_unless(_osync_queue_write_long_long_int(server_queue, 0, &error));
		abort_unless(error == NULL);
			
		abort_unless(_osync_queue_write_int(server_queue, 0, &error));
		abort_unless(error == NULL);
		
		sleep(1);
		
		osync_queue_disconnect(server_queue, &error);
		fail_unless(error == NULL, NULL);
		
		osync_queue_free(server_queue);
		
		g_free(testbed);
		exit(0);
	} else {
		fail_unless(osync_queue_connect(server_queue, OSYNC_QUEUE_RECEIVER, &error), NULL);
		fail_unless(error == NULL, NULL);
		
		message = osync_queue_get_message(server_queue);
		abort_unless(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_ERROR);
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
	
	//Test main loop integration
	//callbacks
	//timeouts
	
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
