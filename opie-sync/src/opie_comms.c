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
	char *remote_filename;
	char *local_filename; /* use fd instead where possible */
	int local_fd;
	int resource_type;
} fetch_pair;

enum temp_file_type {
	TT_STANDARD = 1,
	TT_VISIBLE = 2,
	TT_DEBUG = 3,
	TT_DEBUG_CREATE = 4
};

const char *OPIE_ADDRESS_FILE  = "Applications/addressbook/addressbook.xml";
const char *OPIE_TODO_FILE     = "Applications/todolist/todolist.xml";
const char *OPIE_CALENDAR_FILE = "Applications/datebook/datebook.xml";
const char *OPIE_CATEGORY_FILE = "Settings/Categories.xml";

int opie_curl_fwrite(void* buffer, size_t size, size_t nmemb, void* stream);
gboolean ftp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch);
gboolean scp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch);
gboolean ftp_put_files(OpieSyncEnv* env, GList* files_to_put);
gboolean scp_put_files(OpieSyncEnv* env, GList* files_to_put);



/* FIXME we aren't calling comms_init or comms_shutdown anywhere! */

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
 * Set up a temporary file and add it to a list
 */
int list_add_temp_file(GList **file_list, const char *remote_file, int resource_type, int tmpfilemode) {
	fetch_pair *pair = g_malloc(sizeof(fetch_pair));
	pair->remote_filename = g_strdup(remote_file);
	pair->resource_type = resource_type;
	if(tmpfilemode == TT_DEBUG || tmpfilemode == TT_DEBUG_CREATE) {
		/* Bypass normal temporary file handling (for debugging purposes) */
		char *basename = g_path_get_basename(remote_file);
		pair->local_filename = g_strdup_printf("/tmp/%s", basename);
		g_free(basename);
		if(tmpfilemode == TT_DEBUG_CREATE)
			pair->local_fd = open(pair->local_filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		else
			pair->local_fd = open(pair->local_filename, O_RDWR | O_EXCL);
		if(pair->local_fd == -1) {
			osync_trace( TRACE_INTERNAL, "failed to open file" );
		}
	}
	else {
		char *template = g_strdup("/tmp/opie-sync.XXXXXX");
		pair->local_fd = mkstemp(template);
		if(pair->local_fd == -1) {
			osync_trace( TRACE_INTERNAL, "failed to create temporary file" );
			g_free(template);
			return -1;
		}
		pair->local_filename = template;
		if(tmpfilemode != TT_VISIBLE) {
			if(unlink(template) == -1) {
				osync_trace( TRACE_INTERNAL, "failed to unlink temporary file" );
			}
		}
	}
	
	*file_list = g_list_append(*file_list, pair);
	return pair->local_fd; 
}


/*
 * Free a file list as built by list_add_temp_files 
 */
void list_cleanup(GList *file_list) {
	guint len = g_list_length(file_list);
	guint t;
	/* free each of the items in turn */
	for(t = 0; t < len; ++t)
	{
		fetch_pair* pair = g_list_nth_data(file_list, t);
		g_free(pair->local_filename);
		g_free(pair);
	}
	g_list_free(file_list);
}


/*
 * Manually clean up temp files
 */
void cleanup_temp_files(GList* file_list, int tmpfilemode) {
	guint len = g_list_length(file_list);
	guint t;
	
	for(t = 0; t < len; ++t) {
		fetch_pair* pair = g_list_nth_data(file_list, t);
		if(tmpfilemode == TT_VISIBLE) {
			if(unlink(pair->local_filename) == -1) {
				osync_trace( TRACE_INTERNAL, "failed to unlink temporary file" );
			}
		}
		if(pair->local_fd > 0)
			close(pair->local_fd);
	}
}


/*
 * backup data from an fd to a file
 */
int backup_file(const char *backupfile, int fd) {
	int destfd = 0;
	int rc = TRUE;
	int bufsize = 1024;
	int rbytes, wbytes;
	char *buf = NULL;
	
	destfd = open(backupfile, O_CREAT | O_WRONLY | O_EXCL, 0600);
	if(destfd == -1) {
		perror("error creating backup file");
		goto error;
	}
	
	/* Rewind to start */
	lseek(fd, 0, SEEK_SET);
	
	buf = g_malloc0(bufsize);
	while(TRUE) {
		rbytes = read(fd, buf, bufsize);
		if(rbytes == -1) {
			perror("error reading during backup");
			close(destfd);
			goto error;
		}
		else if(rbytes > 0) {
			wbytes = write(destfd, buf, rbytes);
			if(wbytes == -1) {
				perror("error writing to backup file");
				close(destfd);
				goto error;
			}
		}
		else  {
			/* finished */
			close(destfd);
			break;
		}
	}
	
	/* Rewind to start */
	lseek(fd, 0, SEEK_SET);

error:
	g_free(buf);
	
	return rc;
}


/*
 * backup a list of files
 */
int backup_files(const char *backupdir, GList* file_list) {
	guint len = g_list_length(file_list);
	guint t;
	time_t currtime;
	int rc = TRUE;
	char *backuppath = NULL;
	char *datestamp = NULL;

	/* Construct a path */
	time(&currtime);
	struct tm *tm_ptr = localtime(&currtime);
	datestamp = g_strdup_printf("%04d%02d%02d%02d%02d%02d", 
															(tm_ptr->tm_year + 1900), (tm_ptr->tm_mon + 1), tm_ptr->tm_mday, 
															tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
	backuppath = g_build_filename(backupdir, datestamp, NULL);
	if(g_mkdir_with_parents(backuppath, 0700)) {
		perror("error creating backup directory");
		goto error;
	}
	
	for(t = 0; t < len; ++t) {
		fetch_pair* pair = g_list_nth_data(file_list, t);
		if(pair->local_fd > 0) {
			/* Build full path to file */
			char *basename = g_path_get_basename(pair->remote_filename);
			char *backupfile = g_build_filename(backuppath, basename, NULL);
			/* Run the backup */
			rc = backup_file(backupfile, pair->local_fd);
			g_free(backupfile);
			g_free(basename);
			if(!rc) {
				/* Error occurred */
				break;
			}
		}
	}

error:
	g_free(datestamp);
	g_free(backuppath);
	
	return rc;
} 



/*
 * opie_connect_and_fetch
 */
gboolean opie_connect_and_fetch(OpieSyncEnv* env, opie_object_type object_types)
{
	gboolean rc = TRUE;
	int tmpfilemode;
	
	/* files to fetch */
	GList* files_to_fetch = NULL;
	
	if(!env)
		return FALSE;
	
	if(env->conn_type == OPIE_CONN_NONE) {
		tmpfilemode = TT_DEBUG;
	}
	else if (env->conn_type == OPIE_CONN_SCP) {
		tmpfilemode = TT_VISIBLE;
	}
	else {
		tmpfilemode = TT_STANDARD;
	}
	
	if(object_types & OPIE_OBJECT_TYPE_PHONEBOOK)
		list_add_temp_file(&files_to_fetch, OPIE_ADDRESS_FILE, OPIE_OBJECT_TYPE_PHONEBOOK, tmpfilemode);
	
	if(object_types & OPIE_OBJECT_TYPE_TODO)
		list_add_temp_file(&files_to_fetch, OPIE_TODO_FILE, OPIE_OBJECT_TYPE_TODO, tmpfilemode);
	
	if(object_types & OPIE_OBJECT_TYPE_CALENDAR)
		list_add_temp_file(&files_to_fetch, OPIE_CALENDAR_FILE, OPIE_OBJECT_TYPE_CALENDAR, tmpfilemode);
	
	/* always fetch the categories file */
	list_add_temp_file(&files_to_fetch, OPIE_CATEGORY_FILE, OPIE_OBJECT_TYPE_CATEGORIES, tmpfilemode);

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

	if(rc)
	{
		if(env->backupdir) 
		{
			rc = backup_files(env->backupdir, files_to_fetch);
		}
		
		if(rc)
		{
			/* Parse files */
			guint len = g_list_length(files_to_fetch);
			guint t;
			for(t = 0; t < len; ++t)
			{
				fetch_pair* pair = g_list_nth_data(files_to_fetch, t);
				xmlDoc **doc = NULL;
				switch(pair->resource_type) {
					case OPIE_OBJECT_TYPE_PHONEBOOK:
						doc = &env->contacts_doc;
						if(pair->local_fd <= 0)
							*doc = opie_xml_create_contacts_doc(); 
						break;
					case OPIE_OBJECT_TYPE_TODO:
						doc = &env->todos_doc;
						if(pair->local_fd <= 0)
							*doc = opie_xml_create_todos_doc(); 
						break;
					case OPIE_OBJECT_TYPE_CALENDAR:
						doc = &env->calendar_doc;
						if(pair->local_fd <= 0)
							*doc = opie_xml_create_calendar_doc(); 
						break;
					case OPIE_OBJECT_TYPE_CATEGORIES:
						doc = &env->categories_doc;
						if(pair->local_fd <= 0)
							*doc = opie_xml_create_categories_doc(); 
						break;
					default:
						osync_trace( TRACE_INTERNAL, "unrecognised resource type %d", pair->resource_type );
						return FALSE;
				}
				
				if(pair->local_fd > 0) {
					*doc = opie_xml_fd_open(pair->local_fd);
					/* Flag document as unchanged */
					(*doc)->_private = (void *)1;
					close(pair->local_fd);
					pair->local_fd = -1;
				}
				
				if(!*doc)
					return FALSE;
			}
		}
	}
	
	cleanup_temp_files(files_to_fetch, tmpfilemode);
	list_cleanup(files_to_fetch);
	
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

	if (env->url && env->username && env->password )
	{
		char* separator_path;
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
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
			
			fd = fdopen(pair->local_fd, "w+"); 
			if(!fd)
			{
				OPIE_DEBUG("Failed to open temporary file\n");
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

			if(res == CURLE_FTP_COULDNT_RETR_FILE || res == CURLE_FTP_ACCESS_DENIED)
			{
				/* This is not unlikely (eg. blank device). Note that Opie's FTP
				 server returns "access denied" on non-existent directory. */
				OPIE_DEBUG("FTP file doesn't exist, ignoring\n");
				/* Close the fd and set it to -1 to indicate the file wasn't there */
				pair->local_fd = -1;
			}
			else if(res != CURLE_OK) 
			{
				/* could not get the file */
				fprintf(stderr, "FTP download failed (error %d)\n", res);
				rc = FALSE;
				break;
			}
			else
			{
				OPIE_DEBUG("FTP ok\n");
			}

			fflush(fd);
			if(pair->local_fd > 0) {
				free(fd);   /* don't fclose, we still need it */
				lseek(pair->local_fd, 0, SEEK_SET);
			}
			else {
				fclose(fd);
			}

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
	int contacts_fd = 0;
	int todos_fd = 0;
	int calendar_fd = 0;
	int categories_fd = 0;
	gboolean rc = TRUE;
	int tmpfilemode;

	/* files to fetch */
	GList* files_to_put = NULL;

	if ( !env ) return FALSE;

	if(env->conn_type == OPIE_CONN_NONE) {
		tmpfilemode = TT_DEBUG_CREATE;
	}
	else if (env->conn_type == OPIE_CONN_SCP) {
		tmpfilemode = TT_VISIBLE;
	}
	else {
		tmpfilemode = TT_STANDARD;
	}
	
	if ( object_types & OPIE_OBJECT_TYPE_PHONEBOOK) {
		if(env->contacts_doc && env->contacts_doc->_private == 0) {
			contacts_fd = list_add_temp_file(&files_to_put, OPIE_ADDRESS_FILE, OPIE_OBJECT_TYPE_PHONEBOOK, tmpfilemode);
			if(opie_xml_save_to_fd(env->contacts_doc, contacts_fd) == -1) {
				osync_trace(TRACE_EXIT_ERROR, "failed to write contacts to temporary file");
				goto error;
			}
			fsync(contacts_fd);
			lseek(contacts_fd, 0, SEEK_SET);
		}
	}

	if(object_types & OPIE_OBJECT_TYPE_TODO) {
		if(env->todos_doc && env->todos_doc->_private == 0) {
			todos_fd = list_add_temp_file(&files_to_put, OPIE_TODO_FILE, OPIE_OBJECT_TYPE_TODO, tmpfilemode);
			if(opie_xml_save_to_fd(env->todos_doc, todos_fd) == -1) {
				osync_trace(TRACE_EXIT_ERROR, "failed to write todos to temporary file");
				goto error;
			}
			fsync(todos_fd);
			lseek(todos_fd, 0, SEEK_SET);
		}
	}
	
	if(object_types & OPIE_OBJECT_TYPE_CALENDAR) {
		if(env->calendar_doc && env->calendar_doc->_private == 0) {
			calendar_fd = list_add_temp_file(&files_to_put, OPIE_CALENDAR_FILE, OPIE_OBJECT_TYPE_CALENDAR, tmpfilemode);
			if(opie_xml_save_to_fd(env->calendar_doc, calendar_fd) == -1) {
				osync_trace(TRACE_EXIT_ERROR, "failed to write events to temporary file");
				goto error;
			}
			fsync(calendar_fd);
			lseek(calendar_fd, 0, SEEK_SET);
		}
	}

	if(env->categories_doc && env->categories_doc->_private == 0) {
		/* always write the categories file */
		categories_fd = list_add_temp_file(&files_to_put, OPIE_CATEGORY_FILE, OPIE_OBJECT_TYPE_CATEGORIES, tmpfilemode);
		if(opie_xml_save_to_fd(env->categories_doc, categories_fd) == -1) {
			osync_trace(TRACE_EXIT_ERROR, "failed to write categories to temporary file");
			goto error;
		}
		fsync(categories_fd);
		lseek(categories_fd, 0, SEEK_SET);
	}

	if(files_to_put) {
		/* check which connection method was requested */
		switch (env->conn_type)
		{
			case OPIE_CONN_NONE:
				/* Do nothing */
				osync_trace(TRACE_INTERNAL, "Skipping Put" );
				break;
			
			case OPIE_CONN_FTP:
				/* attempt an FTP connection */
				OPIE_DEBUG("Attempting FTP Put File.\n");
				rc = ftp_put_files(env, files_to_put);
				break;
				
			case OPIE_CONN_SCP:
				/* attempt and scp connection */
				OPIE_DEBUG("Attempting scp Put File.\n");
				rc = scp_put_files(env, files_to_put);
				break;
				
			default:
				/* unknown connection type */
				rc = FALSE;
				break;
		}
		
		if((!rc) && (env->conn_type != OPIE_CONN_NONE) && env->backupdir) {
			/* If something went wrong and the user has a backup directory set,
				we write the files to their backups dir to avoid possible data loss */ 
			char *backupdir = g_build_filename(env->backupdir, "upload_failures", NULL);
			fprintf(stderr, "Error during upload to device, writing files to %s", backupdir); 
			backup_files(backupdir, files_to_put);
			g_free(backupdir);
		}
		
		cleanup_temp_files(files_to_put, tmpfilemode);
		list_cleanup(files_to_put);
	}
	else {
		OPIE_DEBUG("No changes to write\n");
	}
	
	osync_trace(TRACE_EXIT, "%s(%d)", __func__, rc );
	return rc;
	
error:
  list_cleanup(files_to_put);
  return FALSE;
}


/*
 * ftp_put_files
 */
gboolean ftp_put_files(OpieSyncEnv* env, GList* files_to_put) 
{
	gboolean rc = TRUE;
	struct stat file_info;
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	char* separator_path;
	guint len = g_list_length(files_to_put);
	guint t;
	
	if (env->url && env->username && env->password )
	{
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
		} else {
			separator_path = g_strdup( "/" );
		}
		
		/* put each of the requested files */
		for(t = 0; t < len; ++t)
		{
			fetch_pair* pair = g_list_nth_data(files_to_put, t);
		
			char *ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
			                         env->username,
			                         env->password,
			                         env->url,
			                         env->device_port,
			                         separator_path,
			                         pair->remote_filename);
	
			/* get the file size of the local file */
			fstat(pair->local_fd, &file_info);
		
			hd_src = fdopen(pair->local_fd, "rb+");
			curl = curl_easy_init();
			
			if(hd_src)
			{
				curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE) ;
				curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
				curl_easy_setopt(curl, CURLOPT_INFILE, hd_src);
				curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_info.st_size);
				curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
				
#ifdef _OPIE_PRINT_DEBUG
				curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif
				
				res = curl_easy_perform(curl);
				
				if(res != CURLE_OK) 
				{
					fprintf(stderr, "FTP upload failed (error %d)\n", res);
					rc = FALSE;
				} 
				else
				{
					OPIE_DEBUG("FTP upload ok\n");
					rc = TRUE;
				}
				
				/* cleanup */
				free(hd_src);   /* don't fclose, we still need it */
				curl_easy_cleanup(curl);
			}
			else
			{ 
				/* fopen of local file failed */
				rc = FALSE;
			}
			
			g_free(ftpurl);
			
			if(!rc)
				break;
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
 * scp_fetch_files
 */
gboolean scp_fetch_files(OpieSyncEnv* env, GList* files_to_fetch)
{
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	guint t;
	guint len = g_list_length(files_to_fetch);

	int scpretval;
	
	if(env->url && env->device_port && env->username) 
	{
		/* fetch each of the requested files */
		for(t = 0; t < len; ++t)
		{
			fetch_pair* pair = g_list_nth_data(files_to_fetch, t);

			/* We have to close the temp file, because we want 
			   sftp to be able to write to it */
			close(pair->local_fd);
			
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
			/* reopen the temp file */ 
			pair->local_fd = open(pair->local_filename, O_RDWR | O_EXCL);
		}
	}  
	return rc;
}


/*
 * scp_put_files
 */
gboolean scp_put_files(OpieSyncEnv* env, GList* files_to_put)
{
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval = 0;
	char batchfile[] = "/tmp/opie_syncXXXXXX";
	int batchfd = 0;
	guint t;
	guint len = g_list_length(files_to_put);

	/* to use SFTP to upload we'll have to create a batch file with all of
		the commands we want to execute */
	if((batchfd = mkstemp(batchfile)) < 0)   
	{
		/* could not create temp batch file */
		char* errmsg = g_strdup_printf("SFTP could not create batch file: %s\n",
																	strerror(errno));
		OPIE_DEBUG(errmsg);
		g_free(errmsg);
		rc = FALSE;
																	
	}
	else
	{
		/* ok - print the commands to the file */
		GString *batchbuf = g_string_new("");
		for(t = 0; t < len; ++t)
		{
			fetch_pair* pair = g_list_nth_data(files_to_put, t);
			g_string_append_printf(batchbuf, "put %s %s\n", pair->local_filename, pair->remote_filename);
		}
		g_string_append_printf(batchbuf, "bye\n");
		
		if(write(batchfd, (void *)batchbuf->str, batchbuf->len) < 0)
		{
			/* could not write to temp file */
			char* errmsg = g_strdup_printf("SFTP could not write to batch file: %s\n",
																		strerror(errno));
			OPIE_DEBUG(errmsg);
			g_free(errmsg);
			rc = FALSE;
			close(batchfd);
		}
		else
		{ 
			fsync(batchfd);
			close(batchfd);
			
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
		
		g_string_free(batchbuf, TRUE);
	}
	
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


