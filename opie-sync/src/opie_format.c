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


#include "opie_comms.h"
#include "opie_xml_utils.h"
#include "opie_format.h"

#include <string.h>

#include <opensync/opensync.h>
#include <opensync/opensync-time.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-xmlformat.h>

enum OpieTodoState {
	OPIE_TODO_STATE_STARTED     = 0,
	OPIE_TODO_STATE_POSTPONED   = 1,
	OPIE_TODO_STATE_FINISHED    = 2,
	OPIE_TODO_STATE_NOT_STARTED = 3
};


/** Convert Opie XML contact to OpenSync XML contact 
 * 
 **/
static osync_bool conv_opie_xml_contact_to_xml_contact(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	int j;
//	anon_data* anon;
	gchar** emailtokens;
	struct _xmlAttr *iprop;
	OSyncXMLField *out_xmlfield = NULL;
		
	/* Get the root node of the input document */
	xmlDoc *idoc = xmlRecoverMemory(input, inpsize);
	if (!idoc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to read xml contact");
		goto error;
	}
	
	xmlNode *icur = xmlDocGetRootElement(idoc);
	if (!icur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	/* Create a new output xml document */
	OSyncXMLFormat *out_xmlformat = osync_xmlformat_new("contact", error);
	
	if(!strcasecmp(icur->name, "Contact"))
	{
		/* this is a contact element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if (!strcasecmp(iprop->name, "FileAs"))
				{
					/* File-as. This is what the Evo plugin does, so copy it. */
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "FormattedName", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Emails"))
				{
					emailtokens = g_strsplit(iprop->children->content," ",3);
					for(j=0;emailtokens[j]!=NULL;j++) 
					{
						out_xmlfield = osync_xmlfield_new(out_xmlformat, "EMail", error);
						osync_xmlfield_set_key_value(out_xmlfield, "Content", emailtokens[j]);
					}
					g_strfreev(emailtokens);
				}
				else if(!strcasecmp(iprop->name, "Categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Categories", error);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						osync_xmlfield_add_key_value(out_xmlfield, "Category", categorytokens[j]);
					}
					g_strfreev(categorytokens);
				}
				else if(!strcasecmp(iprop->name, "DefaultEmail"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "EMail", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					/* this is the preferred email address */
					osync_xmlfield_set_attr(out_xmlfield, "Preferred", "true");
				}
				else if(!strcasecmp(iprop->name, "HomePhone"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Home");
					/* Removed the VOICE tags for the moment as they are assumed if not present, 
					   and if they are KDEPIM shows them up as "Other" */
					/* osync_xmlfield_set_attr(out_xmlfield, "Type", "Voice"); */
				}
				else if(!strcasecmp(iprop->name, "HomeFax"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Home");
					osync_xmlfield_set_attr(out_xmlfield, "Type", "Fax");
				}
				else if(!strcasecmp(iprop->name, "HomeMobile"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Home");
					osync_xmlfield_set_attr(out_xmlfield, "Type", "Cellular");
				}
				else if(!strcasecmp(iprop->name, "BusinessPhone"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Work");
					/* Removed the VOICE tags for the moment as they are assumed if not present, 
					   and if they are KDEPIM shows them up as "Other" */
					/* osync_xmlfield_set_attr(out_xmlfield, "Type", "Voice"); */
				}
				else if(!strcasecmp(iprop->name, "BusinessFax"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Work");
					osync_xmlfield_set_attr(out_xmlfield, "Type", "Fax");
				}
				else if(!strcasecmp(iprop->name, "BusinessMobile"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Work");
					osync_xmlfield_set_attr(out_xmlfield, "Type", "Cellular");
				}
				else if(!strcasecmp(iprop->name, "BusinessPager"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Telephone", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Location", "Work");
					osync_xmlfield_set_attr(out_xmlfield, "Type", "Pager"); /* FIXME is this still supported? */
				}
				else if(!strcasecmp(iprop->name, "HomeWebPage"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Url", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "BusinessWebPage"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Spouse"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Spouse", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Birthday"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Birthday", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Anniversary"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Anniversary", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Nickname"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Nickname", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Children"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Notes"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Note", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "rid"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "rinfo"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Gender"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Assistant"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Assistant", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Manager"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Manager", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Profession"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Profession", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "JobTitle"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Role", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else
				{
					/* FIXME handle unknown fields somehow? */
				}
			}
		}
	}

	GSList *attrs = NULL;
	GSList *keys = NULL;
	
	/* Name */
	dual_list_append(&attrs, "LastName",   &keys, "LastName");
	dual_list_append(&attrs, "FirstName",  &keys, "FirstName");
	dual_list_append(&attrs, "MiddleName", &keys, "Additional");
	dual_list_append(&attrs, "Suffix",     &keys, "Suffix");
	out_xmlfield = xml_attrs_to_xmlfield_keys(icur, out_xmlformat, "Name", attrs, keys, error);
	dual_list_clear(&attrs, &keys);
	
	/* Organization */
	dual_list_append(&attrs, "Company",    &keys, "Name");
	dual_list_append(&attrs, "Department", &keys, "Department");
	dual_list_append(&attrs, "Office",     &keys, "Unit");
	out_xmlfield = xml_attrs_to_xmlfield_keys(icur, out_xmlformat, "Organization", attrs, keys, error);
	dual_list_clear(&attrs, &keys);
	
	/* Home Address */
	dual_list_append(&attrs, "HomeStreet",  &keys, "Street");
	dual_list_append(&attrs, "HomeCity",    &keys, "Locality");
	dual_list_append(&attrs, "HomeState",   &keys, "Region");
	dual_list_append(&attrs, "HomeZip",     &keys, "PostalCode");
	dual_list_append(&attrs, "HomeCountry", &keys, "Country");
	out_xmlfield = xml_attrs_to_xmlfield_keys(icur, out_xmlformat, "Address", attrs, keys, error);
	if(out_xmlfield)
		osync_xmlfield_set_attr(out_xmlfield, "Location", "Home");
	dual_list_clear(&attrs, &keys);
	
	/* Work Address */
	dual_list_append(&attrs, "BusinessStreet",  &keys, "Street");
	dual_list_append(&attrs, "BusinessCity",    &keys, "Locality");
	dual_list_append(&attrs, "BusinessState",   &keys, "Region");
	dual_list_append(&attrs, "BusinessZip",     &keys, "PostalCode");
	dual_list_append(&attrs, "BusinessCountry", &keys, "Country");
	out_xmlfield = xml_attrs_to_xmlfield_keys(icur, out_xmlformat, "Address", attrs, keys, error);
	if(out_xmlfield)
		osync_xmlfield_set_attr(out_xmlfield, "Location", "Work");
	dual_list_clear(&attrs, &keys);
	
	
	*free_input = TRUE;
	*output = (char *)out_xmlformat;
	*outpsize = sizeof(out_xmlformat);
	
	xmlFreeDoc(idoc);

	// FIXME: remove this later by adding in a pre-sorted way?
	osync_xmlformat_sort(out_xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(out_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Output XMLFormat is:\n%s", str);
	g_free(str);

	if (!osync_xmlformat_validate(out_xmlformat, error))
		goto error;

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}    


/** Convert OpenSync XML contact to Opie XML contact 
 * 
 **/
static osync_bool conv_xml_contact_to_opie_xml_contact(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	enum PhoneType {
		PT_HOME = 1,
		PT_WORK = 2,
		PT_PREF = 4,
		PT_VOICE = 8,
		PT_FAX = 16,
		PT_CELL = 32,
		PT_PAGER = 64
	};

	GString *emails = NULL;

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
                    __func__, input, inpsize, output, 
                    outpsize, free_input, config, error);

	OSyncXMLFormat *in_xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(in_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	if (strcmp("contact", osync_xmlformat_get_objtype(in_xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(in_xmlformat));
		goto error;
	}

	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_contact = xmlNewDocNode(odoc, NULL, "Contact", NULL); 

	
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Name", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "LastName",   on_contact, "LastName");
			xmlfield_key_to_attr(in_xmlfield, "FirstName",  on_contact, "FirstName");
			xmlfield_key_to_attr(in_xmlfield, "Suffix",     on_contact, "Suffix");
			xmlfield_key_to_attr(in_xmlfield, "Additional", on_contact, "MiddleName");
		}
		else if(!strcmp("Organization", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Name",       on_contact, "Company");
			xmlfield_key_to_attr(in_xmlfield, "Department", on_contact, "Department");
			xmlfield_key_to_attr(in_xmlfield, "Unit",       on_contact, "Office");
		}
		else if(!strcmp("Telephone", fieldname)) {
			unsigned int type = 0;
			const char *teltype = osync_xmlfield_get_attr(in_xmlfield, "Type");
			if(teltype) {
				if(!strcmp(teltype, "Voice"))
					type |= PT_VOICE;
				else if(!strcmp(teltype, "Cellular"))
					type |= PT_CELL;
				else if(!strcmp(teltype, "Fax"))
					type |= PT_FAX;
				else if(!strcmp(teltype, "Pager"))
					type |= PT_PAGER;
				else {
					// ??????
				}
			}
			const char *telloc = osync_xmlfield_get_attr(in_xmlfield, "Location");
			if ( !strcmp( telloc, "Home" ) )
				type |= PT_HOME;
			else if ( !strcmp( telloc, "Work") )
				type |= PT_WORK;
		
			const char *number = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			
			/* Telephone numbers */
			if ( type & PT_PAGER ) {
				xmlSetProp(on_contact, "BusinessPager", number);
			}
			else if ( type & PT_WORK ) {
				if ( type & PT_FAX ) 
					xmlSetProp(on_contact, "BusinessFax", number);
				else if ( type & PT_CELL ) 
					xmlSetProp(on_contact, "BusinessMobile", number);
				else {
					/* PT_VOICE or anything else */
					xmlSetProp(on_contact, "BusinessPhone", number);
				}
			}
			else if ( type & PT_FAX ) 
				xmlSetProp(on_contact, "HomeFax", number);
			else if ( type & PT_CELL ) 
				xmlSetProp(on_contact, "HomeMobile", number);
			else {
				/* PT_VOICE or anything else */
				xmlSetProp(on_contact, "HomePhone", number);
			}
		}
		else if(!strcmp("EMail", fieldname)) {
			if(!emails)
				emails = g_string_new("");
			
			const char *emailaddr = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			g_string_append(emails, emailaddr);
			g_string_append_c(emails, ' ');

                        const char *pref = osync_xmlfield_get_attr(in_xmlfield, "Preferred");
			if(pref && (!strcasecmp(pref, "true"))) {
				xmlSetProp(on_contact, "DefaultEmail", emailaddr);
			}
		}
		else if(!strcmp("Address", fieldname)) {
			const char *addrloc = osync_xmlfield_get_attr(in_xmlfield, "Type");
			if (addrloc && (!strcmp(addrloc, "Work"))) {
				xmlfield_key_to_attr(in_xmlfield, "Street",     on_contact, "BusinessStreet");
				xmlfield_key_to_attr(in_xmlfield, "Locality",   on_contact, "BusinessCity");
				xmlfield_key_to_attr(in_xmlfield, "Region",     on_contact, "BusinessState");
				xmlfield_key_to_attr(in_xmlfield, "PostalCode", on_contact, "BusinessZip");
				xmlfield_key_to_attr(in_xmlfield, "Country",    on_contact, "BusinessCountry");
			}
			else {
				/* Default to home */
				xmlfield_key_to_attr(in_xmlfield, "Street",     on_contact, "HomeStreet");
				xmlfield_key_to_attr(in_xmlfield, "Locality",   on_contact, "HomeCity");
				xmlfield_key_to_attr(in_xmlfield, "Region",     on_contact, "HomeState");
				xmlfield_key_to_attr(in_xmlfield, "PostalCode", on_contact, "HomeZip");
				xmlfield_key_to_attr(in_xmlfield, "Country",    on_contact, "HomeCountry");
			}
		}
		else if(!strcmp("Role", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "JobTitle");
		}
		else if(!strcmp("Note", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Notes");
		}
		else if(!strcmp("Spouse", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Spouse");
		}
		else if(!strcmp("Nickname", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Nickname");
		}
		else if(!strcmp("Assistant", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Assistant");
		}
		else if(!strcmp("Manager", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Manager");
		}
		else if(!strcmp("Profession", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Profession");
		}
		else if(!strcmp("Birthday", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Birthday");
		}
		else if(!strcmp("Anniversary", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "Anniversary");
		}
		else if(!strcmp("Url", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "HomeWebPage");
		}
		else if(!strcmp("FormattedName", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_contact, "FileAs");
		}
		else if(!strcmp("Categories", fieldname)) {
			xmlfield_categories_to_attr(in_xmlfield, on_contact, "Categories");
		}
		
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
	
	if(emails) {
		g_strchomp(emails->str);
		xmlSetProp(on_contact, "Emails", emails->str);
		g_string_free(emails, TRUE);
	}
	
// TODO: Entries to be handled
/*   unsigned int rid; */
/*   unsigned int rinfo; */
/*   char* business_webpage; */
/*   int gender; */
/*   char* children; */
/*   GList* anons; */

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
	*output = xml_node_to_text(odoc, on_contact);
	*outpsize = strlen(*output);
	
	xmlFree(odoc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** Convert Opie XML todo to OpenSync XML todo 
 * 
 **/
static osync_bool conv_opie_xml_todo_to_xml_todo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	struct _xmlAttr *iprop;
	int j;
	OSyncXMLField *out_xmlfield = NULL;
		
	/* Get the root node of the input document */
	xmlDoc *idoc = xmlRecoverMemory(input, inpsize);
	if (!idoc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to read xml todo");
		goto error;
	}
	
	xmlNode *icur = xmlDocGetRootElement(idoc);
	if (!icur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	/* Create a new output xml document */
	OSyncXMLFormat *out_xmlformat = osync_xmlformat_new("todo", error);
	
	if(!strcasecmp(icur->name, "Task"))
	{
		/* this is a todo element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if(!strcasecmp(iprop->name, "Summary")) 
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Summary", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Description"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Description", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Priority"))
				{
					/* Priority is 1-5 on Opie, 0-9 in OpenSync XML (0 = undefined) */
					int priority = atoi(iprop->children->content);
					char *prio = g_strdup_printf("%d", priority);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Priority", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", prio);
					g_free(prio);
				}
				else if(!strcasecmp(iprop->name, "Progress"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "PercentComplete", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "State"))
				{
					int state = atoi(iprop->children->content);
					char *status;
					switch(state) {
						case OPIE_TODO_STATE_STARTED:
							status = "IN-PROCESS";
							break;
						case OPIE_TODO_STATE_POSTPONED:
							status = "CANCELLED";
							break;
						case OPIE_TODO_STATE_FINISHED:
							status = "COMPLETED";
							break;
						case OPIE_TODO_STATE_NOT_STARTED:
						default:
							status = "NEEDS-ACTION";
					}
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Status", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", status);
				}
				else if(!strcasecmp(iprop->name, "StartDate"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "DateStarted", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					osync_xmlfield_set_attr(out_xmlfield, "Value", "DATE");
				}
				else if(!strcasecmp(iprop->name, "Categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Categories", error);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						osync_xmlfield_add_key_value(out_xmlfield, "Category", categorytokens[j]);
					}
					g_strfreev(categorytokens);
				}
			}
		}
		
		/* Complete / Completed Date */
		char *completed = xmlGetProp(icur, "Completed");
		if(completed) {
			if(!strcmp(completed, "1")) {
				char *completeDate = xmlGetProp(icur, "CompletedDate");
				if(completeDate) { 
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Completed", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", completeDate);
					// RFC2445 says the default value type is DATE-TIME. But Opie only
					// stores DATE as completed date => alter VALUE to DATE
					osync_xmlfield_set_attr(out_xmlfield, "Value", "DATE");
					xmlFree(completeDate);
				}
			}
			xmlFree(completed);
		}
		
		/* Due date */
		GDate *duedate = NULL;
		char *duedatestr = NULL;
		char *hasDate = xmlGetProp(icur, "HasDate");
		if(hasDate) { 
			if(!strcmp(hasDate, "1")) {
				char *datedaystr   = xmlGetProp(icur, "DateDay");
				char *datemonthstr = xmlGetProp(icur, "DateMonth");
				char *dateyearstr  = xmlGetProp(icur, "DateYear");
				if(datedaystr && datemonthstr && dateyearstr) {
					int dateyear = atoi(dateyearstr);
					int datemonth = atoi(datemonthstr);
					int dateday = atoi(datedaystr);
					duedatestr = g_strdup_printf("%04d%02d%02d", dateyear, datemonth, dateday);
					duedate = g_date_new_dmy(dateday, datemonth, dateyear);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Due", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", duedatestr);
					// RFC2445 says the default value type is DATE-TIME. But Opie only
					// stores DATE as due date => alter VALUE to DATE
					osync_xmlfield_set_attr(out_xmlfield, "Value", "DATE");
				}
				if(datedaystr)   xmlFree(datedaystr);
				if(datemonthstr) xmlFree(datemonthstr);
				if(dateyearstr)  xmlFree(dateyearstr);
			}
			xmlFree(hasDate);
		}
	
		/* Recurrence */
		xml_recur_attr_to_xmlfield(icur, out_xmlformat, duedate, error);
		
		/* Alarms */
		char *alarmstr = xmlGetProp(icur, "Alarms");
		if(alarmstr && duedatestr) {
			char *duetimestr = g_strdup_printf("%sT000000", duedatestr);
			time_t duetime = osync_time_vtime2unix(duetimestr, 0);
			xml_todo_alarm_attr_to_xmlfield(alarmstr, out_xmlformat, &duetime, error);
			g_free(duetimestr);
		}
		if(duedatestr)
			g_free(duedatestr);
		if(alarmstr)
			xmlFree(alarmstr);
		
		if(duedate)
			g_date_free(duedate);
	}

	*free_input = TRUE;
	*output = (char *)out_xmlformat;
	*outpsize = sizeof(out_xmlformat);
	
	xmlFreeDoc(idoc);

	// FIXME: remove this later by adding in a pre-sorted way?
	osync_xmlformat_sort(out_xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(out_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Output XMLFormat is:\n%s", str);
	g_free(str);

	if (!osync_xmlformat_validate(out_xmlformat, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** Convert OpenSync XML todo to Opie XML todo 
 * 
 **/
static osync_bool conv_xml_todo_to_opie_xml_todo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	const char *completedstr = NULL;
	const char *startedstr = NULL;
	const char *duestr = NULL;
			
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, input, inpsize, output, 
							outpsize, free_input, config, error);

	OSyncXMLFormat *in_xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(in_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	if (strcmp("todo", osync_xmlformat_get_objtype(in_xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(in_xmlformat));
		goto error;
	}

	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_todo = xmlNewDocNode(odoc, NULL, "Task", NULL);
	
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Summary", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_todo, "Summary");
		}
		else if(!strcmp("Description", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_todo, "Description");
		}
		else if(!strcmp("Priority", fieldname)) {
			/* Priority is 1-5 on Opie, 0-9 in OpenSync XML (0 = undefined) */
			const char *prio = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if (prio) {
				int priority = atoi(prio);
				if (priority < 1) {
					/* Invalid or (more likely) unspecified priority */
					priority = 3;
				}
				else if (priority > 5) {
					priority = 5;
				}
				char *prio_str = g_strdup_printf("%d", priority);
				xmlSetProp(on_todo, "Priority", prio_str);
				g_free(prio_str);
			}
		}
		else if(!strcmp("Categories", fieldname)) {
			xmlfield_categories_to_attr(in_xmlfield, on_todo, "Categories");
		}
		else if(!strcmp("Completed", fieldname)) {
			completedstr = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if(completedstr) {
				struct tm *completed = osync_time_vtime2tm(completedstr);
				char *completedstr_out = g_strdup_printf("%04d%02d%02d", completed->tm_year + 1900, (completed->tm_mon + 1), completed->tm_mday);
				xmlSetProp(on_todo, "Completed", "1");
				xmlSetProp(on_todo, "CompletedDate", completedstr_out);
				g_free(completedstr_out);
				g_free(completed);
			}
		}
		else if(!strcmp("DateStarted", fieldname)) {
			startedstr = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if(startedstr) {
				struct tm *started = osync_time_vtime2tm(startedstr);
				char *startedstr_out = g_strdup_printf("%04d%02d%02d", (started->tm_year + 1900), (started->tm_mon + 1), started->tm_mday);
				xmlSetProp(on_todo, "StartDate", startedstr_out);
				g_free(startedstr_out);
				g_free(started);
			}
		}
		else if(!strcmp("Due", fieldname)) {
			duestr = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if(duestr) {
				struct tm *due = osync_time_vtime2tm(duestr);
				char *dueyear  = g_strdup_printf("%04d", (due->tm_year + 1900));
				char *duemonth = g_strdup_printf("%02d", (due->tm_mon + 1));
				char *dueday   = g_strdup_printf("%02d", due->tm_mday);
				xmlSetProp(on_todo, "HasDate",   "1");
				xmlSetProp(on_todo, "DateYear",  dueyear);
				xmlSetProp(on_todo, "DateMonth", duemonth);
				xmlSetProp(on_todo, "DateDay",   dueday);
				g_free(dueyear);
				g_free(duemonth);
				g_free(dueday);
				g_free(due);
			}
		}
		else if(!strcmp("PercentComplete", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_todo, "Progress");
		}
		else if(!strcmp("Status", fieldname)) {
			const char *status = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if(status) {
				int state;
				if(!strcasecmp(status, "IN-PROCESS")) {
					state = OPIE_TODO_STATE_NOT_STARTED;
				}
				else if (!strcasecmp(status, "CANCELLED")) {
					state = OPIE_TODO_STATE_POSTPONED;
				}
				else if (!strcasecmp(status, "COMPLETED")) {
					state = OPIE_TODO_STATE_FINISHED;
				}
				else {
					state = OPIE_TODO_STATE_NOT_STARTED;
				}
				char *statestr = g_strdup_printf("%d", state);
				xmlSetProp(on_todo, "State", statestr); 
				g_free(statestr);
			}
		}
		else if(!strcmp("RecurrenceRule", fieldname)) {
			xmlfield_recur_to_attr(in_xmlfield, on_todo);
		}
		
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
	
	/* Convert alarms */
	xmlformat_todo_alarms_to_attr(in_xmlformat, on_todo, duestr);
	
	if(!completedstr)
		xmlSetProp(on_todo, "Completed", "0");
	
	if(!startedstr)
		xmlSetProp(on_todo, "StartDate", "0");
	
	if(!duestr)
		xmlSetProp(on_todo, "HasDate", "0");
	
	*free_input = TRUE;
	*output = xml_node_to_text(odoc, on_todo);
	*outpsize = strlen(*output);
	
	xmlFree(odoc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert Opie XML event to OpenSync XML event 
 * 
 **/
static osync_bool conv_opie_xml_event_to_xml_event(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	struct _xmlAttr *iprop;
	OSyncXMLField *out_xmlfield = NULL;
	GDate *startdate = NULL;
	int j;
		
	/* Get the root node of the input document */
	xmlDoc *idoc = xmlRecoverMemory(input, inpsize);
	if (!idoc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to read xml event");
		goto error;
	}
	
	xmlNode *icur = xmlDocGetRootElement(idoc);
	if (!icur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	/* Create a new output xml document */
	OSyncXMLFormat *out_xmlformat = osync_xmlformat_new("event", error);
	
	if(!strcasecmp(icur->name, "event"))
	{
		/* Check if this is an all-day event */
		int allday = 0;
		char *eventtype = xmlGetProp(icur, "type");
		if(eventtype) {
			if(!strcasecmp(eventtype, "AllDay")) {
				allday = 1;
			}
			xmlFree(eventtype);
		}
		
		/* this is an event element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if(!strcasecmp(iprop->name, "description"))
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Summary", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "note")) 
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Description", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "location")) 
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Location", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "created")) 
				{
					time_t createtime = (time_t)atoi(iprop->children->content);
					char *createvtime = osync_time_unix2vtime(&createtime);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "DateCreated", error);
					osync_xmlfield_set_key_value(out_xmlfield, "Content", iprop->children->content);
					g_free(createvtime);
				}
				else if(!strcasecmp(iprop->name, "start")) 
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "DateStarted", error);
					time_t starttime = (time_t)atoi(iprop->children->content);
					if(allday) {
						struct tm *localtm = g_malloc0(sizeof(struct tm));
						localtime_r(&starttime, localtm);
						char *startvdate = g_strdup_printf("%04d%02d%02d", localtm->tm_year + 1900, (localtm->tm_mon + 1), localtm->tm_mday);
						osync_xmlfield_set_key_value(out_xmlfield, "Content", startvdate);
						osync_xmlfield_set_attr(out_xmlfield, "Value", "DATE");
						g_free(startvdate);
						g_free(localtm);
					}
					else {
						char *startvtime = osync_time_unix2vtime(&starttime); 
						osync_xmlfield_set_key_value(out_xmlfield, "Content", startvtime);
						g_free(startvtime);
					}
					/* Record the start date for use later */
					startdate = g_date_new();

#ifdef OLD_GLIB_VER
					/* This is deprecated */
					GTime g_starttime = (GTime)starttime;
					g_date_set_time(startdate, g_starttime);
#else
					g_date_set_time_t(startdate, starttime);
#endif /* OLD_GLIB_VER */

				}
				else if(!strcasecmp(iprop->name, "end")) 
				{
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "DateEnd", error);
					time_t endtime = (time_t)atoi(iprop->children->content);
					if(allday) {
						struct tm *localtm = g_malloc0(sizeof(struct tm));
						endtime += 1;
						localtime_r(&endtime, localtm);
						char *endvdate = g_strdup_printf("%04d%02d%02d", localtm->tm_year + 1900, (localtm->tm_mon + 1), localtm->tm_mday);
						osync_xmlfield_set_key_value(out_xmlfield, "Content", endvdate);
						osync_xmlfield_set_attr(out_xmlfield, "Value", "DATE");
						g_free(endvdate);
						g_free(localtm);
					}
					else {
						char *endvtime = osync_time_unix2vtime(&endtime);
						osync_xmlfield_set_key_value(out_xmlfield, "Content", endvtime);
						g_free(endvtime);
					}
				}
				else if(!strcasecmp(iprop->name, "categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					out_xmlfield = osync_xmlfield_new(out_xmlformat, "Categories", error);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						osync_xmlfield_add_key_value(out_xmlfield, "Category", categorytokens[j]);
					}
					g_strfreev(categorytokens);
				}
			}
			/* FIXME Stuff to handle:
			timezone?
			*/
		}
		
		/* Alarm */
		char *alarmminsstr = xmlGetProp(icur, "alarm");
		if(alarmminsstr) {
			out_xmlfield = osync_xmlfield_new(out_xmlformat, "Alarm", error);
			
			int alarmsound = 0;
			char *alarmsoundstr = xmlGetProp(icur, "sound");
			if(alarmsoundstr) {
				if(!strcmp(alarmsoundstr, "loud"))
					alarmsound = 1;
				xmlFree(alarmsoundstr);
			}
			if(alarmsound == 1)
				osync_xmlfield_set_key_value(out_xmlfield, "AlarmAction", "AUDIO");
			else
				osync_xmlfield_set_key_value(out_xmlfield, "AlarmAction", "DISPLAY");
			
			int alarmseconds = -(atoi(alarmminsstr) * 60);
			char *alarmdu = osync_time_sec2alarmdu(alarmseconds);
			osync_xmlfield_set_key_value(out_xmlfield, "AlarmTrigger", alarmdu);
			g_free(alarmdu);
			xmlFree(alarmminsstr);
		}
		
		/* Recurrence */
		xml_recur_attr_to_xmlfield(icur, out_xmlformat, startdate, error);
	}

	*free_input = TRUE;
	*output = (char *)out_xmlformat;
	*outpsize = sizeof(out_xmlformat);
	
	xmlFreeDoc(idoc);

	// FIXME: remove this later by adding in a pre-sorted way?
	osync_xmlformat_sort(out_xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(out_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Output XMLFormat is:\n%s", str);
	g_free(str);

	if (!osync_xmlformat_validate(out_xmlformat, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert OpenSync XML event to Opie XML event 
 * 
 **/
static osync_bool conv_xml_event_to_opie_xml_event(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	time_t start_time = 0;
	time_t end_time = 0;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, input, inpsize, output, 
							outpsize, free_input, config, error);

	OSyncXMLFormat *in_xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(in_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	if (strcmp("event", osync_xmlformat_get_objtype(in_xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(in_xmlformat));
		goto error;
	}
	
	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_event = xmlNewDocNode(odoc, NULL, "event", NULL);
	
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Summary", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_event, "description");
		}
		else if(!strcmp("Description", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_event, "note");
		}
		else if(!strcmp("Location", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_event, "location");
		}
		else if(!strcmp("DateCreated", fieldname)) {
			xmlfield_vtime_to_attr_time_t(in_xmlfield, on_event, "created");
		}
		else if(!strcmp("DateStarted", fieldname)) {
			xmlfield_vtime_to_attr_time_t(in_xmlfield, on_event, "start");
		}
		else if(!strcmp("DateEnd", fieldname)) {
			xmlfield_vtime_to_attr_time_t(in_xmlfield, on_event, "end");
		}
		else if(!strcmp("Categories", fieldname)) {
			xmlfield_categories_to_attr(in_xmlfield, on_event, "Categories");
		}
		else if(!strcmp("RecurrenceRule", fieldname)) {
			xmlfield_recur_to_attr(in_xmlfield, on_event);
		}
		
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
	
	/* Check for all-day event */
	int timediff = end_time - start_time;
	if((timediff == 86399) || (timediff == 86400)) {
		xmlSetProp(on_event, "type", "AllDay");
		if(timediff == 86400) {
			/* Opie expects end to be start + 86399 */
			char *endtimestr = xmlGetProp(on_event, "end");
			int endtime = atoi(endtimestr) - 1;
			xmlFree(endtimestr);
			endtimestr = g_strdup_printf("%d", endtime);
			xmlSetProp(on_event, "end", endtimestr);
			g_free(endtimestr);
			xmlSetProp(on_event, "type", "AllDay");
		}
	}
	
	/* Alarm */
	xmlformat_cal_alarms_to_attr(in_xmlformat, on_event, &start_time);
	
	*free_input = TRUE;
	*output = xml_node_to_text(odoc, on_event);
	*outpsize = strlen(*output);
	
	xmlFree(odoc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert Opie XML note (which is internal to the plugin, see opie_comms.c) to OpenSync XML note
 * 
 **/
static osync_bool conv_opie_xml_note_to_xml_note(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
		
	OSyncXMLField *out_xmlfield = NULL;
	
	/* Get the root node of the input document */
	xmlDoc *idoc = xmlRecoverMemory(input, inpsize);
	if (!idoc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to read xml note");
		goto error;
	}
	
	xmlNode *icur = xmlDocGetRootElement(idoc);
	if (!icur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		goto error;
	}
	
	/* Create a new output xml document */
	OSyncXMLFormat *out_xmlformat = osync_xmlformat_new("note", error);
	
	if(!strcasecmp(icur->name, "note"))
	{
		// Summary
		char *value = xmlGetProp(icur, "name");
		if(value) {
			out_xmlfield = osync_xmlfield_new(out_xmlformat, "Summary", error);
			osync_xmlfield_set_key_value(out_xmlfield, "Content", value);
			xmlFree(value);
		}
		// Body
		value = xmlGetProp(icur, "content");
		if(value) {
			out_xmlfield = osync_xmlfield_new(out_xmlformat, "Body", error);
			osync_xmlfield_set_key_value(out_xmlfield, "Content", value);
			xmlFree(value);
		}
	}

	*free_input = TRUE;
	*output = (char *)out_xmlformat;
	*outpsize = sizeof(out_xmlformat);
	
	xmlFreeDoc(idoc);

	// FIXME: remove this later by adding in a pre-sorted way?
	osync_xmlformat_sort(out_xmlformat);
	
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(out_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Output XMLFormat is:\n%s", str);
	g_free(str);

	if (!osync_xmlformat_validate(out_xmlformat, error))
		goto error;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert OpenSync XML note to Opie XML note (which is internal to the plugin)
 * 
 **/
static osync_bool conv_xml_note_to_opie_xml_note(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, input, inpsize, output, 
							outpsize, free_input, config, error);

	OSyncXMLFormat *in_xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(in_xmlformat, &str, &size);
	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", str);
	g_free(str);

	if (strcmp("note", osync_xmlformat_get_objtype(in_xmlformat))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s",  osync_xmlformat_get_objtype(in_xmlformat));
		goto error;
	}

	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_note = xmlNewDocNode(odoc, NULL, "note", NULL);
	
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Summary", fieldname)) {
			xmlfield_key_to_attr(in_xmlfield, "Content", on_note, "name");
		}
		else if(!strcmp("Body", fieldname)) {
			const char *value = osync_xmlfield_get_key_value(in_xmlfield, "Content");
			if(value)
				xmlNewTextChild(on_note, NULL, "content", value);
		}
		
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
	
	*free_input = TRUE;
	*output = xml_node_to_text(odoc, on_note);
	*outpsize = strlen(*output);
	
	xmlFree(odoc);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	/* Contact */
	OSyncObjFormat *format = osync_objformat_new(OPIE_FORMAT_XML_CONTACT, "contact", error);
	if (!format)
		return FALSE;
/*	osync_objformat_set_compare_func(format, compare_format1);
	osync_objformat_set_destroy_func(format, destroy_format1);
	osync_objformat_set_duplicate_func(format, duplicate_format1);
	osync_objformat_set_print_func(format, print_format1);*/
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);
	
	/* Todo */
	format = osync_objformat_new(OPIE_FORMAT_XML_TODO, "todo", error);
	if (!format)
		return FALSE;
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	/* Event */
	format = osync_objformat_new(OPIE_FORMAT_XML_EVENT, "event", error);
	if (!format)
		return FALSE;
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	/* Note */
	format = osync_objformat_new(OPIE_FORMAT_XML_NOTE, "note", error);
	if (!format)
		return FALSE;
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}


osync_bool register_converter(OSyncFormatEnv *env, const char *format1_name, const char *format2_name, OSyncFormatConvertFunc conv_format1_to_format2, OSyncFormatConvertFunc conv_format2_to_format1, OSyncError **error)
{
	OSyncObjFormat *format1 = osync_format_env_find_objformat(env, format1_name);
	if (!format1) {
		char *errmsg = g_strdup_printf("Unable to find format \"%s\"", format1_name);
		osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
		g_free(errmsg);
		return FALSE;
	}

	OSyncObjFormat *format2 = osync_format_env_find_objformat(env, format2_name);
	if (!format2) {
		char *errmsg = g_strdup_printf("Unable to find format \"%s\"", format2_name);
		osync_error_set(error, OSYNC_ERROR_GENERIC, errmsg);
		g_free(errmsg);
		return FALSE;
	}

	OSyncFormatConverter *conv = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, conv_format1_to_format2, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format1, conv_format2_to_format1, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	return TRUE;
}


osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	if(!register_converter(env, OPIE_FORMAT_XML_CONTACT, "xmlformat-contact", conv_opie_xml_contact_to_xml_contact, conv_xml_contact_to_opie_xml_contact, error))
		return FALSE;
	if(!register_converter(env, OPIE_FORMAT_XML_TODO,    "xmlformat-todo",    conv_opie_xml_todo_to_xml_todo, conv_xml_todo_to_opie_xml_todo, error))
		return FALSE;
	if(!register_converter(env, OPIE_FORMAT_XML_EVENT,   "xmlformat-event",   conv_opie_xml_event_to_xml_event, conv_xml_event_to_opie_xml_event, error))
		return FALSE;
	if(!register_converter(env, OPIE_FORMAT_XML_NOTE,    "xmlformat-note",    conv_opie_xml_note_to_xml_note, conv_xml_note_to_opie_xml_note, error))
		return FALSE;
	
	return TRUE;
}

int get_version(void)
{
	return 1;
}



void xmlfield_key_to_attr(OSyncXMLField *xmlfield, const char *key, xmlNode *node_to, const char *attrname) {
	const char *value = osync_xmlfield_get_key_value(xmlfield, key);
	if(value && (strlen(value) > 0))
		xmlSetProp(node_to, attrname, value);
}

time_t xmlfield_vtime_to_attr_time_t(OSyncXMLField *xmlfield, xmlNode *node_to, const char *attrname) {
	const char *vtime = osync_xmlfield_get_key_value(xmlfield, "Content");
	time_t utime = 0;
	if(vtime) {
		const char *vtimetype = osync_xmlfield_get_attr(xmlfield, "Value");
		if(vtimetype && !strcasecmp(vtimetype, "DATE")) {
			/* vtime has date but no time, so we treat it as midnight local time */
			struct tm *localtm = osync_time_vtime2tm(vtime);
			utime = mktime(localtm);
			g_free(localtm);
		}
		else
			utime = osync_time_vtime2unix(vtime, 0);
				
		char *timestr = g_strdup_printf("%d", (int)utime);
		xmlSetProp(node_to, attrname, timestr);
		g_free(timestr);
	}
	return utime;
}

OSyncXMLField *xml_attrs_to_xmlfield_keys(xmlNode *node, OSyncXMLFormat *out_xmlformat, const char *fieldname, GSList *attrs, GSList *keys, OSyncError **error) {
	GSList *attrsptr = attrs;
	GSList *keysptr = keys;
	OSyncXMLField *out_xmlfield = NULL;
	
	while(attrsptr) {
		char *attr = ((char *)(attrsptr->data));
		char *value = xmlGetProp(node, attr);
		if(value) {
			char *key = (char *)(keysptr->data);
			if(!out_xmlfield)
				out_xmlfield = osync_xmlfield_new(out_xmlformat, fieldname, error);
			osync_xmlfield_set_key_value(out_xmlfield, key, value);
			xmlFree(value);
		}
		
		attrsptr = g_slist_next(attrsptr);
		keysptr = g_slist_next(keysptr);
	}
	
	return out_xmlfield;
}

void dual_list_append(GSList **list1, void *item1, GSList **list2, void *item2) {
	*list1 = g_slist_append(*list1, item1);
	*list2 = g_slist_append(*list2, item2);
}

void dual_list_clear(GSList **list1, GSList **list2) {
	g_slist_free(*list1);
	*list1 = NULL;
	g_slist_free(*list2);
	*list2 = NULL;
}

void xmlfield_categories_to_attr(OSyncXMLField *in_xmlfield, xmlNode *node_to, const char *category_attr) {
	int i;
	
	GString *categories = g_string_new("");
	int keycount = osync_xmlfield_get_key_count(in_xmlfield);
	for ( i = 0; i < keycount; i++ ) {
		if(!strcmp(osync_xmlfield_get_nth_key_name(in_xmlfield, i), "Category")) {
			const char *cat_name = osync_xmlfield_get_nth_key_value(in_xmlfield, i);
			g_string_append_printf(categories, "%s|", cat_name);
		}
	}
	
	if(categories->len > 0) {
		g_string_truncate(categories, categories->len - 1);
		xmlSetProp(node_to, category_attr, categories->str);
	}
	g_string_free(categories, TRUE);
}

void xml_recur_attr_to_xmlfield(xmlNode *item_node, OSyncXMLFormat *out_xmlformat, GDate *startdate, OSyncError **error) {
	/* Recurrence for todos and events */
	char *recurType = xmlGetProp(item_node, "rtype");
	if(recurType) {
		OSyncXMLField *out_xmlfield = osync_xmlfield_new(out_xmlformat, "RecurrenceRule", error);
		
		/* Frequency */
		if(!strcmp(recurType, "Daily")) {
			osync_xmlfield_set_key_value(out_xmlfield, "Frequency", "DAILY");
		}
		else if(!strcmp(recurType, "Weekly")) {
			osync_xmlfield_set_key_value(out_xmlfield, "Frequency", "WEEKLY");
			
			/* Weekdays */
			char *weekdays = xmlGetProp(item_node, "rweekdays");
			if(weekdays) {
				int weekdaysnum = atoi(weekdays);
				if(weekdaysnum > 0) {
					GString *byday = g_string_new("");
					g_string_append(byday, "BYDAY=");
					if(weekdaysnum & 1)
						g_string_append(byday, "MO,");
					if(weekdaysnum & 2)
						g_string_append(byday, "TU,");
					if(weekdaysnum & 4)
						g_string_append(byday, "WE,");
					if(weekdaysnum & 8)
						g_string_append(byday, "TH,");
					if(weekdaysnum & 16)
						g_string_append(byday, "FR,");
					if(weekdaysnum & 32)
						g_string_append(byday, "SA,");
					if(weekdaysnum & 64)
						g_string_append(byday, "SU,");
				
					/* Remove the trailing comma */
					g_string_truncate(byday, strlen(byday->str) - 1);
				
					osync_xmlfield_set_key_value(out_xmlfield, "ByDay", byday->str);
					g_string_free(byday, TRUE);
				}
				xmlFree(weekdays);
			}
		}
		else if(!strcmp(recurType, "MonthlyDate")) {
			osync_xmlfield_set_key_value(out_xmlfield, "Frequency", "MONTHLY");
			if(startdate) {
				char *bymonthday = g_strdup_printf("%i", (int)g_date_get_day(startdate));
				osync_xmlfield_set_key_value(out_xmlfield, "ByMonthDay", bymonthday);
				g_free(bymonthday);
			}
		}
		else if(!strcmp(recurType, "MonthlyDay")) {
			osync_xmlfield_set_key_value(out_xmlfield, "Frequency", "MONTHLY");
			if(startdate) {
				int weekno;
				char *weeknostr = xmlGetProp(item_node, "rposition");
				if(weeknostr) {
					weekno = atoi(weeknostr);
					xmlFree(weeknostr);
				}
				else {
					weekno = -1;
				}
				
				char *byday = NULL;
				GDateWeekday weekday = g_date_get_weekday(startdate);
				switch (weekday) {
					case G_DATE_MONDAY:
						byday = g_strdup_printf("%iMO", weekno);
						break;
					case G_DATE_TUESDAY:
						byday = g_strdup_printf("%iTU", weekno);
						break;
					case G_DATE_WEDNESDAY:
						byday = g_strdup_printf("%iWE", weekno);
						break;
					case G_DATE_THURSDAY:
						byday = g_strdup_printf("%iTH", weekno);
						break;
					case G_DATE_FRIDAY:
						byday = g_strdup_printf("%iFR", weekno);
						break;
					case G_DATE_SATURDAY:
						byday = g_strdup_printf("%iSA", weekno);
						break;
					case G_DATE_SUNDAY:
						byday = g_strdup_printf("%iSU", weekno);
						break;
					case G_DATE_BAD_WEEKDAY:
						break;
				}
				if(byday) {
					osync_xmlfield_set_key_value(out_xmlfield, "ByDay", byday);
					g_free(byday);
				}
			}
		}
		else if(!strcmp(recurType, "Yearly")) {
			osync_xmlfield_set_key_value(out_xmlfield, "Frequency", "YEARLY");
		}

		/* Interval */
		char *interval = xmlGetProp(item_node, "rfreq");
		if(interval) {
			osync_xmlfield_set_key_value(out_xmlfield, "Interval", interval);
			xmlFree(interval);
		}
		
		/* End date */
		char *hasEndDate = xmlGetProp(item_node, "rhasenddate");
		if(hasEndDate) {
			char *recurendstr = xmlGetProp(item_node, "enddt");
			if(recurendstr) {
				time_t recurendtime = (time_t)atoi(recurendstr);
				char *recurendvtime = osync_time_unix2vtime(&recurendtime); 
				osync_xmlfield_set_key_value(out_xmlfield, "Until", recurendvtime);
				g_free(recurendvtime);
				xmlFree(recurendstr);
			}
		}
		
		xmlFree(recurType);
	}
}

void xmlfield_recur_to_attr(OSyncXMLField *in_xmlfield, xmlNode *node_to) {
	/* Recurrence for todos and events */
	char *enddt = NULL;
	char *weekdaysrule = NULL;
	char *rfreq = NULL;
	int i;
	
	enum 
	{
		RECUR_TYPE_NONE         = 0,
		RECUR_TYPE_DAILY        = 1,
		RECUR_TYPE_WEEKLY       = 2,
		RECUR_TYPE_MONTHLY_DAY  = 3,
		RECUR_TYPE_MONTHLY_DATE = 4,
		RECUR_TYPE_YEARLY       = 5
	} rectype;
	rectype = RECUR_TYPE_NONE;
	
	/* read recurrence key values */
	int keycount = osync_xmlfield_get_key_count(in_xmlfield);
	for ( i = 0; i < keycount; i++ ) {
		const char *keyname = osync_xmlfield_get_nth_key_name(in_xmlfield, i);
		const char *keyvalue = osync_xmlfield_get_nth_key_value(in_xmlfield, i);
		if (!strcasecmp(keyname, "FREQ")) {
			if (!strcasecmp(keyvalue, "DAILY")) {
				rectype = RECUR_TYPE_DAILY;
			}
			else if (!strcasecmp(keyvalue, "WEEKLY")) {
				rectype = RECUR_TYPE_WEEKLY;
			}
			else if (!strcasecmp(keyvalue, "MONTHLY")) {
				if(rectype != RECUR_TYPE_MONTHLY_DATE)
					rectype = RECUR_TYPE_MONTHLY_DAY;
			}
			else if (!strcasecmp(keyvalue, "YEARLY")) {
				rectype = RECUR_TYPE_YEARLY;
			}
		}
		else if (!strcasecmp(keyname, "BYDAY")) {
			weekdaysrule = g_strdup(keyvalue);
		}
		else if (!strcasecmp(keyname, "BYMONTHDAY")) {
			if(rectype != RECUR_TYPE_YEARLY)
				rectype = RECUR_TYPE_MONTHLY_DATE;
		}
		else if (!strcasecmp(keyname, "INTERVAL")) {
			rfreq = g_strdup(keyvalue);
		}
		else if (!strcasecmp(keyname, "UNTIL")) {
			time_t utime = osync_time_vtime2unix(keyvalue, 0);
			enddt = g_strdup_printf("%d", (int)utime);
		}
	}

	switch(rectype) {
		case RECUR_TYPE_NONE:
			break;
		case RECUR_TYPE_DAILY:
			xmlSetProp(node_to, "rtype", "Daily");
			break;
		case RECUR_TYPE_WEEKLY:
			xmlSetProp(node_to, "rtype", "Weekly");
			break;
		case RECUR_TYPE_MONTHLY_DAY:
			xmlSetProp(node_to, "rtype", "MonthlyDay");
			break;
		case RECUR_TYPE_MONTHLY_DATE:
			xmlSetProp(node_to, "rtype", "MonthlyDate");
			break;
		case RECUR_TYPE_YEARLY:
			xmlSetProp(node_to, "rtype", "Yearly");
			break;
	}
	
	if(weekdaysrule) {
		if(rectype == RECUR_TYPE_WEEKLY) {
			/* Weekly recurrence */
			int weekdays = 0;
			int i;
			gchar** weekdaystokens = g_strsplit(weekdaysrule, ",", 7);
			for (i=0; weekdaystokens[i] != NULL; i++) {
				if (strstr(weekdaystokens[i], "MO"))
					weekdays |= 1;
				else if (strstr(weekdaystokens[i], "TU"))
					weekdays |= 2;
				else if (strstr(weekdaystokens[i], "WE"))
					weekdays |= 4;
				else if (strstr(weekdaystokens[i], "TH"))
					weekdays |= 8;
				else if (strstr(weekdaystokens[i], "FR"))
					weekdays |= 16;
				else if (strstr(weekdaystokens[i], "SA"))
					weekdays |= 32;
				else if (strstr(weekdaystokens[i], "SU"))
					weekdays |= 64;
			}
			char *rweekdays = g_strdup_printf("%d", weekdays);
			xmlSetProp(node_to, "rweekdays", rweekdays);
			g_free(rweekdays);
		}
		else {
			/* MonthlyDate recurrence */
			int weekno = 0;
			char *tmp_wday = g_strdup("XX");
			sscanf(weekdaysrule, "%d%2s", &weekno, tmp_wday);
			g_free(tmp_wday);
			char *rposition = g_strdup_printf("%d", weekno);
			xmlSetProp(node_to, "rposition", rposition);
			g_free(rposition);
		}
		g_free(weekdaysrule);
	}
	
	if(rfreq) {
		xmlSetProp(node_to, "rfreq", rfreq);
		g_free(rfreq);
	}
	
	if(enddt) {
		xmlSetProp(node_to, "rhasenddate", "1");
		xmlSetProp(node_to, "enddt", enddt);
		g_free(enddt);
	}
	else {
		xmlSetProp(node_to, "rhasenddate", "0");
	}
}

void xml_todo_alarm_attr_to_xmlfield(const char *alarmstr, OSyncXMLFormat *out_xmlformat, time_t *starttime, OSyncError **error) {
	/* Convert Alarms attribute on todo items to OpenSync XML */
	if(alarmstr && strlen(alarmstr) > 0) {
		gchar** alarmentries = g_strsplit(alarmstr, ";", 0);
		int i,j;
		for(j=0; alarmentries[j] != NULL; j++) {
			OSyncXMLField *out_xmlfield = osync_xmlfield_new(out_xmlformat, "Alarm", error);
			
			// Opie alarm entry format: ddmmyyyyhhmmss:0:<0=silent,1=loud>:[;nextalarmentry]
			char *alarmdatestr = NULL;
			int alarmsound = 0;
			gchar** alarmargs = g_strsplit(alarmentries[j], ":", 0);
			for(i=0; alarmargs[i]!=NULL; i++) {
				if(i==0) {
					if(strlen(alarmargs[i]) == 14) {
						char *dateonly = g_strndup(alarmargs[i], 8);
						alarmdatestr = g_strdup_printf("%sT%s", dateonly, alarmargs[i] + 8);
						g_free(dateonly);
					}
				}
				else if(i==2)
					alarmsound = atoi(alarmargs[i]);
			}
			g_strfreev(alarmargs);
			
			if(alarmsound == 1)
				osync_xmlfield_set_key_value(out_xmlfield, "AlarmAction", "AUDIO");
			else
				osync_xmlfield_set_key_value(out_xmlfield, "AlarmAction", "DISPLAY");
			
			if(alarmdatestr) {
				struct tm *alarmtm = osync_time_vtime2tm(alarmdatestr);
				time_t alarmtime = mktime(alarmtm);
				g_free(alarmtm);
				char *alarmdatestr_utc = osync_time_unix2vtime(&alarmtime);
				
				if(starttime) {
					char *alarmdu = osync_time_sec2alarmdu((int)difftime(alarmtime, *starttime)); 
					if(alarmdu) {
						osync_xmlfield_set_key_value(out_xmlfield, "AlarmTrigger", alarmdu);
						g_free(alarmdu);
					}
				}
				g_free(alarmdatestr_utc);
				
				g_free(alarmdatestr);
			}
		}
		g_strfreev(alarmentries);
	}
}

void xmlformat_todo_alarms_to_attr(OSyncXMLFormat *in_xmlformat, xmlNode *node_to, const char *duedate) {
	/* Convert OpenSync XML Alarm entries on a todo node to Opie Alarms attribute value */
	
	GString *alarms = g_string_new("");
	
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Alarm", fieldname)) {
			char *alarmdatestr = NULL;
			const char *trigger = osync_xmlfield_get_key_value(in_xmlfield, "AlarmTrigger");
			if(trigger) {
				struct tm *alarmtm = NULL;
				time_t alarmtime = 0;
					
				if(duedate) {
					int alarmdiff = osync_time_alarmdu2sec(trigger);
					alarmtm = osync_time_vtime2tm(duedate);
					alarmtime = timegm(alarmtm);
					alarmtime += alarmdiff;
				}
				
				if(alarmtm) {
					struct tm *alarmtm_local = g_malloc0(sizeof(struct tm));
					localtime_r(&alarmtime, alarmtm_local);
					alarmdatestr = g_strdup_printf("%02d%02d%04d%02d%02d%02d", 
																				alarmtm_local->tm_mday, alarmtm_local->tm_mon + 1, alarmtm_local->tm_year + 1900, 
																				alarmtm_local->tm_hour, alarmtm_local->tm_min, alarmtm_local->tm_sec);
					g_free(alarmtm_local);
					g_free(alarmtm);
				}
			}
			
			if(alarmdatestr) {
				int alarmsound = 0;
				const char *alarmaction = osync_xmlfield_get_key_value(in_xmlfield, "AlarmAction");
				if(alarmaction) {
					if(!strcmp(alarmaction, "AUDIO"))
						alarmsound = 1;
				}
				g_string_append_printf(alarms, "%s:0:%d:;", alarmdatestr, alarmsound);
			}
			
		}
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
	
	if(alarms->len > 0) {
		g_string_truncate(alarms, alarms->len - 1);
		xmlSetProp(node_to, "Alarms", alarms->str);
	}
	g_string_free(alarms, TRUE);
}

void xmlformat_cal_alarms_to_attr(OSyncXMLFormat *in_xmlformat, xmlNode *node_to, time_t *starttime) {
	/* Convert OpenSync XML Alarm entries on a calendar event node to Opie alarm/sound attribute values */
	
	int alarmseconds = 15 * 60; // Default 15 minutes
		
	OSyncXMLField *in_xmlfield = osync_xmlformat_get_first_field(in_xmlformat);
	while(in_xmlfield) {
		const char *fieldname = osync_xmlfield_get_name(in_xmlfield);
		if(!strcmp("Alarm", fieldname)) {
			const char *trigger = osync_xmlfield_get_key_value(in_xmlfield, "AlarmTrigger");
			if(trigger)
				alarmseconds = osync_time_alarmdu2sec(trigger);
			
			const char *alarmaction = osync_xmlfield_get_key_value(in_xmlfield, "AlarmAction");
			int alarmsound = 0;
			if(alarmaction) {
				if(!strcmp(alarmaction, "AUDIO"))
					alarmsound = 1;
			}
			
			char *alarmstr = g_strdup_printf("%d", alarmseconds / 60);
			xmlSetProp(node_to, "alarm", alarmstr);
			g_free(alarmstr);
			
			if(alarmsound == 1)
				xmlSetProp(node_to, "sound", "loud");
			else
				xmlSetProp(node_to, "sound", "silent");
			
			/* Opie calendar only supports one alarm, so take the first one */
			break;
		}
		
		in_xmlfield = osync_xmlfield_get_next(in_xmlfield);
	}
}
