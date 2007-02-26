/** @file
 *
 * kdepim-sync unit tests
 *
 * Copyright 2006 Mandriva Conectiva
 *
 * @author Gustavo De Nardin <Mandriva Conectiva S/A>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 dated
 * June, 1991.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See file COPYING for details.
 *
 * @version
 * $Id$
 */

#include <kabc/resource.h>
#include <kabc/resourcefile.h>
#include <kabc/addressbook.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include <opensync/opensync.h>

#include <check.h>
#include <stdlib.h>

// Trick for testing private parts of a class (learned from
// <http://kmymoney2.sourceforge.net/phb/unit-testing-howto.html>
// "Accessing private members")
#define private public
#include "kaddrbook.h"
#undef private

/** Temporary resource file for testing. */
#define TMPVCF "/tmp/t.vcf"

static KApplication *mApplication = 0;

static OSyncEnv *env = 0;
static OSyncGroup *grp = 0;
static OSyncMember *member = 0;
static OSyncContext *ctx = 0;
static OSyncHashTable *hashtable = 0;


/** Initialize KDE.
 */
void setup_kde()
{
	KAboutData aboutData(
		"kdepim-unit-tests",			// internal program name
		"OpenSync-KDE-plugin-unit-tests",	// displayable program name.
		"0.0",					// version string
		"OpenSync KDEPIM plugin unit tests",	// short program description
		KAboutData::License_GPL,		// license type
		"(C) 2006 Mandriva Conectiva",		// copyright statement
		0,					// any free form text
		"http://www.opensync.org",		// program home page address
		"http://www.opensync.org/newticket"	// bug report email address
		);

	KCmdLineArgs::init(&aboutData);

	if (kapp)
		mApplication = kapp;
	else {
		KApplication::disableAutoDcopRegistration();
		mApplication = new KApplication(false, false);
	}

	mApplication->disableSessionManagement();
}


/** Finalize KDE.
 */
void teardown_kde()
{
}


/** Setup minimal required Opensync structures.
 */
void setup_osync()
{
	env = osync_env_new();

	// register the objtypes before osync_group_new()
	osync_env_register_objtype(env, "contact");
	osync_env_register_objtype(env, "event");
	osync_env_register_objtype(env, "task");
	osync_env_register_objtype(env, "note");

	grp = osync_group_new(env);
	member = osync_member_new(grp);
	ctx = osync_context_new(member);
	hashtable = osync_hashtable_new();

	// put anchor.db somewhere
	osync_member_set_configdir(member, "/tmp");
}


void teardown_osync()
{
	osync_hashtable_free(hashtable);
	hashtable = 0;

	osync_context_free(ctx);
	ctx = 0;

	osync_member_free(member);
	member = 0;

	osync_group_free(grp);
	grp = 0;

	osync_env_free(env);
	env = 0;
}


void setup()
{
	setup_osync();
	setup_kde();
}

void teardown()
{
	teardown_kde();
	teardown_osync();
}


START_TEST(check_anchors)
{
	const char *anchor = "synced";
	const char *anchor_value = "true";
	char anchor_path[512 + sizeof("/anchor.db") + 1];

	snprintf(anchor_path, sizeof(anchor_path), "%s/anchor.db", osync_member_get_configdir(member));

	KContactDataSource kcds(member, hashtable);

	remove(anchor_path);

	fail_unless(!osync_anchor_compare(member, anchor, anchor_value),
		    "Anchor should not match.");
	fail_unless(!osync_member_get_slow_sync(member, "contact"),
		    "Slow sync should not be set for 'contact'.");

	fail_unless(kcds.connect(ctx),
		    "Failed connecting addrbook.");

	fail_unless(osync_anchor_compare(member, anchor, anchor_value),
		    "Anchor should match.");
	fail_unless(osync_member_get_slow_sync(member, "contact"),
		    "Slow sync should be set for 'contact'.");


	osync_anchor_update(member, anchor, anchor_value);
	osync_member_set_slow_sync(member, "contact", FALSE);
	fail_unless(kcds.connect(ctx),
		    "Failed connecting addrbook.");

	fail_unless(osync_anchor_compare(member, anchor, anchor_value),
		    "Anchor should match.");
	fail_unless(!osync_member_get_slow_sync(member, "contact"),
		    "Slow sync should not be set for 'contact'.");
}
END_TEST


START_TEST(check_contacts)
{
	const QString bug_vcard = "BEGIN:VCARD\nVERSION:2.1\nFN:A\\, B\nEND:VCARD";

	KContactDataSource kcds(member, hashtable);
	mark_point();

	KABC::AddressBook addrbook;
	mark_point();

	fail_unless(addrbook.addResource(new KABC::ResourceFile(TMPVCF)),
		    "Failed opening resource file '%s'.", TMPVCF);

	fail_unless(kcds.connect(ctx),
		    "Failed connecting addrbook.");

	KABC::Addressee a;
	a.setFormattedName("Doe, John");
	a.setGivenName("John");
	a.setFamilyName("Doe");
	mark_point();

	QString testhash = kcds.calc_hash(a);
	fail_unless(testhash == a.revision().toString(),
		    "calc_hash() failed");

	QDateTime revision = QDateTime::currentDateTime();
	a.setRevision(revision);

	fail_unless(revision.toString() == kcds.calc_hash(a),
		    "calc_hash() failed.");
	fail_unless(a.revision().toString() == kcds.calc_hash(a),
		    "calc_hash() failed.");

	KABC::Addressee c = kcds.converter->parseVCard(bug_vcard);
	bool kde_has_bug = (c.formattedName() == "A\\, B");
	fail_unless(kcds._kde_has_bug_91773() == kde_has_bug,
		    "KDE VCard parsing bug not detected.");

	OSyncChange *oc = kcds._addressee_to_change(&a);
	mark_point();

	KABC::Addressee b = kcds.converter->parseVCard(osync_change_get_data(oc));
	mark_point();

	fail_unless(a.givenName() == b.givenName());
	fail_unless(a.familyName() == b.familyName());
	fail_unless(a.revision().toString() == osync_change_get_hash(oc),
		    "Wrong hash in OSyncChange.");
	if (kcds._kde_has_bug_91773())
		fail_unless(b.formattedName() == "Doe\\, John");
	else
		fail_unless(b.formattedName() == a.formattedName());
}
END_TEST


START_TEST(check_events)
{
	//TODO;
}
END_TEST


START_TEST(check_todos)
{
	//TODO;
}
END_TEST


START_TEST(check_notes)
{
	//TODO;
}
END_TEST


Suite *test_suite(void)
{
        Suite *s = suite_create("check_kdepim");
        TCase *tcase = tcase_create("kdepim_test_case");

        suite_add_tcase(s, tcase);
	tcase_add_checked_fixture(tcase, setup, teardown);

	tcase_add_test(tcase, check_anchors);
        tcase_add_test(tcase, check_contacts);
//TODO        tcase_add_test(tcase, check_events);
//TODO        tcase_add_test(tcase, check_todos);
//TODO        tcase_add_test(tcase, check_notes);

        return s;
}


int main(void)
{
        int nf;
        Suite *s = test_suite();
        SRunner *sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        nf = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
