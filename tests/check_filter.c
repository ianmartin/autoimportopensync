#include "support.h"

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
	osync_filter_free(filter);

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

	fail_unless(!system("test \"x$(ls data1)\" = \"xtestdata\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"xtestdata2\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_custom)
{
	char *testbed = setup_testbed("filter_sync_custom");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncFilter *filter = osync_filter_add_custom(group, NULL, NULL, NULL, NULL, "contact", "vcard_categories_filter");
	osync_filter_set_config(filter, "test");
	
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

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_save_and_load)
{
	char *testbed = setup_testbed("filter_save_and_load");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	OSyncFilter *filter1 = osync_filter_add(group, leftmember, rightmember, "1", "2", "3", OSYNC_FILTER_DENY);
	OSyncFilter *filter2 = osync_filter_add(group, rightmember, leftmember, "4", "5", "6", OSYNC_FILTER_ALLOW);
	OSyncFilter *filter3 = osync_filter_add_custom(group, leftmember, rightmember, "7", "8", "9", "vcard_categories_filter");
	osync_filter_set_config(filter3, "test");
	
	fail_unless(osync_group_num_filters(group) == 3, NULL);
	fail_unless(osync_group_nth_filter(group, 0) == filter1, NULL);
	fail_unless(osync_group_nth_filter(group, 1) == filter2, NULL);
	fail_unless(osync_group_nth_filter(group, 2) == filter3, NULL);
	
	mark_point();
	osync_group_save(group, NULL);
	mark_point();
	osync_env_finalize(osync, NULL);
	osync_env_free(osync);
	osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();

	fail_unless(osync_group_num_filters(group) == 3, NULL);
	filter1 = osync_group_nth_filter(group, 0);
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter1->sourcememberid == 1, NULL);
	fail_unless(filter1->destmemberid == 2, NULL);
	fail_unless(!strcmp(filter1->sourceobjtype, "1"), NULL);
	fail_unless(!strcmp(filter1->destobjtype, "2"), NULL);
	fail_unless(!strcmp(filter1->detectobjtype, "3"), NULL);
	fail_unless(filter1->action == OSYNC_FILTER_DENY, NULL);
	
	filter1 = osync_group_nth_filter(group, 1);
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter1->sourcememberid == 2, NULL);
	fail_unless(filter1->destmemberid == 1, NULL);
	fail_unless(!strcmp(filter1->sourceobjtype, "4"), NULL);
	fail_unless(!strcmp(filter1->destobjtype, "5"), NULL);
	fail_unless(!strcmp(filter1->detectobjtype, "6"), NULL);
	fail_unless(filter1->action == OSYNC_FILTER_ALLOW, NULL);

	filter1 = osync_group_nth_filter(group, 2);
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter1->sourcememberid == 1, NULL);
	fail_unless(filter1->destmemberid == 2, NULL);
	fail_unless(!strcmp(filter1->sourceobjtype, "7"), NULL);
	fail_unless(!strcmp(filter1->destobjtype, "8"), NULL);
	fail_unless(!strcmp(filter1->detectobjtype, "9"), NULL);
	fail_unless(filter1->hook != NULL, NULL);
	fail_unless(!strcmp(osync_filter_get_config(filter1), "test"), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_vcard_only)
{
	char *testbed = setup_testbed("filter_sync_vcard_only");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	osync_filter_add(group, NULL, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	osync_filter_add(group, NULL, rightmember, NULL, NULL, "contact", OSYNC_FILTER_ALLOW);
	
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
	osync_env_finalize(osync, NULL);
	osync_env_free(osync);

	fail_unless(!system("test \"x$(ls data1/testdata)\" = \"xdata1/testdata\""), NULL);
	fail_unless(!system("test \"x$(ls data1/testdata2)\" = \"xdata1/testdata2\""), NULL);
	fail_unless(!system("test \"x$(ls data1/testdata3)\" = \"xdata1/testdata3\""), NULL);
	fail_unless(!system("test \"x$(ls data1/vcard.vcf)\" = \"xdata1/vcard.vcf\""), NULL);
	
	fail_unless(!system("test \"x$(ls data2/testdata3)\" = \"xdata2/testdata3\""), NULL);
	fail_unless(!system("test \"x$(ls data2/vcard.vcf)\" = \"xdata2/vcard.vcf\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

/*int num_read;

START_TEST (filter_sync_read_only)
{
	char *testbed = setup_testbed("filter_sync_deny_all");
	OSyncEnv *osync = osync_env_new();
	osync_env_initialize(osync, NULL);
	mark_point();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	
	osync_member_set_read_only(leftmember, "data", TRUE);
	
	num_read = 0;
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

	fail_unless(num_read == 1);

	fail_unless(!system("test \"x$(ls data1/testdata)\" = \"xdata1/testdata\""), NULL);
	fail_unless(!system("test \"x$(ls data1/testdata2)\" = \"xdata1/testdata2\""), NULL);
	fail_unless(!system("test \"x$(ls data2/testdata2)\" = \"xdata2/testdata2\""), NULL);

	destroy_testbed(testbed);
}
END_TEST*/

Suite *filter_suite(void)
{
	Suite *s = suite_create("Filter");
	TCase *tc_filter = tcase_create("setup");
	//TCase *tc_filter2 = tcase_create("setup2");
	suite_add_tcase (s, tc_filter);
	tcase_add_test(tc_filter, filter_setup);
	tcase_add_test(tc_filter, filter_sync_deny_all);
	tcase_add_test(tc_filter, filter_sync_custom);
	tcase_add_test(tc_filter, filter_save_and_load);
	tcase_add_test(tc_filter, filter_sync_vcard_only);
	
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
