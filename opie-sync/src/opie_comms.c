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
gboolean ftp_put_notes(OpiePluginEnv* env, xmlDoc *doc);
gboolean local_fetch_notes(OpiePluginEnv* env, xmlDoc *doc, const char *tempsourcepath);
gboolean local_put_notes(OpiePluginEnv* env, xmlDoc *doc, const char *tempdestpath, gboolean delete_files);
gboolean scp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc);
gboolean scp_put_notes(OpiePluginEnv* env, xmlDoc *doc);

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
	if(g_unlink(tempfile->filename)) {
		osync_trace(TRACE_INTERNAL, "%s: failed to delete temp file %s: %s", __func__, tempfile->filename, strerror(errno));
	}
	g_free(tempfile->filename);
	g_free(tempfile);
}


gboolean delete_directory(const char *path) {
	GError *gerror = NULL;
	GDir *dir = g_dir_open(path, 0, &gerror);
	if(!dir) {
		osync_trace(TRACE_EXIT_ERROR, "%s: failed to open local directory %s: %s", __func__, path, gerror->message);
		return FALSE;
	}
	else {
		while(TRUE) {
			const char *localfile = g_dir_read_name(dir);
			if(!localfile)
				break;
			char *localpath = g_build_filename(path, localfile, NULL);
		
			if(g_unlink(localpath)) {
				osync_trace(TRACE_EXIT_ERROR, "error deleting temp file %s: %s", localpath, strerror(errno));
				g_free(localpath);
				g_dir_close(dir);
				return FALSE;
			}
			g_free(localpath);
		}
		g_dir_close(dir);
		if(g_rmdir(path)) {
			osync_trace(TRACE_EXIT_ERROR, "error deleting temp directory %s: %s", path, strerror(errno));
			return FALSE;
		}
	}
	
	return TRUE;
}


/*
 * create a backup file from a string
 */
gboolean backup_file(const char *backupfile, const char *str, int len) {
	osync_trace(TRACE_ENTRY, "%s(%s, %p, %i)", __func__, backupfile, str, len);
	
	int destfd = 0;
	int bufsize = 1024;
	int wbytes;
	int pos = 0;
	char *errmsg = NULL;
	
	destfd = open(backupfile, O_CREAT | O_WRONLY | O_EXCL, 0600);
	if(destfd == -1) {
		errmsg = g_strdup_printf("error creating backup file: %s", strerror(errno));
		goto error;
	}
	
	while(TRUE) {
		if(len - pos < bufsize)
			bufsize = len - pos;
		
		wbytes = write(destfd, str + pos, bufsize);
		if(wbytes == -1) {
			errmsg = g_strdup_printf("error writing to backup file: %s", strerror(errno));
			goto error;
		}
		
		pos += wbytes;
		if(pos == len)  {
			/* finished */
			close(destfd);
			break;
		}
	}
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, TRUE);
	return TRUE;
	
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, errmsg);
	g_free(errmsg);
	return FALSE;
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
		osync_trace(TRACE_EXIT_ERROR, "error creating backup directory: %s", strerror(errno));
		goto error;
	}
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return backuppath;

error:
	g_free(datestamp);
	g_free(backuppath);
	
	return NULL;
}


char *get_remote_notes_path(OpiePluginEnv *env)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

	char *notes_path;
	if(env->notes_type == NOTES_TYPE_OPIE_NOTES)
		notes_path = g_build_filename(env->dev_root_path, "Documents/text/plain", NULL);
	else
		notes_path = g_strdup(env->dev_root_path);
	
	osync_trace(TRACE_EXIT, "%s(%s)", __func__, notes_path);
	return notes_path;
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
				*doc = opie_xml_create_notes_doc();
				rc = local_fetch_notes(env, *doc, NULL);
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
				rc = scp_fetch_notes(env, *doc);
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
			else
				rc = FALSE;
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
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	char *basename = g_path_get_basename(remotefile);
	char *localfile = g_build_filename(env->localdir, basename, NULL);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	char *basename = g_path_get_basename(remotefile);
	char *localfile = g_build_filename(env->localdir, basename, NULL);
	g_free(basename);
	gboolean rc;
	
	OSyncError *error = NULL;
	rc = osync_file_write(localfile, data, strlen(data), 0660, &error);
	g_free(localfile);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}

/*
 * Fetch notes from disk (from the notes subdirectory of localdir)
 */
gboolean local_fetch_notes(OpiePluginEnv* env, xmlDoc *doc, const char *tempsourcepath)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s)", __func__, env, doc, tempsourcepath);
	gboolean rc = TRUE;
	char *notesdir = NULL;
	
	if(tempsourcepath) {
		notesdir = g_strdup(tempsourcepath);
	}
	else {
		notesdir = g_build_filename(env->localdir, "notes", NULL);
		if(g_mkdir_with_parents(notesdir, 0700)) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to create path %s: %s", __func__, notesdir, strerror(errno));
			g_free(notesdir);
			return FALSE;
		}
	}
	
	GError *gerror = NULL;
	GDir *dir = g_dir_open(notesdir, 0, &gerror);
	if(!dir) {
		osync_trace(TRACE_EXIT_ERROR, "%s: failed to open local directory %s: %s", __func__, notesdir, gerror->message);
		g_free(notesdir);
		return FALSE;
	}
	else {
		GPatternSpec *pspec = g_pattern_spec_new("*.txt");
		while(TRUE) {
			const char *localfile = g_dir_read_name(dir);
			if(!localfile)
				break;
			
			if(g_pattern_match_string(pspec, localfile)) {
				char *localpath = g_build_filename(notesdir, localfile, NULL);
				int len = 0;
				char *str = NULL;
				OSyncError *error = NULL;
				rc = osync_file_read(localpath, &str, &len, &error);
				g_free(localpath);
				if(rc) {
					/* FIXME this is probably not correct for calling opie_xml_add_note_node() */
					char *filename = g_strdup(localfile);
					int namelen = strlen(filename);
					if(namelen > 4)
						filename[namelen-4] = 0;
					
					opie_xml_add_note_node(doc, filename, filename, str);
					g_free(filename);
					g_free(str);
				}
				else {
					osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
					g_dir_close(dir);
					g_free(notesdir);
					g_pattern_spec_free(pspec);
					return FALSE;
				}
			}
		}
		g_pattern_spec_free(pspec);
	}
	g_dir_close(dir);
	
  g_free(notesdir);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}

/*
 * Write note changes to disk (into the notes subdirectory of localdir)
 */
gboolean local_put_notes(OpiePluginEnv* env, xmlDoc *doc, const char *tempdestpath, gboolean delete_files)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %i)", __func__, env, doc, tempdestpath, delete_files);
	gboolean rc = TRUE;
	char *notesdir = NULL;
	
	if(tempdestpath) {
		notesdir = g_strdup(tempdestpath);
	}
	else {
		notesdir = g_build_filename(env->localdir, "notes", NULL);
		if(g_mkdir_with_parents(notesdir, 0700)) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to create local path %s: %s", __func__, notesdir, strerror(errno));
			g_free(notesdir);
			return FALSE;
		}
	}
	
	xmlNode *node = opie_xml_get_first(doc, "notes", "note");
	while(node) {
		char *changedflag = xmlGetProp(node, "changed");
		if(changedflag) {
			xmlFree(changedflag);
			
			char *notename = xmlGetProp(node, "name");
			if(notename) {
				char *notefile = g_strdup_printf("%s.txt", notename);
				char *notepath = g_build_filename(notesdir, notefile, NULL);
				g_free(notefile);
				
				char *deletedflag = xmlGetProp(node, "deleted");
				if(deletedflag) {
					/* This note has been marked deleted, so delete it */
					xmlFree(deletedflag);
					
					if(delete_files) {
						if(g_unlink(notepath)) {
							osync_trace(TRACE_EXIT_ERROR, "%s: failed to create local path %s: %s", __func__, notesdir, strerror(errno));
							g_free(notesdir);
							xmlFree(notename);
							return FALSE;
						}
					}
				}
				else {
					/* Changed note, write it */
					char *content = xmlNodeGetContent(node);
					if(content) {
						OSyncError *error = NULL;
						osync_bool rc = osync_file_write(notepath, content, strlen(content), 0660, &error);
						xmlFree(content);
						if(!rc) {
							osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
							g_free(notepath);
							g_free(notesdir);
							xmlFree(notename);
							return FALSE;
						}
					}
				}
				
				g_free(notepath);
				xmlFree(notename);
			}
		}
		node = opie_xml_get_next(node);
	}
	
  g_free(notesdir);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return TRUE;
}


/*
 * ftp_fetch_file
 */
gboolean ftp_fetch_file(OpiePluginEnv* env, const char *remotefile, GString **data)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	gboolean rc = TRUE;
	char* ftpurl = NULL;
	CURL *curl = NULL;
	CURLcode res;

	if (env->host && env->username && env->password )
	{
		char* separator_path = g_strdup_printf("%s/", env->dev_root_path);

		/* fetch each of the requested files */
		ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->host,
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

	if (env->host && env->username && env->password )
	{
		char *remotepath = get_remote_notes_path(env);
		if(!remotepath) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to get remote notes path", __func__);
			return FALSE;
		}

		ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s/",
			                         env->username,
			                         env->password,
			                         env->host,
			                         env->device_port,
			                         remotepath);
			
		/* curl init */
		curl = curl_easy_init();
		
		GString *bufstr = g_string_new("");
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, bufstr);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_strwrite);
		
		osync_trace(TRACE_INTERNAL, "retrieving directory: %s", ftpurl);
		
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
							osync_trace(TRACE_INTERNAL, "retrieving file: %s", ftpfileurl);
							curl_easy_setopt(curl, CURLOPT_URL, ftpfileurl);
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, bufstr);
							res = curl_easy_perform(curl);
							osync_trace(TRACE_INTERNAL, "done retrieving, result = %i", res);
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
		g_free(remotepath);
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
	char *ftpurl;
	
	if (env->host && env->username && env->password )
	{
		char *remotepath = get_remote_notes_path(env);
		if(!remotepath) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to get remote notes path", __func__);
			return FALSE;
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
						                        env->host,
						                        env->device_port,
						                        remotepath);
						
						struct curl_slist *cmdlist = NULL;
						char *command = g_strdup_printf("DELE %s/%s.txt", remotepath, notename);
						cmdlist = curl_slist_append(cmdlist, command);
						curl_easy_setopt(curl, CURLOPT_QUOTE, cmdlist);
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, opie_curl_nullwrite);
					}
					else {
						/* Changed note, upload it */
						ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s/%s.txt",
						                        env->username,
						                        env->password,
						                        env->host,
						                        env->device_port,
						                        remotepath,
						                        notename);
						
						curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE);
						curl_easy_setopt(curl, CURLOPT_READDATA, content);
						curl_easy_setopt(curl, CURLOPT_READFUNCTION, opie_curl_strread);
						curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
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
		
		g_free(remotepath);
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
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %s, %p)", __func__, env, objtype, remotefile, doc);
	
	gboolean rc = TRUE;

	if(doc && doc->_private == 0) {
		char *data = NULL;
		if(objtype != OPIE_OBJECT_TYPE_NOTE) {
			xmlDocDumpFormatMemoryEnc(doc, (xmlChar **) &data, NULL, NULL, 1);
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
					rc = local_put_notes(env, doc, NULL, TRUE);
				}
				else 
					rc = local_put_file(env, remotefile, data);
			
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
					rc = scp_put_notes(env, doc);
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
		osync_trace(TRACE_INTERNAL, "No address/todo/calendar changes to write");
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
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, remotefile, data);
	
	gboolean rc = TRUE;
	CURL *curl;
	CURLcode res;
	char* separator_path;
	
	if (env->host && env->username && env->password )
	{
		char* separator_path = g_strdup_printf("%s/", env->dev_root_path);
		
		char *ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->host,
		                          env->device_port,
		                          separator_path,
		                          remotefile);

		curl = curl_easy_init();
		
		curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE) ;
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_READDATA, data);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, opie_curl_strread);
		m_totalwritten = 0;
		
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
	
	if(env->host && env->device_port && env->username) {
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
																env->host,
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
																	env->host,
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
	
	char* scpcommand = NULL;
	int scpretval = 0;
	int scpexitstatus = 0;
	char *errmsg = NULL;

	TempFile *tmpfile = create_temp_file();
	if(!tmpfile) {
		/* could not create temp file */
		errmsg = g_strdup("failed to create temp file"); 
		goto error;
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
				errmsg = g_strdup_printf("error writing to temp file: %s", strerror(errno));
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
																env->host,
																dirname);
		g_free(dirname);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if((scpretval == -1) || (scpexitstatus != 0)) {
			errmsg = g_strdup("ssh create path failed");
			goto error;
		}
		g_free(scpcommand);
		
		/* transfer the file */ 
		scpcommand = g_strdup_printf("scp -q -B %s %s@%s:%s",
																tmpfile->filename,
																env->username,
																env->host,
																remotefile);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);

		if((scpretval == -1) || (scpexitstatus != 0)) {
			errmsg = g_strdup("scp upload failed");
			goto error;
		}
		else {
			osync_trace( TRACE_INTERNAL, "scp upload ok" );
		}

		g_free(scpcommand);
	}
	
	if(tmpfile)
		cleanup_temp_file(tmpfile);
	
	osync_trace(TRACE_EXIT, "%s(%d)", __func__, TRUE );
	return TRUE;
	
error:

	if(tmpfile)
		cleanup_temp_file(tmpfile);
	if(scpcommand)
		g_free(scpcommand);
	
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, errmsg );
	return FALSE;
}



/*
 * scp_fetch_notes
 */
gboolean scp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval, scpexitstatus;
	char *remotepath = NULL;
	
	if(env->host && env->device_port && env->username) {
		remotepath = get_remote_notes_path(env);
		if(!remotepath) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to get remote notes path", __func__);
			return FALSE;
		}
		
		/* Create a temp directory */
		char *randstr = g_strdup_printf("opie-sync-%i", g_random_int_range(0, 2000000000));
		char *temppath = g_build_filename(g_get_tmp_dir(), randstr, NULL);
		g_free(randstr);
		
		if(g_mkdir(temppath, 0700)) {
			/* Create failed */
			osync_trace(TRACE_INTERNAL, "failed to create temp dir %s: %s", temppath, strerror(errno));
			g_free(temppath);
			goto error;
		}
		
		/* A crude test to see if files exist. You can't combine 
		  this with scp unfortunately because scp seems to return 1 
		  on any error */
		scpcommand = g_strdup_printf("ssh -o BatchMode=yes %s@%s \"ls %s/*.txt > /dev/null\"",
																env->username,
																env->host,
																remotepath);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if(scpexitstatus != 1) {
			if((scpretval == -1) || (scpexitstatus != 0)) {
				rc = FALSE;
				osync_trace( TRACE_INTERNAL, "ssh login failed" );
				goto error;
			}
			g_free(scpcommand);
			
			/* Fetch all text files from the remote path into the temp directory */
			scpcommand = g_strdup_printf("scp -p -q -B %s@%s:%s/*.txt %s",
																	env->username,
																	env->host,
																	remotepath,
																	temppath);
			
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
			
			/* Now fetch the docs from the local temp dir */
			rc = local_fetch_notes(env, doc, temppath);
		}
		
		/* Delete temp dir (regardless of whether preceding code succeeded) */
		if(!delete_directory(temppath))
			rc = FALSE;
		g_free(temppath);
	}
	
error:	
	
	if(scpcommand);
		g_free(scpcommand);
		
	if(remotepath)
		g_free(remotepath);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
	return rc;
}

/*
 * scp_put_notes
 */
gboolean scp_put_notes(OpiePluginEnv* env, xmlDoc *doc)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, env, doc);
	
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval, scpexitstatus;
	char *temppath = NULL;
	char *remotepath = NULL;
	
	if(env->host && env->device_port && env->username) {
		remotepath = get_remote_notes_path(env);
		if(!remotepath) {
			osync_trace(TRACE_EXIT_ERROR, "%s: failed to get remote notes path", __func__);
			return FALSE;
		}
		
		/* Create a temp directory */
		char *randstr = g_strdup_printf("opie-sync-%i", g_random_int_range(0, G_MAXUINT32));
		temppath = g_build_filename(g_get_tmp_dir(), randstr, NULL);
		g_free(randstr);
		
		if(g_mkdir(temppath, 0700)) {
			/* Create failed */
			osync_trace(TRACE_INTERNAL, "failed to create temp dir %s: %s", temppath, strerror(errno));
			goto error;
		}
		
		/* Write notes to temp dir */
		rc = local_put_notes(env, doc, temppath, FALSE);
		if(!rc) {
			osync_trace(TRACE_INTERNAL, "failed to write notes to temp dir");
			goto error;
		}
		
		/* create remote path */
		scpcommand = g_strdup_printf("ssh -o BatchMode=yes %s@%s \"mkdir -p %s/\"",
																env->username,
																env->host,
																remotepath);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if((scpretval == -1) || (scpexitstatus != 0)) {
			osync_trace( TRACE_INTERNAL, "failed to create remote path" );
			rc = FALSE;
			goto error;
		}
		g_free(scpcommand);
		
		/* Copy all text files from the temp path into the remote path */
		scpcommand = g_strdup_printf("scp -q -B %s/* %s@%s:%s",
																temppath,
																env->username,
																env->host,
																remotepath);
		
		scpretval = pclose(popen(scpcommand,"w"));
		scpexitstatus = WEXITSTATUS(scpretval);
		
		if((scpretval == -1) || (scpexitstatus != 0)) {
			osync_trace( TRACE_INTERNAL, "scp transfer failed" );
			rc = FALSE;
			goto error;
		}
		else {
			osync_trace( TRACE_INTERNAL, "scp transfer ok" );
		}
		
		/* Delete files that have been deleted */
	
		GString *deletedfiles = g_string_new("");
		xmlNode *node = opie_xml_get_first(doc, "notes", "note");
		while(node) {
			char *deletedflag = xmlGetProp(node, "deleted");
			if(deletedflag) {
				/* This note has been marked deleted, so delete it */
				xmlFree(deletedflag);
				
				char *notename = xmlGetProp(node, "name");
				if(notename) {
					g_string_append_printf(deletedfiles, "%s.txt ", notename);
					xmlFree(notename);
				}
			}
			node = opie_xml_get_next(node);
		}
		
		if(deletedfiles->len > 0) {
			g_free(scpcommand);
			scpcommand = g_strdup_printf("ssh -o BatchMode=yes %s@%s \"cd %s/ && rm -f %s\"",
																	env->username,
																	env->host,
																	remotepath,
																	deletedfiles->str);
			
			scpretval = pclose(popen(scpcommand,"w"));
			scpexitstatus = WEXITSTATUS(scpretval);
			
			if((scpretval == -1) || (scpexitstatus != 0)) {
				osync_trace( TRACE_INTERNAL, "ssh delete note files failed" );
				rc = FALSE;
				goto error;
			}
		}
		
		g_string_free(deletedfiles, TRUE);
	}
	
error:
	
	if(temppath) {
		/* Delete temp dir (regardless of whether preceding code succeeded) */
		if(!delete_directory(temppath))
			rc = FALSE;
		g_free(temppath);
	}
	
	if(remotepath)
		g_free(remotepath);
	
	if(scpcommand);
		g_free(scpcommand);
	
	osync_trace(TRACE_EXIT, "%s(%i)", __func__, rc );
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
