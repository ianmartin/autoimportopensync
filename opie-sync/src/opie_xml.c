/* 
   MultiSync Opie Plugin - Synchronize Opie/Zaurus Devices
   Copyright (C) 2003 Tom Foottit <tom@foottit.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
   SOFTWARE IS DISCLAIMED.
*/

/*
 *  $Id: opie_xml.c,v 1.11 2004/02/20 15:55:14 irix Exp $
 */

#include "opie_xml.h"
#include "opie_sync.h"
#include "opie_comms.h"

#include <string.h>
//#include <expat.h>

#define PARSE_BUF_SIZE (512)

#if 0
/* start and end expat callbacks for contact */
void contact_start_hndl(void *data, const char *el, const char **attr); 
void contact_end_hndl(void *data, const char *el); 
void contact_char_hndl(void *data, const char *txt, int txtlen);

/* start and end expat callbacks for calendar */
void cal_start_hndl(void *data, const char *el, const char **attr);
void cal_end_hndl(void *data, const char *el); 
void cal_char_hndl(void *data, const char *txt, int txtlen);

/* start and end expat callbacks for todo */
void todo_start_hndl(void *data, const char *el, const char **attr); 
void todo_end_hndl(void *data, const char *el); 
void todo_char_hndl(void *data, const char *txt, int txtlen);

/* start and end expat callbacks for categories */
void category_start_hndl(void *data, const char *el, const char **attr); 
void category_end_hndl(void *data, const char *el); 
void category_char_hndl(void *data, const char *txt, int txtlen);
#endif


/* escape XML in text */
gchar* opie_xml_markup_escape_text(const gchar *text,
                                   gssize       length);  

/* utility function for opie_xml_markup_escape_text */
static void opie_xml_append_escaped_text (GString     *str,
                                          const gchar *text,
                                          gssize       length);    

/* globals */
gboolean in_rid = FALSE;
unsigned int cal_rid_max = 1;
unsigned int todo_rid_max = 1;
unsigned int contact_rid_max = 1;
unsigned int cal_rinfo = 1;
unsigned int todo_rinfo = 1;
unsigned int contact_rinfo = 1;


#if 0

/*
 * parse_cal_data
 */
void parse_cal_data(char* cal_file, 
                    GList** calendar)
{
  XML_Parser p = XML_ParserCreate(NULL);
  int done, len;
  char buf[PARSE_BUF_SIZE];
  FILE* fd;
  
  if(!p)
    return;
  
  /* open the XML file */
  fd = fopen(cal_file, "r");
  if(!fd)
    return;
  
  /* pass our GList into each handler */
  XML_SetUserData(p, (void *) calendar);
  XML_SetElementHandler(p, cal_start_hndl, cal_end_hndl);
  XML_SetCharacterDataHandler(p, cal_char_hndl);
  
  /* parse the XML */    
  while(1)
  {
    fgets(buf, sizeof(buf), fd);
    len = strlen(buf);
    
    if(ferror(fd))
      break;
    
    done = feof(fd);
    if(!XML_Parse(p, buf, len, done))
    {
      /* parse error */
      break;
    }
    
    if(done)
      break;  
  }
  
  fclose(fd);
}

#endif


/*
 * parse_contact_data
 */
void parse_contact_data(char* contact_file, 
                        GList** contacts)
{
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, contact_file, contacts);
	xmlDoc *doc = NULL;
	xmlNode *cur = NULL;
  int j;
  contact_data* contact = NULL;
  anon_data* anon;
  gchar** emailtokens;
	struct _xmlAttr *prop;

	doc = xmlParseFile(contact_file);

	if (!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to parse contacts XML file");
		goto error;
	}

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		osync_trace(TRACE_INTERNAL, "Unable to get addressbook root element");
		goto error_free_doc;
	}

	printf("XML-root: %s\n", cur->name);
	

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if(!strcasecmp(cur->name, "Contacts"))
			break;
		cur = cur->next;
	}

	if (!cur) {
		osync_trace(TRACE_INTERNAL, "Unable to get contacts element");
		goto error_free_doc;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if(!strcasecmp(cur->name, "Contact"))
		{
			/* this is a contact element - the attributes are the data we care about */
			contact = g_malloc0(sizeof(contact_data));
			
			for (prop = cur->properties; prop; prop=prop->next) 
			{
				if (prop->children && prop->children->content) 
				{
					/* key on the attribute name */
					if(!strcasecmp(prop->name, "FirstName"))
					{
						contact->first_name = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "MiddleName"))
					{
						contact->middle_name = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "LastName"))
					{
						contact->last_name = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name,"Suffix"))
					{
						contact->suffix = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "FileAs"))
					{
						contact->file_as = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Department"))
					{
						contact->department = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Company"))
					{
						contact->company = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Emails"))
					{
						emailtokens = g_strsplit(prop->children->content," ",3);
						
						for(j=0;emailtokens[j]!=NULL;j++) 
						{
							contact->emails = g_list_append(contact->emails, 
																							g_strdup(emailtokens[j]));
						}
						g_strfreev(emailtokens);
					}
					else if(!strcasecmp(prop->name, "Categories"))
					{
						gchar** categorytokens = g_strsplit(prop->children->content,";",20);
						
						for(j=0;categorytokens[j]!=NULL;j++) 
						{
							contact->cids = g_list_append(contact->cids, 
																						g_strdup(categorytokens[j]));
						}
						g_strfreev(categorytokens);
					}
					else if(!strcasecmp(prop->name, "DefaultEmail"))
					{
						contact->default_email = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomePhone"))
					{
						contact->home_phone = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeFax"))
					{
						contact->home_fax = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeMobile"))
					{
						contact->home_mobile = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeStreet"))
					{
						contact->home_street = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeCity"))
					{
						contact->home_city = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeState"))
					{
						contact->home_state = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeZip"))
					{
						contact->home_zip = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeCountry"))
					{
						contact->home_country = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "HomeWebPage"))
					{
						contact->home_webpage = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessPhone"))
					{
						contact->business_phone = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessFax"))
					{
						contact->business_fax = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessMobile"))
					{
						contact->business_mobile = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessPager"))
					{
						contact->business_pager = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessStreet"))
					{
						contact->business_street = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessCity"))
					{
						contact->business_city = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessState"))
					{
						contact->business_state = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessZip"))
					{
						contact->business_zip = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessCountry"))
					{
						contact->business_country = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "BusinessWebPage"))
					{
						contact->business_webpage = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Spouse"))
					{
						contact->spouse = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Birthday"))
					{
						contact->birthday = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Anniversary"))
					{
						contact->anniversary = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Nickname"))
					{
						contact->nickname = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Children"))
					{
						contact->children = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Notes"))
					{
						contact->notes = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Uid"))
					{
						contact->uid = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "rid"))
					{
						contact->rid = atoi(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "rinfo"))
					{
						contact->rinfo = atoi(prop->children->content);
						contact_rinfo = contact->rinfo;
					}
					else if(!strcasecmp(prop->name, "Gender"))
					{
						contact->gender = atoi(prop->children->content);   // FIXME: used to be char*+1 (?)
					}
					else if(!strcasecmp(prop->name, "Assistant"))
					{
						contact->assistant = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Manager"))
					{
						contact->manager = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Office"))
					{
						contact->office = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "Profession"))
					{
						contact->profession = g_strdup(prop->children->content);
					}
					else if(!strcasecmp(prop->name, "JobTitle"))
					{
						contact->jobtitle = g_strdup(prop->children->content);
					}
					else
					{
						/* unknown attribute - store in the anon list */
						anon = g_malloc0(sizeof(anon_data)); 
						anon->attr = g_strdup(prop->name);
						anon->val = g_strdup(prop->children->content);
						contact->anons = g_list_append(contact->anons, anon);
					}
				}
			}

			*contacts = g_list_append(*contacts, contact);  
		}

		cur = cur->next;
	}

	xmlFreeDoc(doc);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_free_doc:
	xmlFreeDoc(doc);
error:
	return;
}    

#if 0

/*
 * parse_todo_data
 */
void parse_todo_data(char* todo_file, 
                     GList** todos)
{
  XML_Parser p = XML_ParserCreate(NULL);
  int done, len;
  char buf[PARSE_BUF_SIZE];
  FILE* fd;
  
  if(!p)
    return;
  
  /* open the XML file */
  fd = fopen(todo_file, "r");
  if(!fd)
    return;
  
  /* pass our GList into each handler */
  XML_SetUserData(p, (void *) todos);
  XML_SetElementHandler(p, todo_start_hndl, todo_end_hndl);
  XML_SetCharacterDataHandler(p, todo_char_hndl);
  
  /* parse the XML */    
  while(1)
  {
    fgets(buf, sizeof(buf), fd);
    len = strlen(buf);
    
    if(ferror(fd))
      break;
    
    done = feof(fd);
    if(!XML_Parse(p, buf, len, done))
    {
      /* parse error */
      break;
    }
    
    if(done)
      break;  
  }
  
  fclose(fd);
  
}    


/*
 * parse_category_data
 */
void parse_category_data(char* cat_file,
                         GList** categories)
{
  XML_Parser p = XML_ParserCreate(NULL);
  int done, len;
  char buf[PARSE_BUF_SIZE];
  FILE* fd;
  
  if(!p)
    return;
  
  /* open the XML file */
  fd = fopen(cat_file, "r");
  if(!fd)
    return;
  
  /* pass our GList into each handler */
  XML_SetUserData(p, (void *) categories);
  XML_SetElementHandler(p, category_start_hndl, category_end_hndl);
  XML_SetCharacterDataHandler(p, category_char_hndl);
  
  /* parse the XML */    
  while(1)
  {
    fgets(buf, sizeof(buf), fd);
    len = strlen(buf);
    
    if(ferror(fd))
      break;
    
    done = feof(fd);
    if(!XML_Parse(p, buf, len, done))
    {
      /* parse error */
      break;
    }
    
    if(done)
      break;  
  }
  
  fclose(fd);
}


/*
 * contact_start_hndl
 */
void contact_start_hndl(void *data, const char *el, const char **attr) 
{
  /* data is a GList */
  GList** contact_list = (GList**)data;
  int i,j;
  contact_data* contact;
  anon_data* anon;
  gchar** emailtokens;
  if(!strcasecmp(el, "Contact"))
  {
    /* this is a contact element - the attributes are the data we care about */
    contact = g_malloc0(sizeof(contact_data));
    
    for (i = 0; attr[i]; i += 2)
    {
      /* key on the attribute name */
      if(!strcasecmp(attr[i], "FirstName"))
      {
        contact->first_name = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "MiddleName"))
      {
	      contact->middle_name = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "LastName"))
      {
        contact->last_name = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i],"Suffix"))
      {
	      contact->suffix = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "FileAs"))
      {
        contact->file_as = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Department"))
      {
        contact->department = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Company"))
      {
        contact->company = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Emails"))
      {
        emailtokens = g_strsplit(attr[i+1]," ",3);
        
	      for(j=0;emailtokens[j]!=NULL;j++) 
        {
          contact->emails = g_list_append(contact->emails, 
                                          g_strdup(emailtokens[j]));
	      }
	      g_strfreev(emailtokens);
      }
      else if(!strcasecmp(attr[i], "Categories"))
      {
        gchar** categorytokens = g_strsplit(attr[i+1],";",20);
        
	      for(j=0;categorytokens[j]!=NULL;j++) 
        {
          contact->cids = g_list_append(contact->cids, 
                                        g_strdup(categorytokens[j]));
	      }
	      g_strfreev(categorytokens);
      }
      else if(!strcasecmp(attr[i], "DefaultEmail"))
      {
        contact->default_email = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomePhone"))
      {
        contact->home_phone = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeFax"))
      {
        contact->home_fax = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeMobile"))
      {
        contact->home_mobile = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeStreet"))
      {
        contact->home_street = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeCity"))
      {
        contact->home_city = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeState"))
      {
        contact->home_state = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeZip"))
      {
        contact->home_zip = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeCountry"))
      {
        contact->home_country = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HomeWebPage"))
      {
        contact->home_webpage = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessPhone"))
      {
        contact->business_phone = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessFax"))
      {
        contact->business_fax = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessMobile"))
      {
        contact->business_mobile = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessPager"))
      {
        contact->business_pager = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessStreet"))
      {
        contact->business_street = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessCity"))
      {
        contact->business_city = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessState"))
      {
        contact->business_state = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessZip"))
      {
        contact->business_zip = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessCountry"))
      {
        contact->business_country = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "BusinessWebPage"))
      {
        contact->business_webpage = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Spouse"))
      {
        contact->spouse = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Birthday"))
      {
        contact->birthday = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Anniversary"))
      {
        contact->anniversary = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Nickname"))
      {
        contact->nickname = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Children"))
      {
        contact->children = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Notes"))
      {
        contact->notes = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Uid"))
      {
        contact->uid = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "rid"))
      {
        contact->rid = atoi(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "rinfo"))
      {
        contact->rinfo = atoi(attr[i+1]);
        contact_rinfo = contact->rinfo;
      }
      else if(!strcasecmp(attr[i], "Gender"))
      {
        contact->gender = atoi(attr[i+1] + 1);
      }
      else if(!strcasecmp(attr[i], "Assistant"))
      {
        contact->assistant = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Manager"))
      {
        contact->manager = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Office"))
      {
        contact->office = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Profession"))
      {
        contact->profession = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "JobTitle"))
      {
        contact->jobtitle = g_strdup(attr[i+1]);
      }
      else
      {
        /* unknown attribute - store in the anon list */
        anon = g_malloc0(sizeof(anon_data)); 
        anon->attr = g_strdup(attr[i]);
        anon->val = g_strdup(attr[i+1]);
        contact->anons = g_list_append(contact->anons, anon);
      }
      
    } /* for each attribute  */
    
    *contact_list = g_list_append(*contact_list, contact);  
  } 
  else if(!strcasecmp(el, "Rid"))
  {
    in_rid = TRUE;
  }
}


/*
 * contact_end_hndl
 */
void contact_end_hndl(void *data, const char *el) 
{
  if(!strcasecmp(el, "Rid"))
  {
    in_rid = FALSE;
  }
}


/*
 * contact_char_hndl
 */
void contact_char_hndl(void *data, const char *txt, int txtlen)
{
  if(in_rid)
  {
    char* rid = g_malloc0(txtlen + 1);
    memcpy(rid, txt, txtlen);
    rid[txtlen] = 0;
  
    /* extract the max rid */
    contact_rid_max = strtoul(rid, NULL, 10);
    
    g_free(rid);
  }
}


/*
 * cal_start_hndl
 */
void cal_start_hndl(void *data, const char *el, const char **attr)
{
  /* data is a GList */
  GList** cal_list = (GList**)data;
  int i, j;
  cal_data* cal;
  anon_data* anon;
  
  if(!strcasecmp(el, "event"))
  {
    /* this is a calendar element - the attributes are the data we care about */
    cal = g_malloc0(sizeof(cal_data));
    
    for (i = 0; attr[i]; i += 2)
    {
      /* key on the attribute name */
      if(!strcasecmp(attr[i], "Uid"))
      {
        cal->uid = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Categories"))
      {
        gchar** categorytokens = g_strsplit(attr[i+1],";",20);
        
	      for(j=0;categorytokens[j]!=NULL;j++) 
        {
          cal->cids = g_list_append(cal->cids, 
                                    g_strdup(categorytokens[j]));
	      }
	      g_strfreev(categorytokens);
      }
      else if(!strcasecmp(attr[i], "description"))
      {
        /* the opie "description" is the vCal "summary" entry */
        cal->summary = g_strdup(attr[i+1]);
      }
      else if((!strcasecmp(attr[i], "note")) || (!strcasecmp(attr[i], "summary")))
      {
        /* the opie "note" is the vCal "description" entry */
        cal->desc = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "start"))
      {
        cal->start_date = (time_t)strtoul(attr[i+1], NULL, 10);
      }
      else if(!strcasecmp(attr[i], "end"))
      {
        cal->end_date = (time_t)strtoul(attr[i+1], NULL, 10) + 1;
      }
      else if(!strcasecmp(attr[i], "created"))
      {
        cal->created_date = (time_t)strtoul(attr[i+1], NULL, 10);
      }
      else if(!strcasecmp(attr[i], "location"))
      {
        cal->location = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "type"))
      {
        if(!strcasecmp(attr[i+1], "AllDay"))
        {
          cal->all_day = TRUE;
        }
      }
      else if(!strcasecmp(attr[i], "rid"))
      {
        cal->rid = atoi(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "rinfo"))
      {
        cal->rinfo = atoi(attr[i+1]);
        cal_rinfo = cal->rinfo;
      }
      else if(!strcasecmp(attr[i], "alarm"))
      {
        cal->alarm = g_malloc0(sizeof(alarm_data));
        cal->alarm->time_type = ALARM_MIN;
        cal->alarm->action_type = ALARM_ACTION_LOUD;
        cal->alarm->related = g_strdup("START");
        
        cal->alarm->duration = strtoul(attr[i+1], NULL, 10);
        
        if(cal->summary)
        {
          cal->alarm->desc = g_strdup(cal->summary);
        }
        else if(cal->desc)
        {
          cal->alarm->desc = g_strdup(cal->desc);          
        }
      }
      else if(!strcasecmp(attr[i], "sound"))
      {
        /* alarm sound */
        if(cal->alarm)
        {
          if(!strcasecmp(attr[i+1], "loud"))
          {
            cal->alarm->action_type = ALARM_ACTION_LOUD;  
          }
          else
          {
            cal->alarm->action_type = ALARM_ACTION_SILENT;
          }
        }
      }
      
      /* recurrence data */
      else if(!strcasecmp(attr[i], "rtype"))
      {
        if(!cal->recurrence)
          cal->recurrence = g_malloc0(sizeof(recurrence_data));
          
        if(!strcasecmp(attr[i+1], "Daily"))
          cal->recurrence->type = RECURRENCE_DAILY;
          
        if(!strcasecmp(attr[i+1], "Weekly"))
          cal->recurrence->type = RECURRENCE_WEEKLY;
          
        if(!strcasecmp(attr[i+1], "MonthlyDay"))
          cal->recurrence->type = RECURRENCE_MONTHLY;
          
        if(!strcasecmp(attr[i+1], "Yearly"))
          cal->recurrence->type = RECURRENCE_YEARLY;
          
      }      
      else if(!strcasecmp(attr[i], "rfreq"))
      {
        if(!cal->recurrence)
          cal->recurrence = g_malloc0(sizeof(recurrence_data));
          
        cal->recurrence->frequency = strtoul(attr[i+1], NULL, 10);  
      }
      else if(!strcasecmp(attr[i], "rposition"))
      {
        if(!cal->recurrence)
          cal->recurrence = g_malloc0(sizeof(recurrence_data));
          
        cal->recurrence->position = strtoul(attr[i+1], NULL, 10);  
      }
      else if(!strcasecmp(attr[i], "enddt"))
      {
        if(!cal->recurrence)
          cal->recurrence = g_malloc0(sizeof(recurrence_data));
          
        cal->recurrence->end_date = (time_t)strtoul(attr[i+1], NULL, 10);  
      }
      else if(!strcasecmp(attr[i], "rweekdays"))
      {
        if(!cal->recurrence)
          cal->recurrence = g_malloc0(sizeof(recurrence_data));
          
        cal->recurrence->weekdays = atoi(attr[i+1]);  
      }
      else
      {
        /* unknown attribute - store in the anon list */
        anon = g_malloc0(sizeof(anon_data)); 
        anon->attr = g_strdup(attr[i]);
        anon->val = g_strdup(attr[i+1]);
        cal->anons = g_list_append(cal->anons, anon);
      }
            
    } /* for each attribute  */
    
    *cal_list = g_list_append(*cal_list, cal);  
  } 
  else if(!strcasecmp(el, "Rid"))
  {
    in_rid = TRUE;
  }  
  
}


/*
 * cal_end_hndl
 */
void cal_end_hndl(void *data, const char *el)
{
  if(!strcasecmp(el, "Rid"))
  {
    in_rid = FALSE;
  }
}


/*
 * cal_char_hndl
 */
void cal_char_hndl(void *data, const char *txt, int txtlen)
{
  if(in_rid)
  {
    char* rid = g_malloc0(txtlen + 1);
    memcpy(rid, txt, txtlen);
    rid[txtlen] = 0;
  
    /* extract the max rid */
    cal_rid_max = strtoul(rid, NULL, 10);
    
    g_free(rid);
  }
}



/*
 * todo_start_hndl
 */
void todo_start_hndl(void *data, const char *el, const char **attr)
{
  /* data is a GList */
  GList** todo_list = (GList**)data;
  int i, j;
  todo_data* todo;
  anon_data* anon;
  
  if(!strcasecmp(el, "Task"))
  {
    /* this is a contact element - the attributes are the data we care about */
    todo = g_malloc0(sizeof(todo_data));
    
    for (i = 0; attr[i]; i += 2)
    {
      /* key on the attribute name */
      if(!strcasecmp(attr[i], "Uid"))
      {
        todo->uid = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Categories"))
      {
        gchar** categorytokens = g_strsplit(attr[i+1],";",20);
        
	      for(j=0;categorytokens[j]!=NULL;j++) 
        {
          todo->cids = g_list_append(todo->cids, 
                                     g_strdup(categorytokens[j]));
	      }
	      g_strfreev(categorytokens);
      }
      else if(!strcasecmp(attr[i], "Completed"))
      {
        todo->completed = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "HasDate"))
      {
        todo->hasdate = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "DateYear"))
      {
        todo->dateyear = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "DateMonth"))
      {
        todo->datemonth = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "DateDay"))
      {
        todo->dateday = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Priority"))
      {
        todo->priority = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Progress"))
      {
        todo->progress = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Description"))
      {
        todo->desc = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "Summary"))
      {
        todo->summary = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "rid"))
      {
        todo->rid = atoi(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "rinfo"))
      {
        todo->rinfo = atoi(attr[i+1]);
        todo_rinfo = todo->rinfo;
      }
      else
      {
        /* unknown attribute - store in the anon list */
        anon = g_malloc0(sizeof(anon_data)); 
        anon->attr = g_strdup(attr[i]);
        anon->val = g_strdup(attr[i+1]);
        todo->anons = g_list_append(todo->anons, anon);
      }
      
    } /* for each attribute  */
    
    *todo_list = g_list_append(*todo_list, todo);  
  } 
  else if(!strcasecmp(el, "Rid"))
  {
    in_rid = TRUE;
  }  
  
}


/*
 * todo_end_hndl
 */
void todo_end_hndl(void *data, const char *el)
{
  if(!strcasecmp(el, "Rid"))
  {
    in_rid = FALSE;
  }
}


/*
 * todo_char_hndl
 */
void todo_char_hndl(void *data, const char *txt, int txtlen)
{
  if(in_rid)
  {
    char* rid = g_malloc0(txtlen + 1);
    memcpy(rid, txt, txtlen);
    rid[txtlen] = 0;
  
    /* extract the max rid */
    todo_rid_max = strtoul(rid, NULL, 10);
    
    g_free(rid);
  }
}

/*
 * category_start_hndl
 */
void category_start_hndl(void *data, const char *el, const char **attr)
{
  /* data is a GList */
  GList** category_list = (GList**)data;
  int i;
  category_data* cat;
  
  if(!strcasecmp(el, "Category"))
  {
    /* this is a category element - the attributes are the data we care about */
    cat = g_malloc0(sizeof(category_data));
    
    for (i = 0; attr[i]; i += 2)
    {
      /* key on the attribute name */
      if(!strcasecmp(attr[i], "id"))
      {
        cat->cid = g_strdup(attr[i+1]);
      }
      else if(!strcasecmp(attr[i], "name"))
      {
        cat->category_name = g_strdup(attr[i+1]);
      }
      
    } /* for each attribute  */
    
    *category_list = g_list_append(*category_list, cat);  
  }   
}


/*
 * category_end_hndl
 */
void category_end_hndl(void *data, const char *el)
{
}


/*
 * category_char_hndl
 */
void category_char_hndl(void *data, const char *txt, int txtlen)
{
}


/*
 * cal_data_to_xml
 */
char* cal_data_to_xml(OpieSyncEnv* env, GList* calendar)
{
  GString *xmlstr, *bodystr;
  GList* li;
  GList* current_cid;
  GList* current_anon;
  cal_data* cal;
  char* retval;
  struct tm *day;
  int end_date_added = 0;
  
  xmlstr = g_string_new("<?xml version=\"1.0\"?>\n");
  g_string_append(xmlstr, "<!DOCTYPE DATEBOOK><DATEBOOK>\n");
    
 
  bodystr = g_string_new("<events>\n");
  
  for(li = calendar; li != NULL; li = g_list_next(li))
  {
    cal = (cal_data*)li->data;      
    
    g_string_append(bodystr, "<event");
    
    /* set the contact attributes as appropriate */
    if(cal->uid)
      g_string_sprintfa(bodystr, " uid=\"%s\"", cal->uid);
      
    if(env->device_type == OPIE_SYNC_QTOPIA_2)
    {
      /* need the rid */
      if(0 == cal->rid)
      {
        cal_rid_max++;
        cal->rid = cal_rid_max;
      }
      g_string_sprintfa(bodystr, " rid=\"%u\"", cal->rid);
      
      /* and the rinfo */
      if(0 == cal->rinfo)
      {
        cal->rinfo = cal_rinfo;
      }
      g_string_sprintfa(bodystr, " rinfo=\"%u\"", cal->rinfo);
    }

    if(cal->cids) 
    {
      g_string_sprintfa(bodystr, " Categories=\"");
      current_cid = cal->cids;
      while(current_cid != NULL) 
      {
        if(current_cid->data)
        {
          /* first cid should not have a ; before it */
          if(current_cid != cal->cids)
            g_string_sprintfa(bodystr,";");

      	  g_string_sprintfa(bodystr,"%s", (char *)current_cid->data);
        }
	      current_cid = current_cid->next;
      }
      g_string_sprintfa(bodystr,"\"");
    }
    
    if(cal->summary)
    {
      g_string_sprintfa(bodystr, 
                        " description=\"%s\"", 
                        opie_xml_markup_escape_text(cal->summary, 
                                                    strlen(cal->summary)));
      
      /* set the summary as the desc if not present */
      if(!cal->desc)
      {
        g_string_sprintfa(bodystr, " note=\"%s\"", opie_xml_markup_escape_text(cal->summary, 
                                                                               strlen(cal->summary)));
      }
    }
      
    if(cal->desc)
      g_string_sprintfa(bodystr, " note=\"%s\"", opie_xml_markup_escape_text(cal->desc, 
                                                                             strlen(cal->desc)));
    
    if(cal->location)
      g_string_sprintfa(bodystr, " location=\"%s\"", opie_xml_markup_escape_text(cal->location, 
                                                                          strlen(cal->location)));
      
    if(cal->start_date)
      g_string_sprintfa(bodystr, " start=\"%u\"", (unsigned int)cal->start_date);
      
    if(cal->end_date) {
      g_string_sprintfa(bodystr, " end=\"%u\"", (unsigned int)cal->end_date - 1);
      end_date_added = 1;
    }
      
    if(cal->created_date)
    {
      g_string_sprintfa(bodystr, " created=\"%u\"", (unsigned int)cal->created_date);
    }
    else
    {
      /* opie needs this for recurrence, so set it to the start date if not already set */
      /* TODO - this is supposed to be unique (!) in opie, so make it so */
      if(cal->start_date)
        g_string_sprintfa(bodystr, " created=\"%u\"", (unsigned int)cal->start_date);
    }
    
    if(cal->all_day) {
      /* If no end was added, add one as Old Opie 1.0 datebook seems to need it */
      if(!end_date_added)
        g_string_sprintfa(bodystr, " end=\"%u\"", (unsigned int)cal->start_date);
	
      g_string_sprintfa(bodystr, " type=\"AllDay\"");
      
    }
      
    if(cal->alarm)
    {
      /* opie cal durations always in minutes */
      unsigned int duration = (cal->alarm->duration) * (cal->alarm->time_type);
      g_string_sprintfa(bodystr, " alarm=\"%u\"", duration);
      
      if(cal->alarm->action_type == ALARM_ACTION_LOUD)
      {
        g_string_sprintfa(bodystr, " sound=\"loud\"");  
      }
      else
      {
        g_string_sprintfa(bodystr, " sound=\"silent\"");  
      }
    } /* alarm */
    
    if(cal->recurrence)
    {
      /* type */
      if(cal->recurrence->type == RECURRENCE_DAILY)
      {
        g_string_sprintfa(bodystr, " rtype=\"Daily\"");
      }
      else if(cal->recurrence->type == RECURRENCE_WEEKLY)
      {
        g_string_sprintfa(bodystr, " rtype=\"Weekly\"");
      }
      else if(cal->recurrence->type == RECURRENCE_MONTHLY)
      {
        g_string_sprintfa(bodystr, " rtype=\"MonthlyDay\"");
      }
      else if(cal->recurrence->type == RECURRENCE_YEARLY)
      {
        g_string_sprintfa(bodystr, " rtype=\"Yearly\"");
      }
      
      /* frequency */
      if(0 != cal->recurrence->frequency)
        g_string_sprintfa(bodystr, " rfreq=\"%u\"", (unsigned int)cal->recurrence->frequency);
      
      /* position */
      if(0 != cal->recurrence->position)
        g_string_sprintfa(bodystr, " rposition=\"%u\"", (unsigned int)cal->recurrence->position);
      
      /* end date */
      if(0 != cal->recurrence->end_date)
      {
        g_string_sprintfa(bodystr, " rhasenddate=\"1\"");
        g_string_sprintfa(bodystr, " enddt=\"%u\"", (unsigned int)cal->recurrence->end_date);         
      }
      else
      {
        g_string_sprintfa(bodystr, " rhasenddate=\"0\"");   
      }
      
      /* weekdays */
      if(0 != cal->recurrence->weekdays) 
      {
        g_string_sprintfa(bodystr, " rweekdays=\"%d\"", cal->recurrence->weekdays);
      }
      else if (0 != cal->recurrence->frequency) 
      {
        day = localtime((time_t *)&cal->start_date);
        day->tm_wday--;
        if (day->tm_wday == -1)
          day->tm_wday = 6;
        g_string_sprintfa(bodystr, " rweekdays=\"%d\"", 1 << day->tm_wday);
      }
      
    } /* recurrence */

    /* anonymous attributes */
    if(cal->anons) 
    {
      current_anon = cal->anons;
      while(current_anon != NULL) 
      {
        if(current_anon->data && 
           ((anon_data*)(current_anon->data))->attr && 
           ((anon_data*)(current_anon->data))->val)
        {
          g_string_sprintfa(bodystr, 
                            " %s=\"%s\"", 
                            ((anon_data*)(current_anon->data))->attr, 
                            opie_xml_markup_escape_text(((anon_data*)(current_anon->data))->val, 
                                                        strlen(((anon_data*)(current_anon->data))->val)));
        }        
	current_anon = current_anon->next;
      }
    }
      
    g_string_append(bodystr, " />\n");
  }
    
  g_string_append(bodystr, "</events>\n</DATEBOOK>");

  if(env->device_type == OPIE_SYNC_QTOPIA_2)
  {
    /* need the rid max */
    g_string_sprintfa(xmlstr, "<RIDMax>%u</RIDMax>\n", cal_rid_max);
  }

  /* append the bodystr onto the xmlstr */
  g_string_append(xmlstr, bodystr->str);

  retval = g_strdup(xmlstr->str);  
  g_string_free(xmlstr, FALSE);
  g_string_free(bodystr, FALSE);
  return retval; 
}

#endif


/*
 * contact_data_to_xml
 */
char* contact_data_to_xml(OpieSyncEnv* env, GList* contacts)
{
  GString *xmlstr, *bodystr;
  GList* li;
  GList* current_email;
  GList* current_cid;
  GList* current_anon;
  contact_data* contact;
  char* retval;
  
  xmlstr = g_string_new("<?xml version=\"1.0\"?>");
  g_string_append(xmlstr, "<!DOCTYPE Addressbook >");
  g_string_append(xmlstr, "<Addressbook>\n");
  
  
  bodystr = g_string_new("<Groups></Groups>\n");
  g_string_append(bodystr, "<Contacts>\n");
  
  for(li = contacts; li != NULL; li = g_list_next(li))
  {
    contact = (contact_data*)li->data;
    
    g_string_append(bodystr, "<Contact");
    
    /* set the contact attributes as appropriate */
    if(contact->uid)
      g_string_sprintfa(bodystr, " Uid=\"%s\"", contact->uid);
      
    if(env->device_type == OPIE_SYNC_QTOPIA_2)
    {
      /* need the rid */
      if(0 == contact->rid)
      {
        contact_rid_max++;
        contact->rid = contact_rid_max;
      }
      g_string_sprintfa(bodystr, " rid=\"%u\"", contact->rid);
      
      /* and the rinfo */
      if(0 == contact->rinfo)
      {
        contact->rinfo = contact_rinfo;
      }
      g_string_sprintfa(bodystr, " rinfo=\"%u\"", contact->rinfo);
    }

    if(contact->cids) 
    {
      g_string_sprintfa(bodystr, " Categories=\"");
      current_cid = contact->cids;
      while(current_cid != NULL) 
      {
        if(current_cid->data)
        {
          /* first cid should not have a ; before it */
          if(current_cid != contact->cids)
            g_string_sprintfa(bodystr,";");
          
      	  g_string_sprintfa(bodystr,"%s", (char *)current_cid->data);
        }
        
	      current_cid= current_cid->next;
      }
      g_string_sprintfa(bodystr,"\"");
    }
    
    if(contact->first_name)
      g_string_sprintfa(bodystr, " FirstName=\"%s\"", opie_xml_markup_escape_text(contact->first_name, strlen(contact->first_name)));
    if(contact->middle_name)
      g_string_sprintfa(bodystr, " MiddleName=\"%s\"", opie_xml_markup_escape_text(contact->middle_name, strlen(contact->middle_name)));
    if(contact->last_name)
      g_string_sprintfa(bodystr, " LastName=\"%s\"", opie_xml_markup_escape_text(contact->last_name, strlen(contact->last_name)));
    if(contact->suffix)
      g_string_sprintfa(bodystr, " Suffix=\"%s\"", opie_xml_markup_escape_text(contact->suffix, strlen(contact->suffix)));

    if(contact->file_as)
    {
      g_string_sprintfa(bodystr, " FileAs=\"%s\"", opie_xml_markup_escape_text(contact->file_as, strlen(contact->file_as)));
    }
    else
    {
      /* file as not set - try setting it to something else */
      if(contact->first_name)
        g_string_sprintfa(bodystr, " FileAs=\"%s\"", opie_xml_markup_escape_text(contact->first_name, strlen(contact->first_name)));
      else if(contact->last_name)
        g_string_sprintfa(bodystr, " FileAs=\"%s\"", opie_xml_markup_escape_text(contact->last_name, strlen(contact->last_name)));
      else if(contact->company)
        g_string_sprintfa(bodystr, " FileAs=\"%s\"", opie_xml_markup_escape_text(contact->company, strlen(contact->company)));
      else if(contact->department)
        g_string_sprintfa(bodystr, " FileAs=\"%s\"", opie_xml_markup_escape_text(contact->department, strlen(contact->department)));
    }
    
    if(contact->department)
    {
      g_string_sprintfa(bodystr, " Department=\"%s\"", opie_xml_markup_escape_text(contact->department, strlen(contact->department)));
    }
    
    if(contact->company)
    {
      g_string_sprintfa(bodystr, " Company=\"%s\"", opie_xml_markup_escape_text(contact->company, strlen(contact->company)));
    }
    
    if(contact->emails) 
    {
      g_string_sprintfa(bodystr, " Emails=\"");
      current_email = contact->emails;
      
      while (current_email != NULL) 
      {
        /* first email should not have a space before it */
        if (current_email!=contact->emails)
          g_string_sprintfa(bodystr," ");
        
        if(current_email->data)
          g_string_sprintfa(bodystr,"%s", opie_xml_markup_escape_text(current_email->data, strlen(current_email->data)));
        
	      current_email=current_email->next;
      }
      g_string_sprintfa(bodystr,"\"");
    }

    if(contact->default_email)
      g_string_sprintfa(bodystr, " DefaultEmail=\"%s\"", opie_xml_markup_escape_text(contact->default_email, strlen(contact->default_email)));

    if(contact->home_phone)
      g_string_sprintfa(bodystr, " HomePhone=\"%s\"", opie_xml_markup_escape_text(contact->home_phone, strlen(contact->home_phone)));

    if(contact->home_fax)
      g_string_sprintfa(bodystr, " HomeFax=\"%s\"", opie_xml_markup_escape_text(contact->home_fax, strlen(contact->home_fax)));

    if(contact->home_mobile)
      g_string_sprintfa(bodystr, " HomeMobile=\"%s\"", opie_xml_markup_escape_text(contact->home_mobile, strlen(contact->home_mobile)));

    if(contact->home_street)
      g_string_sprintfa(bodystr, " HomeStreet=\"%s\"", opie_xml_markup_escape_text(contact->home_street, strlen(contact->home_street)));

    if(contact->home_city)
      g_string_sprintfa(bodystr, " HomeCity=\"%s\"", opie_xml_markup_escape_text(contact->home_city, strlen(contact->home_city)));

    if(contact->home_state)
      g_string_sprintfa(bodystr, " HomeState=\"%s\"", opie_xml_markup_escape_text(contact->home_state, strlen(contact->home_state)));

    if(contact->home_zip)
      g_string_sprintfa(bodystr, " HomeZip=\"%s\"", opie_xml_markup_escape_text(contact->home_zip, strlen(contact->home_zip)));

    if(contact->home_country)
      g_string_sprintfa(bodystr, " HomeCountry=\"%s\"", opie_xml_markup_escape_text(contact->home_country, strlen(contact->home_country)));

    if(contact->home_webpage)
      g_string_sprintfa(bodystr, " HomeWebPage=\"%s\"", opie_xml_markup_escape_text(contact->home_webpage, strlen(contact->home_webpage)));

    if(contact->business_phone)
      g_string_sprintfa(bodystr, " BusinessPhone=\"%s\"", opie_xml_markup_escape_text(contact->business_phone, strlen(contact->business_phone)));

    if(contact->business_fax)
      g_string_sprintfa(bodystr, " BusinessFax=\"%s\"", opie_xml_markup_escape_text(contact->business_fax, strlen(contact->business_fax)));

    if(contact->business_mobile)
      g_string_sprintfa(bodystr, " BusinessMobile=\"%s\"", opie_xml_markup_escape_text(contact->business_mobile, strlen(contact->business_mobile)));

    if(contact->business_pager)
      g_string_sprintfa(bodystr, " BusinessPager=\"%s\"", opie_xml_markup_escape_text(contact->business_pager, strlen(contact->business_pager)));

    if(contact->business_street)
      g_string_sprintfa(bodystr, " BusinessStreet=\"%s\"", opie_xml_markup_escape_text(contact->business_street, strlen(contact->business_street)));

    if(contact->business_city)
      g_string_sprintfa(bodystr, " BusinessCity=\"%s\"", opie_xml_markup_escape_text(contact->business_city, strlen(contact->business_city)));

    if(contact->business_state)
      g_string_sprintfa(bodystr, " BusinessState=\"%s\"", opie_xml_markup_escape_text(contact->business_state, strlen(contact->business_state)));

    if(contact->business_zip)
      g_string_sprintfa(bodystr, " BusinessZip=\"%s\"", opie_xml_markup_escape_text(contact->business_zip, strlen(contact->business_zip)));

    if(contact->business_country)
      g_string_sprintfa(bodystr, " BusinessCountry=\"%s\"", opie_xml_markup_escape_text(contact->business_country, strlen(contact->business_country)));

    if(contact->business_webpage)
      g_string_sprintfa(bodystr, " BusinessWebPage=\"%s\"", opie_xml_markup_escape_text(contact->business_webpage, strlen(contact->business_webpage)));

    if(contact->spouse)
      g_string_sprintfa(bodystr, " Spouse=\"%s\"", opie_xml_markup_escape_text(contact->spouse, strlen(contact->spouse)));

    if(contact->birthday)
      g_string_sprintfa(bodystr, " Birthday=\"%s\"", opie_xml_markup_escape_text(contact->birthday, strlen(contact->birthday)));

    if(contact->anniversary)
      g_string_sprintfa(bodystr, " Anniversary=\"%s\"", opie_xml_markup_escape_text(contact->anniversary, strlen(contact->anniversary)));

    if(contact->nickname)
      g_string_sprintfa(bodystr, " Nickname=\"%s\"", opie_xml_markup_escape_text(contact->nickname, strlen(contact->nickname)));

    if(contact->children)
      g_string_sprintfa(bodystr, " Children=\"%s\"", opie_xml_markup_escape_text(contact->children, strlen(contact->children)));

    if(contact->notes)
      g_string_sprintfa(bodystr, " Notes=\"%s\"", opie_xml_markup_escape_text(contact->notes, strlen(contact->notes)));
      
    if(contact->gender)
      g_string_sprintfa(bodystr, " Gender=\"%d\"", contact->gender);

    if(contact->assistant)
      g_string_sprintfa(bodystr, " Assistant=\"%s\"", opie_xml_markup_escape_text(contact->assistant, strlen(contact->assistant)));
     
    if(contact->manager)
      g_string_sprintfa(bodystr, " Manager=\"%s\"", opie_xml_markup_escape_text(contact->manager, strlen(contact->manager)));
     
    if(contact->office)
      g_string_sprintfa(bodystr, " Office=\"%s\"", opie_xml_markup_escape_text(contact->office, strlen(contact->office)));
     
    if(contact->profession)
      g_string_sprintfa(bodystr, " Profession=\"%s\"", opie_xml_markup_escape_text(contact->profession, strlen(contact->profession)));
     
    if(contact->jobtitle)
      g_string_sprintfa(bodystr, " JobTitle=\"%s\"", opie_xml_markup_escape_text(contact->jobtitle, strlen(contact->jobtitle)));     

    /* anonymous attributes */
    if(contact->anons) 
    {
      current_anon = contact->anons;
      while(current_anon != NULL) 
      {
        if(current_anon->data && 
           ((anon_data*)(current_anon->data))->attr && 
           ((anon_data*)(current_anon->data))->val)
        {
          g_string_sprintfa(bodystr, 
                            " %s=\"%s\"", 
                            ((anon_data*)(current_anon->data))->attr, 
                            opie_xml_markup_escape_text(((anon_data*)(current_anon->data))->val, 
                                                        strlen(((anon_data*)(current_anon->data))->val)));
        }        
	current_anon = current_anon->next;
      }
    }
              
    g_string_append(bodystr, " />\n");
  }
    
  g_string_append(bodystr, "</Contacts>\n</Addressbook>");

  if(env->device_type == OPIE_SYNC_QTOPIA_2)
  {
    /* need the rid max */
    g_string_sprintfa(xmlstr, "<RIDMax>%u</RIDMax>\n", contact_rid_max);
  }
  
  /* append the bodystr onto the xmlstr */
  g_string_append(xmlstr, bodystr->str);

  retval = g_strdup(xmlstr->str);  
  g_string_free(xmlstr, FALSE);
  g_string_free(bodystr, FALSE);
  return retval; 
}

#if 0

/*
 * todo_data_to_xml
 */
char* todo_data_to_xml(OpieSyncEnv* env, GList* todos)
{
  GString *xmlstr, *bodystr;
  GList* li;
  GList* current_cid;
  GList* current_anon;
  todo_data* todo;
  char* retval;
  
  xmlstr = g_string_new("<?xml version=\"1.0\"?>\n");
  g_string_append(xmlstr, "<!DOCTYPE Tasks>\n");
  g_string_append(xmlstr, "<Tasks>\n");
  
  
  bodystr = g_string_new("");
  
  for(li = todos; li != NULL; li = g_list_next(li))
  {
    todo = (todo_data*)li->data;      
    
    g_string_append(bodystr, "<Task");
    
    /* set the contact attributes as appropriate */
    if(todo->uid)
      g_string_sprintfa(bodystr, " Uid=\"%s\"", todo->uid);
          
    if(env->device_type == OPIE_SYNC_QTOPIA_2)
    {
      /* need the rid */
      if(0 == todo->rid)
      {
        todo_rid_max++;
        todo->rid = todo_rid_max;
      }
      g_string_sprintfa(bodystr, " rid=\"%u\"", todo->rid);
      
      /* and the rinfo */
      if(0 == todo->rinfo)
      {
        todo->rinfo = todo_rinfo;
      }
      g_string_sprintfa(bodystr, " rinfo=\"%u\"", todo->rinfo);
    }

    if(todo->cids) 
    {
      g_string_sprintfa(bodystr, " Categories=\"");
      current_cid = todo->cids;
      while(current_cid != NULL) 
      {
        if(current_cid->data)
        {
          /* first cid should not have a ; before it */
          if(current_cid != todo->cids)
            g_string_sprintfa(bodystr,";");

      	  g_string_sprintfa(bodystr,"%s", (char *)current_cid->data);
        }
	      current_cid= current_cid->next;
      }
      g_string_sprintfa(bodystr,"\"");
    }
    
    if(todo->completed)
      g_string_sprintfa(bodystr, " Completed=\"%s\"", todo->completed);

    if(todo->hasdate)
      g_string_sprintfa(bodystr, " HasDate=\"%s\"", todo->hasdate);

    if(todo->dateyear)
      g_string_sprintfa(bodystr, " DateYear=\"%s\"", todo->dateyear);
      
    if(todo->datemonth)
      g_string_sprintfa(bodystr, " DateMonth=\"%s\"", todo->datemonth);
      
    if(todo->dateday)
      g_string_sprintfa(bodystr, " DateDay=\"%s\"", todo->dateday);
      
    if(todo->priority)
      g_string_sprintfa(bodystr, " Priority=\"%s\"", todo->priority);
      
    if(todo->progress)
      g_string_sprintfa(bodystr, " Progress=\"%s\"", todo->progress);

    if(todo->desc)
      g_string_sprintfa(bodystr, " Description=\"%s\"", opie_xml_markup_escape_text(todo->desc, strlen(todo->desc)));
          
    if(todo->summary)
      g_string_sprintfa(bodystr, " Summary=\"%s\"", opie_xml_markup_escape_text(todo->summary, strlen(todo->summary)));

    /* anonymous attributes */
    if(todo->anons) 
    {
      current_anon = todo->anons;
      while(current_anon != NULL) 
      {
        if(current_anon->data && 
           ((anon_data*)(current_anon->data))->attr && 
           ((anon_data*)(current_anon->data))->val)
        {
          g_string_sprintfa(bodystr, 
                            " %s=\"%s\"", 
                            ((anon_data*)(current_anon->data))->attr, 
                            opie_xml_markup_escape_text(((anon_data*)(current_anon->data))->val, 
                                                        strlen(((anon_data*)(current_anon->data))->val)));
        }        
	current_anon = current_anon->next;
      }
    }    
          
    g_string_append(bodystr, " />\n");
  }
  
  g_string_append(bodystr, "</Tasks>");
  
  if(env->device_type == OPIE_SYNC_QTOPIA_2)
  {
    /* need the rid max */
    g_string_sprintfa(xmlstr, "<RIDMax>%u</RIDMax>\n", todo_rid_max);
  }
  
  /* append the bodystr onto the xmlstr */
  g_string_append(xmlstr, bodystr->str);

  retval = g_strdup(xmlstr->str);  
  g_string_free(xmlstr, FALSE);
  g_string_free(bodystr, FALSE);
  return retval; 
}


/*
 * category_data_to_xml
 */
char* category_data_to_xml(OpieSyncEnv* env, GList* categories)
{
  GString *xmlstr, *bodystr;
  GList* li;
  category_data* cat;
  char* retval;
  
  xmlstr = g_string_new("<?xml version=\"1.0\"?>\n");
  g_string_append(xmlstr, "<!DOCTYPE CategoryList>\n");
  g_string_append(xmlstr, "<Categories>\n");
  
  
  bodystr = g_string_new("");
  
  for(li = categories; li != NULL; li = g_list_next(li))
  {
    cat = (category_data*)li->data;      
    
    if(cat)
    {
      g_string_append(bodystr, "<Category");

      if(cat->cid)
        g_string_sprintfa(bodystr, " id=\"%s\"", cat->cid);

      if(cat->category_name)
        g_string_sprintfa(bodystr, " name=\"%s\"", opie_xml_markup_escape_text(cat->category_name, strlen(cat->category_name)));

      g_string_append(bodystr, " />\n");
    }
  }
  
  g_string_append(bodystr, "</Categories>");
    
  /* append the bodystr onto the xmlstr */
  g_string_append(xmlstr, bodystr->str);

  retval = g_strdup(xmlstr->str);  
  g_string_free(xmlstr, FALSE);
  g_string_free(bodystr, FALSE);
  return retval; 
}

#endif


/*
 * opie_xml_append_escaped_text
 *
 * borrowed (slightly modified) from glib2 since we don't have this in glib1
 * remove when ported to gnome2/glib2
 */
static void opie_xml_append_escaped_text (GString     *str,
                                          const gchar *text,
                                          gssize       length)    

{
  const gchar *p;
  const gchar *end;
  char  tmp[2];

  p = text;
  end = text + length;

  while (p != end)
  {
    switch (*p)
    {
      case '&':
        g_string_append (str, "&amp;");
        break;

      case '<':
        g_string_append (str, "&lt;");
        break;

      case '>':
        g_string_append (str, "&gt;");
        break;

      case '\'':
        g_string_append (str, "&apos;");
        break;

      case '"':
        g_string_append (str, "&quot;");
        break;

      default:
        sprintf(tmp, "%c", *p);
        g_string_append (str, tmp);
        break;
    }

    ++p;
  }
}


/*
 * opie_xml_markup_escape_text
 *
 * borrowed (slightly modified) from glib2 since we don't have this in glib1
 * remove when ported to gnome2/glib2
 */
gchar* opie_xml_markup_escape_text(const gchar *text,
                                   gssize       length)  

{
  GString *str;
  gchar* ret;
  
  if(NULL == text)
    return NULL;

  if (length < 0)
    length = strlen(text);

  str = g_string_new(NULL);

  opie_xml_append_escaped_text(str, text, length);

  ret = str->str;
  g_string_free(str, FALSE);
  
  return ret;
}


