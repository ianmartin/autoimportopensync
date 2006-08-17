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
        GList* current_email;
	
	if (inpsize != sizeof(contact_data)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong size");
		goto error;
	}
	
	//Create a new xml document
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");

	//Names
	if (entry->last_name || entry->first_name) {
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Name", NULL);
		//Last Name
		if (entry->last_name)
			osxml_node_add(current, "LastName", entry->last_name);
	
		//First Name
		if (entry->first_name)
			osxml_node_add(current, "FirstName", entry->first_name);

		// Suffix
		if (entry->suffix)
			osxml_node_add(current, "Suffix", entry->suffix);

		// Middle name
		if (entry->middle_name)
			osxml_node_add(current, "Additional", entry->middle_name );
//TODO
// what about PREFIX?
	}
	
	//Company
 	if (entry->company || entry->department ) {
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Organization", NULL);
		if ( entry->company )
                    osxml_node_add(current, "Name", entry->company);
		if ( entry->department )
                    osxml_node_add(current, "Department", entry->department);
	}

	// Telephone numbers
	if ( entry->home_phone ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->home_phone );
        osxml_node_add(current, "Type", "HOME" );
        osxml_node_add(current, "Type", "VOICE" );
	}
    if ( entry->home_fax ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->home_fax );
        osxml_node_add(current, "Type", "HOME" );
        osxml_node_add(current, "Type", "FAX" );
	}
    if ( entry->home_mobile ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->home_mobile );
        osxml_node_add(current, "Type", "CELL" );
        osxml_node_add(current, "Type", "HOME" );
	}
    
    if ( entry->business_phone ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->business_phone );
        osxml_node_add(current, "Type", "WORK" );
        osxml_node_add(current, "Type", "VOICE" );
    }
    if ( entry->business_fax ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->business_fax );
        osxml_node_add(current, "Type", "WORK" );
        osxml_node_add(current, "Type", "FAX" );
	}
    if ( entry->business_mobile ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->business_mobile );
        osxml_node_add(current, "Type", "WORK" );
        osxml_node_add(current, "Type", "CELL" );
	}
    if ( entry->business_pager ) {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Telephone", NULL);
        osxml_node_add(current, "Content", entry->business_pager );
        osxml_node_add(current, "Type", "WORK" );
        osxml_node_add(current, "Type", "PAGER" );
	}

    // Email addresses
    if ( entry->emails ) {
        current_email = entry->emails;
        while ( current_email != NULL ) 
        {
            if (current_email->data) {
                current = xmlNewTextChild(root, NULL, (xmlChar*)"EMail", NULL);
                osxml_node_add(current, "Content", current_email->data );
                if ( entry->default_email &&
                     strcasecmp( entry->default_email, current_email->data ) == 0 )
                {
                    // this is the preferred email address
                    osxml_node_add(current, "Type", "PREF" );
                }
            }
            current_email=current_email->next;
        }
    }

    // Home Address
    if ( entry->home_street || entry->home_city || entry->home_state
         || entry->home_zip || entry->home_country )
    {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Address", NULL);
        if ( entry->home_street )
            osxml_node_add(current, "Street", entry->home_street );
        if ( entry->home_city )
            osxml_node_add(current, "City", entry->home_city );
        if ( entry->home_state )
            osxml_node_add(current, "Region", entry->home_state );
        if ( entry->home_zip )
            osxml_node_add(current, "PostalCode", entry->home_zip );
        if ( entry->home_country )
            osxml_node_add(current, "Country", entry->home_country );
        osxml_node_add(current, "Type", "HOME" );
    }            

    // Business Address
    if ( entry->business_street || entry->business_city || entry->business_state
         || entry->business_zip || entry->business_country )
    {
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Address", NULL);
        if ( entry->business_street )
            osxml_node_add(current, "Street", entry->business_street );
        if ( entry->business_city )
            osxml_node_add(current, "City", entry->business_city );
        if ( entry->business_state )
            osxml_node_add(current, "Region", entry->business_state );
        if ( entry->business_zip )
            osxml_node_add(current, "PostalCode", entry->business_zip );
        if ( entry->business_country )
            osxml_node_add(current, "Country", entry->business_country );
        osxml_node_add(current, "Type", "WORK" );
    }            

	//Title
	if (entry->jobtitle) {
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Title", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->jobtitle);
	}
	
	
	//Note
	if (entry->notes) {
		current = xmlNewTextChild(root, NULL, (xmlChar*)"Note", NULL);
		xmlNewTextChild(current, NULL, (xmlChar*)"Content", (xmlChar*)entry->notes);
	}

/*
	GList *c = NULL;
	current = NULL;
	for (c = entry->categories; c; c = c->next) {
		if (!current)
			current = xmlNewTextChild(root, NULL, (xmlChar*)"Categories", NULL);
		osxml_node_add(current, "Category", (char *)c->data);
	}
*/

    // Spouse
	if ( entry->spouse ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Spouse", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->spouse );
	}

    // Nickname
	if ( entry->nickname ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Nickname", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->nickname );
	}

    // Assistant
	if ( entry->assistant ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Assistant", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->assistant );
	}
    if ( entry->assistant )
        current = xmlNewTextChild(root, NULL, (xmlChar*)"Assistant", (xmlChar*)entry->assistant );
            
    // Manager
	if ( entry->manager ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Manager", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->manager );
	}
            
    // Profession
	if ( entry->profession ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Profession", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->profession );
	}

    // Birthday (do we need to create an xml datetime??)
	if ( entry->birthday ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Birthday", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->birthday );
	}

    // Anniversary
	if ( entry->anniversary ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"Anniversary", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->anniversary );
	}

    // File-as. This is what the Evo plugin does, so copy it.
	if ( entry->file_as ) {
		current = xmlNewTextChild( root, NULL, (xmlChar*)"FormattedName", NULL );
		xmlNewTextChild( current, NULL, (xmlChar*)"Content", (xmlChar*)entry->file_as );
	}

// TODO: Entries to be handled
/*   char* uid; */
/*   GList* cids; */
/*   unsigned int rid; */
/*   unsigned int rinfo; */
/*   char* home_webpage; */
/*   char* business_webpage; */
/*   int gender; */
/*   char* children; */
/*   char* office; */
/*   GList* anons; */

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
    // only list the ones we use
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
	
	//Get the root node of the input document
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

	/* Start the new entry */
	contact_data *entry = osync_try_malloc0(sizeof(contact_data), error);
	if (!entry)
		goto error;
	
	//Name
	xmlNode *cur = osxml_get_node(root, "Name");
	if (cur) {
		entry->last_name = osxml_find_node(cur, "LastName");
		entry->first_name = osxml_find_node(cur, "FirstName");
		entry->suffix = osxml_find_node(cur, "Suffix");
                entry->middle_name = osxml_find_node(cur, "Additional");
	} else {
            osync_trace(TRACE_INTERNAL, "No Name node found" );
	}
	osync_trace(TRACE_INTERNAL, "Name = %s %s %s", entry->first_name, 
				entry->middle_name, entry->last_name );

	//Company
	cur = osxml_get_node(root, "Organization");
	if (cur) {
        entry->company = osxml_find_node(cur, "Name");
        entry->department = osxml_find_node(cur, "Department");
    }

	//Telephone
	xobj = osxml_get_nodeset((xmlDoc *)root, "/Telephone");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ )
    {
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
        }
        xmlXPathFreeObject(xobj2);
		char *number = osxml_find_node(cur, "Content");

		osync_trace(TRACE_INTERNAL, "Telephone type %d %s",
					type, number );		

        // Telephone numbers
        if ( type & PT_PAGER ) {
            entry->business_pager = number;
        }
        else if ( type & PT_WORK ) {
            if ( type & PT_VOICE )
                entry->business_phone = number;
            else if ( type & PT_FAX ) 
                entry->business_fax = number;
            else if ( type & PT_CELL ) 
                entry->business_mobile = number;
            else {
                // ???
            }
        }
        else if ( type & PT_VOICE )
            entry->home_phone = number;
        else if ( type & PT_FAX ) 
            entry->home_fax = number;
        else if ( type & PT_CELL ) 
            entry->home_mobile = number;
        else {
            // ???
        }
    }
	xmlXPathFreeObject(xobj);
	
	// EMail
	xobj = osxml_get_nodeset((xmlDoc *)root, "/EMail");
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ )
    {
        cur = nodes->nodeTab[i];
        entry->emails = g_list_append(
            entry->emails, 
            g_strdup( osxml_find_node(cur, "Content") )
            );

        xmlXPathObject *xobj2 = osxml_get_nodeset((xmlDoc *)cur, "/Type");
        xmlNodeSet *nodes2 = xobj2->nodesetval;
        int numnodes2 = (nodes2) ? nodes2->nodeNr : 0;
        int j;
        for ( j = 0; j < numnodes2; j++ )
        {
            xmlNode *cur2 = nodes2->nodeTab[j];
            char *type = (char*)xmlNodeGetContent(cur2);
            if ( type != NULL && strcasecmp( type, "PREF" ) == 0 ) {
                entry->default_email = (char*)xmlNodeGetContent(cur);
                break;
            }
        }
        xmlXPathFreeObject(xobj2);
	}
	xmlXPathFreeObject(xobj);
	
	// Addresses
	xobj = osxml_get_nodeset((xmlDoc *)root, "/Address" );
	nodes = xobj->nodesetval;
	numnodes = (nodes) ? nodes->nodeNr : 0;
	for ( i = 0; i < numnodes; i++ )
        {
            cur = nodes->nodeTab[i];
            char *type = osxml_find_node(cur, "Type");
            if ( strcasecmp( type, "HOME" ) == 0 ) 
            {
                entry->home_street = (char*)osxml_find_node(cur, "Street");
                entry->home_city = (char*)osxml_find_node(cur, "City");
                entry->home_state = (char*)osxml_find_node(cur, "Region");
                entry->home_zip = (char*)osxml_find_node(cur, "PostalCode");
                entry->home_country = (char*)osxml_find_node(cur, "Country");
            }
            else if ( strcasecmp( type, "WORK" ) == 0 ) 
            {
                entry->business_street = (char*)osxml_find_node(cur, "Street");
                entry->business_city = (char*)osxml_find_node(cur, "City");
                entry->business_state = (char*)osxml_find_node(cur, "Region");
                entry->business_zip = (char*)osxml_find_node(cur, "PostalCode");
                entry->business_country = (char*)osxml_find_node(cur, "Country");
            } 
            else
            {
                // TODO put it in anon???
            }
	}

	//Title
	cur = osxml_get_node(root, "Title");
	if (cur) entry->jobtitle = osxml_find_node(cur, "Content");

	//Note
	cur = osxml_get_node(root, "Note");
	if (cur) entry->notes = osxml_find_node(cur, "Content");
	
/*
	//Categories
	cur = osxml_get_node(root, "Categories");
	if (cur) {
		for (cur = cur->children; cur; cur = cur->next) {
			entry->categories = g_list_append(entry->categories, (char*)xmlNodeGetContent(cur));
		}
	}
*/

	// Spouse
	cur = osxml_get_node(root, "Spouse");
	if (cur) entry->spouse = osxml_find_node(cur, "Content");

	// Nickname
	cur = osxml_get_node(root, "Nickname");
	if (cur) entry->nickname = osxml_find_node(cur, "Content");

	// Assistant
	cur = osxml_get_node(root, "Assistant");
	if (cur) entry->assistant = osxml_find_node(cur, "Content");

	// Manager
	cur = osxml_get_node(root, "Manager");
	if (cur) entry->manager = osxml_find_node(cur, "Content");

	// Profession
	cur = osxml_get_node(root, "Profession");
	if (cur) entry->profession = osxml_find_node(cur, "Content");

	// Birthday
	cur = osxml_get_node(root, "Birthday");
	if (cur) entry->birthday = osxml_find_node(cur, "Content");

	// Anniversary
	cur = osxml_get_node(root, "Anniversary");
	if (cur) entry->anniversary = osxml_find_node(cur, "Content");

	// File-as
	cur = osxml_get_node(root, "FormattedName");
	if (cur) entry->file_as = osxml_find_node(cur, "Content");

// TODO: Entries to be handled
/*   char* uid; */
/*   GList* cids; */
/*   unsigned int rid; */
/*   unsigned int rinfo; */
/*   char* home_webpage; */
/*   char* business_webpage; */
/*   int gender; */
/*   char* children; */
/*   char* office; */
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
