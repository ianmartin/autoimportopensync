/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"
#include "gnokii_contact_utils.h"
#include "gnokii_contact_format.h"
#include <opensync/opensync-xml.h>

/*
 * Converts the gnokii contact object type (gn_phonebook_entry) into XML.
 */
static osync_bool conv_gnokii_contact_to_xml(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, output, outpsize, free_input, error);

	int i;
	char *tmp = NULL;
	xmlNode *current = NULL;

	gn_phonebook_entry *contact = (gn_phonebook_entry *) input;

	if (inpsize != sizeof(gn_phonebook_entry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	xmlDoc *doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");

	// Name
	if (contact->name) {
		current = xmlNewChild(root, NULL, (xmlChar*)"FormattedName", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*) contact->name);

		// FIXME: evo2 workaround - evo2 requires a Name / N filed :(
		current = xmlNewChild(root, NULL, (xmlChar*)"Name", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"FirstName", (xmlChar*) contact->name);

	}

	// Group
	if (contact->caller_group != GN_PHONEBOOK_GROUP_None) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);

		switch (contact->caller_group) {
			case GN_PHONEBOOK_GROUP_Family:
				tmp = g_strdup("Family");
				break;
			case GN_PHONEBOOK_GROUP_Vips:
				tmp = g_strdup("VIP");	// evo2 needs VIP not VIPs
				break;
			case GN_PHONEBOOK_GROUP_Friends:
				tmp = g_strdup("Friends");
				break;
			case GN_PHONEBOOK_GROUP_Work:
				tmp = g_strdup("Work");
				break;
			case GN_PHONEBOOK_GROUP_Others:
				tmp = g_strdup("Others");
				break;
			case GN_PHONEBOOK_GROUP_None:
			default:
				break;
		}

		xmlNewChild(current, NULL, (xmlChar*)"Category", (xmlChar*) tmp);

		g_free(tmp);
	}

	
	// Record Date / Revision
	if (contact->date.year) {
		// TODO timezone
		tmp = g_strdup_printf("%04d-%02d-%02dT%02d:%02d:%02d",
				contact->date.year + 1900,
				contact->date.month + 1,
				contact->date.day,
				contact->date.hour,
				contact->date.minute,
				contact->date.second);
		
		current = xmlNewChild(root, NULL, (xmlChar*)"Revision", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*) tmp);

		g_free(tmp);
	}

	// subentries
	for (i=0; i <= contact->subentries_count; i++) {

		switch (contact->subentries[i].entry_type) {
			case GN_PHONEBOOK_ENTRY_Name:
				break;
			case GN_PHONEBOOK_ENTRY_Email:
				current = xmlNewChild(root, NULL, (xmlChar*)"EMail", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_Postal:
				current = xmlNewChild(root, NULL, (xmlChar*)"AddressLabel", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_Number:
				current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_Group:
				current = xmlNewChild(root, NULL, (xmlChar*)"Categories", NULL);
				xmlNewChild(current, NULL, (xmlChar*) "Category", (xmlChar*)contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_URL:
				current = xmlNewChild(root, NULL, (xmlChar*)"Url", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);

				break;
			case GN_PHONEBOOK_ENTRY_Note:
				current = xmlNewChild(root, NULL, (xmlChar*)"Note", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);
				break;
			// Unused	
			case GN_PHONEBOOK_ENTRY_Ringtone:
			case GN_PHONEBOOK_ENTRY_Pointer:
			case GN_PHONEBOOK_ENTRY_Logo:
			case GN_PHONEBOOK_ENTRY_Date:
			case GN_PHONEBOOK_ENTRY_LogoSwitch:	
			case GN_PHONEBOOK_ENTRY_RingtoneAdv:	
				break;
		}

		if (contact->subentries[i].entry_type != GN_PHONEBOOK_ENTRY_Number)
			continue;

		switch (contact->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				xmlNewChild(current, NULL, (xmlChar*) "Type", (xmlChar*) "HOME");
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				xmlNewChild(current, NULL, (xmlChar*) "Type", (xmlChar*) "CELL");
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				xmlNewChild(current, NULL, (xmlChar*) "Type", (xmlChar*) "FAX");
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				xmlNewChild(current, NULL, (xmlChar*) "Type", (xmlChar*) "WORK");
				break;
			case GN_PHONEBOOK_NUMBER_General:
				xmlNewChild(current, NULL, (xmlChar*) "Type", (xmlChar*) "GERNERAL");
				break;
			default:
				break;	
		}	
	}	

	*free_input = TRUE;
	*output = (char *)doc;
	*outpsize = sizeof(doc);

#ifndef HIDE_SENSITIVE
	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)doc));
#endif	
	
	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;
}

/* 
 * Converts from XML to the gnokii contact object type (gn_phonebook_entry).
 */  
static osync_bool conv_xml_contact_to_gnokii(void *conv_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, conv_data, input, inpsize, 
			output, outpsize, free_input, error);
#ifndef HIDE_SENSITIVE
	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", osxml_write_to_string((xmlDoc *)input));
#endif	

	char *tmp = NULL;
	xmlNode *cur = NULL;
	xmlNode *sub = NULL;
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	xmlXPathObject *xobj = NULL; 
	xmlNodeSet *nodes = NULL; 
	int numnodes = 0;
	int subcount = 0;
	int i;


	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}

	if (xmlStrcmp(root->name, (const xmlChar *) "contact")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong (contact) xml root element");
		goto error;
	}

	// prepare contact 
	gn_phonebook_entry *contact = NULL;
	contact = (gn_phonebook_entry *) malloc(sizeof(gn_phonebook_entry));

	memset(contact, 0, sizeof(gn_phonebook_entry));

	// FormattedName - XXX Also Node "Name"?
	 cur = osxml_get_node(root, "FormattedName");
	 if (cur)
		 strncpy(contact->name, (char *) xmlNodeGetContent(cur), GN_PHONEBOOK_NAME_MAX_LENGTH);
	
	// Telephone
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/Telephone");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		contact->subentries[subcount].entry_type = GN_PHONEBOOK_ENTRY_Number;
		
		sub = osxml_get_node(cur, "Content");
		tmp = gnokii_contact_util_cleannumber((char *) xmlNodeGetContent(sub));
		strncpy(contact->subentries[subcount].data.number, tmp, GN_PHONEBOOK_NAME_MAX_LENGTH);

		// FIXME - is there any other cool osync_xml function?!
		sub = osxml_get_node(cur, "Type");
		if (sub) {
			tmp = (char *) xmlNodeGetContent(sub);
			if (!strcasecmp(tmp, "WORK"))
				contact->subentries[subcount].number_type = GN_PHONEBOOK_NUMBER_Work; 
			else if (!strcasecmp(tmp, "HOME"))
				contact->subentries[subcount].number_type = GN_PHONEBOOK_NUMBER_Home; 
			else if (!strcasecmp(tmp, "FAX"))
				contact->subentries[subcount].number_type = GN_PHONEBOOK_NUMBER_Fax; 
			else if (!strcasecmp(tmp, "CELL"))
				contact->subentries[subcount].number_type = GN_PHONEBOOK_NUMBER_Mobile; 
			else
				contact->subentries[subcount].number_type = GN_PHONEBOOK_NUMBER_General;
		}
		subcount++;
		g_free(tmp);
	}
	xmlXPathFreeObject(xobj);

	// URL 
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/Url");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];

		contact->subentries[subcount].entry_type = GN_PHONEBOOK_ENTRY_URL;

		sub = osxml_get_node(cur, "Content");

		strncpy(contact->subentries[subcount].data.number, (char *) xmlNodeGetContent(sub), GN_PHONEBOOK_NAME_MAX_LENGTH);

		subcount++;
	}
	xmlXPathFreeObject(xobj);

	// EMail
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];

		contact->subentries[subcount].entry_type = GN_PHONEBOOK_ENTRY_Email;
		sub = osxml_get_node(cur, "Content");
		strncpy(contact->subentries[subcount].data.number, (char *) xmlNodeGetContent(sub), GN_PHONEBOOK_NAME_MAX_LENGTH);

		subcount++;
	}
	xmlXPathFreeObject(xobj);

	// Note 
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/Note");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];

		contact->subentries[subcount].entry_type = GN_PHONEBOOK_ENTRY_Note;
		sub = osxml_get_node(cur, "Content");
		strncpy(contact->subentries[subcount].data.number, (char *) xmlNodeGetContent(sub), GN_PHONEBOOK_NAME_MAX_LENGTH);

		subcount++;
	}
	xmlXPathFreeObject(xobj);

	// Category / Callergroup
	// TODO - fix category - group 
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/Categories");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;

	osync_trace(TRACE_INTERNAL, "categories: %i", numnodes);

	contact->caller_group = GN_PHONEBOOK_GROUP_None; 
	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		
		tmp = (char *) xmlNodeGetContent(cur);
		if (!strcasecmp(tmp, "FAMILY")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Family; 
		} else if (!strcasecmp(tmp, "VIPS") || !strcasecmp(tmp, "VIP")) {	// we handle VIP and VIPs as VIP type needed for evo2 
			contact->caller_group = GN_PHONEBOOK_GROUP_Vips;
		} else if (!strcasecmp(tmp, "FRIENDS")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Friends;
		} else if (!strcasecmp(tmp, "WORK")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Work;
		} else if (!strcasecmp(tmp, "OTHERS")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Others;
		}
		
	}
	xmlXPathFreeObject(xobj);

	// TODO: Addresss, Organization?, Group 

	// Adress
	xobj = osxml_get_nodeset((xmlDoc *)input, "/contact/AddressLabel");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for (i=0; i < numnodes; i++) {
		cur = nodes->nodeTab[i];
		
		contact->subentries[subcount].entry_type = GN_PHONEBOOK_ENTRY_Postal;

		sub = osxml_get_node(cur, "Content");

		if (sub)
			strncpy(contact->subentries[subcount].data.number, (char *) xmlNodeGetContent(sub), GN_PHONEBOOK_NAME_MAX_LENGTH); 

		subcount++;
	}
	xmlXPathFreeObject(xobj);

	contact->subentries_count = subcount;

	osync_trace(TRACE_INTERNAL, "TEST: name: %s\n", contact->name);
	 
	*free_input = TRUE;
	*output = (void *)contact;
	*outpsize = sizeof(gn_phonebook_entry);

	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static void destroy_gnokii_contact(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	gn_phonebook_entry *contact = (gn_phonebook_entry *) input;
	
	if (inpsize != sizeof(gn_phonebook_entry)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Wrong size!", __func__);
		return;
	}

	g_free(contact);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*
 * Print the gnokii format in a human readable form.
 */ 
/*
static char *print_gnokii_contact(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);

	char *output = NULL;
	gn_phonebook_entry *contact = (gn_phonebook_entry *) osync_change_get_data(change);

	// TODO

	osync_trace(TRACE_EXIT, "%s: %s", __func__, output);
	return output;
}
*/


void gnokii_contact_format_get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	
	//Tell OpenSync that we want to register a new format
	osync_env_register_objformat(env, "contact", "gnokii-contact");
	//Now we can set the function on your format we have created above
//	osync_env_format_set_compare_func(env, "gnokii-contact", compare_format1);
//	osync_env_format_set_duplicate_func(env, "gnokii-contact", duplicate_format1);
	osync_env_format_set_destroy_func(env, "gnokii-contact", destroy_gnokii_contact);
//	osync_env_format_set_print_func(env, "gnokii-contact", print_gnokii_contact);
	
	osync_env_register_converter(env, CONVERTER_CONV, "gnokii-contact", "xml-contact", conv_gnokii_contact_to_xml);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact", "gnokii-contact", conv_xml_contact_to_gnokii);

}

