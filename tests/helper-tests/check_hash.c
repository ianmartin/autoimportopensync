#include "support.h"

#include <opensync/opensync.h>
#include <opensync/opensync-helper.h>


static int count_entries = 0;

static void reset_hashtable_counters()
{
	count_entries = 0;
}

void foreach_hash(const char *uid, const char *hash, void *data)
{
	count_entries++;
}

START_TEST (hashtable_new)
{
	OSyncError *error = NULL;
	char *testbed = setup_testbed(NULL);

	reset_hashtable_counters();

	char *hashpath = g_strdup_printf("%s%chashtable.db", testbed, G_DIR_SEPARATOR);
	OSyncHashTable *table = osync_hashtable_new(hashpath, "contact", &error);
	g_free(hashpath);
	fail_unless(!error, NULL);
	fail_unless(table != NULL, NULL);

	/***** load */
	fail_unless(osync_hashtable_load(table, &error), NULL);

	/* check for empty hashtable */
	fail_unless(osync_hashtable_num_entries(table) == 0, NULL);

	/* search non exisiting entry */
	fail_unless(osync_hashtable_get_hash(table, "doesntexist") == NULL, NULL);

	/* No hash, no call of foreach_hash() */
	osync_hashtable_foreach(table, foreach_hash, NULL);
	fail_unless(count_entries == 0, NULL);


	/* No hashs, no deleted entries */
	fail_unless(osync_hashtable_get_deleted(table) == NULL, NULL);

	/* Request slowsync, even if it's empty.. e.g. first sync. */
	fail_unless(osync_hashtable_slowsync(table, &error), NULL);
	fail_unless(!error, NULL);

	/* committed all - first sync without any entries ... */
	fail_unless(osync_hashtable_save(table, &error), NULL);
	fail_unless(!error, NULL);

	/* ref and unref */
	fail_unless(osync_hashtable_ref(table) == table, NULL);
	osync_hashtable_unref(table);

	osync_hashtable_unref(table);

	destroy_testbed(testbed);
}
END_TEST

START_TEST (hashtable_reload)
{
	OSyncError *error = NULL;
	char *testbed = setup_testbed(NULL);

	reset_hashtable_counters();

	char *hashpath = g_strdup_printf("%s%chashtable.db", testbed, G_DIR_SEPARATOR);
	OSyncHashTable *table = osync_hashtable_new(hashpath, "contact", &error);
	fail_unless(!error, NULL);
	fail_unless(table != NULL, NULL);

	/***** load */
	fail_unless(osync_hashtable_load(table, &error), NULL);

	OSyncChange *fakechange = osync_change_new(&error);

	osync_change_set_uid(fakechange, "test1");

	char *rndhash = osync_rand_str(g_random_int_range(100, 200));

	osync_change_set_hash(fakechange, rndhash);
	osync_change_set_changetype(fakechange, OSYNC_CHANGE_TYPE_ADDED);

	osync_hashtable_update_change(table, fakechange);
	osync_change_unref(fakechange);

	/*** store - commit hashtable */
	fail_unless(osync_hashtable_save(table, &error), NULL);
	fail_unless(!error, NULL);

	osync_hashtable_unref(table);
	table = NULL;

	/** reload the hashtable */
	OSyncHashTable *newtable = osync_hashtable_new(hashpath, "contact", &error);
	fail_unless(!error, NULL);
	fail_unless(newtable != NULL, NULL);

	/* 0 entries - since not loaded! */
	fail_unless(osync_hashtable_num_entries(newtable) == 0, NULL);

	/* load and count and compare hashs */
	fail_unless(osync_hashtable_load(newtable, &error), NULL);

	fail_unless(osync_hashtable_num_entries(newtable) == 1, NULL);

	const char *newhash = osync_hashtable_get_hash(newtable, "test1");
	fail_unless(newhash != NULL, NULL);
	fail_unless(!strcmp(newhash, rndhash), NULL);
	g_free(rndhash);


	g_free(hashpath);

	destroy_testbed(testbed);
}
END_TEST

void compare_uid_hash(const char *uid, const char *hash, void *data)
{
	fail_unless(!strcmp(uid, hash), NULL);
}

START_TEST (hashtable_stress)
{
	OSyncError *error = NULL;
	char *testbed = setup_testbed(NULL);

	reset_hashtable_counters();

	char *hashpath = g_strdup_printf("%s%chashtable.db", testbed, G_DIR_SEPARATOR);
	OSyncHashTable *table = osync_hashtable_new(hashpath, "contact", &error);
	fail_unless(!error, NULL);
	fail_unless(table != NULL, NULL);

	/***** load */
	fail_unless(osync_hashtable_load(table, &error), NULL);

	/* commit 10k changes with uniques values for UID. And this value also
	   also as HASH. So we can validate the result with UID == HASH. */
	unsigned int i = 0;
	unsigned int NUMENTRIES = 10000;
	for (i=0; i < NUMENTRIES; i++) {
		char *value = g_strdup_printf("%u", i);
		OSyncChange *fakechange = osync_change_new(&error);

		/* UID == HASH */
		osync_change_set_uid(fakechange, value);
		osync_change_set_hash(fakechange, value);

		osync_change_set_changetype(fakechange, OSYNC_CHANGE_TYPE_ADDED);
		osync_hashtable_update_change(table, fakechange);

		osync_change_unref(fakechange);
		g_free(value);
	}

	/*** store - commit 10k hash entries to hashtable */
	fail_unless(osync_hashtable_save(table, &error), NULL);
	fail_unless(!error, NULL);

	osync_hashtable_unref(table);
	table = NULL;

	/** reload the hashtable */
	OSyncHashTable *newtable = osync_hashtable_new(hashpath, "contact", &error);
	fail_unless(!error, NULL);
	fail_unless(newtable != NULL, NULL);

	/* 0 entries - since not loaded! */
	fail_unless(osync_hashtable_num_entries(newtable) == 0, NULL);

	/* load and count and compare hashs */
	fail_unless(osync_hashtable_load(newtable, &error), NULL);

	fail_unless(osync_hashtable_num_entries(newtable) == NUMENTRIES, "Only %i of %i\n", osync_hashtable_num_entries(newtable), NUMENTRIES);

	osync_hashtable_foreach(newtable, compare_uid_hash, NULL);

	g_free(hashpath);

	destroy_testbed(testbed);
}
END_TEST

Suite *env_suite(void)
{
	Suite *s = suite_create("Hashtable");

	create_case(s, "hashtable_new", hashtable_new);
	create_case(s, "hashtable_reload", hashtable_reload);
	create_case(s, "hashtable_stress", hashtable_stress);

	return s;
}

int main(void)
{
	int nf;

	check_env();
	
	Suite *s = env_suite();
	
	SRunner *sr;
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
