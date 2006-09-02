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

#include <opensync/opensync-xml.h>


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
					/* FIXME handle categories */
					/*
					gchar** categorytokens = g_strsplit(iprop->children->content,";",20);
					
					for(j=0;categorytokens[j]!=NULL;j++) 
					{
						contact->cids = g_list_append(contact->cids, 
																					g_strdup(categorytokens[j]));
					}
					g_strfreev(categorytokens);
					*/
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
					/* FIXME handle this field */
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
					/* FIXME handle this field */
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
	
/*
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
*/

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

	/* File-as */
	cur = osxml_get_node(root, "FormattedName");
	if (cur)
		xml_node_to_attr(cur, "Content", on_contact, "FileAs");

// TODO: Entries to be handled
/*   char* uid; */
/*   GList* cids; */
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
					/* Priority is 1-5 on Opie, 0-9 in OpenSync XML 
						This conversion matches the behaviour of the Palm plugin.
					*/
					char *tmp = g_strdup_printf("%i", atoi(iprop->children->content) + 2);
					on_curr = xmlNewTextChild(on_root, NULL, (xmlChar*)"Priority", NULL);
					xmlNewTextChild(on_curr, NULL, (xmlChar*)"Content", (xmlChar*)tmp);
					g_free(tmp);
				}
			}
			/* FIXME Stuff to handle:
				Progress (percentage)
				Categories
				Uid
				StartDate
				State (0=Started, 1=Postponed, 2=Finished, 3=Not started)
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
					char *duedate = g_strdup_printf("%s%s%s", dateyear, datemonth, dateday); 
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
		/* Priority is 1-5 on Opie, 0-9 in OpenSync XML 
			This conversion matches the behaviour of the Palm plugin.
			FIXME what if the priority is > 7 ?
		*/
		char *prio = (char *)xmlNodeGetContent(icur);
		if (prio) {
			int priority = atoi(prio) - 2;
			xmlFree(prio);
			if (priority < 1) {
				//Never go lower than 1
				priority = 1;
			}
			if (atoi(prio) == 0) {
				//Default to priority 5
				priority = 5;
			}
			prio = g_strdup_printf("%d", priority);
			xmlSetProp(on_todo, "Priority", prio);
			g_free(prio);
		}
	}
	
	/* Completed */
	icur = osxml_get_node(root, "Completed");
	if (icur) {
		char *completedstr = (char *) xmlNodeGetContent(icur);
		struct tm *completed = osync_time_vtime2tm(completedstr);
		xmlFree(completedstr);
		completedstr = g_strdup_printf("%04d%02d%02d", completed->tm_year, (completed->tm_mon + 1), completed->tm_mday);
		xmlSetProp(on_todo, "Completed", "1");
		xmlSetProp(on_todo, "CompletedDate", completedstr);
		g_free(completedstr);
	}
	else {
		xmlSetProp(on_todo, "Completed", "0");
	}
	
	/* Due date */
	icur = osxml_get_node(root, "DateDue");
	if (icur) {
		char *duestr = (char *) xmlNodeGetContent(icur);
		struct tm *due = osync_time_vtime2tm(duestr);
		xmlFree(duestr);
		char *dueyear  = g_strdup_printf("%04d", due->tm_year);
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
	else {
		xmlSetProp(on_todo, "HasDate", "0");
	}
		
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


void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "contact");
	osync_env_register_objformat(env, "contact", "opie-xml-contact");
	osync_env_format_set_destroy_func(env, "opie-xml-contact", destroy_opie_contact);
	osync_env_register_objtype(env, "todo");
	osync_env_register_objformat(env, "todo", "opie-xml-todo");

	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-contact", "xml-contact",      conv_opie_xml_contact_to_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-contact",      "opie-xml-contact", conv_xml_contact_to_opie_xml_contact);
	osync_env_register_converter(env, CONVERTER_CONV, "opie-xml-todo",    "xml-todo",         conv_opie_xml_todo_to_xml_todo);
	osync_env_register_converter(env, CONVERTER_CONV, "xml-todo",         "opie-xml-todo",    conv_xml_todo_to_opie_xml_todo);
}

void xml_node_to_attr(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname) {
	char *value = osxml_find_node(node_from, nodename);
	xmlSetProp(node_to, attrname, value);
	xmlFree(value);
}
