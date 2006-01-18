/*

   Copyright 2005 Paul Eggleton & Holger Hans Peter Freyther

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


#include "opie_format.h"
#include "opie_comms.h"

#include <opensync/opensync-xml.h>


static osync_bool conv_opie_contact_to_xml(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	contact_data *entry = (contact_data *)input;
	xmlNode *current = NULL;
	
	if (inpsize != sizeof(contact_data)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");

	//Names
	if (entry->last_name || entry->first_name) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Name", NULL);
		//Last Name
		if (entry->last_name)
			osxml_node_add(current, "LastName", entry->last_name);
	
		//First Name
		if (entry->first_name)
			osxml_node_add(current, "FirstName", entry->first_name);
	}
	
	//Company
	if(entry->company) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Organization", NULL);
		osxml_node_add(current, "Name", entry->company);
	}

/*
	//Telephones and email
	int i;
	for (i = 3; i <= 7; i++) {
		tmp = return_next_entry(entry, i);
		if (tmp) {
			if (entry->address.phoneLabel[i - 3] == 4) {
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
			} else
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
		
			xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
			g_free(tmp);
			
			switch (entry->address.phoneLabel[i - 3]) {
				case 0:
					osxml_node_add(current, "Work", NULL);
					osxml_node_add(current, "Voice", NULL);
					break;
				case 1:
					osxml_node_add(current, "Home", NULL);
					break;
				case 2:
					osxml_node_add(current, "Work", NULL);
					osxml_node_add(current, "Fax", NULL);
					break;
				case 3:
					osxml_node_add(current, "Voice", NULL);
					break;
				case 5:
					osxml_node_add(current, "Pref", NULL);
					break;
				case 6:
					osxml_node_add(current, "Pager", NULL);
					break;
				case 7:
					osxml_node_add(current, "Cellular", NULL);
					break;
			}
		}
		
	}

	//Address
	if (has_entry(entry, 8) || has_entry(entry, 9) || has_entry(entry, 10) || has_entry(entry, 11) || has_entry(entry, 12)) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Address", NULL);
		//Street
		tmp = return_next_entry(entry, 8);
		if (tmp) {
			osxml_node_add(current, "Street", tmp);
			g_free(tmp);
		}
	
		//City
		tmp = return_next_entry(entry, 9);
		if (tmp) {
			osxml_node_add(current, "City", tmp);
			g_free(tmp);
		}
		
		//Region
		tmp = return_next_entry(entry, 10);
		if (tmp) {
			osxml_node_add(current, "Region", tmp);
			g_free(tmp);
		}
		
		//Code
		tmp = return_next_entry(entry, 11);
		if (tmp) {
			osxml_node_add(current, "PostalCode", tmp);
			g_free(tmp);
		}
		
		//Country
		tmp = return_next_entry(entry, 12);
		if (tmp) {
			osxml_node_add(current, "Country", tmp);
			g_free(tmp);
		}
	}
*/	
	//Title
	if (entry->jobtitle) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Title", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->jobtitle);
	}
	
	
	//Note
	if (entry->notes)
		current = xmlNewChild(root, NULL, (xmlChar*)"Note", (xmlChar*)entry->notes);

/*
	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}
*/

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool conv_xml_to_opie_contact(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
	
	//Get the root node of the input document
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"contact")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		goto error;
	}

	/* Start the new entry */
	contact_data *entry = osync_try_malloc0(sizeof(contact_data), error);
	if (!entry)
		goto error;
	
	//Name
	xmlNode *cur = osxml_get_node(root, "Name");
	if (cur) {
		entry->first_name = osxml_find_node(cur, "LastName");
		entry->last_name = osxml_find_node(cur, "FirstName");
	}

	//Company
	cur = osxml_get_node(root, "Organization");
	if (cur)
		entry->company = osxml_find_node(cur, "Name");


/*
	//Telephone
	int i = 0;
	xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)input, "/Telephone");
	xmlNodeSet *nodes = xobj->nodesetval;
	int numnodes = (nodes) ? nodes->nodeNr : 0;
	for (i = 0; i < 4 && i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		entry->address.entry[3 + i] = (char*)osxml_find_node(cur, "Content");

		if (osxml_has_property(cur, "Work") && osxml_has_property(cur, "Voice")) {
			entry->address.phoneLabel[i] = 0;
		} else if (osxml_has_property(cur, "HOME") && !(osxml_has_property(cur, "FAX"))) {
			entry->address.phoneLabel[i] = 1;
		} else if (osxml_has_property(cur, "FAX")) {
			entry->address.phoneLabel[i] = 2;
		} else if (!(osxml_has_property(cur, "WORK")) && !(osxml_has_property(cur, "HOME")) && osxml_has_property(cur, "VOICE")) {
			entry->address.phoneLabel[i] = 3;
		} else if (osxml_has_property(cur, "PREF") && !(osxml_has_property(cur, "FAX"))) {
			entry->address.phoneLabel[i] = 5;
		} else if (osxml_has_property(cur, "PAGER")) {
			entry->address.phoneLabel[i] = 6;
		} else if (osxml_has_property(cur, "CELL")) {
			entry->address.phoneLabel[i] = 7;
		} else osync_trace(TRACE_INTERNAL, "Unknown TEL entry");
	}
	xmlXPathFreeObject(xobj);
	
	//EMail
	xobj = osxml_get_nodeset((xmlDoc *)input, "/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	int n;
	for (n = 0; i < 4 && n < numnodes; n++) {
		cur = nodes->nodeTab[n];
		entry->address.entry[3 + i] = (char*)osxml_find_node(cur, "Content");
		entry->address.phoneLabel[i] = 4;
		i++;
	}
	xmlXPathFreeObject(xobj);
	
	//Address
	cur = osxml_get_node(root, "Organization");
	if (cur) {
		entry->address.entry[8] = osxml_find_node(cur, "Address");
		entry->address.entry[9] = osxml_find_node(cur, "City");
		entry->address.entry[10] = osxml_find_node(cur, "Region");
		entry->address.entry[11] = osxml_find_node(cur, "Code");
		entry->address.entry[12] = osxml_find_node(cur, "Country");
	}
*/
	//Title
	cur = osxml_get_node(root, "Title");
	if (cur)
		entry->jobtitle = osxml_find_node(cur, "Content");

	//Note
	cur = osxml_get_node(root, "Note");
	if (cur)
		entry->notes = (char*)xmlNodeGetContent(cur);
	
/*
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
*/

	/* Now convert to the charset */
/*	for (i = 0; i < 19; i++) {*/
	  /*if (entry->address.entry[i]) {
	    char *tmp = g_convert(entry->address.entry[i], strlen(entry->address.entry[i]), conn->codepage ,"utf8", NULL, NULL, NULL);
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = tmp;
	  }
	  if (entry->address.entry[i] && !strlen(entry->address.entry[i])) {
	    free(entry->address.entry[i]);
	    entry->address.entry[i] = NULL;
	    palm_debug(conn, 3, "Address %i: %s", i, entry->address.entry[i]);
	  }*/
/*	  osync_trace(TRACE_INTERNAL, "entry %i: %s", i, entry->address.entry[i]);
	}*/
	
	*free_input = TRUE;
	*output = (void *)entry;
	*outpsize = sizeof(contact_data);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static void destroy_opie_contact(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	contact_data *entry = (contact_data *)input;
	g_assert(inpsize == sizeof(contact_data));
	
	free_contact_data(entry);	

	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "opie-contact");
	osync_env_format_set_destroy_func(env, "opie-contact", destroy_opie_contact);

	osync_env_register_converter(env, CONVERTER_CONV, "opie-contact", "xml-contact", conv_opie_contact_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "opie-contact", conv_xml_to_opie_contact);
}
