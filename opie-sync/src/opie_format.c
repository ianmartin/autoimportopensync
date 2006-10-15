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
						on_organisation = xmlNewTextChild(on_root, NULL, (xmlChar*)"Organisation", NULL);
					
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
					osxml_node_add(on_phone, "Type", "VOICE" );
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
					osxml_node_add(on_phone, "Type", "VOICE" );
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
					on_temp = xmlNewTextChild( on_root, NULL, (xmlChar*)"Title", NULL );
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
		xml_node_to_attr(cur, "Organisation", on_contact, "Company");
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
			if ( type & PT_VOICE )
				xmlSetProp(on_contact, "BusinessPhone", number);
			else if ( type & PT_FAX ) 
				xmlSetProp(on_contact, "BusinessFax", number);
			else if ( type & PT_CELL ) 
				xmlSetProp(on_contact, "BusinessMobile", number);
			else {
				// ???
			}
		}
		else if ( type & PT_VOICE )
			xmlSetProp(on_contact, "HomePhone", number);
		else if ( type & PT_FAX ) 
			xmlSetProp(on_contact, "HomeFax", number);
		else if ( type & PT_CELL ) 
			xmlSetProp(on_contact, "HomeMobile", number);
		else {
			// ???
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
		xmlFree(emailaddr);
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
					char *emailaddr = (char*)xmlNodeGetContent(cur); 
					xmlSetProp(on_contact, "DefaultEmail", emailaddr);
					xmlFree(emailaddr);
					break;
				}
				xmlFree(type);
			}
		}
		xmlXPathFreeObject(xobj2);
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
	cur = osxml_get_node(root, "Title");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "JobTitle");

	/* Note */
	cur = osxml_get_node(root, "Note");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "Notes");
	
	/* Categories */
	cur = osxml_get_node(root, "Categories");
	if (cur)
		xml_categories_to_attr(cur, on_contact, "Categories");

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
/*   char* home_webpage; */
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
			/* FIXME Stuff to handle:
				Alarms datehhmmss:0:<0=silent,1=loud>:[;nextalarmentry]
			*/
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
		char *hasDate = xmlGetProp(icur, "HasDate");
		if(hasDate) { 
			if(!strcmp(hasDate, "1")) {
				char *dateday   = xmlGetProp(icur, "DateDay");
				char *datemonth = xmlGetProp(icur, "DateMonth");
				char *dateyear  = xmlGetProp(icur, "DateYear");
				if(dateday && datemonth && dateyear) {
					char *duedate = g_strdup_printf("%04d%02d%02d", atoi(dateyear), atoi(datemonth), atoi(dateday));
					on_curr = xmlNewTextChild(on_root, NULL, (xmlChar*)"DateDue", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)duedate);
					g_free(duedate);
					// RFC2445 says the default value type is DATE-TIME. But Opie only
					// stores DATE as due date => alter VALUE to DATE
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
				}
				if(dateday)   xmlFree(dateday);
				if(datemonth) xmlFree(datemonth);
				if(dateyear)  xmlFree(dateyear);
			}
			xmlFree(hasDate);
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
	
	if (xmlStrcmp(root->name, (const xmlChar *)"Todo")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Wrong xml root element");
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
					priority = 1;
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
	
	/* Categories */
	icur = osxml_get_node(root, "Categories");
	if (icur)
		xml_categories_to_attr(icur, on_todo, "Categories");

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
					time_t starttime = (time_t)atoi(iprop->children->content);
					char *startvtime = osync_time_unix2vtime(&starttime); 
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateStarted", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)startvtime);
					if(allday)
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
					g_free(startvtime);
					/* Record the start date for use later */
					startdate = g_date_new();
#ifdef NEW_GLIB_VER
					g_date_set_time_t(startdate, starttime);
#else
					/* This is deprecated */
					GTime g_starttime = (GTime)starttime;					
					g_date_set_time(startdate, g_starttime);
#endif /* NEW_GLIB_VER */
				}
				else if(!strcasecmp(iprop->name, "end")) 
				{
					time_t endtime = (time_t)atoi(iprop->children->content);
					char *endvtime = osync_time_unix2vtime(&endtime); 
					on_curr = xmlNewTextChild( on_root, NULL, (xmlChar*)"DateEnd", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)endvtime);
					if(allday)
						xmlNewTextChild(on_curr, NULL, (xmlChar*)"Value", (xmlChar*)"DATE");
					g_free(endvtime);
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
			"alarm"
			"sound" (alarm sound - "silent" for none)
			timezone?
			*/
		}
		
		/* Recurrence */
		char *recurType = xmlGetProp(icur, "rtype");
		if(recurType) {
			xmlNode *on_recur = xmlNewTextChild(on_root, NULL, (xmlChar*) "RecurrenceRule", NULL);
			
			/* Frequency */
			if(!strcmp(recurType, "Daily")) {
				xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=DAILY");
			}
			else if(!strcmp(recurType, "Weekly")) {
				xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*) "FREQ=WEEKLY");
				
				/* Weekdays */
				char *weekdays = xmlGetProp(icur, "rweekdays");
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
					char *weeknostr = xmlGetProp(icur, "rposition");
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
			char *interval = xmlGetProp(icur, "rfreq");
			if(interval) {
				char *intervalstr = g_strdup_printf("INTERVAL=%s", interval);
				xmlNewTextChild(on_recur, NULL, (xmlChar*)"Rule", (xmlChar*)intervalstr);
				xmlFree(interval);
				g_free(intervalstr);
			}
			
			/* End date */
			char *hasEndDate = xmlGetProp(icur, "rhasenddate");
			if(hasEndDate) {
				char *recurendstr = xmlGetProp(icur, "enddt");
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
	
	if (xmlStrcmp(root->name, (const xmlChar *)"Event")) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, 
										"Wrong xml root element");
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
		xml_node_vtime_to_attr_time_t(icur, "Content", on_event, "created");
	}
	
	/* Start */
	icur = osxml_get_node(root, "DateStarted");
	if (icur) {
		start_time = xml_node_vtime_to_attr_time_t(icur, "Content", on_event, "start");
	}
	
	/* End */
	icur = osxml_get_node(root, "DateEnd");
	if (icur) {
		end_time = xml_node_vtime_to_attr_time_t(icur, "Content", on_event, "end");
	}
	
	/* Check for all-day event */
	if(start_time - end_time == 86399) {
		xmlSetProp(on_event, "type", "AllDay");
	}
	
	/* Recurrence */
	icur = osxml_get_node(root, "RecurrenceRule");
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
			gchar **rule = g_strsplit(rulestr, "=", 1);
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
				rectype = RECUR_TYPE_MONTHLY_DATE;
			}
			else if (!strcasecmp(rule[0], "INTERVAL")) {
				rfreq = g_strdup(rule[1]);
			}
			else if (!strcasecmp(rule[0], "UNTIL")) {
				time_t utime = osync_time_vtime2unix(rule[1]);
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
				xmlSetProp(on_event, "rtype", "Daily");
				break;
			case RECUR_TYPE_WEEKLY:
				xmlSetProp(on_event, "rtype", "Weekly");
				break;
			case RECUR_TYPE_MONTHLY_DAY:
				xmlSetProp(on_event, "rtype", "MonthlyDay");
				break;
			case RECUR_TYPE_MONTHLY_DATE:
				xmlSetProp(on_event, "rtype", "MonthlyDate");
				break;
			case RECUR_TYPE_YEARLY:
				xmlSetProp(on_event, "rtype", "Yearly");
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
				xmlSetProp(on_event, "rweekdays", rweekdays);
				g_free(rweekdays);
			}
			else {
				/* MonthlyDate recurrence */
				int weekno = 0;
				char *tmp_wday = g_strdup("XX");
				sscanf(weekdaysrule, "%d%2s", &weekno, tmp_wday);
				g_free(tmp_wday);
				char *rposition = g_strdup_printf("%d", weekno);
				xmlSetProp(on_event, "rposition", rposition);
				g_free(rposition);
			}
			g_free(weekdaysrule);
		}
		if(rfreq) {
			xmlSetProp(on_event, "rfreq", rfreq);
			g_free(rfreq);
		}
		if(enddt) {
			xmlSetProp(on_event, "rhasenddate", "1");
			xmlSetProp(on_event, "enddt", enddt);
			g_free(enddt);
		}
		else {
			xmlSetProp(on_event, "rhasenddate", "0");
		}
	}
	
	/* Categories */
	icur = osxml_get_node(root, "Categories");
	if (icur)
		xml_categories_to_attr(icur, on_event, "categories");

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



void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "opie-xml-contact");
	osync_env_format_set_destroy_func(env, "opie-xml-contact", destroy_opie_contact); /* FIXME do we need this for all types? */
	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "opie-xml-todo");
	osync_env_register_objtype(env, "event");
	osync_env_register_objformat(env, "event", "opie-xml-event");

	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-contact", "xml-contact",      conv_opie_xml_contact_to_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact",      "opie-xml-contact", conv_xml_contact_to_opie_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-todo",    "xml-todo",         conv_opie_xml_todo_to_xml_todo);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo",         "opie-xml-todo",    conv_xml_todo_to_opie_xml_todo);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-event",   "xml-event",        conv_opie_xml_event_to_xml_event);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-event",        "opie-xml-event",   conv_xml_event_to_opie_xml_event);
}

void xml_node_to_attr(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname) {
	char *value = osxml_find_node(node_from, nodename);
	xmlSetProp(node_to, attrname, value);
	xmlFree(value);
}

time_t xml_node_vtime_to_attr_time_t(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname) {
	char *vtime = osxml_find_node(node_from, nodename);
	time_t utime = osync_time_vtime2unix(vtime);
	char *timestr = g_strdup_printf("%d", (int)utime);
	xmlSetProp(node_to, attrname, timestr);
	g_free(timestr);
	xmlFree(vtime);
	return utime;
}

void xml_categories_to_attr(xmlNode *categories_node, xmlNode *node_to, const char *category_attr) {
	xmlNode *cur;
	GString *categories = g_string_new("");
	for (cur = categories_node->children; cur; cur = cur->next) {
		if(!strcmp(cur->name, "Category")) {
			char *cat_name = xmlNodeGetContent(cur);
			g_string_append_printf(categories, "%s|", cat_name);
			xmlFree(cat_name);
		}
	}
	if(categories->len > 0) {
		g_string_truncate(categories, categories->len - 1);
		xmlSetProp(node_to, category_attr, categories->str);
	}
	g_string_free(categories, TRUE);
}
