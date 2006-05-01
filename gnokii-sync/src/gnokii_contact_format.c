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
	}

	// Number
	if (contact->number) {
		current = xmlNewChild(root, NULL, (xmlChar*)"Telephone", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*) contact->number);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*) "HOME");
	}

	// Record Date / Revision
	if (contact->date.year) {
		// TODO timezone
		tmp = g_strdup_pr/ntf("%04d-%02d-%02dT%02d:%02d:%02d",
				contact->date.year + 1900,
				contact->date.month + 1,
				contact->date.day,
				contact->date.hour,
				contact->date.minute,
				contact->date.second);
		
		current = xmlNewChild(root, NULL, (xmlChar*)"Revision", NULL);
		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*) tmp);
	}

	for (i=1; i <= contact->subentries_count; i++) {

		switch (contact->subentries[i].entry_type) {
			case GN_PHONEBOOK_ENTRY_Name:
				break;
			case GN_PHONEBOOK_ENTRY_Email:
				current = xmlNewChild(root, NULL, (xmlChar*)"EMail", NULL);
				xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_Postal:
				current = xmlNewChild(root, NULL, (xmlChar*)"Address", NULL);
				// FIXME: cellphone doesn't store addresses in a detailed structure...
				xmlNewChild(current, NULL, (xmlChar*)"Street", (xmlChar*)contact->subentries[i].data.number);
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

		xmlNewChild(current, NULL, (xmlChar*)"Content", (xmlChar*)contact->subentries[i].data.number);

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

//	char *tmp;
//	xmlNode *cur = NULL;
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);

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

	// TODO  xml -> gn_phonebook_entry

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

