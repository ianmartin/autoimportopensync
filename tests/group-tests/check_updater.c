#include "support.h"

#include <opensync/opensync.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync_internals.h>

static int status_callback_calls = 0;

static void updater_reset_counters()
{
	status_callback_calls = 0;
}

void updater_status_cb(OSyncUpdater *updater, OSyncUpdaterStatus *status, void *userdata)
{
	status_callback_calls++;
}

START_TEST (updater_init)
{
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);


	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);
}
END_TEST

START_TEST (updater_without_loaded_group)
{
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	/* DON't load the group. Updater should fail since it doesn't
	   find any configuration data to backup/update. */

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_set_callback(updater, updater_status_cb);

	/* Intended to fail - no loaded group got passed by */
	/* TODO: move this sanity check to osync_updater_new() so it fails
	   there */
	fail_unless(!osync_updater_process_and_block(updater, &error), NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);
}
END_TEST

START_TEST (updater_action_required)
{
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	/* FIXME: Empty and unloaded group */
	fail_unless(osync_updater_action_required(updater) == TRUE, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);
}
END_TEST

START_TEST (updater_updates_directory)
{
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(!strcmp(osync_updater_get_updates_directory(updater), OPENSYNC_UPDATESDIR), NULL); 

	osync_updater_set_updates_directory(updater, g_get_tmp_dir());
	fail_unless(!strcmp(osync_updater_get_updates_directory(updater), g_get_tmp_dir()), NULL); 


	osync_updater_unref(updater);
	osync_group_unref(group);
}
END_TEST

START_TEST (updater_invalid_stylesheet)
{
	char *testbed = setup_testbed("updater_oldsetup");

	OSyncError *error = NULL;


	fail_unless(error == NULL, osync_error_print(&error));

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	/* Group1: invalid stylesheet */
	osync_group_load(group, "configs/group1", &error);
	fail_unless(error == NULL, NULL);

	osync_updater_set_updates_directory(updater, "configs/group1/");

	fail_unless(osync_updater_action_required(updater), NULL);

	/* Intended to fail. Invalid stylesheet */
	fail_unless(!osync_updater_process_and_block(updater, &error), NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (updater_valid_stylesheet)
{
	char *testbed = setup_testbed("updater_oldsetup");

	OSyncError *error = NULL;


	fail_unless(error == NULL, osync_error_print(&error));

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	osync_group_load(group, "configs/group1", &error);
	fail_unless(error == NULL, NULL);

	osync_updater_set_updates_directory(updater, "updates/");

	fail_unless(osync_updater_action_required(updater), NULL);

	fail_unless(osync_updater_process_and_block(updater, &error), NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (updater_group_three_steps)
{
	char *testbed = setup_testbed("updater_oldsetup");

	OSyncError *error = NULL;


	fail_unless(error == NULL, osync_error_print(&error));

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncUpdater *updater = osync_updater_new(group, &error);
	fail_unless(updater != NULL, NULL);
	fail_unless(error == NULL, NULL);

	/* Fake Group, Member and Plugin Configuration version to 3 */
	osync_updater_set_group_version(updater, 3);
	osync_updater_set_member_version(updater, 3);
	osync_updater_set_plugin_version(updater, 3);

	osync_group_load(group, "configs/group1", &error);
	fail_unless(error == NULL, NULL);

	osync_updater_set_updates_directory(updater, "updates/");

	fail_unless(osync_updater_action_required(updater), NULL);

	fail_unless(osync_updater_process_and_block(updater, &error), NULL);
	fail_unless(error == NULL, NULL);

	osync_updater_unref(updater);
	osync_group_unref(group);

	char *buf;
	ssize_t size;
	GError *gerror = NULL;

	g_file_get_contents("configs/group1/syncgroup.conf", &buf, &size, &gerror);
	fail_unless(gerror == NULL, NULL);
	fail_unless(g_pattern_match_simple("<?xml version=\"*\"?>*<versionthree>0</versionthree>*", buf), "Buffer: %s", buf);
	g_free(buf);

	g_file_get_contents("configs/group1/1/mock-sync.conf", &buf, &size, &gerror);
	fail_unless(gerror == NULL, NULL);
	fail_unless(g_pattern_match_simple("<?xml version=\"*\"?>*<versionthree/></directory>*", buf), "Buffer: %s", buf);
	g_free(buf);

	g_file_get_contents("configs/group1/2/mock-sync.conf", &buf, &size, &gerror);
	fail_unless(gerror == NULL, NULL);
	fail_unless(g_pattern_match_simple("<?xml version=\"*\"?>*<versionthree/></directory>*", buf), "Buffer: %s", buf);
	g_free(buf);

	g_file_get_contents("configs/group1/1/syncmember.conf", &buf, &size, &gerror);
	fail_unless(gerror == NULL, NULL);
	fail_unless(g_pattern_match_simple("<?xml version=\"*\"?>*<versionthree/></name>*", buf), "Buffer: %s", buf);
	g_free(buf);

	g_file_get_contents("configs/group1/2/syncmember.conf", &buf, &size, &gerror);
	fail_unless(gerror == NULL, NULL);
	fail_unless(g_pattern_match_simple("<?xml version=\"*\"?>*<versionthree/></name>*", buf), "Buffer: %s", buf);
	g_free(buf);

	destroy_testbed(testbed);
}
END_TEST

Suite *group_suite(void)
{
  Suite *s = suite_create("Updater");
  //Suite *s2 = suite_create("Updater");

  create_case(s, "updater_init", updater_init);
  create_case(s, "updater_without_loaded_group", updater_without_loaded_group);
  create_case(s, "updater_action_required", updater_action_required);
  create_case(s, "updater_updates_directory", updater_updates_directory);
  create_case(s, "updater_invalid_stylesheet", updater_invalid_stylesheet);

  create_case(s, "updater_valid_stylesheet", updater_valid_stylesheet);
  create_case(s, "updater_group_three_steps", updater_group_three_steps);

  return s;
}

int main(void)
{
	int nf;
	
	Suite *s = group_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
