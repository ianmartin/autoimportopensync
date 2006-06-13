#include "support.h"

START_TEST (simple_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group, TRUE);

	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (simple_lock_stale)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group, FALSE);

	fail_unless(g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (simple_seq_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group, TRUE);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group, TRUE);

	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (simple_seq_stale_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group, FALSE);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_STALE, NULL);
	osync_group_unlock(group, TRUE);

	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	fail_unless(osync_group_lock(group) == OSYNC_LOCKED, NULL);
	
	osync_group_unlock(group, TRUE);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_lock2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	OSyncGroup *group2 = osync_group_new(osync, NULL);
	osync_group_load(group2, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	fail_unless(osync_group_lock(group2) == OSYNC_LOCKED, NULL);
	
	osync_group_unlock(group, TRUE);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (multi_unlock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	osync_group_unlock(group, TRUE);
	osync_group_unlock(group, FALSE);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	
	osync_group_unlock(group, FALSE);
	osync_group_unlock(group, TRUE);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_STALE, NULL);
	
	osync_group_unlock(group, TRUE);
	osync_group_unlock(group, FALSE);
	
	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_engine_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	OSyncGroup *group2 = osync_group_new(osync, NULL);
	osync_group_load(group2, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	OSyncEngine *engine2 = osengine_new(group2, &error);
	osengine_set_enginestatus_callback(engine2, engine_status, NULL);
	
	fail_unless(osengine_init(engine, &error), NULL);
	fail_unless(!osengine_init(engine2, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	osync_error_unref(&error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	fail_unless(!synchronize_once(engine2, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	osync_error_unref(&error);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	osengine_finalize(engine);
	
	fail_unless(osengine_init(engine2, &error), NULL);
	fail_unless(synchronize_once(engine2, &error), NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	osengine_finalize(engine2);
	
	osengine_free(engine);
	osengine_free(engine2);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_engine_unclean)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncEnv *osync = init_env();
	OSyncGroup *group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(!osync_group_get_slow_sync(group, "data"), NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osengine_new(group, &error);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	
	fail_unless(osengine_init(engine, &error), NULL);

	/* Quit the engine thread, before free()ing it
	 *
	 * We want to simulate a unclean engine exit (so we can't use
	 * osengine_finalize() here), but we don't want the old engine thread to
	 * be running and stealing the messages going to the second engine.
	 */
	if (engine->thread) {
		g_main_loop_quit(engine->syncloop);
		g_thread_join(engine->thread);
	}

	osengine_free(engine);
	osync_group_free(group);

	group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	engine = osengine_new(group, &error);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	
	fail_unless(!osync_group_get_slow_sync(engine->group, "data"), NULL);
	
	num_engine_prev_unclean = 0;
	fail_unless(osengine_init(engine, &error), NULL);
	fail_unless(num_engine_prev_unclean == 1, NULL);
	
	fail_unless(osync_group_get_slow_sync(engine->group, "data"), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	osengine_finalize(engine);
	osengine_free(engine);
	osync_group_free(group);
	
	group = osync_group_new(osync, NULL);
	osync_group_load(group, "configs/group", NULL);
	engine = osengine_new(group, &error);
	osengine_set_enginestatus_callback(engine, engine_status, NULL);
	
	fail_unless(!osync_group_get_slow_sync(engine->group, "data"), NULL);
	fail_unless(osengine_init(engine, &error), NULL);
	fail_unless(!osync_group_get_slow_sync(engine->group, "data"), NULL);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	osengine_finalize(engine);
	osengine_free(engine);
	
	fail_unless(num_engine_prev_unclean == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *lock_suite(void)
{
	Suite *s = suite_create("Locks");
	//Suite *s2 = suite_create("Locks");
	create_case(s, "simple_lock", simple_lock);
	create_case(s, "simple_lock_stale", simple_lock_stale);
	create_case(s, "simple_seq_lock", simple_seq_lock);
	create_case(s, "simple_seq_stale_lock", simple_seq_stale_lock);
	create_case(s, "dual_lock", dual_lock);
	create_case(s, "dual_lock2", dual_lock2);
	create_case(s, "multi_unlock", multi_unlock);
	create_case(s, "dual_sync_engine_lock", dual_sync_engine_lock);
	create_case(s, "dual_sync_engine_unclean", dual_sync_engine_unclean);
	

	return s;
}

int main(void)
{
	int nf;

	Suite *s = lock_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
