#include "support.h"

START_TEST (filter_setup)
{
	char *testbed = setup_testbed("filter_setup");
	OSyncEnv *osync = init_env();
	
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

START_TEST (filter_flush)
{
	char *testbed = setup_testbed("filter_setup");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	OSyncFilter *filter1 = osync_filter_add(group, leftmember, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	OSyncFilter *filter2 = osync_filter_add(group, leftmember, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter2 != NULL, NULL);
	
	mark_point();
	fail_unless(osync_group_num_filters(group) == 2, NULL);
	fail_unless(osync_group_nth_filter(group, 0) == filter1, NULL);
	fail_unless(osync_group_nth_filter(group, 1) == filter2, NULL);
	
	mark_point();
	osync_group_flush_filters(group);
	fail_unless(osync_group_num_filters(group) == 0, NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_deny_all)
{
	char *testbed = setup_testbed("filter_sync_deny_all");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	
	osync_filter_add(group, leftmember, NULL, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	osync_filter_add(group, NULL, leftmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	
	mark_point();
	OSyncError *error = NULL;
  	OSyncEngine *engine = osengine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osengine_finalize(engine);

	fail_unless(!system("test \"x$(ls data1)\" = \"xtestdata\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"xtestdata2\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_custom)
{
	char *testbed = setup_testbed("filter_sync_custom");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncFilter *filter = osync_filter_add_custom(group, NULL, NULL, NULL, NULL, "contact", "vcard_categories_filter");
	osync_filter_set_config(filter, "test");
	
	mark_point();
	OSyncError *error = NULL;
  	OSyncEngine *engine = osengine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osengine_finalize(engine);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

static OSyncFilterAction vcard_cats(OSyncChange *change, char *config)
{
	//Check what categories are supported here.
	return OSYNC_FILTER_IGNORE;
}

START_TEST (filter_save_and_load)
{
	char *testbed = setup_testbed("filter_save_and_load");
	OSyncEnv *osync = init_env();
	osync_env_register_objtype(osync, "test");
	osync_env_register_objformat(osync, "test", "format1");
	osync_env_register_filter_function(osync, "vcard_cats", "test", "format1", vcard_cats);
	
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	OSyncFilter *filter1 = osync_filter_add(group, leftmember, rightmember, "1", "2", "3", OSYNC_FILTER_DENY);
	OSyncFilter *filter2 = osync_filter_add(group, rightmember, leftmember, "4", "5", "6", OSYNC_FILTER_ALLOW);
	OSyncFilter *filter3 = osync_filter_add_custom(group, leftmember, rightmember, "7", "8", "9", "vcard_cats");
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
	osync = init_env();
	osync_env_register_objtype(osync, "test");
	osync_env_register_objformat(osync, "test", "format1");
	osync_env_register_filter_function(osync, "vcard_cats", "test", "format1", vcard_cats);
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
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	osync_filter_add(group, NULL, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
	osync_filter_add(group, NULL, rightmember, NULL, NULL, "contact", OSYNC_FILTER_ALLOW);
	
	mark_point();
	OSyncError *error = NULL;
  	OSyncEngine *engine = osengine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
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

START_TEST(filter_destobjtype_delete)
{
	/* Check if the destobjtype of the changes is being
	 * set when the change type os DELETE */
	char *testbed = setup_testbed("destobjtype_delete");
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();
	
	mark_point();
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	mark_point();
	fail_unless(engine != NULL, NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	mark_point();

	/* Synchronize once, delete a file, and synchronize again */

	fail_unless(!system("rm data1/file"), NULL);

	synchronize_once(engine, NULL);
	mark_point();
	osengine_finalize(engine);

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
  	OSyncEngine *engine = osengine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osengine_finalize(engine);

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
	//Suite *s2 = suite_create("Filter");
	
	create_case(s, "filter_setup", filter_setup);
	create_case(s, "filter_flush", filter_flush);
	create_case(s, "filter_sync_deny_all", filter_sync_deny_all);
	create_case(s, "filter_sync_custom", filter_sync_custom);
	create_case(s, "filter_save_and_load", filter_save_and_load);
	create_case(s, "filter_sync_vcard_only", filter_sync_vcard_only);
	create_case(s, "filter_destobjtype_delete", filter_destobjtype_delete);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = filter_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
