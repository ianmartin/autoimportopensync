/*
 * evolution2_sync - A plugin for the opensync framework
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
 
#include "evolution2_sync.h"

/*Load the state from a xml file and return it in the conn struct*/
osync_bool evo2_parse_settings(evo_environment *env, char *data, int size)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);

	//set defaults
	env->addressbook_path = NULL;
	env->calendar_path = NULL;
	env->tasks_path = NULL;

	doc = xmlParseMemory(data, size);

	if (!doc) {
		osync_debug("EVO2-SYNC", 1, "Could not parse data!\n");
		return FALSE;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_debug("EVO2-SYNC", 0, "data seems to be empty");
		xmlFreeDoc(doc);
		return FALSE;
	}

	if (xmlStrcmp(cur->name, "config")) {
		osync_debug("EVO2-SYNC", 0, "data seems not to be a valid configdata.\n");
		xmlFreeDoc(doc);
		return FALSE;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"adress_path")) {
				env->addressbook_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_path")) {
				env->calendar_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"tasks_path")) {
				env->tasks_path = g_strdup(str);	
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}
