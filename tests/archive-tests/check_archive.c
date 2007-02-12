#include "support.h"

#include <opensync/opensync.h>
#include <opensync/opensync-archive.h>


START_TEST (archive_new)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new((const char *)"archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_archive_ref(archive);
	osync_archive_unref(archive);
	
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (archive_load_changes)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncList *ids;
	OSyncList *uids;
	OSyncList *mappingids;
	OSyncList *memberids;
	osync_archive_load_changes(archive, "contact", &ids, &uids, &mappingids, &memberids, &error);
		
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (archive_save_change)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncList *ids;
	OSyncList *uids;
	OSyncList *mappingids;
	OSyncList *memberids;
	osync_archive_load_changes(archive, "contact", &ids, &uids, &mappingids, &memberids, &error);
	
	long long int id = osync_archive_save_change(archive, 0, "uid", "contact", 1, 1, &error);
	fail_unless(id != 0, NULL);
	fail_unless(error == NULL, NULL);
		
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (archive_save_data)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncList *ids;
	OSyncList *uids;
	OSyncList *mappingids;
	OSyncList *memberids;
	osync_archive_load_changes(archive, "contact", &ids, &uids, &mappingids, &memberids, &error);
	
	long long int id = osync_archive_save_change(archive, 0, "uid", "contact", 1, 1, &error);
	fail_unless(id != 0, NULL);
	fail_unless(error == NULL, NULL);
	
	const char *testdata = "testdata";
	unsigned int testsize = strlen(testdata);
	fail_unless(osync_archive_save_data(archive, "uid", testdata, testsize, &error) == TRUE, NULL);
	fail_unless(error == NULL, NULL);
		
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (archive_load_data)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncList *ids;
	OSyncList *uids;
	OSyncList *mappingids;
	OSyncList *memberids;
	osync_archive_load_changes(archive, "contact", &ids, &uids, &mappingids, &memberids, &error);
	
	long long int id = osync_archive_save_change(archive, 0, "uid", "contact", 1, 1, &error);
	fail_unless(id != 0, NULL);
	fail_unless(error == NULL, NULL);
	
	const char *testdata = "testdata";
	unsigned int testsize = strlen(testdata);
	fail_unless(osync_archive_save_data(archive, "uid", testdata, testsize, &error) == TRUE, NULL);
	fail_unless(error == NULL, NULL);
	
	char *buffer;
	unsigned int size;
	fail_unless(osync_archive_load_data(archive, "uid", &buffer, &size, &error) == TRUE, NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(size == testsize);
	fail_unless(memcmp(buffer, testdata, testsize) == 0);
		
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (archive_load_data_with_closing_db)
{
	char *testbed = setup_testbed("merger");

	OSyncError *error = NULL;
	OSyncArchive *archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncList *ids;
	OSyncList *uids;
	OSyncList *mappingids;
	OSyncList *memberids;
	osync_archive_load_changes(archive, "contact", &ids, &uids, &mappingids, &memberids, &error);
	
	long long int id = osync_archive_save_change(archive, 0, "uid", "contact", 1, 1, &error);
	fail_unless(id != 0, NULL);
	fail_unless(error == NULL, NULL);
	
	const char *testdata = "testdata";
	unsigned int testsize = strlen(testdata);
	fail_unless(osync_archive_save_data(archive, "uid", testdata, testsize, &error) == TRUE, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_archive_unref(archive);
	archive = osync_archive_new("archive.db", &error);
	fail_unless(archive != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	char *buffer;
	unsigned int size;
	fail_unless(osync_archive_load_data(archive, "uid", &buffer, &size, &error) == TRUE, NULL);
	fail_unless(error == NULL, NULL);
	fail_unless(size == testsize);
	fail_unless(memcmp(buffer, testdata, testsize) == 0);
		
	osync_archive_unref(archive);

	destroy_testbed(testbed);
}
END_TEST

Suite *archive_suite(void)
{
	Suite *s = suite_create("Archive");
	create_case(s, "archive_new", archive_new);
	create_case(s, "archive_load_changes", archive_load_changes);
	create_case(s, "archive_save_change", archive_save_change);
	create_case(s, "archive_save_data", archive_save_data);
	create_case(s, "archive_load_data", archive_load_data);
	create_case(s, "archive_load_data_with_closing_db", archive_load_data_with_closing_db);
	return s;
}

int main(void)
{
	int nf;

	Suite *s = archive_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
