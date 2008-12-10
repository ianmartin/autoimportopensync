#include <stdlib.h>

#include "support.h"
#include "engine_support.h"

#include <opensync/opensync-group.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-client.h>
#include <opensync/opensync-engine.h>
#include <opensync/opensync-context.h>

#include "opensync/engine/opensync_engine_internals.h"
#include "opensync/engine/opensync_engine_private.h"

#include "opensync/group/opensync_group_internals.h"
#include "opensync/client/opensync_client_internals.h"

void conflict_callback_fail(OSyncEngine *engine, OSyncMappingEngine *mapping_engine, void *userdata)
{
	fail(NULL);
}

/* Functional Test: Mapping with multiple SAME&SIMILAR entries
 *
 * (Content)    Entry A       Entry B           
 * Member 1      xxx           xxy
 * Member 2:     xxx
 *
 * The mock-format compare function will return "SIMILAR" when files
 * have the same size, but different content (e.g. "xxx", "xyz").
 *
 * On a Slow-Sync the Mapping-Engine MUST NOT cause a conflict in this
 * scenario, since Entry A on Member 1 and 2 are the same. And Entry B
 * is unmapped, and would be created as new change (ADDED) in Member 2.
 *
 * This is a regression test for #883:
 * Engine mapped several times different entries/changes and marked the
 * mapping with a "conflict" and didn't released Entry B.
 *
 */

START_TEST (mapping_engine_same_similar_conflict)
{
	char *testbed = setup_testbed("mapping_engine");
	char *formatdir = g_strdup_printf("%s/formats", testbed);
	char *plugindir = g_strdup_printf("%s/plugins", testbed);


	osync_testing_system_abort("cp entryA.txt entryB.txt data1/");

	osync_testing_system_abort("cp entryA.txt data2/");

	g_setenv("MOCK_FORMAT_PATH_COMPARE_NO", "1", TRUE);

	
	OSyncError *error = NULL;
	OSyncGroup *group = osync_group_new(&error);
	fail_unless(group != NULL, NULL);
	fail_unless(error == NULL, NULL);
	
	osync_group_set_schemadir(group, testbed);
	fail_unless(osync_group_load(group, "configs/group", &error), NULL);
	fail_unless(error == NULL, NULL);
	
	OSyncEngine *engine = osync_engine_new(group, &error);
	fail_unless(engine != NULL, NULL);
	fail_unless(error == NULL, NULL);
	osync_group_unref(group);


	osync_engine_set_conflict_callback(engine, conflict_callback_fail, NULL);

	osync_engine_set_schemadir(engine, testbed);
	osync_engine_set_plugindir(engine, plugindir);
	osync_engine_set_formatdir(engine, formatdir);
	
	fail_unless(osync_engine_initialize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	fail_unless(osync_engine_synchronize_and_block(engine, &error), NULL);
	fail_unless(error == NULL, NULL);

	fail_unless(osync_testing_diff("data1", "data2"), NULL);
	
	fail_unless(osync_engine_finalize(engine, &error), NULL);
	fail_unless(error == NULL, NULL);
	
	osync_engine_unref(engine);

	
	g_free(formatdir);
	g_free(plugindir);

	destroy_testbed(testbed);
}
END_TEST

Suite *mapping_engine_suite(void)
{
	Suite *s = suite_create("MappingEngine");
	
	create_case(s, "mapping_engine_same_similar_conflict", mapping_engine_same_similar_conflict);
	
	return s;
}

int main(void)
{
	int nf;

	check_env();

	Suite *s = mapping_engine_suite();
	
	SRunner *sr;
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
