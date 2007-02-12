/*
not working disabled test
*/
#include "support.h"

START_TEST (multisync_conflict_ignore)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_ignore, GINT_TO_POINTER(3));
	
	system("cp newdata data3/testdata1");
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 2, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	sleep(2);
	
	system("rm -f data2/testdata");
	system("cp newdata data3/testdata");

	system("cp newdata3 data1/testdata1");
	system("cp newdata2 data2/testdata1");
	system("cp newdata1 data3/testdata1");
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	synchronize_once(engine, NULL);

	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	
	osengine_set_conflict_callback(engine, conflict_handler_choose_modified, GINT_TO_POINTER(3));
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 5, NULL);
	fail_unless(num_conflicts == 2, NULL);
	fail_unless(num_written == 4, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");

	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST


START_TEST (multisync_conflict_ignore2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_ignore, NULL);
	
	system("cp newdata data3/testdata1");
	system("cp newdata1 data3/testdata");
	
	synchronize_once(engine, NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" != \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" != \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 2, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("cp newdata2 data2/testdata");
	
	osengine_set_conflict_callback(engine, conflict_handler_choose_first, GINT_TO_POINTER(3));
	synchronize_once(engine, NULL);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 3, NULL);
	fail_unless(num_conflicts == 1, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	maptable = mappingtable_load(group, 2, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 1, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata1", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata1", "mockformat", "data");
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 2);
    check_hash(table, "testdata");
    check_hash(table, "testdata1");
	osync_hashtable_close(table);
	
	system("rm -f data1/*");

	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST


START_TEST(multisync_easy_new_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_easy_new();
#else /*CHECK_VERSION > 903*/
	multisync_easy_new(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_triple_del_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_triple_del();
#else /*CHECK_VERSION > 903*/
	multisync_triple_del(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_hybrid_choose2_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_hybrid_choose2();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_hybrid_choose2(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_conflict_handler_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_delayed_conflict_handler();
#else /*CHECK_VERSION > 903*/
	multisync_delayed_conflict_handler(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_slow_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_delayed_slow();
#else /*CHECK_VERSION > 903*/
	multisync_delayed_slow(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_ignore();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_ignore(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore2_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	setenv("NO_TIMEOUTS", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_ignore2();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_ignore2(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
	unsetenv("NO_TIMEOUTS");
}
END_TEST

START_TEST(multisync_conflict_hybrid_duplicate_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_hybrid_duplicate();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_hybrid_duplicate(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_multi_conflict_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_multi_conflict();
#else /*CHECK_VERSION > 903*/
	multisync_multi_conflict(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_zero_changes_b)
{
	setenv("BATCH_COMMIT", "7", TRUE);
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_load(osync, "configs/group", NULL);
	
	OSyncEngine *engine = init_engine(group);
	osengine_set_conflict_callback(engine, conflict_handler_duplication, GINT_TO_POINTER(3));
	
	synchronize_once(engine, NULL);

	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" = \"x\""), NULL);
	
	OSyncMappingTable *maptable = mappingtable_load(group, 1, 0);
	check_mapping(maptable, 1, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 2, -1, 3, "testdata", "mockformat", "data");
	check_mapping(maptable, 3, -1, 3, "testdata", "mockformat", "data");
	mappingtable_close(maptable);
	
	OSyncHashTable *table = hashtable_load(group, 1, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 1);
    check_hash(table, "testdata");
	osync_hashtable_close(table);
	
	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 1, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 2, NULL);
	fail_unless(num_engine_end_conflicts == 1, NULL);

	setenv("NUM_BATCH_COMMITS", "0", TRUE);
	
	synchronize_once(engine, NULL);

	fail_unless(num_engine_connected == 1, NULL);
	fail_unless(num_engine_read == 1, NULL);
	fail_unless(num_engine_wrote == 1, NULL);
	fail_unless(num_engine_disconnected == 1, NULL);
	fail_unless(num_read == 0, NULL);
	fail_unless(num_conflicts == 0, NULL);
	fail_unless(num_written == 0, NULL);
	fail_unless(num_engine_end_conflicts == 0, NULL);

	system("rm -f data1/*");
		
	unsetenv("NUM_BATCH_COMMITS");
	
	synchronize_once(engine, NULL);
	osengine_finalize(engine);
	
	maptable = mappingtable_load(group, 0, 0);
    mappingtable_close(maptable);
	
	table = hashtable_load(group, 1, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 2, 0);
	osync_hashtable_close(table);
	
	table = hashtable_load(group, 3, 0);
	osync_hashtable_close(table);
	
	fail_unless(!system("test \"x$(ls data1)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data2)\" = \"x\""), NULL);
	fail_unless(!system("test \"x$(ls data3)\" = \"x\""), NULL);
	
	destroy_testbed(testbed);
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_hybrid_choose2_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_hybrid_choose2();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_hybrid_choose2(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_delayed_conflict_handler_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_delayed_conflict_handler();
#else /*CHECK_VERSION > 903*/
	multisync_delayed_conflict_handler(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_conflict_ignore_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_conflict_ignore();
#else /*CHECK_VERSION > 903*/
	multisync_conflict_ignore(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST

START_TEST(multisync_multi_conflict_b2)
{
	setenv("BATCH_COMMIT", "2", TRUE);
	/*needed because of an incompatible API change in 0.94*/
#if CHECK_VERSION <= 903
	multisync_multi_conflict();
#else /*CHECK_VERSION > 903*/
	multisync_multi_conflict(0);
#endif /*CHECK_VERSION*/
	unsetenv("BATCH_COMMIT");
}
END_TEST
#endif
Suite *multisync_suite(void)
{
	Suite *s = suite_create("Multisync");
	//Suite *s2 = suite_create("Multisync");
	create_case(s, "multisync_conflict_ignore", multisync_conflict_ignore);
	create_case(s, "multisync_conflict_ignore2", multisync_conflict_ignore2);
	
	create_case(s, "multisync_easy_new_b", multisync_easy_new_b);
	create_case(s, "multisync_triple_del_b", multisync_triple_del_b);
	create_case(s, "multisync_conflict_hybrid_choose2_b", multisync_conflict_hybrid_choose2_b);
	create_case(s, "multisync_delayed_conflict_handler_b", multisync_delayed_conflict_handler_b);
	create_case(s, "multisync_delayed_slow_b", multisync_delayed_slow_b);
	create_case(s, "multisync_conflict_ignore_b", multisync_conflict_ignore_b);
	create_case(s, "multisync_conflict_ignore2_b", multisync_conflict_ignore2_b);
	create_case(s, "multisync_conflict_hybrid_duplicate_b", multisync_conflict_hybrid_duplicate_b);
	create_case(s, "multisync_multi_conflict_b", multisync_multi_conflict_b);
	create_case(s, "multisync_zero_changes_b", multisync_zero_changes_b);
	
	create_case(s, "multisync_conflict_hybrid_choose2_b2", multisync_conflict_hybrid_choose2_b2);
	create_case(s, "multisync_delayed_conflict_handler_b2", multisync_delayed_conflict_handler_b2);
	create_case(s, "multisync_conflict_ignore_b2", multisync_conflict_ignore_b2);
	create_case(s, "multisync_multi_conflict_b2", multisync_multi_conflict_b2);

	return s;
}

int main(void)
{
	int nf;

	Suite *s = multisync_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_set_fork_status (sr, CK_NOFORK);
	//check_fork();
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
