#include "support.h"

#include <opensync/opensync.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync_internals.h>
#include "opensync/engine/opensync_engine_internals.h"
#include "opensync/group/opensync_group_internals.h"

START_TEST (simple_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group);
	osync_group_unref(group);

	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (simple_seq_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	osync_group_unlock(group);
	osync_group_unref(group);

	fail_unless(!g_file_test("configs/group/lock", G_FILE_TEST_EXISTS), NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	fail_unless(osync_group_lock(group) == OSYNC_LOCKED, NULL);
	
	osync_group_unlock(group);
	osync_group_unref(group);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_lock2)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	OSyncGroup *group2 = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group2, "configs/group", NULL);
	
	fail_unless(osync_group_lock(group) == OSYNC_LOCK_OK, NULL);
	fail_unless(osync_group_lock(group2) == OSYNC_LOCKED, NULL);
	
	osync_group_unlock(group);
	osync_group_unref(group);
	osync_group_unref(group2);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_engine_lock)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	OSyncGroup *group2 = osync_group_new(NULL);
	osync_group_set_schemadir(group2, testbed);
	osync_group_load(group2, "configs/group", NULL);
	
	OSyncError *error = NULL;

	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	OSyncEngine *engine2 = osync_engine_new(group2, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	osync_engine_set_enginestatus_callback(engine2, engine_status, NULL);

	osync_engine_set_plugindir(engine2, testbed);
	osync_engine_set_formatdir(engine2, testbed);

	fail_unless(osync_engine_initialize(engine, &error), osync_error_print(&error));
	fail_unless(!osync_engine_initialize(engine2, &error), osync_error_print(&error));
	fail_unless(osync_error_is_set(&error), osync_error_print(&error));
	osync_error_unref(&error);
	
	fail_unless(synchronize_once(engine, &error), NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	fail_unless(!synchronize_once(engine2, &error), NULL);
	fail_unless(osync_error_is_set(&error), NULL);
	osync_error_unref(&error);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	osync_engine_finalize(engine, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	
	fail_unless(osync_engine_initialize(engine2, &error), NULL);
	fail_unless(synchronize_once(engine2, &error), NULL);
	fail_unless(num_engine_prev_unclean == 0, NULL);
	osync_engine_finalize(engine2, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	
	osync_engine_unref(engine);
	osync_engine_unref(engine2);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);
	
	osync_group_unref(group);
	osync_group_unref(group2);
	destroy_testbed(testbed);
}
END_TEST

START_TEST (dual_sync_engine_unclean)
{
	char *testbed = setup_testbed("multisync_easy_new");
	
	OSyncGroup *group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	
	OSyncError *error = NULL;
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);

	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, osync_error_print(&error));

	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);

	/* Quit the engine thread, before free()ing it
	 *
	 * We want to simulate a unclean engine exit (so we can't use
	 * osync_engine_finalize() here), but we don't want the old engine thread to
	 * be running and stealing the messages going to the second engine.
	 */
	if (engine->thread) {
		osync_thread_stop(engine->thread);
		osync_thread_free(engine->thread);
		engine->thread = NULL;
	}

	osync_engine_finalize(engine, &error);
	osync_engine_unref(engine);
	osync_group_unref(group);

	/* Ugly hack to simulate a engine crash.
	 * 
	 * osync_group_unref() cleans up the lockfile. osync_group_unref can't be skipped,
	 * since the process would keep the exclusive lock of the lockfile.
	 *
	 * So we just create a dummy lockfile without any exclusive lock:
	 */

	int lock_fd = g_open("configs/group/lock", O_CREAT | O_WRONLY, 00700);
	fail_unless(lock_fd > 0); 
	close(lock_fd);


	group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	engine = osync_engine_new(group, &error);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);

	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	num_engine_prev_unclean = 0;
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(num_engine_prev_unclean == 1, NULL);
	
	GList *o;
	for (o = engine->object_engines; o; o = o->next) {
		OSyncObjEngine *objengine = o->data;
		fail_unless(osync_obj_engine_get_slowsync(objengine), "Slow Sync got NOT set for ObjEngine! But previous sync was unclean!");
	}

	fail_unless(synchronize_once(engine, &error), NULL);
	osync_engine_finalize(engine, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	osync_engine_unref(engine);
	osync_group_unref(group);
	
	group = osync_group_new(NULL);
	osync_group_set_schemadir(group, testbed);
	osync_group_load(group, "configs/group", NULL);
	engine = osync_engine_new(group, &error);
	osync_engine_set_enginestatus_callback(engine, engine_status, NULL);
	
	osync_engine_set_plugindir(engine, testbed);
	osync_engine_set_formatdir(engine, testbed);
	
	for (o = engine->object_engines; o; o = o->next) {
		OSyncObjEngine *objengine = o->data;
		fail_unless(!osync_obj_engine_get_slowsync(objengine), "Slow Sync got set for ObjEngine! But previous sync was clean!");
	}
	fail_unless(osync_engine_initialize(engine, &error), NULL);

	for (o = engine->object_engines; o; o = o->next) {
		OSyncObjEngine *objengine = o->data;
		fail_unless(!osync_obj_engine_get_slowsync(objengine), "Slow Sync got set for ObjEngine! But previous sync was clean!");
	}

	
	fail_unless(synchronize_once(engine, &error), NULL);
	osync_engine_finalize(engine, &error);
	fail_unless(error == NULL, osync_error_print(&error));
	osync_engine_unref(engine);
	
	fail_unless(num_engine_prev_unclean == 0, NULL);
	
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data2)\" == \"x\""), NULL);
	fail_unless(!system("test \"x$(diff -x \".*\" data1 data3)\" == \"x\""), NULL);

	osync_group_unref(group);
	
	destroy_testbed(testbed);
}
END_TEST

Suite *lock_suite(void)
{
	Suite *s = suite_create("Locks");
	//Suite *s2 = suite_create("Locks");
	create_case(s, "simple_lock", simple_lock);
	create_case(s, "simple_seq_lock", simple_seq_lock);
	create_case(s, "dual_lock", dual_lock);
	create_case(s, "dual_lock2", dual_lock2);
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
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
