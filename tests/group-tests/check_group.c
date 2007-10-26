#include "support.h"

#include <opensync/opensync.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync_internals.h>


START_TEST (group_last_sync)
{
	char *testbed = setup_testbed("filter_save_and_load");
	
	OSyncError *error = NULL;

	OSyncGroupEnv *group_env = osync_group_env_new(&error);	   
	fail_unless(error == NULL, NULL);


	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	
	osync_group_env_add_group(group_env, group, &error);
	fail_unless(error == NULL, osync_error_print(&error));

	fail_unless(osync_group_env_num_groups(group_env) == 1, NULL);
	mark_point();
	
	osync_group_set_last_synchronization(group, (time_t)1000);
	
	fail_unless((int)osync_group_get_last_synchronization(group) == 1000, NULL);
	
	fail_unless(osync_group_save(group, &error), NULL);
	
	osync_group_env_free(group_env);
	
	group_env = osync_group_env_new(&error);
	fail_unless(error == NULL, NULL);

	group = osync_group_new(&error);
	fail_unless(error == NULL, NULL);

	osync_group_load(group, "configs/group", &error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);

	osync_group_env_add_group(group_env, group, &error);
	fail_unless(error == NULL);

	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_group_env_num_groups(group_env) == 1, NULL);
	mark_point();
	
	fail_unless((int)osync_group_get_last_synchronization(group) == 1000, NULL);
	
	osync_group_env_free(group_env);
	destroy_testbed(testbed);
}
END_TEST

Suite *group_suite(void)
{
  Suite *s = suite_create("Group");

  create_case(s, "group_last_sync", group_last_sync);

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
