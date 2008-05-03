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

#include <string.h>
#include <glib.h>

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-time.h>

#include "gnokii_contact_utils.h"

/*
 * Converts the gnokii contact object type (gn_phonebook_entry) into XML.
 */
static osync_bool conv_gnokii_contact_to_xmlformat(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);

	OSyncXMLField *xmlfield = NULL;

	int i;
	char *tmp = NULL;

	gn_phonebook_entry *contact = (gn_phonebook_entry *) input;

	if (inpsize != sizeof(gn_phonebook_entry)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return FALSE;
	}

	OSyncXMLFormat *xmlformat = osync_xmlformat_new("contact", error);


	// Name
	if (strlen(contact->name)) {

		xmlfield = osync_xmlfield_new(xmlformat, "FormattedName", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", contact->name);

		// FIXME: evo2 workaround - evo2 requires a Name / N filed :(
		/*
		xmlfield = osync_xmlfield_new(xmlformat, "Name", error);
		osync_xmlfield_set_key_value(xmlfield, "FirstName", contact->name);
		*/
	}

	// Group
	if (contact->caller_group != GN_PHONEBOOK_GROUP_None) {
		xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);

		switch (contact->caller_group) {
			case GN_PHONEBOOK_GROUP_Family:
				osync_xmlfield_set_key_value(xmlfield, "Category", "Family");
				break;
			case GN_PHONEBOOK_GROUP_Vips:
				osync_xmlfield_set_key_value(xmlfield, "Category", "VIP");
				break;
			case GN_PHONEBOOK_GROUP_Friends:
				osync_xmlfield_set_key_value(xmlfield, "Category", "Friends");
				break;
			case GN_PHONEBOOK_GROUP_Work:
				osync_xmlfield_set_key_value(xmlfield, "Category", "Work");
				break;
			case GN_PHONEBOOK_GROUP_Others:
				osync_xmlfield_set_key_value(xmlfield, "Category", "Others");
				break;
			case GN_PHONEBOOK_GROUP_None:
				break;
		}

	}

	
	// Record Date / Revision
	if (contact->date.year) {
		// TODO timezone
		tmp = g_strdup_printf("%04d%02d%02dT%02d%02d%02d",
				contact->date.year + 1900,
				contact->date.month + 1,
				contact->date.day,
				contact->date.hour,
				contact->date.minute,
				contact->date.second);
		
		xmlfield = osync_xmlfield_new(xmlformat, "Revision", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", tmp);

		g_free(tmp);
	}

	// subentries
	for (i=0; i <= contact->subentries_count; i++) {

		OSyncXMLField *phonefield = NULL;

		switch (contact->subentries[i].entry_type) {
			case GN_PHONEBOOK_ENTRY_Name:
				break;
			case GN_PHONEBOOK_ENTRY_Email:
				phonefield = osync_xmlfield_new(xmlformat, "EMail", error);
				break;
			case GN_PHONEBOOK_ENTRY_Postal:
				phonefield = osync_xmlfield_new(xmlformat, "AddressLabel", error);
				break;
			case GN_PHONEBOOK_ENTRY_Number:
				phonefield = osync_xmlfield_new(xmlformat, "Telephone", error);
				break;
			case GN_PHONEBOOK_ENTRY_Group:
				// TODO: Review what group means!
				xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);
				osync_xmlfield_set_key_value(xmlfield, "Category", contact->subentries[i].data.number);
				break;
			case GN_PHONEBOOK_ENTRY_URL:
				phonefield = osync_xmlfield_new(xmlformat, "Url", error);
				break;
			case GN_PHONEBOOK_ENTRY_Note:
				phonefield = osync_xmlfield_new(xmlformat, "Note", error);
				break;
			// Unused	
			case GN_PHONEBOOK_ENTRY_Ringtone:
			case GN_PHONEBOOK_ENTRY_Pointer:
			case GN_PHONEBOOK_ENTRY_Logo:
			case GN_PHONEBOOK_ENTRY_Date:
			case GN_PHONEBOOK_ENTRY_LogoSwitch:	
			case GN_PHONEBOOK_ENTRY_RingtoneAdv:	
			// TODO support new >= 0.6.14 entries
			case GN_PHONEBOOK_ENTRY_Location:
			case GN_PHONEBOOK_ENTRY_Image:
			case GN_PHONEBOOK_ENTRY_UserID:
			case GN_PHONEBOOK_ENTRY_PTTAddress:
			case GN_PHONEBOOK_ENTRY_FirstName:
			case GN_PHONEBOOK_ENTRY_LastName:
			case GN_PHONEBOOK_ENTRY_PostalAddress:
			case GN_PHONEBOOK_ENTRY_ExtendedAddress:
			case GN_PHONEBOOK_ENTRY_Street:
			case GN_PHONEBOOK_ENTRY_City:
			case GN_PHONEBOOK_ENTRY_StateProvince:
			case GN_PHONEBOOK_ENTRY_ZipCode:
			case GN_PHONEBOOK_ENTRY_Country:
			case GN_PHONEBOOK_ENTRY_FormalName:
			case GN_PHONEBOOK_ENTRY_JobTitle:
			case GN_PHONEBOOK_ENTRY_Company:
			case GN_PHONEBOOK_ENTRY_Nickname:
			case GN_PHONEBOOK_ENTRY_Birthday:
				break;
		}

		if (phonefield)
			osync_xmlfield_set_key_value(phonefield, "Content", contact->subentries[i].data.number);


		if (contact->subentries[i].entry_type != GN_PHONEBOOK_ENTRY_Number)
			continue;

		switch (contact->subentries[i].number_type) {
			case GN_PHONEBOOK_NUMBER_Home:
				// Home value is a special type: "Telephone Location"
				osync_xmlfield_set_attr(phonefield, "Location", "Home");
				break;
			case GN_PHONEBOOK_NUMBER_Mobile:
				osync_xmlfield_set_attr(phonefield, "Type", "Cellular");
				break;
			case GN_PHONEBOOK_NUMBER_Fax:
				osync_xmlfield_set_attr(phonefield, "Type", "Fax");
				break;
			case GN_PHONEBOOK_NUMBER_Work:
				osync_xmlfield_set_attr(phonefield, "Location", "Work");
				break;
			case GN_PHONEBOOK_NUMBER_None:	
			case GN_PHONEBOOK_NUMBER_Common:	
			case GN_PHONEBOOK_NUMBER_General:
				osync_xmlfield_set_attr(phonefield, "Type", "Voice");
				break;
		}	
	}	

	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);

        // XXX: remove this later?
        osync_xmlformat_sort(xmlformat);
        
        unsigned int size;
        char *str;
        osync_xmlformat_assemble(xmlformat, &str, &size);
        osync_trace(TRACE_SENSITIVE, "Output XMLFormat is:\n%s", str);
        g_free(str);

	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;
}

static void _xmlfield_formattedname(gn_phonebook_entry *contact, OSyncXMLField *xmlfield)
{
	const char *name = osync_xmlfield_get_key_value(xmlfield, "Content");
	strncpy(contact->name, name, GN_PHONEBOOK_NAME_MAX_LENGTH);
}

static void _xmlfield_telephone(gn_phonebook_entry *contact, OSyncXMLField *xmlfield, int *subcount)
{
	const char *tele = osync_xmlfield_get_key_value(xmlfield, "Content");

	char *number = gnokii_contact_util_cleannumber(tele);
	strncpy(contact->subentries[*subcount].data.number, number, GN_PHONEBOOK_NAME_MAX_LENGTH);
	g_free(number);

	contact->subentries[*subcount].entry_type = GN_PHONEBOOK_ENTRY_Number;

	const char *type = osync_xmlfield_get_attr(xmlfield, "Type");
	const char *location = osync_xmlfield_get_attr(xmlfield, "Location");

	if (location && !strcasecmp(location, "Work"))
		contact->subentries[*subcount].number_type = GN_PHONEBOOK_NUMBER_Work; 
	else if (location && !strcasecmp(location, "Home"))
		contact->subentries[*subcount].number_type = GN_PHONEBOOK_NUMBER_Home; 
	else if (type && !strcasecmp(type, "Fax"))
		contact->subentries[*subcount].number_type = GN_PHONEBOOK_NUMBER_Fax; 
	else if (type && !strcasecmp(type, "Cellular"))
		contact->subentries[*subcount].number_type = GN_PHONEBOOK_NUMBER_Mobile; 
	else
		contact->subentries[*subcount].number_type = GN_PHONEBOOK_NUMBER_General;

	(*subcount)++;
}

static void _xmlfield_url(gn_phonebook_entry *contact, OSyncXMLField *xmlfield, int *subcount)
{
	const char *url = osync_xmlfield_get_key_value(xmlfield, "Content");

	contact->subentries[*subcount].entry_type = GN_PHONEBOOK_ENTRY_URL;
	strncpy(contact->subentries[*subcount].data.number, url, GN_PHONEBOOK_NAME_MAX_LENGTH);
	(*subcount)++;
}

static void _xmlfield_email(gn_phonebook_entry *contact, OSyncXMLField *xmlfield, int *subcount)
{
	const char *email = osync_xmlfield_get_key_value(xmlfield, "Content");
	contact->subentries[*subcount].entry_type = GN_PHONEBOOK_ENTRY_Email;
	strncpy(contact->subentries[*subcount].data.number, email, GN_PHONEBOOK_NAME_MAX_LENGTH);

	(*subcount)++;
}

static void _xmlfield_note(gn_phonebook_entry *contact, OSyncXMLField *xmlfield, int *subcount)
{
	const char *note = osync_xmlfield_get_key_value(xmlfield, "Content");

	contact->subentries[*subcount].entry_type = GN_PHONEBOOK_ENTRY_Note;
	strncpy(contact->subentries[*subcount].data.number, note, GN_PHONEBOOK_NAME_MAX_LENGTH);
	(*subcount)++;
}

static void _xmlfield_addresslabel(gn_phonebook_entry *contact, OSyncXMLField *xmlfield, int *subcount)
{
	const char *label = osync_xmlfield_get_key_value(xmlfield, "Content");
	contact->subentries[*subcount].entry_type = GN_PHONEBOOK_ENTRY_Postal;
	strncpy(contact->subentries[*subcount].data.number, label, GN_PHONEBOOK_NAME_MAX_LENGTH); 
	(*subcount)++;
}

static void _xmlfield_category(gn_phonebook_entry *contact, OSyncXMLField *xmlfield)
{
	contact->caller_group = GN_PHONEBOOK_GROUP_None; 

	int i;
	int numnodes = osync_xmlfield_get_key_count(xmlfield);
	for (i=0; i < numnodes; i++) {
		const char *category = osync_xmlfield_get_nth_key_value(xmlfield, i);
		
		if (!strcasecmp(category, "FAMILY")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Family; 
		} else if (!strcasecmp(category, "VIPS") || !strcasecmp(category, "VIP")) { // FIXME: evo2-sync workaround fix that in vformat plugin
			contact->caller_group = GN_PHONEBOOK_GROUP_Vips;
		} else if (!strcasecmp(category, "FRIENDS")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Friends;
		} else if (!strcasecmp(category, "WORK")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Work;
		} else if (!strcasecmp(category, "OTHERS")) {
			contact->caller_group = GN_PHONEBOOK_GROUP_Others;
		}

	}
}



/* 
 * Converts from XML to the gnokii contact object type (gn_phonebook_entry).
 */  
static osync_bool conv_xmlformat_to_gnokii_contact(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %s, %p)", __func__, input, inpsize, 
			output, outpsize, free_input, config, error);

	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_SENSITIVE, "Input XMLFormat is:\n%s", str);
	g_free(str);

	int subcount = 0;

	// prepare contact 
	gn_phonebook_entry *contact = osync_try_malloc0(sizeof(gn_phonebook_entry), error);

	if (strcmp("contact", osync_xmlformat_get_objtype(xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(xmlformat));
		goto error;
	}

	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for (; xmlfield; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		osync_trace(TRACE_INTERNAL, "Field: %s", osync_xmlfield_get_name(xmlfield));

		if (!strcmp("FormattedName", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_formattedname(contact, xmlfield);
		else if (!strcmp("Telephone", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_telephone(contact, xmlfield, &subcount);
		else if (!strcmp("Url", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_url(contact, xmlfield, &subcount);
		else if (!strcmp("EMail", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_email(contact, xmlfield, &subcount);
		else if (!strcmp("Note", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_note(contact, xmlfield, &subcount);
		else if (!strcmp("AddressLabel", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_addresslabel(contact, xmlfield, &subcount);
		else if (!strcmp("Categories", osync_xmlfield_get_name(xmlfield)))
			_xmlfield_category(contact, xmlfield);
	}




	contact->subentries_count = subcount;

	osync_trace(TRACE_SENSITIVE, "TEST: name: %s\n", contact->name);
	 
	*free_input = TRUE;
	*output = (void *)contact;
	*outpsize = sizeof(gn_phonebook_entry);

	osync_trace(TRACE_EXIT, "%s", __func__);	
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


static void destroy_gnokii_contact(char *input, unsigned int inpsize)
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

void get_format_info(OSyncFormatEnv *env, OSyncError **error)
{

        /* register gnokii-contact format */
        OSyncObjFormat *format = osync_objformat_new("gnokii-contact", "contact", error);
        if (!format) {
                osync_trace(TRACE_ERROR, "Unable to register gnokii-contct format: %s", osync_error_print(error));
                osync_error_unref(error);
                return;
        }
        
//      osync_objformat_set_compare_func(format, compare_contact);
        osync_objformat_set_destroy_func(format, destroy_gnokii_contact);
//      osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
//        osync_objformat_set_print_func(format, print_gnokii_contact);
//      osync_objformat_set_copy_func(format, copy_xmlformat);
//      osync_objformat_set_create_func(format, create_contact);
        
//        osync_objformat_set_revision_func(format, get_revision);
        

//        osync_objformat_must_marshal(format);
//        osync_objformat_set_marshal_func(format, marshal_xmlformat);
//        osync_objformat_set_demarshal_func(format, demarshal_xmlformat);

        
        osync_format_env_register_objformat(env, format);
        osync_objformat_unref(format);

}

void get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-contact");
	OSyncObjFormat *gnokii_contact = osync_format_env_find_objformat(env, "gnokii-contact");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, gnokii_contact, conv_xmlformat_to_gnokii_contact, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, gnokii_contact, xmlformat, conv_gnokii_contact_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
}

int get_version(void)
{
	return 1;
}

