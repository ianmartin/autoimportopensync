/*
 * file-sync - A plugin for the opensync framework
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
 
#include "file_sync.h"

/*Load the state from a xml file and return it in the conn struct*/
osync_bool fs_parse_settings(filesyncinfo *env, xmlDocPtr doc, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	xmlNodePtr cur;

	//set defaults
	env->path = NULL;
	env->recursive = TRUE;

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	if (strcmp (cur->name, "opensync-plugin-config") != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	cur = cur->children;

	while (cur != NULL) {
		if (!strcmp (cur->name, "option")) {
			char *name, *value;
			
			if ((name = xmlGetProp (cur, "name"))) {
				if (!strcmp (name, "recursive")) {
					if ((value = xmlGetProp (cur, "value")) && !strcmp (value, "true"))
						env->recursive = TRUE;
					else
						env->recursive = FALSE;
					xmlFree (value);
				} else if (!strcmp (name, "path")) {
					env->path = xmlNodeGetContent (cur);
				}
				xmlFree (name);
			}
		}
		cur = cur->next;
	}
	
	if (!env->path) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Path not set");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}


static xmlDocPtr
fs_get_default_config (void)
{
	xmlNodePtr root, node;
	xmlDocPtr doc;
	
	doc = xmlNewDoc ("1.0");
	
	root = xmlNewDocNode (doc, NULL, "opensync-plugin-config", NULL);
	xmlSetProp (root, "plugin", "file");
	xmlSetProp (root, "version", "1.0");
	xmlDocSetRootElement (doc, root);
	
	node = xmlNewChild (root, NULL, "option", NULL);
	xmlSetProp (node, "type", "path");
	xmlSetProp (node, "mode", "directory");
	xmlSetProp (node, "name", "path");
	xmlSetProp (node, "label", "Directory");
	
	node = xmlNewChild (root, NULL, "option", NULL);
	xmlSetProp (node, "type", "bool");
	xmlSetProp (node, "name", "recursive");
	xmlSetProp (node, "label", "Recursive");
	xmlSetProp (node, "value", "true");
	
	return doc;
}

xmlDocPtr
fs_get_config (const char *configdir)
{
	xmlDocPtr doc;
	char *path;
	
	path = g_strdup_printf ("%s/file.xml", configdir);
	if (!(doc = xmlParseFile (path)))
		doc = fs_get_default_config ();
	g_free (path);
	
	return doc;
}

osync_bool
fs_set_config (const char *configdir, xmlDocPtr doc)
{
	char *path;
	
	path = g_strdup_printf ("%s/file.xml", configdir);
	xmlSaveFile (path, doc);
	g_free (path);
	
	return TRUE;
}
