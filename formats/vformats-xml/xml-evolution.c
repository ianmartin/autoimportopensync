/*
 * x-evo - A plugin for evolution extensions for the opensync framework
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
 
#include "opensync-xml.h"

static osync_bool conv_x_evo_to_xml(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	/*xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root)
		return FALSE;
	
	printf("We have to following unknown nodes:\n");
	xmlXPathObject *object = osxml_get_unknown_nodes(root->doc);
	xmlNodeSet *nodes = object->nodesetval;
	
	int size = (nodes) ? nodes->nodeNr : 0;
	
	printf("found %i nodes\n", size);
	
	int i;
	for(i = 0; i < size; i++) {
		g_assert(nodes->nodeTab[i]);
		printf("found node %s\n", nodes->nodeTab[i]->name);
	}
	
	printf("done searching\n");*/
	return TRUE;
}

static osync_bool conv_xml_to_x_evo(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);

	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "xml-contact");
	
	osync_conv_register_extension(env, "xml-contact", conv_x_evo_to_xml, conv_xml_to_x_evo);
}
