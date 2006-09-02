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

/* FIXME don't use hardcoded temp filenames */
static fetch_pair addr_file = { "Applications/addressbook/addressbook.xml", "/tmp/addressbook.xml" };
static fetch_pair todo_file = { "Applications/todolist/todolist.xml", "/tmp/todolist.xml" };
static fetch_pair cal_file = { "Applications/datebook/datebook.xml", "/tmp/datebook.xml" };
static fetch_pair cat_file = { "Settings/Categories.xml", "/tmp/Categories.xml" };

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
 * opie_connect_and_fetch
 */
gboolean opie_connect_and_fetch(OpieSyncEnv* env, opie_object_type object_types)
{
  gboolean rc = TRUE;
  
  /* files to fetch */
  GList* files_to_fetch = NULL;
            
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
  osync_trace( TRACE_INTERNAL, "conn_type = %d", env->conn_type );
  switch (env->conn_type)
  {
    case OPIE_CONN_NONE:
			/* no connection (useful for debugging) */
      OPIE_DEBUG("Skipping Connection.\n");
      break;
      
    case OPIE_CONN_FTP:
      /* attempt an FTP connection */
      OPIE_DEBUG("Attempting FTP Connection.\n");
      rc = ftp_fetch_files(env, files_to_fetch); 
      break;
      
    case OPIE_CONN_SCP:
      /* attempt an scp connection */
      OPIE_DEBUG("Attempting scp Connection.\n");
      rc = scp_fetch_files(env, files_to_fetch);
      break;
      
    default:
      /* unknown connection type */
      rc = FALSE;
      break;     
  }

	/* FIXME: if a file doesn't exist we need to be prepared to create it */
	
  if(rc)
  {
    /* parse each file into the calendar, contacts and todos list */
/*    if(object_types & OPIE_OBJECT_TYPE_CALENDAR)
      parse_cal_data(cal_file.local_filename, calendar);*/

		if(object_types & OPIE_OBJECT_TYPE_PHONEBOOK) {
			env->contacts_file = g_strdup(addr_file.local_filename);
			env->contacts_doc = opie_xml_file_open(env->contacts_file);
		}
      
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
    
  if (env->url &&
      env->username &&
      env->password )
  {  
      char* separator_path;
      if ( env->use_qcop ) 
      {
          qcop_conn *qc_conn   = qcop_connect(env->url, env->username, env->password);
          char* root_path      = qcop_get_root(qc_conn);
          osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
          separator_path = g_strdup_printf("%s/", root_path);

/*    if(!separator_path)
     g_strdup("/");*/
          qcop_disconnect(qc_conn);
          if ( root_path ) g_free(root_path);
      } else {
          separator_path = g_strdup( "/" );
      }

    /* fetch each of the requested files */
    for(t = 0; t < len; ++t)
    {
      fetch_pair* pair = g_list_nth_data(files_to_fetch, t);

      ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
                               env->username,
                               env->password,
                               env->url,
                               env->device_port,
                               separator_path,
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
    g_free(separator_path);
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
gboolean opie_connect_and_put( OpieSyncEnv* env,
                               opie_object_type object_types)
{ 
    osync_trace(TRACE_ENTRY, "%s", __func__ );

  gboolean rc = TRUE;

  /* files to fetch */
  GList* files_to_put = NULL;

  if ( !env ) return FALSE;

  if ( object_types & OPIE_OBJECT_TYPE_PHONEBOOK) {
		printf("OPIE: saving %s\n", addr_file.local_filename);
		xmlSaveFormatFile(addr_file.local_filename, env->contacts_doc, 1);
		files_to_put = g_list_append(files_to_put, &addr_file);
  }

	if(object_types & OPIE_OBJECT_TYPE_TODO)
		files_to_put = g_list_append(files_to_put, &todo_file);
	
	if(object_types & OPIE_OBJECT_TYPE_CALENDAR)
		files_to_put = g_list_append(files_to_put, &cal_file);

	/* always fetch the categories file */
	files_to_put = g_list_append(files_to_put, &cat_file);   

	if ( ! rc ) goto ERROR;
	
	if ( env->conn_type == OPIE_CONN_NONE ) {
		g_list_free(files_to_put);
		osync_trace(TRACE_EXIT, "Skipping Put\n%s(%d)", __func__, rc );
		return TRUE;
	}

	/* check which connection method was requested */
	switch (env->conn_type)
	{
		case OPIE_CONN_FTP:
      /* attempt an FTP connection */
      OPIE_DEBUG("Attempting FTP Put File.\n");
			/* FIXME this isn't using files_to_put */
			rc = ftp_put_file(env, env->contacts_file, OPIE_OBJECT_TYPE_PHONEBOOK); 
      break;
      
    case OPIE_CONN_SCP:
      /* attempt and scp connection */
      OPIE_DEBUG("Attempting scp Put File.\n");
			/* FIXME this isn't using files_to_put */
			rc = scp_put_file(env, env->contacts_file, OPIE_OBJECT_TYPE_PHONEBOOK);
      break;
      
		default:
      /* unknown connection type */
      rc = FALSE;
      break;     
  }

ERROR:
  g_list_free(files_to_put);
  osync_trace(TRACE_EXIT, "%s(%d)", __func__, rc );
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
  char* separator_path;
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
  separator_path = g_strdup_printf("%s/", root_path);
/*  if(!separator_path)
    g_strdup("/");*/
  qcop_disconnect(qc_conn);

  ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
                           env->username,
                           env->password,
                           env->url,
                           env->device_port,
                           separator_path,
                           dest_filename);

  if ( separator_path ) 
      g_free(separator_path);
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


