/*
 * parser_test - test parsing of tomboy notes
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include <string.h>
#include <check.h>
#include <stdio.h>

#include <opensync/opensync.h>
#include <opensync/opensync-xmlformat.h>

#include "../src/tomboy_note.h"
#include "../src/tomboy_note_internal.h"

xmlDocPtr good_doc;
xmlDocPtr bad_doc;

static char * good_content = "﻿<?xml version=\"1.0\" encoding=\"utf-8\"?><note version=\"0.3\" xmlns:link=\"http://beatniksoftware.com/tomboy/link\"\
		xmlns:size=\"http://beatniksoftware.com/tomboy/size\" xmlns=\"http://beatniksoftware.com/tomboy\">\
			<title>Title Test</title>\
			<text xml:space=\"preserve\">\
				<note-content version=\"0.1\">Headline Test\n\n<list>\
						<list-item dir=\"ltr\"><monospace>Test 1</monospace></list-item>\
						<list-item dir=\"ltr\"><size:large><italic>Test 2</italic></size:large></list-item>\
						<list-item dir=\"ltr\"><strikethrough>Test 3</strikethrough></list-item>\
						<list-item dir=\"ltr\">This is a <highlight>long</highlight> Test 4</list-item>\
						<list-item dir=\"ltr\">ABC <link:internal>Test 5</link:internal> DEF</list-item>\
					</list>\
\nadditional content Test\
				</note-content>\
			</text>\
			<last-change-date>2008-06-13T08:38:39.6943800+02:00</last-change-date>\
			<last-metadata-change-date>2008-06-13T08:38:39.6943800+02:00</last-metadata-change-date>\
			<create-date>2007-11-29T17:28:14.0534010+01:00</create-date>\
			<cursor-position>73</cursor-position><width>450</width>\
			<height>360</height><x>475</x><y>380</y>\
			<tags>\
    			<tag>system:notebook:test</tag>\
			</tags>\
			<open-on-startup>False</open-on-startup>\
		</note>";

static char * bad_content = "<test1><test2></test2><test3>Test4</test3></test1>";

void setup() {
	good_doc = xmlReadMemory(good_content, strlen(good_content), "noname.xml", NULL, 0);
	bad_doc = xmlReadMemory(bad_content, strlen(bad_content), "noname.xml", NULL, 0);
}

void teardown() {
	// crash!
	xmlFreeDoc(good_doc);
}


START_TEST (tomboynote_test_validate)
{

	xmlNodePtr rootnode;
	
	//xmlDocDump(stdout, doc);
	rootnode = xmlDocGetRootElement(good_doc);
	//xmlFreeNode(rootnode); // <-- crash
	//printf( "rootnode: type %d name %s %d\n", rootnode->type,rootnode->name, xmlStrEqual(rootnode->name, BAD_CAST "note"));
	fail_unless( tomboynote_validate(good_doc) );
	fail_if( tomboynote_validate(bad_doc) );

}
END_TEST

START_TEST (tomboynote_test_parse_content) 
{
	GString *str;
	
	str = g_string_new("");
	tomboynote_parse_content(good_doc, str);
	//printf("Good: len %d %s\n", str->len, str->str );
	
	g_string_free(str,TRUE);
	str = g_string_new("");
	tomboynote_parse_content_node(xmlDocGetRootElement(good_doc),str);
	//printf("Bad: len %d %s\n", str->len, str->str );
	fail_unless( str->len == 0 );
	
	
}
END_TEST

START_TEST (tomboynote_test_parse_nodes) {
	const char * node_text;
	
	node_text = tomboynote_parse_node(good_doc, "title");
	fail_if( node_text == NULL );
	fail_if( strcmp(node_text, "Title Test") );
	node_text = tomboynote_parse_node(bad_doc, "title");
	fail_if( node_text != NULL );
	
}
END_TEST

START_TEST (tomboynote_test_parse_tags) {
	GList * list;
	list = tomboynote_parse_tags(good_doc);
	fail_if( g_list_length(list) != 1 );
}
END_TEST

START_TEST (tomboynote_test_converter) {
	osync_bool free;
	char *output;
	unsigned int outpsize;
	OSyncError *error;
	//unsigned int size;
	//char *cstr;
	
	fail_unless( conv_tomboynote_to_xmlformat(good_content, strlen(good_content), &output, &outpsize, &free, NULL, NULL) );
	OSyncXMLFormat * xmlformat = (OSyncXMLFormat*)output;
	//osync_xmlformat_assemble(xmlformat, &cstr, &size);
	//printf("Output: %s\n", cstr);
	fail_unless( osync_xmlformat_validate(xmlformat,&error) );
}
END_TEST

START_TEST (tomboynote_test_detector) {
	fail_unless( detect_tomboynote(good_content, strlen(good_content)) );
	fail_if( detect_tomboynote(bad_content, strlen(bad_content)));
}
END_TEST

Suite *  tomboynote_suite() {
	Suite *s = suite_create ("Tomboy Note");
	TCase *tc_parser = tcase_create ("tomboynote_parser");
	tcase_add_checked_fixture (tc_parser, setup, teardown);
	tcase_add_test (tc_parser, tomboynote_test_validate);
	tcase_add_test (tc_parser, tomboynote_test_parse_content);
	tcase_add_test (tc_parser, tomboynote_test_parse_nodes);
	tcase_add_test (tc_parser, tomboynote_test_parse_tags);	
	tcase_add_test (tc_parser, tomboynote_test_converter);
	tcase_add_test (tc_parser, tomboynote_test_detector);
	suite_add_tcase (s, tc_parser);
	return s;
}

int main() {

	int number_failed;
	Suite *s = tomboynote_suite ();
	SRunner *sr = srunner_create (s);
	srunner_run_all (sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
