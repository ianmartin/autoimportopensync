#include "support.h"

#include "opensync/format/opensync_filter_internals.h"

static osync_bool dummy_filter_hook(OSyncData *data, const char *config)
{
	return TRUE;
}

START_TEST (filter_setup)
{
	char *testbed = setup_testbed("filter_setup");

	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);

	mark_point();
	
	OSyncFilter *filter = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	fail_unless(error == NULL, NULL);
	fail_unless(filter != NULL, NULL);
	osync_group_add_filter(group, filter);
	
	mark_point();
	fail_unless(osync_group_num_filters(group) == 1, NULL);
	fail_unless(osync_group_nth_filter(group, 0) == filter, NULL);
	
	mark_point();
	osync_group_remove_filter(group, filter);
	fail_unless(osync_group_num_filters(group) == 0, NULL);
	osync_filter_unref(filter);

	osync_group_unref(group);

	destroy_testbed(testbed);
}
END_TEST

/* filter flushing got dropped with OpenSync 0.30 API
START_TEST (filter_flush)
{
	char *testbed = setup_testbed("filter_setup");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	
	OSyncFilter *filter1 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	OSyncFilter *filter2 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter2 != NULL, NULL);

	osync_group_add_filter(group, filter1);
	osync_group_add_filter(group, filter2);
	
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
*/

START_TEST (filter_sync_deny_all)
{
	char *testbed = setup_testbed("filter_sync_deny_all");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncFilter *filter1 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	OSyncFilter *filter2 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);

	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter2 != NULL, NULL);

	osync_group_add_filter(group, filter1);
	osync_group_add_filter(group, filter2);
	
	mark_point();
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), osync_error_print(&error));
	synchronize_once(engine, &error);
	osync_engine_finalize(engine, &error);

	fail_unless(!system("test \"x$(ls data1)\" = \"xtestdata\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"xtestdata2\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_sync_custom)
{
	char *testbed = setup_testbed("filter_sync_custom");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	OSyncCustomFilter *custom_filter = osync_custom_filter_new("mockobjtype1", "mockformat1", "mockformat1_custom_filter", dummy_filter_hook, &error);
	fail_unless(custom_filter != NULL, NULL);

	OSyncFilter *filter = osync_filter_new_custom(custom_filter, NULL, OSYNC_FILTER_DENY, &error);

	fail_unless(filter != NULL, NULL);

	osync_group_add_filter(group, filter);

	osync_filter_set_config(filter, "test");
	
	mark_point();
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine, &error);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (filter_save_and_load)
{
	char *testbed = setup_testbed("filter_save_and_load");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	/* TODO filter member handling 
	OSyncMember *leftmember = osync_group_nth_member(group, 0);
	OSyncMember *rightmember = osync_group_nth_member(group, 1);
	*/
	OSyncCustomFilter *custom_filter = osync_custom_filter_new("mockobjtype3", "mockformat3", "mockformat1_custom_filter", dummy_filter_hook, &error);
	fail_unless(custom_filter != NULL, NULL);

	OSyncFilter *filter1 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	OSyncFilter *filter2 = osync_filter_new("mockobjtype2", OSYNC_FILTER_ALLOW, &error);

	OSyncFilter *filter3 = osync_filter_new_custom(custom_filter, "test", OSYNC_FILTER_IGNORE, &error);
	
	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter2 != NULL, NULL);
	fail_unless(filter3 != NULL, NULL);

	osync_group_add_filter(group, filter1);
	osync_group_add_filter(group, filter2);
	osync_group_add_filter(group, filter3);

	fail_unless(osync_group_num_filters(group) == 3, NULL);
	fail_unless(osync_group_nth_filter(group, 0) == filter1, NULL);
	fail_unless(osync_group_nth_filter(group, 1) == filter2, NULL);
	fail_unless(osync_group_nth_filter(group, 2) == filter3, NULL);
	
	mark_point();
	osync_group_save(group, NULL);
	mark_point();

	osync_group_load(group, "configs/group", NULL);
	fail_unless(group != NULL, NULL);
	mark_point();

	fail_unless(osync_group_num_filters(group) == 3, NULL);
	filter1 = osync_group_nth_filter(group, 0);
	fail_unless(filter1 != NULL, NULL);
	/* filter aren't member specific anymore. */
	//fail_unless(filter1->sourcememberid == 1, NULL);
	//fail_unless(filter1->destmemberid == 2, NULL);
	fail_unless(!strcmp(osync_filter_get_objtype(filter1), "mockobjtype1"), NULL);
	/* cross objtype syncing isn't supported anymore with 0.30 API */
	//fail_unless(!strcmp(filter1->destobjtype, "2"), NULL);
	//fail_unless(!strcmp(filter1->detectobjtype, "3"), NULL);
	fail_unless(filter1->action == OSYNC_FILTER_DENY, NULL);
	
	filter1 = osync_group_nth_filter(group, 1);
	fail_unless(filter1 != NULL, NULL);
	/* filter aren't member specific anymore. */
	//fail_unless(filter1->sourcememberid == 2, NULL);
	//fail_unless(filter1->destmemberid == 1, NULL);
	/* cross objtype syncing isn't supported anymore with 0.30 API */
	fail_unless(!strcmp(osync_filter_get_objtype(filter1), "mockobjtype2"), NULL);
	/* cross objtype syncing isn't supported anymore with 0.30 API */
	//fail_unless(!strcmp(filter1->destobjtype, "5"), NULL);
	//fail_unless(!strcmp(filter1->detectobjtype, "6"), NULL);
	fail_unless(filter1->action == OSYNC_FILTER_ALLOW, NULL);

	filter1 = osync_group_nth_filter(group, 2);
	fail_unless(filter1 != NULL, NULL);

	/* filter aren't member specific anymore. */
	//fail_unless(filter1->sourcememberid == 1, NULL);
	//fail_unless(filter1->destmemberid == 2, NULL);
	/* cross objtype syncing isn't supported anymore with 0.30 API */

	// FIXME - custom filter handling for objtype
	///fail_unless(!strcmp(osync_filter_get_objtype(filter1), "mockobjtype3"), NULL);

	/* cross objtype syncing isn't supported anymore with 0.30 API */
	//fail_unless(!strcmp(filter1->destobjtype, "8"), NULL);
	//fail_unless(!strcmp(filter1->detectobjtype, "9"), NULL);
	fail_unless(filter1->custom_filter != NULL, NULL);
	fail_unless(filter1->custom_filter->hook != NULL, NULL);
	fail_unless(!strcmp(osync_filter_get_config(filter1), "test"), NULL);

	destroy_testbed(testbed);
}
END_TEST

/* TODO: Test filtering of different objtypes within one resource/database/path.
   Mixed objtypes in mock-sync (based on file-sync) currenlty not supported.
   Reimplement this when: data(wildcard objtype) <-> certain-objtype syncing is working again.

   Make this implementation independet of vformat plugin (feel free to rename vcard to mockobjtypeX).
   This filtering isn't about obfjromat filtering - it's about objtype filtering.
*/

/* FIXME */
#if 0
START_TEST (filter_sync_vcard_only)
{
	char *testbed = setup_testbed("filter_sync_vcard_only");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	/* TODO filtering isn't member sepcific anymore
	OSyncMember *rightmember = osync_group_nth_member(group, 1);

        osync_filter_add(group, NULL, rightmember, NULL, NULL, NULL, OSYNC_FILTER_DENY);
        osync_filter_add(group, NULL, rightmember, NULL, NULL, "contact", OSYNC_FILTER_ALLOW);
	*/
	
	OSyncFilter *filter1 = osync_filter_new("mockobjtype1", OSYNC_FILTER_DENY, &error);
	OSyncFilter *filter2 = osync_filter_new("mockobjtype1", OSYNC_FILTER_ALLOW, &error);

	fail_unless(filter1 != NULL, NULL);
	fail_unless(filter2 != NULL, NULL);

	osync_group_add_filter(group, filter1);
	osync_group_add_filter(group, filter2);
	
	mark_point();
  	OSyncEngine *engine = osync_engine_new(group, &error);
  	mark_point();
  	fail_unless(engine != NULL, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);

	fail_unless(!system("test \"x$(ls data1/testdata)\" = \"xdata1/testdata\""), NULL);
	fail_unless(!system("test \"x$(ls data1/testdata2)\" = \"xdata1/testdata2\""), NULL);
	fail_unless(!system("test \"x$(ls data1/testdata3)\" = \"xdata1/testdata3\""), NULL);
	fail_unless(!system("test \"x$(ls data1/vcard.vcf)\" = \"xdata1/vcard.vcf\""), NULL);
	
	fail_unless(!system("test \"x$(ls data2/testdata3)\" = \"xdata2/testdata3\""), NULL);
	fail_unless(!system("test \"x$(ls data2/vcard.vcf)\" = \"xdata2/vcard.vcf\""), NULL);

	destroy_testbed(testbed);
}
END_TEST
#endif /* FIXME */

START_TEST(filter_destobjtype_delete)
{
	/* Check if the destobjtype of the changes is being
	 * set when the change type os DELETE */
	char *testbed = setup_testbed("destobjtype_delete");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
	fail_unless(group != NULL, NULL);
	mark_point();
	
	mark_point();
	OSyncEngine *engine = osync_engine_new(group, &error);
	mark_point();
	fail_unless(engine != NULL, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	synchronize_once(engine, NULL);
	mark_point();

	/* Synchronize once, delete a file, and synchronize again */

	fail_unless(!system("rm data1/file"), NULL);

	synchronize_once(engine, NULL);
	mark_point();
	osync_engine_finalize(engine, &error);

	destroy_testbed(testbed);
}
END_TEST

/*int num_read;

START_TEST (filter_sync_read_only)
{
	char *testbed = setup_testbed("filter_sync_deny_all");
	OSyncError *error = NULL;

	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_load(group, "configs/group", &error);
	fail_unless(error == NULL, osync_error_print(&error));
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

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	synchronize_once(engine, NULL);
	osync_engine_finalize(engine, &error);

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
	create_case(s, "filter_sync_deny_all", filter_sync_deny_all);
	create_case(s, "filter_sync_custom", filter_sync_custom);
	create_case(s, "filter_save_and_load", filter_save_and_load);
	//create_case(s, "filter_sync_vcard_only", filter_sync_vcard_only); // TODO, see testcase description
	create_case(s, "filter_destobjtype_delete", filter_destobjtype_delete);
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = filter_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
