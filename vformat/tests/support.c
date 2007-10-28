#include "support.h"

char *olddir = NULL;

static void reset_env(void)
{
	unsetenv("CONNECT_ERROR");
	unsetenv("CONNECT_TIMEOUT");
	unsetenv("INIT_NULL");
	unsetenv("GET_CHANGES_ERROR");
	unsetenv("GET_CHANGES_TIMEOUT");
	unsetenv("GET_CHANGES_TIMEOUT2");
	unsetenv("COMMIT_ERROR");
	unsetenv("COMMIT_TIMEOUT");
	unsetenv("SYNC_DONE_ERROR");
	unsetenv("SYNC_DONE_TIMEOUT");
	unsetenv("DISCONNECT_ERROR");
	unsetenv("DISCONNECT_TIMEOUT");
}

char *setup_testbed(char *fkt_name)
{
	
	setuid(65534);
	char *testbed = g_strdup_printf("%s/testbed.XXXXXX", g_get_tmp_dir());
	mkdtemp(testbed);
	
	char *command = NULL;
	if (fkt_name) {
		char *dirname = g_strdup_printf(OPENSYNC_TESTDATA"/%s", fkt_name);
		if (!g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
			osync_trace(TRACE_INTERNAL, "%s: Path %s not exist.", __func__, dirname);
			abort();
		}
		command = g_strdup_printf("cp -R %s/* %s", dirname, testbed);
		osync_trace(TRACE_INTERNAL, "tb_cmd: %s", command);
		if (system(command))
			abort();
		g_free(command);
		g_free(dirname);
	}

	command = g_strdup_printf("cp ../src/*.so*  %s", testbed);
	if (system(command))
		abort();
	g_free(command);
	
	command = g_strdup_printf("chmod -R 700 %s", testbed);
	if (system(command))
		abort();
	g_free(command);
		
	olddir = g_get_current_dir();
	if (chdir(testbed))
		abort();
	
	osync_trace(TRACE_INTERNAL, "Seting up %s at %s", fkt_name, testbed);
	reset_env();
	return testbed;
}

void destroy_testbed(char *path)
{
	char *command = g_strdup_printf("rm -rf %s", path);
	if (olddir) {
		chdir(olddir);
		g_free(olddir);
	}
	system(command);
	g_free(command);
	osync_trace(TRACE_INTERNAL, "Tearing down %s", path);
	g_free(path);
}

void create_case(Suite *s, const char *name, TFun function)
{
	TCase *tc_new = tcase_create(name);
	tcase_set_timeout(tc_new, 30);
	suite_add_tcase (s, tc_new);
	tcase_add_test(tc_new, function);
}


void create_case_timeout(Suite *s, const char *name, TFun function, int timeout)
{
	TCase *tc_new = tcase_create(name);
	tcase_set_timeout(tc_new, timeout);
	suite_add_tcase (s, tc_new);
	tcase_add_test(tc_new, function);
}

