/*
 * xml - A plugin for xml objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "xml.h"

static OSyncConvCmpResult compare_xml(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

static void destroy_xml(char *data, size_t size)
{
	xmlFreeDoc((xmlDoc *)data);
}

void get_info(OSyncFormatEnv *env)
{
	//Test
	printf("tests2\n");
	xmlDoc *doc = xmlNewDoc("1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");
	osxml_node_add(root, "test1", "data1");
	
	xmlNode *sub = osxml_node_add(root, "test2", "data2");
	osxml_node_add(sub, "test4", "data4");
	
	int i;
	for (i = 0; i < 1000; i++) {
		sub = osxml_node_add(root, "test2", "data2");
		osxml_node_add(sub, "test3", "data3");
	}
	
	for (i = 0; i < 100; i++) {
		sub = osxml_node_add(root, "test2", "data7");
		osxml_node_add(sub, "test3", "data3");
	}
	
	sub = osxml_node_add(root, "test2", "data2");
	osxml_node_add(sub, "test5", "data5");
	
	///sfsfsdfsdf
	xmlDoc *doc2 = xmlNewDoc("1.0");
	xmlNode *root2 = osxml_node_add_root(doc2, "contact");
	
	xmlNode *sub2;
	for (i = 0; i < 100; i++) {
		sub2 = osxml_node_add(root2, "test2", "data7");
		osxml_node_add(sub2, "test3", "data3");
	}
	
	for (i = 0; i < 1000; i++) {
		sub2 = osxml_node_add(root2, "test2", "data2");
		osxml_node_add(sub2, "test3", "data3");
	}
	

	
	sub2 = osxml_node_add(root2, "test2", "data2");
	osxml_node_add(sub2, "test4", "data4");
	
	sub = osxml_node_add(root2, "test2", "data2");
	osxml_node_add(sub, "test5", "data5");
	
	osxml_node_add(root2, "test1", "data1");
	
	//int *i[] =
	
	/*OSyncXMLScore score[] =
	{
	{1, "/contact/test2"},
	{1, "/contact/test1"},
	{0, NULL}
	};*/
	//memset(score, 0, sizeof(score));
	//score[0] = (OSyncXMLScore){1, "/contact/test2"};
	//score[1] = (OSyncXMLScore){5, "/contact/test1"};
	//score[2] = NULL;
	
	//printf("%i\n", osxml_compare(doc, doc2, score));
	
	printf("done tests2\n");
	osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *mxml = osync_conv_register_objformat(env, "contact", "x-opensync-xml");
	osync_conv_format_set_compare_func(mxml, compare_xml);
	osync_conv_format_set_destroy_func(mxml, destroy_xml);
}
