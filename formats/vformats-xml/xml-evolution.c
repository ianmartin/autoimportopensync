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
#include <glib.h>

static osync_bool init_x_evo_to_xml(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	/*xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root)
		return FALSE;
	if (!(root = root->parent))
		return FALSE;
	
	osync_trace(TRACE_INTERNAL, "We have to following unknown nodes:");
	xmlXPathObject *object = osxml_get_unknown_nodes(root->doc);
	xmlNodeSet *nodes = object->nodesetval;
	
	int size = (nodes) ? nodes->nodeNr : 0;
	
	osync_trace(TRACE_INTERNAL, "found %i nodes", size);
	OSyncXMLEncoding encoding;
	
	int i;
	for(i = 0; i < size; i++) {
		if (!nodes->nodeTab[i])
			continue;
		
		osync_trace(TRACE_INTERNAL, "found node %s", nodes->nodeTab[i]->name);
		if (!strcmp(nodes->nodeTab[i]->name, "UnknownNode")) {
			char *name = osxml_find_node(nodes->nodeTab[i], "NodeName");
			char *content = osxml_find_node(nodes->nodeTab[i], "Content");
			osync_trace(TRACE_INTERNAL, "Name is %s", name);
			xmlNode *current = NULL;
			
			if (!strcmp(name, "X-AIM-AIM")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "IM-AIM", content, encoding);
			}
			
			if (!strcmp(name, "X-ICQ-ICQ")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "IM-ICQ", content, encoding);
			}
			
			if (!strcmp(name, "X-YAHOO-YAHOO")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "IM-Yahoo", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-FILE-AS")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "FileAs", content, encoding);
			}
			
			if (!strcmp(name, "X-GROUPWISE-GROUPWISE")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "GroupwiseDirectory", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-MANAGER")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "Manager", content, encoding);
			}
			
			if (!strcmp(name, "X-MOZILLA-HTML")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "WantsHtml", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-ASSISTANT")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "Assistant", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-SPOUSE")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "Spouse", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-BLOG-URL")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "BlogUrl", content, encoding);
			}
			
			if (!strcmp(name, "CALURI")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "CalendarUrl", content, encoding);
			}
			
			if (!strcmp(name, "FBURL")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "FreeBusyUrl", content, encoding);
			}
			
			if (!strcmp(name, "X-EVOLUTION-VIDEO-URL")) {
				current = xmlNewChild(root, NULL, "", NULL);
				osxml_node_set(current, "VideoUrl", content, encoding);
			}
			
			if (current) {
				xmlUnlinkNode(nodes->nodeTab[i]);
				xmlFreeNode(nodes->nodeTab[i]);
				nodes->nodeTab[i] = NULL;
			}
		} else {
			osxml_map_unknown_param(nodes->nodeTab[i], "X-EVOLUTION-UI-SLOT", "SlotID");
		}
	}*/
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

static osync_bool init_xml_to_x_evo(void *input)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, input);
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "xml-contact");
	
	osync_env_register_extension(env, "xml-contact", "evolution", init_x_evo_to_xml, init_xml_to_x_evo);
}
