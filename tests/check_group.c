#include "support.h"

START_TEST (group_last_sync)
{
	char *testbed = setup_testbed("filter_save_and_load");
	
	OSyncEnv *env = init_env();
	OSyncGroup *group = osync_group_load(env, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(env) == 1, NULL);
	mark_point();
	
	osync_group_set_last_synchronization(group, (time_t)1000);
	
	fail_unless((int)osync_group_get_last_synchronization(group) == 1000, NULL);
	
	OSyncError *error = NULL;
	fail_unless(osync_group_save(group, &error), NULL);
	
	fail_unless(osync_env_finalize(env, NULL), NULL);
	osync_env_free(env);
	
	env = init_env();
	group = osync_group_load(env, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	fail_unless(osync_env_num_groups(env) == 1, NULL);
	mark_point();
	
	fail_unless((int)osync_group_get_last_synchronization(group) == 1000, NULL);
	
	fail_unless(osync_env_finalize(env, NULL), NULL);
	osync_env_free(env);
	destroy_testbed(testbed);
}
END_TEST

Suite *group_suite(void)
{
  Suite *s = suite_create("Group");
  TCase *tc_core = tcase_create("Core");

  suite_add_tcase (s, tc_core);
  tcase_add_test(tc_core, group_last_sync);
  
  return s;
}

int main(void)
{
	int nf;
	
	Suite *s = group_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
