#include "support.h"

#include <opensync/opensync-version.h>

START_TEST (version_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncVersion *version = osync_version_new(&error);
	fail_unless(version != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_version_ref(version);
	osync_version_unref(version);
	
	osync_version_unref(version);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (version_matches)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncVersion *version = osync_version_new(&error);
	fail_unless(version != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_version_set_plugin(version, "SyncML");
	osync_version_set_modelversion(version, "7650");
	osync_version_set_firmwareversion(version, "*");
	osync_version_set_softwareversion(version, "*");
	osync_version_set_hardwareversion(version, "*");

	OSyncVersion *pattern = osync_version_new(&error);
	fail_unless(pattern != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_version_set_plugin(pattern, "Sync[A-Z]");
	osync_version_set_priority(pattern, "100");
	osync_version_set_modelversion(pattern, "[0-9]");
	osync_version_set_firmwareversion(pattern, "");
	osync_version_set_softwareversion(pattern, "");
	osync_version_set_hardwareversion(pattern, "");

	fail_unless(osync_version_matches(pattern, version, &error) > 0, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_version_set_firmwareversion(pattern, "[0-9]");

	fail_unless(osync_version_matches(pattern, version, &error) == 0, NULL);
	fail_unless(error == NULL, NULL);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (version_load_from_descriptions)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncList *versions = osync_version_load_from_descriptions(&error);
	//fail_unless(versions != NULL, NULL);
	fail_unless(error == NULL, NULL);

	OSyncList *cur = osync_list_first(versions);
	while(cur) {
		osync_version_unref(cur->data);
		cur = cur->next;
	}
	osync_list_free(versions);

	destroy_testbed(testbed);
}
END_TEST

Suite *version_suite(void)
{
	Suite *s = suite_create("Version");
	create_case(s, "version_new", version_new);
	create_case(s, "version_matches", version_matches);
	create_case(s, "version_load_from_descriptions", version_load_from_descriptions);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = version_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
