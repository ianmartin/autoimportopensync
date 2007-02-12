#include "support.h"
#include <time.h>

static void conv_vcal(const char *filename)
{
	char *command = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);		
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	osync_change_set_objtype(change, osync_objformat_get_objtype(sourceformat));
	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo10"))
		targetformat = osync_conv_find_objformat(conv_env, "vtodo20");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo20"))
		targetformat = osync_conv_find_objformat(conv_env, "vtodo10");

	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent10"))
		targetformat = osync_conv_find_objformat(conv_env, "vevent20");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent20"))
		targetformat = osync_conv_find_objformat(conv_env, "vevent10");

	fail_unless(targetformat != NULL, NULL);
	
	OSyncChange *newchange = osync_change_copy(change, &error);
	fail_unless(newchange != NULL, NULL);
	
	//Convert to
	fail_unless(osync_change_convert(conv_env, change, targetformat, &error), NULL);
	
	//Detect the output
	osync_change_set_objformat_string(change, "plain");
	fail_unless(osync_change_detect_objformat(conv_env, change, &error) == targetformat, NULL);
	
	//Compare old to new
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	//Convert back
	fail_unless(osync_change_convert(conv_env, change, sourceformat, &error), NULL);

	//Detect the output again
	osync_change_set_objformat_string(change, "plain");
	fail_unless(osync_change_detect_objformat(conv_env, change, &error) == sourceformat, NULL);
	
	//Compare again
	fail_unless(osync_change_compare(newchange, change) == CONV_DATA_SAME, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
}

static void compare_vcal(const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	char *command1 = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s lfile", g_get_current_dir(), lfilename);
	char *command2 = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s rfile", g_get_current_dir(), rfilename);
	char *testbed = setup_testbed(NULL);
	system(command1);
	g_free(command1);
	system(command2);
	g_free(command2);
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	fail_unless(osync_file_read("lfile", &buffer, &size, &error), NULL);
	
	OSyncChange *lchange = osync_change_new();
	osync_change_set_uid(lchange, "lfile");
	osync_change_set_data(lchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(lchange, conv_env);
	osync_change_set_objformat_string(lchange, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, lchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(lchange, sourceformat);
	osync_change_set_objtype(lchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_file_read("rfile", &buffer, &size, &error), NULL);
	
	OSyncChange *rchange = osync_change_new();
	osync_change_set_uid(rchange, "rfile");
	osync_change_set_data(rchange, buffer, size + 1, TRUE);
	osync_change_set_conv_env(rchange, conv_env);
	osync_change_set_objformat_string(rchange, "plain");

	sourceformat = osync_change_detect_objformat(conv_env, rchange, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(rchange, sourceformat);
	osync_change_set_objtype(rchange, osync_objformat_get_objtype(sourceformat));
	
	fail_unless(osync_change_compare(lchange, rchange) == result, NULL);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	destroy_testbed(testbed);
}

static time_t vcal_get_revision(const char *filename)
{
	char *command = g_strdup_printf("cp %s/"OPENSYNC_TESTDATA"%s .", g_get_current_dir(), filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	OSyncEnv *env = init_env();
	
	OSyncFormatEnv *conv_env = osync_conv_env_new(env);
	fail_unless(conv_env != NULL, NULL);

	char *buffer;
	int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new();
	osync_change_set_uid(change, file);
	g_free(file);
	osync_change_set_data(change, buffer, size + 1, TRUE);
	osync_change_set_conv_env(change, conv_env);
	
	osync_change_set_objformat_string(change, "plain");

	OSyncObjFormat *sourceformat = osync_change_detect_objformat(conv_env, change, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_change_set_objformat(change, sourceformat);
	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo10") || !strcmp(osync_objformat_get_name(sourceformat), "vtodo20"))
		targetformat = osync_conv_find_objformat(conv_env, "xml-todo");

	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent10") || !strcmp(osync_objformat_get_name(sourceformat), "vevent20"))
		targetformat = osync_conv_find_objformat(conv_env, "xml-event");
	
	fail_unless(targetformat != NULL, NULL);
	
	fail_unless(osync_change_convert_extension(conv_env, change, targetformat, "evolution", &error), NULL);
	
	time_t time = osync_change_get_revision(change, &error);
	
	osync_conv_env_free(conv_env);
	osync_env_finalize(env, NULL);
	osync_env_free(env);
	
	destroy_testbed(testbed);
	return time;
}

START_TEST (conv_vevent_evolution2_1hour)
{
	conv_vcal("data/vevents/evolution2/1-hour.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_1hour_alarm)
{
	conv_vcal("data/vevents/evolution2/1-hour-alarm.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_1hour_alarm2)
{
	conv_vcal("data/vevents/evolution2/1-hour-alarm2.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_all_day)
{
	conv_vcal("data/vevents/evolution2/all-day.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_all_day2)
{
	conv_vcal("data/vevents/evolution2/all-day2.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_free_busy)
{
	conv_vcal("data/vevents/evolution2/free-busy.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_full_special)
{
	conv_vcal("data/vevents/evolution2/full-special.vcf");
}
END_TEST
START_TEST (conv_vevent_evolution2_rec_every_year)
{
	conv_vcal("data/vevents/evolution2/rec-every-year.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_except)
{
	conv_vcal("data/vevents/evolution2/rec-except.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_for)
{
	conv_vcal("data/vevents/evolution2/rec-for.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_forever)
{
	conv_vcal("data/vevents/evolution2/rec-forever.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_until)
{
	conv_vcal("data/vevents/evolution2/rec-until.vcf");
}
END_TEST

START_TEST (conv_vevent_evolution2_rec_until2)
{
	conv_vcal("data/vevents/evolution2/evo2-recur-until.vcf");
}
END_TEST

START_TEST (conv_vevent_kdepim_1hour_10)
{
	conv_vcal("data/vevents/kdepim/1-hour-1.0.vcs");
}
END_TEST

START_TEST (conv_vevent_kdepim_1hour_20)
{
	conv_vcal("data/vevents/kdepim/1-hour-2.0.ics");
}
END_TEST

START_TEST (cmp_vevent_1hour_1)
{
	compare_vcal("data/vevents/evolution2/1-hour.vcf", "data/vevents/kdepim/1-hour-1.0.vcs", CONV_DATA_SAME);
}
END_TEST

START_TEST (cmp_vevent_1hour_2)
{
	compare_vcal("data/vevents/evolution2/1-hour.vcf", "data/vevents/kdepim/1-hour-2.0.ics", CONV_DATA_SAME);
}
END_TEST

START_TEST (event_get_revision1)
{
	struct tm testtm = {2, 6, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vevents/evolution2/1-hour-alarm.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_get_revision2)
{
	struct tm testtm = {1, 8, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vevents/evolution2/1-hour-alarm2.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_get_revision3)
{
	struct tm testtm = {13, 5, 11, 29, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vevents/evolution2/1-hour.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (event_no_revision)
{
	fail_unless(vcal_get_revision("data/vevents/evolution2/all-day.vcf") == -1, NULL);
}
END_TEST


START_TEST (conv_vtodo_evolution2_simple)
{
	conv_vcal("data/vtodos/evolution2/todo-simple.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full1)
{
	conv_vcal("data/vtodos/evolution2/todo-full1.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full2)
{
	conv_vcal("data/vtodos/evolution2/todo-full2.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full3)
{
	conv_vcal("data/vtodos/evolution2/todo-full3.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full4)
{
	conv_vcal("data/vtodos/evolution2/todo-full4.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full5)
{
	conv_vcal("data/vtodos/evolution2/todo-full5.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full6)
{
	conv_vcal("data/vtodos/evolution2/todo-full6.vcf");
}
END_TEST

START_TEST (conv_vtodo_evolution2_full7)
{
	conv_vcal("data/vtodos/evolution2/todo-full7.vcf");
}
END_TEST

START_TEST (todo_get_revision1)
{
	struct tm testtm = {50, 56, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full1.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_get_revision2)
{
	struct tm testtm = {50, 56, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full2.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_get_revision3)
{
	struct tm testtm = {0, 0, 0, 6, 3 - 1, 2005 - 1900, 0, 0, 0};
	fail_unless(vcal_get_revision("data/vtodos/evolution2/todo-full3.vcf") == mktime(&testtm), NULL);
}
END_TEST

START_TEST (todo_no_revision)
{
	fail_unless(vcal_get_revision("data/vtodos/kdepim/todo-full1.vcs") == -1, NULL);
}
END_TEST

/*
START_TEST (conv_test_crash)
{
	conv_vcal("data/vevents/crash.ics");
}
END_TEST
*/
START_TEST (conv_test_utf8_wrap)
{
	conv_vcal("data/vevents/utf8_wrap");
}
END_TEST

START_TEST (conv_test_qp_wrap)
{
	conv_vcal("data/vevents/qp_wrap");
}
END_TEST

Suite *vcal_suite(void)
{
	Suite *s = suite_create("VCal");
	//Suite *s2 = suite_create("VCal");
	
	create_case(s, "conv_vevent_evolution2_1hour", conv_vevent_evolution2_1hour);
	create_case(s, "conv_vevent_evolution2_1hour_alarm", conv_vevent_evolution2_1hour_alarm);
	create_case(s, "conv_vevent_evolution2_1hour_alarm2", conv_vevent_evolution2_1hour_alarm2);
	create_case(s, "conv_vevent_evolution2_all_day", conv_vevent_evolution2_all_day);
	create_case(s, "conv_vevent_evolution2_all_day2", conv_vevent_evolution2_all_day2);
	create_case(s, "conv_vevent_evolution2_free_busy", conv_vevent_evolution2_free_busy);
	create_case(s, "conv_vevent_evolution2_full_special", conv_vevent_evolution2_full_special);
	create_case(s, "conv_vevent_evolution2_rec_every_year", conv_vevent_evolution2_rec_every_year);
	create_case(s, "conv_vevent_evolution2_rec_except", conv_vevent_evolution2_rec_except);
	create_case(s, "conv_vevent_evolution2_rec_for", conv_vevent_evolution2_rec_for);
	create_case(s, "conv_vevent_evolution2_rec_forever", conv_vevent_evolution2_rec_forever);
	create_case(s, "conv_vevent_evolution2_rec_until", conv_vevent_evolution2_rec_until);
	create_case(s, "conv_vevent_evolution2_rec_until2", conv_vevent_evolution2_rec_until2);
	
	create_case(s, "conv_vevent_kdepim_1hour_10", conv_vevent_kdepim_1hour_10);
	create_case(s, "conv_vevent_kdepim_1hour_20", conv_vevent_kdepim_1hour_20);
	
	create_case(s, "cmp_vevent_1hour_1", cmp_vevent_1hour_1);
	create_case(s, "cmp_vevent_1hour_2", cmp_vevent_1hour_2);
	
	create_case(s, "event_get_revision1", event_get_revision1);
	create_case(s, "event_get_revision2", event_get_revision2);
	create_case(s, "event_get_revision3", event_get_revision3);
	create_case(s, "event_no_revision", event_no_revision);
	
	
	

	create_case(s, "conv_vtodo_evolution2_simple", conv_vtodo_evolution2_simple);
	create_case(s, "conv_vtodo_evolution2_full1", conv_vtodo_evolution2_full1);
	create_case(s, "conv_vtodo_evolution2_full2", conv_vtodo_evolution2_full2);
	create_case(s, "conv_vtodo_evolution2_full3", conv_vtodo_evolution2_full3);
	create_case(s, "conv_vtodo_evolution2_full4", conv_vtodo_evolution2_full4);
	create_case(s, "conv_vtodo_evolution2_full5", conv_vtodo_evolution2_full5);
	create_case(s, "conv_vtodo_evolution2_full6", conv_vtodo_evolution2_full6);
	create_case(s, "conv_vtodo_evolution2_full7", conv_vtodo_evolution2_full7);
	
	create_case(s, "todo_get_revision1", todo_get_revision1);
	create_case(s, "todo_get_revision2", todo_get_revision2);
	create_case(s, "todo_get_revision3", todo_get_revision3);
	create_case(s, "todo_no_revision", todo_no_revision);
/*	create_case(s, "conv_test_crash", conv_test_crash);*/
	create_case(s, "conv_test_utf8_wrap", conv_test_utf8_wrap);
	create_case(s, "conv_test_qp_wrap", conv_test_qp_wrap);
	
	
	
	return s;
}

int main(void)
{
	int nf;

	Suite *s = vcal_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

//	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
