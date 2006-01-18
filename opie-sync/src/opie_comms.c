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
 *  $Id: opie_comms.c,v 1.13.2.1 2004/04/12 20:22:37 irix Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <openssl/md5.h>

#include "opie_debug.h"
#include "opie_comms.h"
#include "opie_xml.h"
/*#include "opie_log.h"*/
#include "opie_qcop.h"

typedef struct {
  char* remote_filename;
  char* local_filename;
} fetch_pair;

int opie_curl_fwrite(void* buffer, size_t size, size_t nmemb, void* stream);
gboolean ftp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch);
gboolean scp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch);
gboolean ftp_put_file(OpieSyncEnv* env, char* filename, 
                      opie_object_type obj_type);
gboolean scp_put_file(OpieSyncEnv* env, char* filename,
                      opie_object_type obj_type);



/*
 * comms_init
 */
void comms_init()
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
}


/*
 * comms_shutdown
 */
void comms_shutdown()
{
  curl_global_cleanup();
}


/*
 * opie_find_category
 *
 * given a category id, find the category name 
 */
const char* opie_find_category(const char* cid, GList* categories)
{
  const char* retval = NULL;
  
  if(cid && categories)
  {
    int i = 0;
    int len = g_list_length(categories);

    /* TODO - improve on linear search... */
    for(i = 0; i < len; ++i)
    {
      category_data* category = g_list_nth_data(categories, i);
      if(category)
      {
        if(!strcmp(category->cid, cid))
        {
          retval = category->category_name;
          break;
        } 
      }
    } /* for */
  }  
  
  return retval;
}


/*
 * opie_add_category
 * 
 * add a category if it is not already in the list 
 * and return the new cid, or if it is already in 
 * the list just return the cid
 */
const char* opie_add_category(const char* name, GList** categories)
{
  const char* retval = NULL;
  gboolean found = FALSE;
  
  if(name)
  {
    int i = 0;
    int len = g_list_length(*categories);

    /* TODO - improve on linear search... */
    for(i = 0; i < len; ++i)
    {
      category_data* category = g_list_nth_data(*categories, i);
      if(category)
      {
        if(!strcmp(category->category_name, name))
        {
          retval = category->cid;
          found = TRUE;
          break;
        } 
      }
    } /* for */
    
    if(!found)
    {
      /* new category, so add it */
      category_data* category = g_malloc0(sizeof(category_data));
      
      /* random cid */
      category->cid = g_strdup_printf("%u", (unsigned int)random());
      category->category_name = g_strdup(name);
      
      *categories = g_list_append(*categories, category);
      
      retval = category->cid;
    }
  }  
  
  return retval;
}



/*
 * opie_connect_and_fetch
 */
gboolean opie_connect_and_fetch(OpieSyncEnv* env,
                                opie_object_type object_types,
                                GList** calendar,
                                GList** contacts,
                                GList** todos,
                                GList** categories)
{
  gboolean rc = TRUE;
  
  /* files to fetch */
  GList* files_to_fetch = NULL;
  fetch_pair addr_file = { "Applications/addressbook/addressbook.xml", "/tmp/addressbook.xml" };
  fetch_pair todo_file = { "Applications/todolist/todolist.xml", "/tmp/todolist.xml" };
  fetch_pair cal_file = { "Applications/datebook/datebook.xml", "/tmp/datebook.xml" };
  fetch_pair cat_file = { "Settings/Categories.xml", "/tmp/Categories.xml" };
            
  if(!env)
    return FALSE;

  if(object_types & OPIE_OBJECT_TYPE_PHONEBOOK)
    files_to_fetch = g_list_append(files_to_fetch, &addr_file);
  
  if(object_types & OPIE_OBJECT_TYPE_TODO)
    files_to_fetch = g_list_append(files_to_fetch, &todo_file);
  
  if(object_types & OPIE_OBJECT_TYPE_CALENDAR)
    files_to_fetch = g_list_append(files_to_fetch, &cal_file);

  /* always fetch the categories file */
  files_to_fetch = g_list_append(files_to_fetch, &cat_file);   

    
    
  /* check which connection method was requested */
  switch (env->conn_type)
  {
    case OPIE_CONN_FTP:
      /* attempt an FTP connection */
      OPIE_DEBUG("Attempting FTP Connection.\n");
      rc = ftp_fetch_files(env, files_to_fetch); 
      break;
      
    case OPIE_CONN_SCP:
      /* attempt and scp connection */
      OPIE_DEBUG("Attempting scp Connection.\n");
      rc = scp_fetch_files(env, files_to_fetch);
      break;
      
    default:
      /* unknown connection type */
      rc = FALSE;
      break;     
  }

  if(rc)
  {
    /* parse each file into the calendar, contacts and todos list */
/*    if(object_types & OPIE_OBJECT_TYPE_CALENDAR)
      parse_cal_data(cal_file.local_filename, calendar);*/

    if(object_types & OPIE_OBJECT_TYPE_PHONEBOOK)
      parse_contact_data(addr_file.local_filename, contacts);
      
/*    if(object_types & OPIE_OBJECT_TYPE_TODO)  
      parse_todo_data(todo_file.local_filename, todos);*/
      
    /* parse the categories file */
/*    parse_category_data(cat_file.local_filename, categories); */
  }  
  
  g_list_free(files_to_fetch);
  
  return rc;
      
}


/*
 * ftp_fetch_files
 */
gboolean ftp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch)
{
  gboolean rc = TRUE;
  char* ftpurl = NULL;
  CURL *curl = NULL;
  CURLcode res;
  guint len = g_list_length(files_to_fetch);
  guint t;
  FILE* fd;
    
  if(env->url &&
     env->username &&
     env->password)
  { 
    qcop_conn *qc_conn   = qcop_connect(env->url, env->username, env->password);
    char* root_path      = qcop_get_root(qc_conn);
    char* seperator_path = g_strdup_printf("%s/", root_path);

/*    if(!seperator_path)
     g_strdup("/");*/
    qcop_disconnect(qc_conn);

    /* fetch each of the requested files */
    for(t = 0; t < len; ++t)
    {
      fetch_pair* pair = g_list_nth_data(files_to_fetch, t);

      ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
                               env->username,
                               env->password,
                               env->url,
                               env->device_port,
                               seperator_path,
                               pair->remote_filename);
              
      fd = fopen(pair->local_filename, "w"); 
      if(!fd)
      {
        OPIE_DEBUG("Failed to open file:\n");
        OPIE_DEBUG(pair->local_filename);
        OPIE_DEBUG("\n");
        g_free(ftpurl);
        rc = FALSE;
        break;
      }

      /* curl init */
      curl = curl_easy_init();
      
      curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
      curl_easy_setopt(curl, CURLOPT_FILE, fd);    
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_fwrite);
      
#ifdef _OPIE_PRINT_DEBUG      
      curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif

      OPIE_DEBUG(ftpurl);
      OPIE_DEBUG("\n");
      
      /* perform the transfer */
      res = curl_easy_perform(curl);

      if(CURLE_OK != res) 
      {
        /* could not get the file */
        OPIE_DEBUG("FTP transfer failed\n");
        rc = FALSE;
        break;
      } 
      else
      {
        OPIE_DEBUG("FTP ok\n");
      }
      
      fclose(fd);
      
      g_free(ftpurl);     
      curl_easy_cleanup(curl);
    }                        
    g_free(seperator_path);
    if(root_path)
      g_free(root_path);
  }
  else
  {
    /* not enough data provided to do the connection */
    rc = FALSE; 
  }
  
  
  return rc; 
}


/*
 * opie_connect_and_put
 */
gboolean opie_connect_and_put(OpieSyncEnv* env,
                              char* contacts_file,
                              opie_object_type obj_type)
{ 
  gboolean rc = FALSE;
  
  /* check which connection method was requested */
  switch (env->conn_type)
  {
    case OPIE_CONN_FTP:
      /* attempt an FTP connection */
      OPIE_DEBUG("Attempting FTP Put File.\n");
      rc = ftp_put_file(env, contacts_file, obj_type); 
      break;
      
    case OPIE_CONN_SCP:
      /* attempt and scp connection */
      OPIE_DEBUG("Attempting scp Put File.\n");
      rc = scp_put_file(env, contacts_file, obj_type);
      break;
      
    default:
      /* unknown connection type */
      rc = FALSE;
      break;     
  }

  return rc;
}


/*
 * ftp_put_file
 */
gboolean ftp_put_file(OpieSyncEnv* env, char* filename, 
                      opie_object_type obj_type)
{
  gboolean rc = TRUE;
  struct stat file_info;
  int hd;
  char* dest_filename;
  CURL *curl;
  CURLcode res;
  FILE *hd_src;
  char* ftpurl;
  char* root_path;
  char* seperator_path;
  qcop_conn* qc_conn = qcop_connect(env->url, env->username, env->password);
  
  /* figure out the dest filename */
  switch(obj_type)
  {
    case OPIE_OBJECT_TYPE_PHONEBOOK:
      dest_filename = g_strdup("Applications/addressbook/addressbook.xml");
      break;   
    case OPIE_OBJECT_TYPE_CALENDAR:
      dest_filename = g_strdup("Applications/datebook/datebook.xml");
      break;   
    case OPIE_OBJECT_TYPE_TODO:
      dest_filename = g_strdup("Applications/todolist/todolist.xml");
      break;
    default:
      /* a bit of a hack for now, but we'll assume categories if not spec'd */
      dest_filename = g_strdup("Settings/Categories.xml");     
      break;
  }


  root_path      = qcop_get_root(qc_conn);
  seperator_path = g_strdup_printf("%s/", root_path);
/*  if(!seperator_path)
    g_strdup("/");*/
  qcop_disconnect(qc_conn);

  ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
                           env->username,
                           env->password,
                           env->url,
                           env->device_port,
                           seperator_path,
                           dest_filename);

  g_free(seperator_path);
  if(root_path)
    g_free(root_path);

  /* get the file size of the local file */
  hd = open(filename, O_RDONLY) ;
  fstat(hd, &file_info);
  close(hd); 
  
  hd_src = fopen(filename, "rb");
  curl = curl_easy_init();
  
  if(hd_src)
  {    
    curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE) ;
    curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
    curl_easy_setopt(curl, CURLOPT_INFILE, hd_src);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_info.st_size);
    
#ifdef _OPIE_PRINT_DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif
    
    res = curl_easy_perform(curl);
    
    if(CURLE_OK != res) 
    {
      rc = FALSE;
      OPIE_DEBUG("FTP upload failed\n");
    } 
    else
    {
      rc = TRUE;
      OPIE_DEBUG("FTP upload ok\n");
    }
    
    /* cleanup */
    fclose(hd_src);
    curl_easy_cleanup(curl);
  }
  else
  { 
    /* fopen of local file failed */
    rc = FALSE;
  }
  
  
  
  if(dest_filename)
    g_free(dest_filename);
  
  g_free(ftpurl);
  
  return rc; 
}


/*
 * scp_fetch_files
 */
gboolean scp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch)
{
  gboolean rc = TRUE;
  char* scpcommand = NULL;
  guint t;
  guint len = g_list_length(files_to_fetch);

  int scpretval;
  
  if(env->url &&
     env->device_port &&
     env->username) 
  {
    /* fetch each of the requested files */
    for(t = 0; t < len; ++t)
    {
      fetch_pair* pair = g_list_nth_data(files_to_fetch, t);

      /* not keeping it quiet for the moment */
      scpcommand = g_strdup_printf("sftp -o Port=%d -o BatchMode=yes %s@%s:%s %s",
                                   env->device_port,
                                   env->username,
                                   env->url,
                                   pair->remote_filename,
                                   pair->local_filename);
      
      scpretval = pclose(popen(scpcommand,"w"));
      
      if((scpretval == -1) || (WEXITSTATUS(scpretval) != 0))
      {
        OPIE_DEBUG("SFTP failed\n");
        rc = FALSE;
        break;
      }
      else 
      {
        OPIE_DEBUG("SFTP ok\n");
      }
      
      g_free(scpcommand);
    }
  }  
  return rc;
}


/*
 * scp_put_file
 */
gboolean scp_put_file(OpieSyncEnv* env, char* filename,
                      opie_object_type obj_type)
{
  gboolean rc = TRUE;
  char* dest_filename = NULL;
  char* dest_dir = NULL;
  char* scpcommand = NULL;
  int scpretval = 0;
  char batchfile[] = "/tmp/opie_syncXXXXXX";
  int fd = 0;

  /* figure out the dest filename */
  switch(obj_type)
  {
    case OPIE_OBJECT_TYPE_PHONEBOOK:
      dest_filename = g_strdup("addressbook.xml");
      dest_dir = g_strdup("Applications/addressbook");
      break;
    case OPIE_OBJECT_TYPE_CALENDAR:
      dest_filename = g_strdup("datebook.xml");
      dest_dir = g_strdup("Applications/datebook");
      break;
    case OPIE_OBJECT_TYPE_TODO:
      dest_filename = g_strdup("todolist.xml");
      dest_dir = g_strdup("Applications/todolist");
      break;
    default:
      /* a bit of a hack for now, but we'll assume categories if not spec'd */
      dest_filename = g_strdup("Categories.xml");     
      dest_dir = g_strdup("Settings");
      break;
  }

  /* to use SFTP to upload we'll have to create a batch file with all of
     the commands we want to execute */
  if((fd = mkstemp(batchfile)) < 0)   
  {
    /* could not create temp file */
    char* errmsg = g_strdup_printf("SFTP could not create batch file: %s\n",
                                   strerror(errno));
    OPIE_DEBUG(errmsg);
    g_free(errmsg);
    rc = FALSE;                                   
                                   
  }
  else
  {
    /* ok - print the commands to the file */
    char* batchbuf = g_strdup_printf("cd %s\nput %s %s\nbye\n", 
                                     dest_dir,
                                     filename, 
                                     dest_filename);
                                     
    if(write(fd, (void *)batchbuf, strlen(batchbuf)) < 0)
    {
      /* could not write to temp file */
      char* errmsg = g_strdup_printf("SFTP could not write to batch file: %s\n",
                                     strerror(errno));
      OPIE_DEBUG(errmsg);
      g_free(errmsg);
      rc = FALSE;
      close(fd);                                         
    }
    else
    { 
      fsync(fd);
      close(fd);
       
      /* transfer the file */ 
      scpcommand = g_strdup_printf("sftp -o Port=%d -o BatchMode=yes -b %s %s@%s",
                                   env->device_port,
                                   batchfile,
                                   env->username,
                                   env->url);
      
      scpretval = pclose(popen(scpcommand,"w"));

      if((scpretval == -1) || (WEXITSTATUS(scpretval) != 0))
      {
        rc = FALSE;
        OPIE_DEBUG("SFTP upload failed\n");
      }
      else
      {
        rc = TRUE;
        OPIE_DEBUG("SFTP upload ok\n");
      }

      /* remove the temporary file we created */
      if(unlink(batchfile) < 0)
      {
        char* errmsg = g_strdup_printf("SFTP could not remove batch file: %s\n",
                                       strerror(errno));
        OPIE_DEBUG(errmsg);
        g_free(errmsg);
      }

      g_free(scpcommand);
    }
  }

  if(dest_filename)
    g_free(dest_filename);
    
  if(dest_dir)
    g_free(dest_dir);
    
  return rc;
}


/*
 * opie_curl_fwrite
 */
int opie_curl_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(buffer, size, nmemb, (FILE *)stream);
  return written; 
}


/*
 * serialize_contact_data
 */
char* serialize_contact_data(OpieSyncEnv* env, GList* contacts)
{
  return contact_data_to_xml(env, contacts);
}


/*
 * free_contact_data
 */
void free_contact_data(contact_data* contact)
{
  GList* current_email;
  GList* current_cid;
  GList* current_anon;
  
  /* free each of the elements */
  if(contact->uid)
    g_free(contact->uid);
    
  if(contact->cids) 
  {
    current_cid = contact->cids;
    
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       free(current_cid->data); 
       
      current_cid = current_cid->next;
    }
    g_list_free(contact->cids);
    contact->cids = NULL;
  }

  if(contact->first_name)
    g_free(contact->first_name);
  
  if(contact->middle_name)
    g_free(contact->middle_name);

  if(contact->last_name)
    g_free(contact->last_name);

  if(contact->suffix)
    g_free(contact->suffix);

  if(contact->file_as)
    g_free(contact->file_as);
  
  if(contact->department)
    g_free(contact->department);
  
  if(contact->company)
    g_free(contact->company);
  
  if(contact->emails) 
  {
    current_email=contact->emails;
    
    while(current_email != NULL) 
    {
      if(current_email->data);
        g_free(current_email->data);
        
      current_email=current_email->next;
    }
    g_list_free(contact->emails);
  }

  if(contact->default_email)
    g_free(contact->default_email);
  
  if(contact->home_phone)
    g_free(contact->home_phone);
  
  if(contact->home_fax)
    g_free(contact->home_fax);
  
  if(contact->home_mobile)
    g_free(contact->home_mobile);
  
  if(contact->home_street)
    g_free(contact->home_street);
  
  if(contact->home_city)
    g_free(contact->home_city);
  
  if(contact->home_state)
    g_free(contact->home_state);
  
  if(contact->home_zip)
    g_free(contact->home_zip);
  
  if(contact->home_country)
    g_free(contact->home_country);
  
  if(contact->home_webpage)
    g_free(contact->home_webpage);
  
  if(contact->business_phone)
    g_free(contact->business_phone);
  
  if(contact->business_fax)
    g_free(contact->business_fax);
  
  if(contact->business_mobile)
    g_free(contact->business_mobile);
  
  if(contact->business_pager)
    g_free(contact->business_pager);
  
  if(contact->business_street)
    g_free(contact->business_street);
  
  if(contact->business_city)
    g_free(contact->business_city);
  
  if(contact->business_state)
    g_free(contact->business_state);
  
  if(contact->business_zip)
    g_free(contact->business_zip);
  
  if(contact->business_country)
    g_free(contact->business_country);
  
  if(contact->business_webpage)
    g_free(contact->business_webpage);
  
  if(contact->spouse)
    g_free(contact->spouse);
  
  if(contact->birthday)
    g_free(contact->birthday);
  
  if(contact->anniversary)
    g_free(contact->anniversary);
  
  if(contact->nickname)
    g_free(contact->nickname);
  
  if(contact->children)
    g_free(contact->children);
  
  if(contact->notes)
    g_free(contact->notes);
    
  if(contact->assistant)
    g_free(contact->assistant);
   
  if(contact->manager)
    g_free(contact->manager);
   
  if(contact->office)
    g_free(contact->office);
   
  if(contact->profession)
    g_free(contact->profession);
   
  if(contact->jobtitle)
    g_free(contact->jobtitle);
   
  if(contact->anons) 
  {
    current_anon = contact->anons;
    
    while(current_anon != NULL) 
    {
      if(current_anon->data)
      {
        if(((anon_data*)(current_anon->data))->attr)
          g_free(((anon_data*)(current_anon->data))->attr); 
       
        if(((anon_data*)(current_anon->data))->val)
          g_free(((anon_data*)(current_anon->data))->val); 
        
        g_free(((anon_data*)(current_anon->data)));
      }
      
      current_anon = current_anon->next;
    }
    g_list_free(contact->anons);
    contact->anons = NULL;
  }
    
  /* free the struct itself */
  g_free(contact);
  contact = NULL;
}


/*
 * serialize_cal_data
 */
char* serialize_cal_data(OpieSyncEnv* env, GList* calendar)
{
/*  return cal_data_to_xml(env, calendar); */
	return g_strdup("");
}


/*
 * free_cal_data
 */
void free_cal_data(cal_data* calendar)
{
  GList* current_cid;
  GList* current_anon;

  /* free the elements */
  if(calendar->uid)
    g_free(calendar->uid);
    
  if(calendar->cids) 
  {
    current_cid = calendar->cids;
    
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       free(current_cid->data); 
       
      current_cid = current_cid->next;
    }
    g_list_free(calendar->cids);
    calendar->cids = NULL;
  }

  if(calendar->summary)
    g_free(calendar->summary);
  if(calendar->desc)
    g_free(calendar->desc);
  if(calendar->location)
    g_free(calendar->location);
    
  /* free the alarm entry if present */  
  if(calendar->alarm)
  {
    if(calendar->alarm->related)
      g_free(calendar->alarm->related);
    if(calendar->alarm->desc)
      g_free(calendar->alarm->desc);
      
    g_free(calendar->alarm);  
  }
  
  /* free the recurrence entry if present */
  if(calendar->recurrence)
    g_free(calendar->recurrence);
   
  if(calendar->anons) 
  {
    current_anon = calendar->anons;
    
    while(current_anon != NULL) 
    {
      if(current_anon->data)
      {
        if(((anon_data*)(current_anon->data))->attr)
          g_free(((anon_data*)(current_anon->data))->attr); 
       
        if(((anon_data*)(current_anon->data))->val)
          g_free(((anon_data*)(current_anon->data))->val); 
        
        g_free(((anon_data*)(current_anon->data)));
      }
      
      current_anon = current_anon->next;
    }
    g_list_free(calendar->anons);
    calendar->anons = NULL;
  }
    
  /* free the struct itself */
  g_free(calendar);
  calendar = NULL;
}


/*
 * serialize_todo_data
 */
char* serialize_todo_data(OpieSyncEnv* env, GList* todos)
{
/*  return todo_data_to_xml(env, todos);*/
	return g_strdup("");
}


/*
 * free_todo_data
 */
void free_todo_data(todo_data* todo)
{
  GList* current_cid;
  GList* current_anon;

  /* free the elements */
  if(todo->uid)
    g_free(todo->uid);
    
  if(todo->cids) 
  {
    current_cid = todo->cids;
    
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       free(current_cid->data); 
       
      current_cid = current_cid->next;
    }
    g_list_free(todo->cids);
    todo->cids = NULL;
  }

  if(todo->completed)
    g_free(todo->completed);
  if(todo->hasdate)
    g_free(todo->hasdate);
  if(todo->dateyear)
    g_free(todo->dateyear);
  if(todo->datemonth)
    g_free(todo->datemonth);
  if(todo->dateday)
    g_free(todo->dateday);
  if(todo->priority)
    g_free(todo->priority);
  if(todo->progress)
    g_free(todo->progress);
  if(todo->desc)
    g_free(todo->desc);  
  if(todo->summary)
    g_free(todo->summary);
   
  if(todo->anons) 
  {
    current_anon = todo->anons;
    
    while(current_anon != NULL) 
    {
      if(current_anon->data)
      {
        if(((anon_data*)(current_anon->data))->attr)
          g_free(((anon_data*)(current_anon->data))->attr); 
       
        if(((anon_data*)(current_anon->data))->val)
          g_free(((anon_data*)(current_anon->data))->val); 
        
        g_free(((anon_data*)(current_anon->data)));
      }
      
      current_anon = current_anon->next;
    }
    g_list_free(todo->anons);
    todo->anons = NULL;
  }
  
  /* free the struct itself */
  g_free(todo);
  todo = NULL;
} 


/*
 * serialize_category_data
 */
char* serialize_category_data(OpieSyncEnv* env, GList* categories)
{
/*  return category_data_to_xml(env, categories);*/
	return g_strdup("");
}


/*
 * free_category_data
 */
void free_category_data(category_data* category)
{
  /* free the elements */
  if(category->cid)
    g_free(category->cid);
  if(category->category_name)
    g_free(category->category_name);
  
  /* free the struct itself */
  g_free(category);
  category = NULL;
} 


/*
 * contact_equals
 */
gboolean contact_equals(contact_data* c1, contact_data* c2)
{
  gboolean rc = FALSE;
  unsigned char* c1_hash = NULL;
  unsigned char* c2_hash = NULL;

  /* TODO
   * Should be caching the hash value rather than recomputing
   * it each time.
   */
  
  if((NULL != c1) && (NULL != c2))
  {
    c1_hash = hash_contact(c1);
    c2_hash = hash_contact(c2);
    
    if(c1_hash && c2_hash)
    {
      if(!memcmp(c1_hash, c2_hash, MD5_DIGEST_LENGTH))
      {
        /* they are the same */
        rc = TRUE;
      }
    } 
  }
  
  g_free(c1_hash);
  g_free(c2_hash);
  
  return rc;
}


/*
 * hash_contact
 */
unsigned char* hash_contact(contact_data* contact)
{
  unsigned char* c_hash;
  GList* current_email;
  GList* current_cid;
  MD5_CTX c;
  
  if(!contact)
    return NULL;

  MD5_Init(&c);
  c_hash = g_malloc0(MD5_DIGEST_LENGTH + 1);
  
  if(contact->uid)
    MD5_Update(&c, contact->uid, strlen(contact->uid));

  if(contact->cids) 
  {
    current_cid = contact->cids;
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       MD5_Update(&c, current_cid->data, strlen(current_cid->data));
      
      current_cid = current_cid->next;
    }
  }
  
  if(contact->first_name)
    MD5_Update(&c, contact->first_name, strlen(contact->first_name));
  
  if(contact->last_name)
    MD5_Update(&c, contact->last_name, strlen(contact->last_name));
  
  if(contact->file_as)
    MD5_Update(&c, contact->file_as, strlen(contact->file_as));
  
  if(contact->department)
    MD5_Update(&c, contact->department, strlen(contact->department));
  
  if(contact->company)
    MD5_Update(&c, contact->company, strlen(contact->company));
  
  if(contact->emails) 
  {
    current_email=contact->emails;
    while (current_email!=NULL) 
    {
      if(current_email->data)
       MD5_Update(&c, current_email->data, strlen(current_email->data));
      
      current_email=current_email->next;
    }
  }
  
  if(contact->default_email)
    MD5_Update(&c, contact->default_email, strlen(contact->default_email));
  
  if(contact->home_phone)
    MD5_Update(&c, contact->home_phone, strlen(contact->home_phone));
  
  if(contact->home_fax)
    MD5_Update(&c, contact->home_fax, strlen(contact->home_fax));
  
  if(contact->home_mobile)
    MD5_Update(&c, contact->home_mobile, strlen(contact->home_mobile));
  
  if(contact->home_street)
    MD5_Update(&c, contact->home_street, strlen(contact->home_street));
  
  if(contact->home_city)
    MD5_Update(&c, contact->home_city, strlen(contact->home_city));
  
  if(contact->home_state)
    MD5_Update(&c, contact->home_state, strlen(contact->home_state));
  
  if(contact->home_zip)
    MD5_Update(&c, contact->home_zip, strlen(contact->home_zip));
  
  if(contact->home_country)
    MD5_Update(&c, contact->home_country, strlen(contact->home_country));
  
  if(contact->home_webpage)
    MD5_Update(&c, contact->home_webpage, strlen(contact->home_webpage));
  
  if(contact->business_phone)
    MD5_Update(&c, contact->business_phone, strlen(contact->business_phone));
  
  if(contact->business_fax)
    MD5_Update(&c, contact->business_fax, strlen(contact->business_fax));
  
  if(contact->business_mobile)
    MD5_Update(&c, contact->business_mobile, strlen(contact->business_mobile));
  
  if(contact->business_pager)
    MD5_Update(&c, contact->business_pager, strlen(contact->business_pager));
  
  if(contact->business_street)
    MD5_Update(&c, contact->business_street, strlen(contact->business_street));
  
  if(contact->business_city)
    MD5_Update(&c, contact->business_city, strlen(contact->business_city));
  
  if(contact->business_state)
    MD5_Update(&c, contact->business_state, strlen(contact->business_state));
  
  if(contact->business_zip)
    MD5_Update(&c, contact->business_zip, strlen(contact->business_zip));
  
  if(contact->business_country)
    MD5_Update(&c, contact->business_country, strlen(contact->business_country));
  
  if(contact->business_webpage)
    MD5_Update(&c, contact->business_webpage, strlen(contact->business_webpage));
  
  if(contact->spouse)
    MD5_Update(&c, contact->spouse, strlen(contact->spouse));
  
  if(contact->birthday)
    MD5_Update(&c, contact->birthday, strlen(contact->birthday));
  
  if(contact->anniversary)
    MD5_Update(&c, contact->anniversary, strlen(contact->anniversary));
  
  if(contact->nickname)
    MD5_Update(&c, contact->nickname, strlen(contact->nickname));
  
  if(contact->children)
    MD5_Update(&c, contact->children, strlen(contact->children));
  
  if(contact->notes)
    MD5_Update(&c, contact->notes, strlen(contact->notes));
  
  if(contact->assistant)
    MD5_Update(&c, contact->assistant, strlen(contact->assistant));
   
  if(contact->manager)
    MD5_Update(&c, contact->manager, strlen(contact->manager));
   
  if(contact->office)
    MD5_Update(&c, contact->office, strlen(contact->office));
   
  if(contact->profession)
    MD5_Update(&c, contact->profession, strlen(contact->profession));
   
  if(contact->jobtitle)
    MD5_Update(&c, contact->jobtitle, strlen(contact->jobtitle));
   
  /* compute the hash */
  MD5_Final(c_hash, &c);
  
  return c_hash;
}


/*
 * todo_equals
 */
gboolean todo_equals(todo_data* t1, todo_data* t2)
{
  gboolean rc = FALSE;
  unsigned char* t1_hash = NULL;
  unsigned char* t2_hash = NULL;

  /* TODO
   * Should be caching the hash value rather than recomputing
   * it each time.
   */
  
  if((NULL != t1) && (NULL != t2))
  {
    t1_hash = hash_todo(t1);
    t2_hash = hash_todo(t2);
    
    if(t1_hash && t2_hash)
    {
      if(!memcmp(t1_hash, t2_hash, MD5_DIGEST_LENGTH))
      {
        /* they are the same */
        rc = TRUE;
      }
    } 
  }
  
  g_free(t1_hash);
  g_free(t2_hash);
  
  return rc;

}


/*
 * hash_todo
 */
unsigned char* hash_todo(todo_data* todo)
{
  unsigned char* t_hash;
  GList* current_cid;
  MD5_CTX c;
  
  if(!todo)
    return NULL;

  MD5_Init(&c);
  t_hash = g_malloc0(MD5_DIGEST_LENGTH + 1);
  
  if(todo->uid)
    MD5_Update(&c, todo->uid, strlen(todo->uid));

  if(todo->cids) 
  {
    current_cid = todo->cids;
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       MD5_Update(&c, current_cid->data, strlen(current_cid->data));
      
      current_cid = current_cid->next;
    }
  }
  
  if(todo->completed)
    MD5_Update(&c, todo->completed, strlen(todo->completed));
  
  if(todo->hasdate)
    MD5_Update(&c, todo->hasdate, strlen(todo->hasdate));
    
  if(todo->dateyear)
    MD5_Update(&c, todo->dateyear, strlen(todo->dateyear));
    
  if(todo->datemonth)
    MD5_Update(&c, todo->datemonth, strlen(todo->datemonth));
    
  if(todo->dateday)
    MD5_Update(&c, todo->dateday, strlen(todo->dateday));
  
  if(todo->priority)
    MD5_Update(&c, todo->priority, strlen(todo->priority));
    
  if(todo->progress)
    MD5_Update(&c, todo->progress, strlen(todo->progress));
  
  if(todo->desc)
    MD5_Update(&c, todo->desc, strlen(todo->desc));
  
  if(todo->summary)
    MD5_Update(&c, todo->summary, strlen(todo->summary));
  
  /* compute the hash */
  MD5_Final(t_hash, &c);
  
  return t_hash;

}


/*
 * cal_equals
 */
gboolean cal_equals(cal_data* c1, cal_data* c2)
{
  gboolean rc = FALSE;
  unsigned char* c1_hash = NULL;
  unsigned char* c2_hash = NULL;

  /* TODO
   * Should be caching the hash value rather than recomputing
   * it each time.
   */
  
  if((NULL != c1) && (NULL != c2))
  {
    c1_hash = hash_cal(c1);
    c2_hash = hash_cal(c2);
    
    if(c1_hash && c2_hash)
    {
      if(!memcmp(c1_hash, c2_hash, MD5_DIGEST_LENGTH))
      {
        /* they are the same */
        rc = TRUE;
      }
    } 
  }
  
  g_free(c1_hash);
  g_free(c2_hash);
  
  return rc;
}


/*
 * hash_cal
 */
unsigned char* hash_cal(cal_data* cal)
{
  unsigned char* c_hash;
  char* tmpstr;
  GList* current_cid;
  MD5_CTX c;
  
  if(!cal)
    return NULL;

  MD5_Init(&c);
  c_hash = g_malloc0(MD5_DIGEST_LENGTH + 1);
  
  if(cal->uid)
    MD5_Update(&c, cal->uid, strlen(cal->uid));

  if(cal->cids) 
  {
    current_cid = cal->cids;
    while(current_cid != NULL) 
    {
      if(current_cid->data)
       MD5_Update(&c, current_cid->data, strlen(current_cid->data));
      
      current_cid = current_cid->next;
    }
  }
  
  if(cal->summary)
    MD5_Update(&c, cal->summary, strlen(cal->summary));
  
  if(cal->desc)
    MD5_Update(&c, cal->desc, strlen(cal->desc));
    
  if(cal->location)
    MD5_Update(&c, cal->location, strlen(cal->location));


  /* don't need the all-day flag since the start/end date will
   * change when it does
   */  

  if(cal->start_date != 0)
  {
    tmpstr = g_strdup_printf("%u", (unsigned int)cal->start_date);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
  }
  
  if(cal->end_date != 0)
  {
    tmpstr = g_strdup_printf("%u", (unsigned int)cal->end_date);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
  }

  /* skip created date - not important */
  
  /* hash the alarm entry */
  if(cal->alarm)
  {
    if(cal->alarm->duration != 0)
    {    
      tmpstr = g_strdup_printf("%u", (unsigned int)cal->alarm->duration);
      MD5_Update(&c, tmpstr, strlen(tmpstr));
      g_free(tmpstr);
    }
    
    /* do we need the other alarm stuff here? */
  }
  
  /* hash the recurrence data */
  if(cal->recurrence)
  {
    /* type */
    tmpstr = g_strdup_printf("%d", cal->recurrence->type);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
    
    /* frequency */
    tmpstr = g_strdup_printf("%u", cal->recurrence->frequency);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
    
    /* position */
    tmpstr = g_strdup_printf("%u", cal->recurrence->position);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
    
    /* end date */
    tmpstr = g_strdup_printf("%u", (unsigned int)cal->recurrence->end_date);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
    
    /* weekdays */
    tmpstr = g_strdup_printf("%d", cal->recurrence->weekdays);
    MD5_Update(&c, tmpstr, strlen(tmpstr));
    g_free(tmpstr);
  }  

  /* compute the hash */
  MD5_Final(c_hash, &c);
  
  return c_hash;

}
