#include "support.h"
#include <sys/wait.h>

#include <opensync/opensync-ipc.h>
#include "opensync/ipc/opensync_queue_internals.h"

#include <opensync/opensync-client.h>
#include "opensync/client/opensync_client_internals.h"

START_TEST (client_new)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncClient *client = osync_client_new(&error);
	fail_unless(client != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_ref(client);
	osync_client_unref(client);
	osync_client_unref(client);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (client_pipes)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncQueue *read1 = NULL;
	OSyncQueue *write1 = NULL;
	OSyncQueue *read2 = NULL;
	OSyncQueue *write2 = NULL;
	OSyncClient *client = osync_client_new(&error);
	fail_unless(client != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_assert(osync_queue_new_pipes(&read1, &write1, &error));
	osync_assert(error == NULL);
		
	fail_unless(osync_queue_connect(read1, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	fail_unless(osync_queue_connect(write1, OSYNC_QUEUE_SENDER, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_assert(osync_queue_new_pipes(&read2, &write2, &error));
	osync_assert(error == NULL);
		
	fail_unless(osync_queue_connect(read2, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	fail_unless(osync_queue_connect(write2, OSYNC_QUEUE_SENDER, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_set_incoming_queue(client, read1);
	osync_client_set_outgoing_queue(client, write2);

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NOOP, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(osync_queue_send_message(write1, NULL, message, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	osync_message_unref(message);
	
	osync_assert(osync_queue_disconnect(read1, &error));
	osync_assert(error == NULL);
	
	message = osync_queue_get_message(write1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
	osync_message_unref(message);

	osync_assert(osync_queue_disconnect(write1, &error));
	osync_assert(error == NULL);
	
	
	
	
	osync_assert(osync_queue_disconnect(read2, &error));
	osync_assert(error == NULL);
	
	message = osync_queue_get_message(write2);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
	osync_message_unref(message);

	osync_assert(osync_queue_disconnect(write2, &error));
	osync_assert(error == NULL);
	
	osync_queue_free(read2);
	osync_queue_free(write1);
	
	osync_client_unref(client);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (client_run)
{
	char *testbed = setup_testbed(NULL);
	
	OSyncError *error = NULL;
	OSyncQueue *read1 = NULL;
	OSyncQueue *write1 = NULL;
	OSyncQueue *read2 = NULL;
	OSyncQueue *write2 = NULL;
	OSyncClient *client = osync_client_new(&error);
	fail_unless(client != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_assert(osync_queue_new_pipes(&read1, &write1, &error));
	osync_assert(error == NULL);
		
	fail_unless(osync_queue_connect(read1, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	fail_unless(osync_queue_connect(write1, OSYNC_QUEUE_SENDER, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_assert(osync_queue_new_pipes(&read2, &write2, &error));
	osync_assert(error == NULL);
		
	fail_unless(osync_queue_connect(read2, OSYNC_QUEUE_RECEIVER, &error), NULL);
	fail_unless(error == NULL, NULL);
		
	fail_unless(osync_queue_connect(write2, OSYNC_QUEUE_SENDER, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_client_set_incoming_queue(client, read1);
	osync_client_set_outgoing_queue(client, write2);

	OSyncMessage *message = osync_message_new(OSYNC_MESSAGE_NOOP, 0, &error);
	fail_unless(message != NULL, NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	
	fail_unless(osync_queue_send_message(write1, NULL, message, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	osync_message_unref(message);
	
	OSyncError *locerror = NULL;
	osync_error_set(&locerror, OSYNC_ERROR_GENERIC, "test");
	fail_unless(osync_client_run(client, &error), NULL);
	fail_unless(!osync_error_is_set(&error), NULL);
	osync_client_error_shutdown(client, locerror);
	osync_error_unref(&locerror);
	
	message = osync_queue_get_message(read2);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_ERROR);
	osync_message_unref(message);
	
	osync_assert(osync_queue_disconnect(read1, &error));
	osync_assert(error == NULL);
	
	message = osync_queue_get_message(write1);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
	osync_message_unref(message);

	osync_assert(osync_queue_disconnect(write1, &error));
	osync_assert(error == NULL);
	
	
	
	
	osync_assert(osync_queue_disconnect(read2, &error));
	osync_assert(error == NULL);
	
	message = osync_queue_get_message(write2);
	osync_assert(osync_message_get_command(message) == OSYNC_MESSAGE_QUEUE_HUP);
	osync_message_unref(message);

	osync_assert(osync_queue_disconnect(write2, &error));
	osync_assert(error == NULL);
	
	osync_queue_free(read2);
	osync_queue_free(write1);
	
	osync_client_unref(client);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *client_suite(void)
{
	Suite *s = suite_create("Client");
//	Suite *s2 = suite_create("Client");
	
	create_case(s, "client_new", client_new);
	create_case(s, "client_pipes", client_pipes);
	create_case(s, "client_run", client_run);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = client_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
