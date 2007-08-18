/* 
   Derived from the MultiSync Opie Plugin
   Copyright (C) 2007 Paul Eggleton <bluelightning@bluelightning.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync_xml.h>

#include "opie_comms.h"
#include "opie_xml.h"
#include "opie_qcop.h"

int opie_curl_fwrite(void* buffer, size_t size, size_t nmemb, void* stream);
int opie_curl_strwrite(void *buffer, size_t size, size_t nmemb, void *stream);
int opie_curl_nullwrite(void *buffer, size_t size, size_t nmemb, void *stream);
int opie_curl_strread(void *buffer, size_t size, size_t nmemb, void *stream);
gboolean local_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data);
gboolean ftp_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data);
gboolean scp_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data);
gboolean local_put_file(OpiePluginEnv* env, const char *remotefile, const char *data);
gboolean ftp_put_file(OpiePluginEnv* env, const char *remotefile, char *data);
gboolean scp_put_file(OpiePluginEnv* env, const char *remotefile, char *data);
gboolean ftp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc);

int m_totalwritten;


typedef struct {
	char *filename; /* use fd instead where possible */
	int fd;
} TempFile;


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


TempFile *create_temp_file(void) {
	osync_trace(TRACE_ENTRY, "%s", __func__);
	
	TempFile *tmpfile = g_malloc(sizeof(TempFile));
	char *template = g_strdup("/tmp/opie-sync.XXXXXX");
	tmpfile->fd = mkstemp(template);
	if(tmpfile->fd == -1) {
		osync_trace( TRACE_EXIT_ERROR, "failed to create temporary file" );
		g_free(template);
		return NULL;
	}
	tmpfile->filename = template;
	
	osync_trace(TRACE_EXIT, "%s(%p)", __func__, tmpfile);
	return tmpfile;
}


void cleanup_temp_file(TempFile *tempfile) {
	if(tempfile->fd > -1)
		close(tempfile->fd);
	unlink(tempfile->filename); /* FIXME check the result of this */
	g_free(tempfile->filename);
	g_free(tempfile);
}


/*
 * create a backup file from a string
 */
int backup_file(const char *backupfile, const char *str, int len) {
	osync_trace(TRACE_ENTRY, "%s(%s, %p, %i)", __func__, backupfile, str, len);
	
	int destfd = 0;
	int rc = FALSE;
	int bufsize = 1024;
	int wbytes;
	int pos = 0;
	
	destfd = open(backupfile, O_CREAT | O_WRONLY | O_EXCL, 0600);
	if(destfd == -1) {
		osync_trace( TRACE_INTERNAL, "error creating backup file" );
		perror("error creating backup file");
		goto error;
	}
	
	while(TRUE) {
		if(len - pos < bufsize)
			bufsize = len - pos;
		
		wbytes = write(destfd, str + pos, bufsize);
		if(wbytes == -1) {
			osync_trace( TRACE_INTERNAL, "error writing to backup file" );
			perror("error writing to backup file");
			close(destfd);
			goto error;
		}
		
		pos += wbytes;
		if(pos == len)  {
			/* finished */
			close(destfd);
			break;
		}
	}
	
	rc = TRUE;

error:
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc);
	return rc;
}

/*
 * Create a date/time stamped directory for backup files
 */
char *create_backup_dir(const char *backupdir)
{
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, backupdir);
	
	time_t currtime;
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
		osync_trace(TRACE_EXIT_ERROR, "error creating backup directory");
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return backuppath;

error:
	g_free(datestamp);
	g_free(backuppath);
	
	return NULL;
}


/*
 * opie_fetch_sink
 */
gboolean opie_fetch_sink(OpieSinkEnv *env)
{
	return opie_fetch_file(env->plugin_env, env->objtype, env->remotefile, &env->doc, env->sink);
}


/*
 * opie_fetch_file
 */
gboolean opie_fetch_file(OpiePluginEnv *env, OPIE_OBJECT_TYPE objtype, const char *remotefile, xmlDoc **doc, OSyncObjTypeSink *sink)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p, %p)", __func__, env, objtype, remotefile, doc, sink);
	
	gboolean rc = TRUE;
	
	GString *data = NULL;
	
	/* check which connection method was requested */
	osync_trace( TRACE_INTERNAL, "conn_type = %d", env->conn_type );
	switch (env->conn_type)
	{
		case OPIE_CONN_NONE:
			/* Read from local file (for debugging) */
			osync_trace( TRACE_INTERNAL, "Fetching local file" );
			if(objtype == OPIE_OBJECT_TYPE_NOTE) {
				/* FIXME enable notes debugging */
			}
			else
				rc = local_fetch_file(env, remotefile, &data);
			
			break;
			
		case OPIE_CONN_FTP:
			/* attempt an FTP connection */
			osync_trace( TRACE_INTERNAL, "Attempting FTP Connection." );
			if(objtype == OPIE_OBJECT_TYPE_NOTE) {
				*doc = opie_xml_create_notes_doc();
				if(*doc)
					rc = ftp_fetch_notes(env, *doc);
				else
					rc = FALSE;
			}
			else
				rc = ftp_fetch_file(env, remotefile, &data);
			break;
			
		case OPIE_CONN_SCP:
			/* attempt an scp connection */
			osync_trace( TRACE_INTERNAL, "Attempting scp Connection." );
			if(objtype == OPIE_OBJECT_TYPE_NOTE) {
				/* FIXME support SCP for notes */
				osync_trace( TRACE_INTERNAL, "SCP not supported for notes." );
				rc = FALSE;
			}
			else {
				rc = scp_fetch_file(env, remotefile, &data);
			}
			break;
			
		default:
			/* unknown connection type */
			rc = FALSE;
			break;
	}

	if(rc && (objtype != OPIE_OBJECT_TYPE_NOTE))
	{
		if(env->backupdir && data) 
		{
			if(env->backuppath == NULL)
				env->backuppath = create_backup_dir(env->backupdir);
			
			if(env->backuppath) {
				/* Build full path to file */
				char *basename = g_path_get_basename(remotefile);
				char *backupfile = g_build_filename(env->backuppath, basename, NULL);
				/* Run the backup */
				rc = backup_file(backupfile, data->str, data->len);
				g_free(backupfile);
				g_free(basename);
			}
		}
		
		if(rc)
		{
			if(!data) {
				/* File didn't exist on the handheld (ie, clean device) */
				if(sink)
					osync_objtype_sink_set_slowsync(sink, TRUE);
				*doc = opie_xml_create_doc(objtype);
				if(*doc == 0)
					rc = FALSE;
			}
			else {
				*doc = opie_xml_string_read(data->str, data->len);
				/* Flag document as unchanged */
				(*doc)->_private = (void *)1;
			}
		}
	}
	
	if(data)
		g_string_free(data, TRUE);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}

/*
 * Fetch a file from disk (don't fail if file doesn't exist)
 */
gboolean local_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, data);
	
	char *basename = g_path_get_basename(remotefile);
	char *localfile = g_strdup_printf("/tmp/%s", basename);
	gboolean rc;
	
	if(g_access(localfile, F_OK) == 0) {
		OSyncError *error = NULL;
		int len = 0;
		char *str = NULL; 
		rc = osync_file_read(localfile, &str, &len, &error);
		*data = g_string_new_len(str, len);
		free(str);
	}
	else {
		*data = NULL;
		rc = TRUE;
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}

/*
 * Write a file to disk
 */
gboolean local_put_file(OpiePluginEnv* env, const char *remotefile, const char *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, data);
	
	char *basename = g_path_get_basename(remotefile);
	char *localfile = g_strdup_printf("/tmp/%s", basename);
	gboolean rc;
	
	OSyncError *error = NULL;
	rc = osync_file_write(localfile, data, strlen(data), 0660, &error);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}


/*
 * ftp_fetch_file
 */
gboolean ftp_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, data);
	
	gboolean rc = TRUE;
	char* ftpurl = NULL;
	CURL *curl = NULL;
	CURLcode res;

	if (env->url && env->username && env->password )
	{
		char* separator_path;
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				osync_trace(TRACE_EXIT_ERROR, "qcop_get_root: %s", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
		} else {
			separator_path = g_strdup( "/" );
		}

		/* fetch each of the requested files */
		ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->url,
		                          env->device_port,
		                          separator_path,
		                          remotefile);

		/* curl init */
		curl = curl_easy_init();
		
		*data = g_string_new("");
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, *data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_strwrite);
		
#ifdef _OPIE_PRINT_DEBUG
		curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif

		osync_trace( TRACE_INTERNAL, "Fetching %s", ftpurl );

		/* perform the transfer */
		res = curl_easy_perform(curl);

		if(res == CURLE_FTP_COULDNT_RETR_FILE || res == CURLE_FTP_ACCESS_DENIED)
		{
			/* This is not unlikely (eg. blank device). Note that Opie's FTP
				server returns "access denied" on non-existent directory. */
			osync_trace( TRACE_INTERNAL, "FTP file doesn't exist, ignoring" );
			/* Free and null the string to indicate the file wasn't there */
			g_string_free(*data, TRUE);
			*data = NULL;
		}
		else if(res != CURLE_OK) 
		{
			/* could not get the file */
			fprintf(stderr, "FTP download failed (error %d)\n", res);
			osync_trace(TRACE_EXIT_ERROR, "FTP download failed (error %d)", res);
			return FALSE;
		}
		else
		{
			osync_trace( TRACE_INTERNAL, "FTP ok" );
		}

		g_free(ftpurl);
		curl_easy_cleanup(curl);
		
		g_free(separator_path);
	}
	else
	{
		/* not enough data provided to do the connection */
		rc = FALSE; 
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc);
  return rc;
}

gboolean ftp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	
	gboolean rc = TRUE;
	char* ftpurl = NULL;
	CURL *curl = NULL;
	CURLcode res;
	int i;
	gchar **direntries;

	if (env->url && env->username && env->password )
	{
		char* separator_path;
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				osync_trace(TRACE_EXIT_ERROR, "qcop_get_root: %s", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
		} else {
			separator_path = g_strdup( "/" );
		}

		ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s",
			                         env->username,
			                         env->password,
			                         env->url,
			                         env->device_port,
			                         separator_path);
			
		/* curl init */
		curl = curl_easy_init();
		
		GString *bufstr = g_string_new("");
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, bufstr);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_strwrite);
		
		/* get the dir listing */
		res = curl_easy_perform(curl);

		/* Go through directory listing and find matching entries */
		GPatternSpec *pspec = g_pattern_spec_new("*.txt");
		direntries = g_strsplit(bufstr->str, "\n", 0);
		g_string_free(bufstr, TRUE);
		for(i=0; direntries[i] != NULL; i++) {
			if(strlen(direntries[i]) > 20) { /* fairly arbitrary, but works to skip "total XXX" line without worrying about i18n */
				if(direntries[i][0] == '-') { /* files only! */
					char *ptr = g_strrstr(direntries[i], " ");
					if(ptr) {
						ptr++;
						if(g_pattern_match_string(pspec, ptr)) {
							GString *bufstr = g_string_new("");
							char *ftpfileurl = g_strdup_printf("%s/%s", ftpurl, ptr);
							curl_easy_setopt(curl, CURLOPT_URL, ftpfileurl);
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, bufstr);
							res = curl_easy_perform(curl);
							g_free(ftpfileurl);
							/* Remove .txt from end of file name */
							int len = strlen(ptr);
							if(len > 4)
								ptr[len-4] = 0;
							opie_xml_add_note_node(doc, ptr, direntries[i], bufstr->str);
							g_string_free(bufstr, TRUE);
						}
					}
				}
			}
		}
		g_pattern_spec_free(pspec);
		g_strfreev(direntries);
		
		if(res == CURLE_FTP_COULDNT_RETR_FILE || res == CURLE_FTP_ACCESS_DENIED)
		{
			/* This is not unlikely (eg. blank device). Note that Opie's FTP
				server returns "access denied" on non-existent directory. */
		}
		else if(res != CURLE_OK) 
		{
			/* could not get the file */
			fprintf(stderr, "FTP download failed (error %d)\n", res);
			osync_trace( TRACE_INTERNAL, "FTP download failed (error %d)", res );
			rc = FALSE;
		}
		else
		{
			osync_trace( TRACE_INTERNAL, "FTP ok" );
		}

		g_free(ftpurl);
		curl_easy_cleanup(curl);
		g_free(separator_path);
	}
	else
	{
		/* not enough data provided to do the connection */
		rc = FALSE; 
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc);
  return rc;
}

gboolean ftp_put_notes(OpiePluginEnv* env, xmlDoc *doc)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	
	gboolean rc = TRUE;
	CURL *curl;
	CURLcode res;
	char* separator_path;
	char *ftpurl;
	
	if (env->url && env->username && env->password )
	{
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				osync_trace(TRACE_EXIT_ERROR, "qcop_get_root: %s", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
		} else {
			separator_path = g_strdup( "/" );
		}
		
		xmlNode *node = opie_xml_get_first(doc, "notes", "note");
		while(node) {
			char *changedflag = xmlGetProp(node, "changed");
			if(changedflag) {
				xmlFree(changedflag);
				
				char *notename = xmlGetProp(node, "name");
				char *content = xmlNodeGetContent(node);
				if(notename && content) {
					curl = curl_easy_init();
					
					char *deletedflag = xmlGetProp(node, "deleted");
					if(deletedflag) {
						/* This note has been marked deleted, so delete it from the device */
						xmlFree(deletedflag);
						ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s",
						                        env->username,
						                        env->password,
						                        env->url,
						                        env->device_port,
						                        separator_path);
						
						struct curl_slist *cmdlist = NULL;
						char *command = g_strdup_printf("DELE %s%s.txt", separator_path, notename);
						cmdlist = curl_slist_append(cmdlist, command);
						curl_easy_setopt(curl, CURLOPT_QUOTE, cmdlist);
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_nullwrite);
					}
					else {
						/* Changed note, upload it */
						ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s.txt",
						                        env->username,
						                        env->password,
						                        env->url,
						                        env->device_port,
						                        separator_path,
						                        notename);
						
						curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE);
						curl_easy_setopt(curl, CURLOPT_READDATA, content);
						curl_easy_setopt(curl, CURLOPT_READFUNCTION, opie_curl_strread);
						m_totalwritten = 0;
					}
					
					curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
#ifdef _OPIE_PRINT_DEBUG
					curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif
					res = curl_easy_perform(curl);
					if(res != CURLE_OK) 
					{
						fprintf(stderr, "FTP notes upload failed (error %d)\n", res);
						osync_trace( TRACE_INTERNAL, "FTP notes upload failed (error %d)", res );
						rc = FALSE;
					}
					else
					{
						osync_trace( TRACE_INTERNAL, "FTP notes upload ok" );
						rc = TRUE;
					}
					
					/* cleanup */
					curl_easy_cleanup(curl);
					
					g_free(ftpurl);
					xmlFree(notename);
					xmlFree(content);
					
					if(!rc)
						break;
				}
			}
			node = opie_xml_get_next(node);
		}
		
		g_free(separator_path);
	}
	else
	{
		/* not enough data provided to do the connection */
		rc = FALSE; 
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc);
	return rc;
}


/*
 * opie_put_sink
 */
gboolean opie_put_sink(OpieSinkEnv *env)
{
	return opie_put_file(env->plugin_env, env->objtype, env->remotefile, env->doc);
}

/*
 * opie_put_file
 */
gboolean opie_put_file(OpiePluginEnv *env, OPIE_OBJECT_TYPE objtype, const char *remotefile, xmlDoc *doc)
{
	osync_trace(TRACE_ENTRY, "%s", __func__ );
	
	gboolean rc = TRUE;

	if(doc && doc->_private == 0) {
		char *data = NULL;
		if(objtype != OPIE_OBJECT_TYPE_NOTE) {
			data = osxml_write_to_string(doc);
			if(!data)
				goto error;
		}

		/* check which connection method was requested */
		switch (env->conn_type)
		{
			case OPIE_CONN_NONE:
				/* Write to local file (for debugging) */
				osync_trace(TRACE_INTERNAL, "Writing local file" );
				if(objtype == OPIE_OBJECT_TYPE_NOTE) {
					/* FIXME enable notes debugging */
				}
				else 
					local_put_file(env, remotefile, data);
			
				break;
			
			case OPIE_CONN_FTP:
				/* attempt an FTP connection */
				osync_trace( TRACE_INTERNAL, "Attempting FTP Put File." );
				if(objtype == OPIE_OBJECT_TYPE_NOTE) {
					rc = ftp_put_notes(env, doc);
				}
				else {
					rc = ftp_put_file(env, remotefile, data);
				}
				break;
				
			case OPIE_CONN_SCP:
				/* attempt and scp connection */
				osync_trace( TRACE_INTERNAL, "Attempting scp Put File." );
				if(objtype == OPIE_OBJECT_TYPE_NOTE) {
					/* FIXME support SCP for notes */
					osync_trace( TRACE_INTERNAL, "SCP not supported for notes." );
					rc = FALSE;
				}
				else {
					rc = scp_put_file(env, remotefile, data);
				}
				break;
				
			default:
				/* unknown connection type */
				rc = FALSE;
				break;
		}
		
		if((!rc) && (env->conn_type != OPIE_CONN_NONE) && env->backupdir && (objtype != OPIE_OBJECT_TYPE_NOTE)) {
			/* If something went wrong and the user has a backup directory set,
				we write the files to their backups dir to avoid possible data loss */ 
			if(env->backuppath == NULL)
				env->backuppath = create_backup_dir(env->backupdir);
			
			if(env->backuppath) {
				/* Build full path to file */
				char *basename = g_path_get_basename(remotefile);
				char *backupfile = g_build_filename(env->backuppath, "upload_failures", basename, NULL);
				/* Run the backup */
				fprintf(stderr, "Error during upload to device, writing file to %s", backupfile); 
				osync_trace( TRACE_INTERNAL, "Error during upload to device, writing file to %s", backupfile );
				rc = backup_file(backupfile, data, strlen(data));
				g_free(backupfile);
				g_free(basename);
			}
		}
		
		if(data)
			free(data);
	}
	else {
		osync_trace(TRACE_INTERNAL, "No address/todo/calendar changes to write", __func__, rc );
	}
	
	osync_trace(TRACE_EXIT, "%s(%d)", __func__, rc );
	return rc;
	
error:
  return FALSE;
}


/*
 * ftp_put_file
 */
gboolean ftp_put_file(OpiePluginEnv* env, const char *remotefile, char *data) 
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, data);
	
	gboolean rc = TRUE;
	CURL *curl;
	CURLcode res;
	char* separator_path;
	
	if (env->url && env->username && env->password )
	{
		if ( env->use_qcop ) 
		{
			char* root_path = qcop_get_root(env->qcopconn);
			if(!root_path) {
				fprintf(stderr, "qcop_get_root: %s\n", env->qcopconn->resultmsg);
				osync_trace(TRACE_EXIT_ERROR, "qcop_get_root: %s", env->qcopconn->resultmsg);
				return FALSE;
			}
			osync_trace( TRACE_INTERNAL, "QCop root path = %s", root_path );
			separator_path = g_strdup_printf("%s/", root_path);
			g_free(root_path);
		} else {
			separator_path = g_strdup( "/" );
		}
		
		char *ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->url,
		                          env->device_port,
		                          separator_path,
		                          remotefile);

		curl = curl_easy_init();
		
		curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE) ;
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_READDATA, data);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, opie_curl_strread);
		
		
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
		
#ifdef _OPIE_PRINT_DEBUG
		curl_easy_setopt(curl, CURLOPT_VERBOSE, TRUE);
#endif
				
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK) 
		{
			fprintf(stderr, "FTP upload failed (error %d)\n", res);
			osync_trace( TRACE_INTERNAL, "FTP upload failed (error %d)", res );
			rc = FALSE;
		} 
		else
		{
			osync_trace( TRACE_INTERNAL, "FTP upload ok" );
			rc = TRUE;
		}
		
		/* cleanup */
		curl_easy_cleanup(curl);
		
		g_free(ftpurl);
		
		g_free(separator_path);
	}
	else
	{
		/* not enough data provided to do the connection */
		rc = FALSE;
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}


/*
 * scp_fetch_file
 */
gboolean scp_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval, scpexitstatus;
	TempFile *tmpfile = NULL;
	
	if(env->url && env->device_port && env->username) {
		/* We have to close the temp file, because we want 
				scp to be able to write to it */
		tmpfile = create_temp_file();
		close(tmpfile->fd);
		tmpfile->fd = -1;
		
		/* A crude test to see if the file exists. You can't combine 
		  this with scp unfortunately because scp seems to return 1 
		  on any error */
		scpcommand = g_strdup_printf("ssh -o BatchMode=yes %s@%s \"ls %s > /dev/null\"",
																env->username,
																env->url,
																remotefile);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if(scpexitstatus != 1) {
			if((scpretval == -1) || (scpexitstatus != 0)) {
				rc = FALSE;
				osync_trace( TRACE_INTERNAL, "ssh login failed" );
				goto error;
			}
			g_free(scpcommand);
			
			/* Fetch the file */
			scpcommand = g_strdup_printf("scp -q -B %s@%s:%s %s",
																	env->username,
																	env->url,
																	remotefile,
																	tmpfile->filename);
			
			scpretval = pclose(popen(scpcommand,"w"));
			scpexitstatus = WEXITSTATUS(scpretval);
			
			if((scpretval == -1) || (scpexitstatus != 0)) {
				osync_trace( TRACE_INTERNAL, "scp transfer failed" );
				rc = FALSE;
				goto error;
			}
			else {
				osync_trace( TRACE_INTERNAL, "scp ok" );
			}
			
			/* read the temp file */ 
			OSyncError *error = NULL;
			int len = 0;
			char *str = NULL; 
			rc = osync_file_read(tmpfile->filename, &str, &len, &error);
			*data = g_string_new_len(str, len);
			free(str);
		}
	}
	
error:	
	
	if(tmpfile)
		cleanup_temp_file(tmpfile);
	if(scpcommand);
		g_free(scpcommand);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}


/*
 * scp_put_file
 */
gboolean scp_put_file(OpiePluginEnv* env, const char *remotefile, char *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval = 0;
	int scpexitstatus = 0;

	TempFile *tmpfile = create_temp_file();
	if(!tmpfile) {
		/* could not create temp file - error message has already been sent to trace */
		rc = FALSE;
	}
	else {
		int bufsize = 1024;
		int wbytes;
		int pos = 0;
		int len = strlen(data);
		
		while(TRUE) {
			if(len - pos < bufsize)
				bufsize = len - pos;
			
			wbytes = write(tmpfile->fd, data + pos, bufsize);
			if(wbytes == -1) {
				osync_trace( TRACE_INTERNAL, "error writing to backup file" );
				perror("error writing to backup file");
				rc = FALSE;
				goto error;
			}
			
			pos += wbytes;
			if(pos == len)  {
				/* finished */
				break;
			}
		}
		
		/* We have to close now */
		close(tmpfile->fd);
		tmpfile->fd = -1;
		
		/* create path */
		char *dirname = g_path_get_dirname(remotefile);
		scpcommand = g_strdup_printf("ssh -o BatchMode=yes %s@%s \"mkdir -p %s\"",
																env->username,
																env->url,
																dirname);
		g_free(dirname);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if((scpretval == -1) || (scpexitstatus != 0)) {
			rc = FALSE;
			osync_trace( TRACE_INTERNAL, "ssh create path failed" );
		}
		g_free(scpcommand);
		
		/* transfer the file */ 
		scpcommand = g_strdup_printf("scp -q -B %s %s@%s:%s",
																tmpfile->filename,
																env->username,
																env->url,
																remotefile);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);

		if((scpretval == -1) || (scpexitstatus != 0)) {
			rc = FALSE;
			osync_trace( TRACE_INTERNAL, "scp upload failed" );
		}
		else {
			rc = TRUE;
			osync_trace( TRACE_INTERNAL, "scp upload ok" );
		}

		g_free(scpcommand);
	}
	
error:

	if(tmpfile)
		cleanup_temp_file(tmpfile);
	
	osync_trace(TRACE_EXIT, "%s(%d)", __func__, rc );
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
 * opie_curl_strwrite
 */
int opie_curl_strwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	g_string_append_len((GString *)stream, buffer, size * nmemb);
	return size * nmemb;
}

/*
 * opie_curl_nullwrite
 */
int opie_curl_nullwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	return size * nmemb;
}

/*
 * opie_curl_strread
 */
int opie_curl_strread(void *buffer, size_t size, size_t nmemb, void *stream)
{
	char *str = ((char *)stream) + m_totalwritten;
	if(str[0] == '\0')
		return 0;
	
	int numbytes = strlen(str);
	if(numbytes >= (nmemb * size))
		numbytes = (nmemb * size);
	memcpy(buffer, str, numbytes);
	
	m_totalwritten += numbytes;
	return numbytes;
}
