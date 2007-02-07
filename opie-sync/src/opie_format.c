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
#include "opie_xml.h"
#include "opie_format.h"
#include "config.h"

#include <opensync/opensync_xml.h>

enum OpieTodoState {
	OPIE_TODO_STATE_STARTED     = 0,
	OPIE_TODO_STATE_POSTPONED   = 1,
	OPIE_TODO_STATE_FINISHED    = 2,
	OPIE_TODO_STATE_NOT_STARTED = 3
};


/** Convert Opie XML contact to OpenSync XML contact 
 * 
 **/
static osync_bool conv_opie_xml_contact_to_xml_contact(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	int j;
//	anon_data* anon;
	gchar** emailtokens;
	struct _xmlAttr *iprop;
	xmlNode *on_name = NULL;
	xmlNode *on_organisation = NULL;
	xmlNode *on_homeaddress = NULL;
	xmlNode *on_workaddress = NULL;
	xmlNode *on_temp = NULL;
		
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
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_root = osxml_node_add_root(odoc, "contact");
	
	if(!strcasecmp(icur->name, "Contact"))
	{
		/* this is a contact element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if ( (!strcasecmp(iprop->name, "FirstName"))
					|| (!strcasecmp(iprop->name, "MiddleName"))
					|| (!strcasecmp(iprop->name, "LastName"))
					|| (!strcasecmp(iprop->name,"Suffix")) )
				{
					if (!on_name)
						on_name = xmlNewTextChild(on_root, NULL, (xmlChar*)"Name", NULL);
					
					if (!strcasecmp(iprop->name, "FirstName"))
						osxml_node_add(on_name, "FirstName", iprop->children->content);
					else if (!strcasecmp(iprop->name, "MiddleName"))
						osxml_node_add(on_name, "Additional", iprop->children->content);
					else if (!strcasecmp(iprop->name, "LastName"))
						osxml_node_add(on_name, "LastName", iprop->children->content);
					else if (!strcasecmp(iprop->name,"Suffix"))
						osxml_node_add(on_name, "Suffix", iprop->children->content);
				}
				else if ( (!strcasecmp(iprop->name, "Company"))
					|| (!strcasecmp(iprop->name, "Department"))
					|| (!strcasecmp(iprop->name, "Office")) )
				{
					if (!on_organisation)
						on_organisation = xmlNewTextChild(on_root, NULL, (xmlChar*)"Organization", NULL);
					
					if (!strcasecmp(iprop->name, "Company"))
						osxml_node_add(on_organisation, "Name", iprop->children->content);
					if (!strcasecmp(iprop->name, "Department"))
						osxml_node_add(on_organisation, "Department", iprop->children->content);
					if (!strcasecmp(iprop->name, "Office"))
						osxml_node_add(on_organisation, "Unit", iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "FileAs"))
				{
					/* File-as. This is what the Evo plugin does, so copy it. */
					xmlNode *on_formattedname = xmlNewTextChild( on_root, NULL, (xmlChar*)"FormattedName", NULL);
					xmlNewTextChild(on_formattedname, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Emails"))
				{
					emailtokens = g_strsplit(iprop->children->content," ",3);
					for(j=0;emailtokens[j]!=NULL;j++) 
					{
						xmlNode *on_email = xmlNewTextChild(on_root, NULL, (xmlChar*)"EMail", NULL);
						xmlNewTextChild(on_email, NULL, (xmlChar*)"Content", (xmlChar*)emailtokens[j]);
					}
					g_strfreev(emailtokens);
				}
				else if(!strcasecmp(iprop->name, "Categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					xmlNode *on_categories = xmlNewTextChild(on_root, NULL, (xmlChar*)"Categories", NULL);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						xmlNewTextChild(on_categories, NULL, (xmlChar*)"Category", (xmlChar*)categorytokens[j]);
					}
					g_strfreev(categorytokens);
				}
				else if(!strcasecmp(iprop->name, "DefaultEmail"))
				{
					xmlNode *on_email = xmlNewTextChild(on_root, NULL, (xmlChar*)"EMail", NULL);
					xmlNewTextChild(on_email, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
					/* this is the preferred email address */
					osxml_node_add(on_email, "Type", "PREF" );
				}
				else if(!strcasecmp(iprop->name, "HomePhone"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "HOME" );
					/* Removed the VOICE tags for the moment as they are assumed if not present, 
					   and if they are KDEPIM shows them up as "Other" */
					/* osxml_node_add(on_phone, "Type", "VOICE" ); */
				}
				else if(!strcasecmp(iprop->name, "HomeFax"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "HOME" );
					osxml_node_add(on_phone, "Type", "FAX" );
				}
				else if(!strcasecmp(iprop->name, "HomeMobile"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "HOME" );
					osxml_node_add(on_phone, "Type", "CELL" );
				}
				else if(!strcasecmp(iprop->name, "BusinessPhone"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "WORK" );
					/* Removed the VOICE tags for the moment as they are assumed if not present, 
					   and if they are KDEPIM shows them up as "Other" */
					/* osxml_node_add(on_phone, "Type", "VOICE" ); */
				}
				else if(!strcasecmp(iprop->name, "BusinessFax"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "WORK" );
					osxml_node_add(on_phone, "Type", "FAX" );
				}
				else if(!strcasecmp(iprop->name, "BusinessMobile"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "WORK" );
					osxml_node_add(on_phone, "Type", "CELL" );
				}
				else if(!strcasecmp(iprop->name, "BusinessPager"))
				{
					xmlNode *on_phone = xmlNewTextChild(on_root, NULL, (xmlChar*)"Telephone", NULL);
					osxml_node_add(on_phone, "Content", iprop->children->content );
					osxml_node_add(on_phone, "Type", "WORK" );
					osxml_node_add(on_phone, "Type", "PAGER" );
				}
				else if ( (!strcasecmp(iprop->name, "HomeStreet"))
					|| (!strcasecmp(iprop->name, "HomeCity"))
					|| (!strcasecmp(iprop->name, "HomeState"))
					|| (!strcasecmp(iprop->name,"HomeZip"))
					|| (!strcasecmp(iprop->name,"HomeCountry")) )
				{
					if (!on_homeaddress)
						on_homeaddress = xmlNewTextChild(on_root, NULL, (xmlChar*)"Address", NULL);
					
					if (!strcasecmp(iprop->name, "HomeStreet"))
						osxml_node_add(on_homeaddress, "Street", iprop->children->content);
					else if (!strcasecmp(iprop->name, "HomeCity"))
						osxml_node_add(on_homeaddress, "City", iprop->children->content);
					else if (!strcasecmp(iprop->name, "HomeState"))
						osxml_node_add(on_homeaddress, "Region", iprop->children->content);
					else if (!strcasecmp(iprop->name, "HomeZip"))
						osxml_node_add(on_homeaddress, "PostalCode", iprop->children->content);
					else if (!strcasecmp(iprop->name,"HomeCountry"))
						osxml_node_add(on_homeaddress, "Country", iprop->children->content);
					osxml_node_add(on_homeaddress, "Type", "HOME" );
				}
				else if ( (!strcasecmp(iprop->name, "BusinessStreet"))
					|| (!strcasecmp(iprop->name, "BusinessCity"))
					|| (!strcasecmp(iprop->name, "BusinessState"))
					|| (!strcasecmp(iprop->name,"BusinessZip"))
					|| (!strcasecmp(iprop->name,"BusinessCountry")) )
				{
					if (!on_workaddress)
						on_workaddress = xmlNewTextChild(on_root, NULL, (xmlChar*)"Address", NULL);
					
					if (!strcasecmp(iprop->name, "BusinessStreet"))
						osxml_node_add(on_workaddress, "Street", iprop->children->content);
					else if (!strcasecmp(iprop->name, "BusinessCity"))
						osxml_node_add(on_workaddress, "City", iprop->children->content);
					else if (!strcasecmp(iprop->name, "BusinessState"))
						osxml_node_add(on_workaddress, "Region", iprop->children->content);
					else if (!strcasecmp(iprop->name, "BusinessZip"))
						osxml_node_add(on_workaddress, "PostalCode", iprop->children->content);
					else if (!strcasecmp(iprop->name,"BusinessCountry"))
						osxml_node_add(on_workaddress, "Country", iprop->children->content);
					osxml_node_add(on_workaddress, "Type", "WORK" );
				}
				else if(!strcasecmp(iprop->name, "HomeWebPage"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Url", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "BusinessWebPage"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Spouse"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Spouse", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Birthday"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Birthday", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Anniversary"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Anniversary", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Nickname"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Nickname", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Children"))
				{
					/* FIXME handle this field */
				}
				else if(!strcasecmp(iprop->name, "Notes"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Note", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Uid"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Uid", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
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
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Assistant", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Manager"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Manager", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "Profession"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Profession", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else if(!strcasecmp(iprop->name, "JobTitle"))
				{
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Role", NULL );
					xmlNewTextChild( on_temp, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content );
				}
				else
				{
					/* FIXME handle unknown fields somehow? */
				}
			}
		}
	}

	*free_input = TRUE;
	*output = (char *)odoc;
	*outpsize = sizeof(odoc);
	
	xmlFreeDoc(idoc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)odoc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}    


/** Convert OpenSync XML contact to Opie XML contact 
 * 
 **/
static osync_bool conv_xml_contact_to_opie_xml_contact(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
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

	int i, numnodes;
	xmlXPathObject *xobj;
	xmlNodeSet *nodes;

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
                    __func__, user_data, input, inpsize, output, 
                    outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", 
                    osxml_write_to_string((xmlDoc *)input));
	
	/* Get the root node of the input document */
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
                                "Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"contact")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
                                "Wrong xml root element");
		goto error;
	}

	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_contact = osxml_node_add_root(odoc, "Contact");
	
	/* Name */
	xmlNode *cur = osxml_get_node(root, "Name");
	if (cur) {
		xml_node_to_attr(cur, "LastName",   on_contact, "LastName");
		xml_node_to_attr(cur, "FirstName",  on_contact, "FirstName");
		xml_node_to_attr(cur, "Suffix",     on_contact, "Suffix");
		xml_node_to_attr(cur, "Additional", on_contact, "MiddleName");
	} else {
		osync_trace(TRACE_INTERNAL, "No Name node found" );
	}

	/* Company */
	cur = osxml_get_node(root, "Organization");
	if (cur) {
		xml_node_to_attr(cur, "Name",         on_contact, "Company");
		xml_node_to_attr(cur, "Department",   on_contact, "Department");
		xml_node_to_attr(cur, "Unit",         on_contact, "Office");
	}

	/* Telephone */
	xobj = osxml_get_nodeset((xmlDoc *)root, "/Telephone");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ ) {
		cur = nodes->nodeTab[i];
		unsigned int type = 0;
		xmlXPathObject *xobj2 = osxml_get_nodeset((xmlDoc *)cur, "/Type");
		xmlNodeSet *nodes2 = xobj2->nodesetval;
		int numnodes2 = (nodes2) ? nodes2->nodeNr : 0;
		osync_trace(TRACE_INTERNAL, "Telephone found %d types\n", numnodes2 );
		int j;
		for ( j = 0; j < numnodes2; j++ )
		{
			xmlNode *cur2 = nodes2->nodeTab[j];
			char *typeName = (char*)xmlNodeGetContent(cur2);
			if(typeName) {
				if ( strcasecmp( typeName, "HOME" ) == 0 )
					type |= PT_HOME;
				else if ( strcasecmp( typeName, "WORK" ) == 0 )
					type |= PT_WORK;
				else if ( strcasecmp( typeName, "VOICE" ) == 0 )
					type |= PT_VOICE;
				else if ( strcasecmp( typeName, "CELL" ) == 0 )
					type |= PT_CELL;
				else if ( strcasecmp( typeName, "FAX" ) == 0 )
					type |= PT_FAX;
				else if ( strcasecmp( typeName, "PAGER" ) == 0 )
					type |= PT_PAGER;
				else {
					// ??????
				}
				xmlFree(typeName);
			}
		}
		xmlXPathFreeObject(xobj2);
		char *number = osxml_find_node(cur, "Content");

		osync_trace(TRACE_INTERNAL, "Telephone type %d %s", type, number );

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
	xmlXPathFreeObject(xobj);
	
	/* EMail */
	xobj = osxml_get_nodeset((xmlDoc *)root, "/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	char *emailaddr;
	GString *emails = g_string_new("");
	for ( i = 0; i < numnodes; i++ ) {
		cur = nodes->nodeTab[i];
		emailaddr = osxml_find_node(cur, "Content");
		g_string_append(emails, emailaddr);
		if(i < numnodes - 1)
			g_string_append_c(emails, ' ');

		xmlXPathObject *xobj2 = osxml_get_nodeset((xmlDoc *)cur, "/Type");
		xmlNodeSet *nodes2 = xobj2->nodesetval;
		int numnodes2 = (nodes2) ? nodes2->nodeNr : 0;
		int j;
		for ( j = 0; j < numnodes2; j++ )
		{
			xmlNode *cur2 = nodes2->nodeTab[j];
			char *type = (char*)xmlNodeGetContent(cur2);
			if ( type != NULL ) {
				if( strcasecmp( type, "PREF" ) == 0 ) {
					xmlSetProp(on_contact, "DefaultEmail", emailaddr);
					break;
				}
				xmlFree(type);
			}
		}
		xmlXPathFreeObject(xobj2);
		xmlFree(emailaddr);
	}
	xmlXPathFreeObject(xobj);
	xmlSetProp(on_contact, "Emails", emails->str);
	g_string_free(emails, TRUE);
	
	
	/* Addresses */
	xobj = osxml_get_nodeset((xmlDoc *)root, "/Address" );
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ ) {
		cur = nodes->nodeTab[i];
		char *type = osxml_find_node(cur, "Type");
		if ( strcasecmp( type, "HOME" ) == 0 ) {
			xml_node_to_attr(cur, "Street",     on_contact, "HomeStreet");
			xml_node_to_attr(cur, "City",       on_contact, "HomeCity");
			xml_node_to_attr(cur, "Region",     on_contact, "HomeState");
			xml_node_to_attr(cur, "PostalCode", on_contact, "HomeZip");
			xml_node_to_attr(cur, "Country",    on_contact, "HomeCountry");
		}
		else if ( strcasecmp( type, "WORK" ) == 0 ) {
			xml_node_to_attr(cur, "Street",     on_contact, "BusinessStreet");
			xml_node_to_attr(cur, "City",       on_contact, "BusinessCity");
			xml_node_to_attr(cur, "Region",     on_contact, "BusinessState");
			xml_node_to_attr(cur, "PostalCode", on_contact, "BusinessZip");
			xml_node_to_attr(cur, "Country",    on_contact, "BusinessCountry");
		} 
		else
		{
			// FIXME put it in anon???
		}
	}

	/* Title */
	cur = osxml_get_node(root, "Role");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "JobTitle");

	/* Note */
	cur = osxml_get_node(root, "Note");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Notes");
	
	/* Categories */
	xml_categories_to_attr(root, on_contact, "Categories");

	/* Spouse */
	cur = osxml_get_node(root, "Spouse");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Spouse");

	/* Nickname */
	cur = osxml_get_node(root, "Nickname");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Nickname");

	/* Assistant */
	cur = osxml_get_node(root, "Assistant");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Assistant");

	/* Manager */
	cur = osxml_get_node(root, "Manager");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Manager");

	/* Profession */
	cur = osxml_get_node(root, "Profession");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Profession");
	
	/* Birthday */
	cur = osxml_get_node(root, "Birthday");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Birthday");

	/* Anniversary */
	cur = osxml_get_node(root, "Anniversary");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Anniversary");

	/* Home webpage */
	cur = osxml_get_node(root, "Url");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "HomeWebPage");

	/* File-as */
	cur = osxml_get_node(root, "FormattedName");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "FileAs");

	/* UID */
	cur = osxml_get_node(root, "Uid");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Uid");

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

static void destroy_opie_contact(char *input, size_t inpsize)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, input, inpsize);
	
	printf("OPIE: freeing\n");
	g_free(input);	

	osync_trace(TRACE_EXIT, "%s", __func__);
}


/** Convert Opie XML todo to OpenSync XML todo 
 * 
 **/
static osync_bool conv_opie_xml_todo_to_xml_todo(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	struct _xmlAttr *iprop;
	xmlNode *on_curr;
	int j;
		
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
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_root = osxml_node_add_root(odoc, "vcal");
	on_root = xmlNewTextChild(on_root, NULL, (xmlChar*)"Todo", NULL);
	
	if(!strcasecmp(icur->name, "Task"))
	{
		/* this is a todo element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if(!strcasecmp(iprop->name, "Summary")) 
				{
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Summary", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Description"))
				{
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Description", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Priority"))
				{
					/* Priority is 1-5 on Opie, 0-9 in OpenSync XML (0 = undefined) */
					int priority = atoi(iprop->children->content);
					char *prio = g_strdup_printf("%d", priority);
					on_curr = xmlNewTextChild(on_root, NULL, (xmlChar*)"Priority", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)prio);
					g_free(prio);
				}
				else if(!strcasecmp(iprop->name, "Progress"))
				{
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"PercentComplete", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
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
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Status", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)status);
				}
				else if(!strcasecmp(iprop->name, "StartDate"))
				{
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateStarted", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
				}
				else if(!strcasecmp(iprop->name, "Uid"))
				{
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Uid", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "Categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					xmlNode *on_categories = xmlNewTextChild(on_root, NULL, (xmlChar*)"Categories", NULL);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						xmlNewTextChild(on_categories, NULL, (xmlChar*)"Category", (xmlChar*)categorytokens[j]);
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
					on_curr = xmlNewTextChild(on_root, NULL, (xmlChar*)"Completed", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)completeDate);
					// RFC2445 says the default value type is DATE-TIME. But Opie only
					// stores DATE as completed date => alter VALUE to DATE
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
					xmlFree(completeDate);
				}
			}
			xmlFree(completed);
		}
		
		/* Due date */
		GDate *duedate = NULL;
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
					char *duedatestr = g_strdup_printf("%04d%02d%02d", dateyear, datemonth, dateday);
					duedate = g_date_new_dmy(dateday, datemonth, dateyear);
					on_curr = xmlNewTextChild(on_root, NULL, (xmlChar*)"DateDue", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)duedatestr);
					g_free(duedatestr);
					// RFC2445 says the default value type is DATE-TIME. But Opie only
					// stores DATE as due date => alter VALUE to DATE
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
				}
				if(datedaystr)   xmlFree(datedaystr);
				if(datemonthstr) xmlFree(datemonthstr);
				if(dateyearstr)  xmlFree(dateyearstr);
			}
			xmlFree(hasDate);
		}
	
		/* Recurrence */
		xml_recur_attr_to_node(icur, on_root, duedate);
		
		/* Alarms */
		char *alarmstr = xmlGetProp(icur, "Alarms");
		if(alarmstr) {
			xml_todo_alarm_attr_to_node(alarmstr, on_root, NULL);
			xmlFree(alarmstr);
		}
		
		if(duedate)
			g_date_free(duedate);
	}

	*free_input = TRUE;
	*output = (char *)odoc;
	*outpsize = sizeof(odoc);
	
	xmlFreeDoc(idoc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)odoc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

/** Convert OpenSync XML todo to Opie XML todo 
 * 
 **/
static osync_bool conv_xml_todo_to_opie_xml_todo(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	xmlNode *icur;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, user_data, input, inpsize, output, 
							outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", 
							osxml_write_to_string((xmlDoc *)input));
	
	/* Get the root node of the input document */
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"vcal")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Wrong xml root element");
		goto error;
	}

	root = osxml_get_node(root, "Todo");
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Unable to find Todo node inside vcal node");
		goto error;
	}
	
	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_todo = osxml_node_add_root(odoc, "Task");
	
	/* Summary */
	icur = osxml_get_node(root, "Summary");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_todo, "Summary");
	}

	/* Description */
	icur = osxml_get_node(root, "Description");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_todo, "Description");
	}

	/* Priority */
	icur = osxml_get_node(root, "Priority");
	if (icur) {
		/* Priority is 1-5 on Opie, 0-9 in OpenSync XML (0 = undefined) */
		icur = osxml_get_node(icur, "Content");
		if (icur) {
			char *prio = (char *)xmlNodeGetContent(icur);
			if (prio) {
				int priority = atoi(prio);
				xmlFree(prio);
				if (priority < 1) {
					/* Invalid or (more likely) unspecified priority */
					priority = 3;
				}
				else if (priority > 5) {
					priority = 5;
				}
				prio = g_strdup_printf("%d", priority);
				xmlSetProp(on_todo, "Priority", prio);
				g_free(prio);
			}
		}
	}
	
	/* Completed */
	icur = osxml_get_node(root, "Completed");
	if (icur) {
		icur = osxml_get_node(icur, "Content");
		if (icur) {
			char *completedstr = (char *) xmlNodeGetContent(icur);
			struct tm *completed = osync_time_vtime2tm(completedstr);
			xmlFree(completedstr);
			completedstr = g_strdup_printf("%04d%02d%02d", completed->tm_year + 1900, (completed->tm_mon + 1), completed->tm_mday);
			xmlSetProp(on_todo, "Completed", "1");
			xmlSetProp(on_todo, "CompletedDate", completedstr);
			g_free(completedstr);
			g_free(completed);
		}
	}
	else {
		xmlSetProp(on_todo, "Completed", "0");
	}
	
	/* Start date */
	icur = osxml_get_node(root, "DateStarted");
	if (icur) {
		icur = osxml_get_node(icur, "Content");
		if (icur) {
			char *startedstr = (char *) xmlNodeGetContent(icur);
			struct tm *started = osync_time_vtime2tm(startedstr);
			xmlFree(startedstr);
			startedstr = g_strdup_printf("%04d%02d%02d", (started->tm_year + 1900), (started->tm_mon + 1), started->tm_mday);
			xmlSetProp(on_todo, "StartDate", startedstr);
			g_free(startedstr);
			g_free(started);
		}
	}
	else {
		xmlSetProp(on_todo, "StartDate", "0");
	}
	
	/* Due date */
	icur = osxml_get_node(root, "DateDue");
	if (icur) {
		icur = osxml_get_node(icur, "Content");
		if (icur) {
			char *duestr = (char *) xmlNodeGetContent(icur);
			struct tm *due = osync_time_vtime2tm(duestr);
			xmlFree(duestr);
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
	else {
		xmlSetProp(on_todo, "HasDate", "0");
	}
	
	/* Progress */	
	icur = osxml_get_node(root, "PercentComplete");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_todo, "Progress");
	}
	
	/* State */
	icur = osxml_get_node(root, "Status");
	if (icur) {
		icur = osxml_get_node(icur, "Content");
		if (icur) {
			char *status = (char *) xmlNodeGetContent(icur);
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
			xmlFree(status);
		}
	}
	
	/* Recurrence */
	xml_recur_node_to_attr(root, on_todo);
	
	/* Alarms */
	xml_todo_alarm_node_to_attr(root, on_todo);
	
	/* Categories */
	xml_categories_to_attr(root, on_todo, "Categories");

	/* UID */
	icur = osxml_get_node(root, "Uid");
	if (icur)
		xml_node_to_attr(icur, "Content", on_todo, "Uid");
	
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
static osync_bool conv_opie_xml_event_to_xml_event(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
	struct _xmlAttr *iprop;
	xmlNode *on_curr;
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
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_root = osxml_node_add_root(odoc, "vcal");
	on_root = xmlNewTextChild(on_root, NULL, (xmlChar*)"Event", NULL);
	
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
		
		/* this is a todo element - the attributes are the data we care about */
		for (iprop = icur->properties; iprop; iprop=iprop->next) 
		{
			if (iprop->children && iprop->children->content)
			{
				if(!strcasecmp(iprop->name, "description"))
				{
					xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Summary", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "note")) 
				{
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Description", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "location")) 
				{
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Location", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
				else if(!strcasecmp(iprop->name, "created")) 
				{
					time_t createtime = (time_t)atoi(iprop->children->content);
					char *createvtime = osync_time_unix2vtime(&createtime); 
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateCreated", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)createvtime);
					g_free(createvtime);
				}
				else if(!strcasecmp(iprop->name, "start")) 
				{
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateStarted", NULL);
					time_t starttime = (time_t)atoi(iprop->children->content);
					if(allday) {
						struct tm *localtm = g_malloc0(sizeof(struct tm));
						localtime_r(&starttime, localtm);
						char *startvdate = g_strdup_printf("%04d%02d%02d", localtm->tm_year + 1900, (localtm->tm_mon + 1), localtm->tm_mday);
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)startvdate);
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
						g_free(startvdate);
						g_free(localtm);
					}
					else {
						char *startvtime = osync_time_unix2vtime(&starttime); 
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)startvtime);
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
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateEnd", NULL);
					time_t endtime = (time_t)atoi(iprop->children->content);
					if(allday) {
						struct tm *localtm = g_malloc0(sizeof(struct tm));
						endtime += 1;
						localtime_r(&endtime, localtm);
						char *endvdate = g_strdup_printf("%04d%02d%02d", localtm->tm_year + 1900, (localtm->tm_mon + 1), localtm->tm_mday);
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)endvdate);
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
						g_free(endvdate);
						g_free(localtm);
					}
					else {
						char *endvtime = osync_time_unix2vtime(&endtime); 
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)endvtime);
						g_free(endvtime);
					}
				}
				else if(!strcasecmp(iprop->name, "categories"))
				{
					gchar** categorytokens = g_strsplit(iprop->children->content, "|", 0);
					xmlNode *on_categories = xmlNewTextChild(on_root, NULL, (xmlChar*)"Categories", NULL);
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						xmlNewTextChild(on_categories, NULL, (xmlChar*)"Category", (xmlChar*)categorytokens[j]);
					}
					g_strfreev(categorytokens);
				}
				else if(!strcasecmp(iprop->name, "uid")) 
				{
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Uid", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)iprop->children->content);
				}
			}
			/* FIXME Stuff to handle:
			timezone?
			*/
		}
		
		/* Alarm */
		char *alarmminsstr = xmlGetProp(icur, "alarm");
		if(alarmminsstr) {
			xmlNode *on_alarm = xmlNewTextChild( on_root, NULL, (xmlChar*)"Alarm", NULL);
			
			int alarmsound = 0;
			char *alarmsoundstr = xmlGetProp(icur, "sound");
			if(alarmsoundstr) {
				if(!strcmp(alarmsoundstr, "loud"))
					alarmsound = 1;
				xmlFree(alarmsoundstr);
			}
			if(alarmsound == 1)
				xmlNewTextChild( on_alarm, NULL, (xmlChar*)"AlarmAction", (xmlChar*)"AUDIO");
			else
				xmlNewTextChild( on_alarm, NULL, (xmlChar*)"AlarmAction", (xmlChar*)"DISPLAY");
			
			int alarmseconds = -(atoi(alarmminsstr) * 60);
			char *alarmdu = osync_time_sec2alarmdu(alarmseconds);
			xmlNode *on_atrigger = xmlNewTextChild( on_alarm, NULL, (xmlChar*)"AlarmTrigger", NULL);
			xmlNewTextChild( on_atrigger, NULL, (xmlChar*)"Content", (xmlChar*)alarmdu);
			xmlNewTextChild( on_atrigger, NULL, (xmlChar*)"Value", (xmlChar*)"DURATION");
			xmlFree(alarmminsstr);
		}
		
		/* Recurrence */
		xml_recur_attr_to_node(icur, on_root, startdate);
	}

	*free_input = TRUE;
	*output = (char *)odoc;
	*outpsize = sizeof(odoc);
	
	xmlFreeDoc(idoc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)odoc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert OpenSync XML event to Opie XML event 
 * 
 **/
static osync_bool conv_xml_event_to_opie_xml_event(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	xmlNode *icur;
	time_t start_time = 0;
	time_t end_time = 0;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, user_data, input, inpsize, output, 
							outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", 
							osxml_write_to_string((xmlDoc *)input));
	
	/* Get the root node of the input document */
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"vcal")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Wrong xml root element");
		goto error;
	}

	root = osxml_get_node(root, "Event");
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Unable to find Event node inside vcal node");
		goto error;
	}
	
	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_event = osxml_node_add_root(odoc, "event");
	
	/* Summary */
	icur = osxml_get_node(root, "Summary");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_event, "description");
	}

	/* Description */
	icur = osxml_get_node(root, "Description");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_event, "note");
	}

	/* Location */
	icur = osxml_get_node(root, "Location");
	if (icur) {
		xml_node_to_attr(icur, "Content", on_event, "location");
	}

	/* Creation Date */
	icur = osxml_get_node(root, "DateCreated");
	if (icur) {
		xml_node_vtime_to_attr_time_t(icur, on_event, "created");
	}
	
	/* Start */
	icur = osxml_get_node(root, "DateStarted");
	if (icur) {
		start_time = xml_node_vtime_to_attr_time_t(icur, on_event, "start");
	}
	
	/* End */
	icur = osxml_get_node(root, "DateEnd");
	if (icur) {
		end_time = xml_node_vtime_to_attr_time_t(icur, on_event, "end");
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
	xml_cal_alarm_node_to_attr(root, on_event, &start_time);
	
	/* Recurrence */
	xml_recur_node_to_attr(root, on_event);
	
	/* Categories */
	xml_categories_to_attr(root, on_event, "categories");

	/* UID */
	icur = osxml_get_node(root, "Uid");
	if (icur)
		xml_node_to_attr(icur, "Content", on_event, "uid");

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
static osync_bool conv_opie_xml_note_to_xml_note(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", __func__, user_data, input, inpsize, output, outpsize, free_input, error);
		
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
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_root = osxml_node_add_root(odoc, "Note");
	
	if(!strcasecmp(icur->name, "note"))
	{
		// Summary
		char *value = xmlGetProp(icur, "name");
		if(value) {
			xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Summary", NULL);
			xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)value);
			xmlFree(value);
		}
		// Body
		value = osxml_find_node(icur, "content");
		if(value) {
			xmlNode *on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"Body", NULL);
			xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)value);
			xmlFree(value);
		}
	}

	*free_input = TRUE;
	*output = (char *)odoc;
	*outpsize = sizeof(odoc);
	
	xmlFreeDoc(idoc);

	osync_trace(TRACE_INTERNAL, "Output XML is:\n%s", osxml_write_to_string((xmlDoc *)odoc));
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/** Convert OpenSync XML note to Opie XML note (which is internal to the plugin)
 * 
 **/
static osync_bool conv_xml_note_to_opie_xml_note(void *user_data, char *input, int inpsize, char **output, int *outpsize, osync_bool *free_input, OSyncError **error)
{
	xmlNode *icur;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %i, %p, %p, %p, %p)", 
							__func__, user_data, input, inpsize, output, 
							outpsize, free_input, error);

	osync_trace(TRACE_INTERNAL, "Input XML is:\n%s", 
							osxml_write_to_string((xmlDoc *)input));
	
	/* Get the root node of the input document */
	xmlNode *root = xmlDocGetRootElement((xmlDoc *)input);
	if (!root) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Unable to get xml root element");
		goto error;
	}
	
	if (xmlStrcmp(root->name, (const xmlChar *)"Note")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Wrong xml root element");
		goto error;
	}

	/* Create a new output xml document */
	xmlDoc *odoc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *on_note = osxml_node_add_root(odoc, "note");
	
	/* Summary */
	icur = osxml_get_node(root, "Summary");
	if (icur) {
		char *value = osxml_find_node(icur, "Content");
		if(value) {
			xmlSetProp(on_note, "name", value);
			xmlFree(value);
		}
	}

	/* Body */
	icur = osxml_get_node(root, "Body");
	if (icur) {
		char *value = osxml_find_node(icur, "Content");
		if(value) {
			osxml_node_add(on_note, "content", value);
			xmlFree(value);
		}
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




void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "opie-xml-contact");
	osync_env_format_set_destroy_func(env, "opie-xml-contact", destroy_opie_contact); /* FIXME do we need this for all types? */
	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "opie-xml-todo");
	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "opie-xml-event");
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "opie-xml-note");

	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-contact", "xml-contact",      conv_opie_xml_contact_to_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact",      "opie-xml-contact", conv_xml_contact_to_opie_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-todo",    "xml-todo",         conv_opie_xml_todo_to_xml_todo);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo",         "opie-xml-todo",    conv_xml_todo_to_opie_xml_todo);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-event",   "xml-event",        conv_opie_xml_event_to_xml_event);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event",        "opie-xml-event",   conv_xml_event_to_opie_xml_event);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-note",    "xml-note",         conv_opie_xml_note_to_xml_note);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-note",         "opie-xml-note",    conv_xml_note_to_opie_xml_note);
}

void xml_node_to_attr(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname) {
	char *value = osxml_find_node(node_from, nodename);
	if(value && (strlen(value) > 0))
		xmlSetProp(node_to, attrname, value);
	xmlFree(value);
}

time_t xml_node_vtime_to_attr_time_t(xmlNode *node_from, xmlNode *node_to, const char *attrname) {
	char *vtime = osxml_find_node(node_from, "Content");
	time_t utime = 0;
	if(vtime) {
		char *vtimetype = osxml_find_node(node_from, "Value");
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
		xmlFree(vtime);
	}
	return utime;
}

void xml_categories_to_attr(xmlNode *item_node, xmlNode *node_to, const char *category_attr) {
	xmlNode *cur;
	xmlNode *categories_node;
	int i, numnodes;
	xmlXPathObject *xobj;
	xmlNodeSet *nodes;
	
	GString *categories = g_string_new("");
	xobj = osxml_get_nodeset((xmlDoc *)item_node, "/Categories" );
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ ) {
		categories_node = nodes->nodeTab[i];
		
		for (cur = categories_node->children; cur; cur = cur->next) {
			if(!strcmp(cur->name, "Category")) {
				char *cat_name = xmlNodeGetContent(cur);
				g_string_append_printf(categories, "%s|", cat_name);
				xmlFree(cat_name);
			}
		}
	}
	
	if(categories->len > 0) {
		g_string_truncate(categories, categories->len - 1);
		xmlSetProp(node_to, category_attr, categories->str);
	}
	g_string_free(categories, TRUE);
}

void xml_recur_attr_to_node(xmlNode *item_node, xmlNode *node_to, GDate *startdate) {
	/* Recurrence for todos and events */
	char *recurType = xmlGetProp(item_node, "rtype");
	if(recurType) {
		xmlNode *on_recur = xmlNewTextChild(node_to, NULL, (xmlChar*) "RecurrenceRule", NULL);
		
		/* Frequency */
		if(!strcmp(recurType, "Daily")) {
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=DAILY");
		}
		else if(!strcmp(recurType, "Weekly")) {
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=WEEKLY");
			
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
				
					xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) byday->str);
					g_string_free(byday, TRUE);
				}
				xmlFree(weekdays);
			}
		}
		else if(!strcmp(recurType, "MonthlyDate")) {
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=MONTHLY");
			if(startdate) {
				char *bymonthday = g_strdup_printf("BYMONTHDAY=%i", (int)g_date_get_day(startdate));
				xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) bymonthday);
				g_free(bymonthday);
			}
		}
		else if(!strcmp(recurType, "MonthlyDay")) {
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=MONTHLY");
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
						byday = g_strdup_printf("BYDAY=%iMO", weekno);
						break;
					case G_DATE_TUESDAY:
						byday = g_strdup_printf("BYDAY=%iTU", weekno);
						break;
					case G_DATE_WEDNESDAY:
						byday = g_strdup_printf("BYDAY=%iWE", weekno);
						break;
					case G_DATE_THURSDAY:
						byday = g_strdup_printf("BYDAY=%iTH", weekno);
						break;
					case G_DATE_FRIDAY:
						byday = g_strdup_printf("BYDAY=%iFR", weekno);
						break;
					case G_DATE_SATURDAY:
						byday = g_strdup_printf("BYDAY=%iSA", weekno);
						break;
					case G_DATE_SUNDAY:
						byday = g_strdup_printf("BYDAY=%iSU", weekno);
						break;
					case G_DATE_BAD_WEEKDAY:
						break;
				}
				if(byday) {
					xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) byday);
					g_free(byday);
				}
			}
		}
		else if(!strcmp(recurType, "Yearly")) {
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=YEARLY");
		}

		/* Interval */
		char *interval = xmlGetProp(item_node, "rfreq");
		if(interval) {
			char *intervalstr = g_strdup_printf("INTERVAL=%s", interval);
			xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*)intervalstr);
			xmlFree(interval);
			g_free(intervalstr);
		}
		
		/* End date */
		char *hasEndDate = xmlGetProp(item_node, "rhasenddate");
		if(hasEndDate) {
			char *recurendstr = xmlGetProp(item_node, "enddt");
			if(recurendstr) {
				time_t recurendtime = (time_t)atoi(recurendstr);
				char *recurendvtime = osync_time_unix2vtime(&recurendtime); 
				char *until = g_strdup_printf("UNTIL=%s", recurendvtime);
				xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) until);
				g_free(recurendvtime);
				g_free(until);
				xmlFree(recurendstr);
			}
		}
		
		xmlFree(recurType);
	}
}

void xml_recur_node_to_attr(xmlNode *item_node, xmlNode *node_to) {
	/* Recurrence for todos and events */
	xmlNode *icur = osxml_get_node(item_node, "RecurrenceRule");
	if (icur) {
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
		
		/* read recurrence rules */
		xmlXPathObject *xobj = osxml_get_nodeset((xmlDoc *)icur, "/Rule" );
		xmlNodeSet *nodes = xobj->nodesetval;
		int numnodes = (nodes) ? nodes->nodeNr : 0;
		for ( i = 0; i < numnodes; i++ ) {
			icur = nodes->nodeTab[i];
			char *rulestr = (char*)xmlNodeGetContent(icur);
			gchar **rule = g_strsplit(rulestr, "=", 2);
			if (!strcasecmp(rule[0], "FREQ")) {
				if (!strcasecmp(rule[1], "DAILY")) {
					rectype = RECUR_TYPE_DAILY;
				}
				else if (!strcasecmp(rule[1], "WEEKLY")) {
					rectype = RECUR_TYPE_WEEKLY;
				}
				else if (!strcasecmp(rule[1], "MONTHLY")) {
					if(rectype != RECUR_TYPE_MONTHLY_DATE)
						rectype = RECUR_TYPE_MONTHLY_DAY;
				}
				else if (!strcasecmp(rule[1], "YEARLY")) {
					rectype = RECUR_TYPE_YEARLY;
				}
			}
			else if (!strcasecmp(rule[0], "BYDAY")) {
				weekdaysrule = g_strdup(rule[1]);
			}
			else if (!strcasecmp(rule[0], "BYMONTHDAY")) {
				if(rectype != RECUR_TYPE_YEARLY)
					rectype = RECUR_TYPE_MONTHLY_DATE;
			}
			else if (!strcasecmp(rule[0], "INTERVAL")) {
				rfreq = g_strdup(rule[1]);
			}
			else if (!strcasecmp(rule[0], "UNTIL")) {
				time_t utime = osync_time_vtime2unix(rule[1], 0);
				enddt = g_strdup_printf("%d", (int)utime);
			}
			xmlFree(rulestr);
			g_strfreev(rule);
		}
		xmlXPathFreeObject(xobj);
	
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
}

void xml_todo_alarm_attr_to_node(const char *alarmstr, xmlNode *node_to, time_t *starttime) {
	/* Convert Alarms attribute on todo items to OpenSync XML */
	if(alarmstr && strlen(alarmstr) > 0) {
		gchar** alarmentries = g_strsplit(alarmstr, ";", 0);
		int i,j;
		for(j=0; alarmentries[j] != NULL; j++) {
			xmlNode *on_alarm = xmlNewTextChild(node_to, NULL, (xmlChar*) "Alarm", NULL);
			
			// Opie alarm entry format: ddmmyyyyhhmmss:0:<0=silent,1=loud>:[;nextalarmentry]
			char *alarmdatestr = NULL;
			int alarmsound = 0;
			gchar** alarmargs = g_strsplit(alarmentries[j], ":", 0);
			for(i=0; alarmargs[i]!=NULL; i++) {
				if(i==0) {
					char *dateonly = g_strndup(alarmargs[i], 8);
					alarmdatestr = g_strdup_printf("%sT%s", dateonly, alarmargs[8]);
					g_free(dateonly);
				}
				else if(i==2)
					alarmsound = atoi(alarmargs[i]);
			}
			g_strfreev(alarmargs);
			
			if(alarmsound == 1)
				xmlNewTextChild(on_alarm, NULL, (xmlChar*)"AlarmAction", (xmlChar*) "AUDIO");
			else
				xmlNewTextChild(on_alarm, NULL, (xmlChar*)"AlarmAction", (xmlChar*) "DISPLAY");
			
			if(alarmdatestr) {
				struct tm *alarmtm = osync_time_vtime2tm(alarmdatestr);
				time_t alarmtime = mktime(alarmtm);
				g_free(alarmtm);
				char *alarmdatestr_utc = osync_time_unix2vtime(&alarmtime);
				
				if(starttime) {
					/* This is nice in theory, but Opie todo events don't support due time so
					  we can't use this code in practice */
					char *alarmdu = osync_time_sec2alarmdu((int)difftime(alarmtime, *starttime)); 
					if(alarmdu) {
						xmlNode *on_trigger = xmlNewTextChild(node_to, NULL, (xmlChar*) "AlarmTrigger", NULL);
						xmlNewTextChild(on_trigger, NULL, (xmlChar*)"Content", (xmlChar*) alarmdu);
						xmlNewTextChild(on_trigger, NULL, (xmlChar*)"Value", (xmlChar*) "DURATION");
					}
				}
				else {
					xmlNode *on_trigger = xmlNewTextChild(node_to, NULL, (xmlChar*) "AlarmTrigger", NULL);
					xmlNewTextChild(on_trigger, NULL, (xmlChar*)"Content", (xmlChar*) alarmdatestr_utc);
					xmlNewTextChild(on_trigger, NULL, (xmlChar*)"Value", (xmlChar*) "DATE-TIME");
					g_free(alarmdatestr_utc);
				}
				
				g_free(alarmdatestr);
			}
		}
		g_strfreev(alarmentries);
	}
}

void xml_todo_alarm_node_to_attr(xmlNode *item_node, xmlNode *node_to) {
	/* Convert OpenSync XML Alarm entries on a todo node to Opie Alarms attribute value */
	xmlNode *cur;
	xmlNode *alarm_node;
	int i, numnodes;
	xmlXPathObject *xobj;
	xmlNodeSet *nodes;
	
	GString *alarms = g_string_new("");
	xobj = osxml_get_nodeset((xmlDoc *)item_node, "/Alarm" );
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ ) {
		alarm_node = nodes->nodeTab[i];
		
		char *alarmdatestr = NULL;
		
		xmlNode *trigger_node = osxml_get_node(alarm_node, "AlarmTrigger");
		if(trigger_node) {
			char *typestr = NULL;
			char *contentstr = NULL;
			cur = osxml_get_node(trigger_node, "Value");
			if(cur)
				typestr = xmlNodeGetContent(cur);
			cur = osxml_get_node(trigger_node, "Content");
			if(cur)
				contentstr = xmlNodeGetContent(cur);
			
			if(contentstr && typestr) {
				struct tm *alarmtm = NULL;
				time_t alarmtime = 0;
				if(!strcmp(typestr, "DATE-TIME")) {
					alarmtm = osync_time_vtime2tm(contentstr);
					alarmtime = timegm(alarmtm);
				}
				else if (!strcmp(typestr, "DURATION")) {
					cur = osxml_get_node(item_node, "DateDue");
					if(cur) {
						cur = osxml_get_node(cur, "Content");
						if(cur) {
							char *duedatestr = xmlNodeGetContent(cur);
							if(duedatestr) {
								int alarmdiff = osync_time_alarmdu2sec(contentstr);
								alarmtm = osync_time_vtime2tm(duedatestr);
								alarmtime = timegm(alarmtm);
								alarmtime += alarmdiff;
								xmlFree(duedatestr);
							}
						}
					}
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
			
			if(contentstr)
				xmlFree(contentstr);
			if(typestr)
				xmlFree(typestr);
		}
		
		if(alarmdatestr) {
			cur = osxml_get_node(alarm_node, "AlarmAction");
			int alarmsound = 0;
			if(cur) {
				char *alarmaction = xmlNodeGetContent(cur);
				if(alarmaction) {
					if(!strcmp(alarmaction, "AUDIO"))
						alarmsound = 1;
					xmlFree(alarmaction);
				}
			}
			g_string_append_printf(alarms, "%s:0:%d:;", alarmdatestr, alarmsound);
		}
	}
	
	if(alarms->len > 0) {
		g_string_truncate(alarms, alarms->len - 1);
		xmlSetProp(node_to, "Alarms", alarms->str);
	}
	g_string_free(alarms, TRUE);
}

void xml_cal_alarm_node_to_attr(xmlNode *item_node, xmlNode *node_to, time_t *starttime) {
	/* Convert OpenSync XML Alarm entries on a calendar event node to Opie alarm/sound attribute values */
	xmlNode *cur;
	int alarmseconds = 15 * 60; /* Default 15 minutes */
		
	xmlNode *alarm_node = osxml_get_node(item_node, "Alarm");
	if(alarm_node) {
		xmlNode *trigger_node = osxml_get_node(alarm_node, "AlarmTrigger");
		if(trigger_node) {
			char *typestr = NULL;
			char *contentstr = NULL;
			cur = osxml_get_node(trigger_node, "Value");
			if(cur)
				typestr = xmlNodeGetContent(cur);
			cur = osxml_get_node(trigger_node, "Content");
			if(cur)
				contentstr = xmlNodeGetContent(cur);
			
			if(contentstr && typestr) {
				if(!strcmp(typestr, "DATE-TIME")) {
					if(starttime) {
						struct tm *alarmtm = osync_time_vtime2tm(contentstr);
						time_t alarmtime = timegm(alarmtm);
						alarmseconds = (int)difftime(alarmtime, *starttime);
						g_free(alarmtm);
					}
				}
				else if (!strcmp(typestr, "DURATION")) {
					alarmseconds = osync_time_alarmdu2sec(contentstr);
				}
			}
			
			if(contentstr)
				xmlFree(contentstr);
			if(typestr)
				xmlFree(typestr);
		
			char *alarmstr = g_strdup_printf("%d", alarmseconds / 60);
			xmlSetProp(node_to, "alarm", alarmstr);
			g_free(alarmstr);
			
			cur = osxml_get_node(alarm_node, "AlarmAction");
			int alarmsound = 0;
			if(cur) {
				char *alarmaction = xmlNodeGetContent(cur);
				if(alarmaction) {
					if(!strcmp(alarmaction, "AUDIO"))
						alarmsound = 1;
					xmlFree(alarmaction);
				}
			}
			
			if(alarmsound == 1)
				xmlSetProp(node_to, "sound", "loud");
			else
				xmlSetProp(node_to, "sound", "silent");
		}
	}
}