#include <check.h>
#include "opensync.h"
#include "engine.h"
#include "engine_internals.h"

char *olddir = NULL;

char *setup_testbed(char *fkt_name)
{
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	char *command = g_strdup_printf("cp -a data/%s/* %s", fkt_name, testbed);
	if (system(command))
		abort();
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	g_free(command);
	printf("Seting up %s at %s\n", fkt_name, testbed);
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir)
		chdir(olddir);
	system(command);
	g_free(command);
	printf("Tearing down %s\n", path);
	g_free(path);
}

START_TEST (filter_setup)
{
	char *testbed = setup_testbed("filter_setup");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	OSyncFilter *filter = osync_filter_add(group, leftmember, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	fail_unless(filter != NULL, NULL);
	
	mark_point();
	fail_unless(osync_group_num_filters(group) == 1, NULL);
	fail_unless(osync_group_nth_filter(group, 0) == filter, NULL);
	
	mark_point();
	osync_filter_remove(group, filter);
	fail_unless(osync_group_num_filters(group) == 0, NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_deny_all)
{
	char *testbed = setup_testbed("filter_sync_deny_all");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	
	osync_filter_add(group, leftmember, NULL, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	osync_filter_add(group, NULL, leftmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	
	mark_point();
	OSyncError *error = NULL;
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	fail_unless(osync_engine_init(engine, &error), NULL);
	mark_point();
	fail_unless(osync_engine_synchronize(engine, &error), NULL);
	mark_point();
	
	osync_engine_wait_sync_end(engine);
	osync_engine_finalize(engine);

	system("ls data1");
	system("ls data2");

	fail_unless(!system("test \"x$(ls data1)\" = \"xtestdata\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"xtestdata2\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

Suite *filter_suite(void)
{
	Suite *s = suite_create("Filter");
	TCase *tc_filter = tcase_create("setup");
	//TCase *tc_filter2 = tcase_create("setup2");
	suite_add_tcase (s, tc_filter);
	tcase_add_test(tc_filter, filter_setup);
	tcase_add_test(tc_filter, filter_sync_deny_all);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = filter_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
