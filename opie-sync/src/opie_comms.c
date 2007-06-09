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

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <openssl/md5.h>

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>

#include "opie_debug.h"
#include "opie_comms.h"
#include "opie_xml.h"
#include "opie_qcop.h"

typedef struct {
	char *remote_filename;
	char *local_filename; /* use fd instead where possible */
	int local_fd;
} RemoteData;

enum temp_file_type {
	TT_STANDARD = 1,
	TT_VISIBLE = 2,
	TT_DEBUG = 3,
	TT_DEBUG_CREATE = 4
};

int opie_curl_fwrite(void* buffer, size_t size, size_t nmemb, void* stream);
int opie_curl_strwrite(void *buffer, size_t size, size_t nmemb, void *stream);
int opie_curl_nullwrite(void *buffer, size_t size, size_t nmemb, void *stream);
int opie_curl_strread(void *buffer, size_t size, size_t nmemb, void *stream);
gboolean ftp_fetch_file(OpiePluginEnv* env, RemoteData *data);
gboolean scp_fetch_file(OpiePluginEnv* env, RemoteData *data);
gboolean ftp_put_file(OpiePluginEnv* env, RemoteData *data);
gboolean scp_put_file(OpiePluginEnv* env, RemoteData *data);
gboolean ftp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc);

int m_totalwritten;


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
 * Set up a temporary file
 */
RemoteData *create_temp_file(const char *remote_file, int tmpfilemode) {
	RemoteData *pair = g_malloc(sizeof(RemoteData));
	pair->remote_filename = g_strdup(remote_file);
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
			return NULL;
		}
		pair->local_filename = template;
		if(tmpfilemode != TT_VISIBLE) {
			if(unlink(template) == -1) {
				osync_trace( TRACE_INTERNAL, "failed to unlink temporary file" );
			}
		}
	}
	
	return pair;
}


/*
 * Clean up and free a RemoteData temp file as built by create_temp_file 
 */
void cleanup_temp_file(RemoteData *data, int tmpfilemode) {
	if(tmpfilemode == TT_VISIBLE) {
		if(unlink(data->local_filename) == -1) {
			osync_trace( TRACE_INTERNAL, "failed to unlink temporary file" );
		}
	}
	if(data->local_fd > 0)
		close(data->local_fd);
	
	g_free(data->local_filename);
	g_free(data);
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
 * Create a date/time stamped directory for backup files
 */
char *create_backup_dir(const char *backupdir)
{
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
		goto error;
	}
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
	gboolean rc = TRUE;
	int tmpfilemode;
	
	if(env->conn_type == OPIE_CONN_NONE) {
		tmpfilemode = TT_DEBUG;
	}
	else if (env->conn_type == OPIE_CONN_SCP) {
		tmpfilemode = TT_VISIBLE;
	}
	else {
		tmpfilemode = TT_STANDARD;
	}
	
	RemoteData *data = NULL;
	if(objtype != OPIE_OBJECT_TYPE_NOTE)
		data = create_temp_file(remotefile, tmpfilemode);
	
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
			if(objtype == OPIE_OBJECT_TYPE_NOTE) {
				*doc = opie_xml_create_notes_doc();
				if(*doc)
					rc = ftp_fetch_notes(env, *doc);
				else
					rc = FALSE;
			}
			else
				rc = ftp_fetch_file(env, data);
			break;
			
		case OPIE_CONN_SCP:
			/* attempt an scp connection */
			OPIE_DEBUG("Attempting scp Connection.\n");
			if(objtype == OPIE_OBJECT_TYPE_NOTE) {
				/* FIXME support SCP for notes */
				OPIE_DEBUG("SCP not supported for notes.\n");
				rc = FALSE;
			}
			else
				rc = scp_fetch_file(env, data);
			break;
			
		default:
			/* unknown connection type */
			rc = FALSE;
			break;
	}

	if(rc && (objtype != OPIE_OBJECT_TYPE_NOTE))
	{
		if(env->backupdir) 
		{
			if(env->backuppath == NULL)
				env->backuppath = create_backup_dir(env->backupdir);
			
			if(env->backuppath) {
				/* Build full path to file */
				char *basename = g_path_get_basename(data->remote_filename);
				char *backupfile = g_build_filename(env->backuppath, basename, NULL);
				/* Run the backup */
				rc = backup_file(backupfile, data->local_fd);
				g_free(backupfile);
				g_free(basename);
			}
		}
		
		if(rc)
		{
			if(data->local_fd <= 0) {
				/* File didn't exist on the handheld (ie, clean device) */
				if(sink)
					osync_objtype_sink_set_slowsync(sink, TRUE);
				*doc = opie_xml_create_doc(objtype);
				if(*doc == 0)
					rc = FALSE;
			}
			else {
				*doc = opie_xml_fd_open(data->local_fd);
				/* Flag document as unchanged */
				(*doc)->_private = (void *)1;
				close(data->local_fd);
				data->local_fd = -1;
			}
		}
	}
	
	if(data)
		cleanup_temp_file(data, tmpfilemode);
	
	return rc;
}


/*
 * ftp_fetch_file
 */
gboolean ftp_fetch_file(OpiePluginEnv* env, RemoteData *data)
{
	gboolean rc = TRUE;
	char* ftpurl = NULL;
	CURL *curl = NULL;
	CURLcode res;
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
		ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->url,
		                          env->device_port,
		                          separator_path,
		                          data->remote_filename);
		
		fd = fdopen(data->local_fd, "w+"); 
		if(!fd)
		{
			OPIE_DEBUG("Failed to open temporary file\n");
			g_free(ftpurl);
			return FALSE;
		}

		/* curl init */
		curl = curl_easy_init();
		
		curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fd);
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
			data->local_fd = -1;
		}
		else if(res != CURLE_OK) 
		{
			/* could not get the file */
			fprintf(stderr, "FTP download failed (error %d)\n", res);
			return FALSE;
		}
		else
		{
			OPIE_DEBUG("FTP ok\n");
		}

		fflush(fd);
		if(data->local_fd > 0) {
			free(fd);   /* don't fclose, we still need it */
			lseek(data->local_fd, 0, SEEK_SET);
		}
		else {
			fclose(fd);
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
	
  return rc; 
}

gboolean ftp_fetch_notes(OpiePluginEnv* env, xmlDoc *doc)
{
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
			rc = FALSE;
		}
		else
		{
			OPIE_DEBUG("FTP ok\n");
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
	
  return rc; 
}

gboolean ftp_put_notes(OpiePluginEnv* env, xmlDoc *doc)
{
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
						rc = FALSE;
					}
					else
					{
						OPIE_DEBUG("FTP notes upload ok\n");
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
	int tmpfilemode;

	if(doc && doc->_private == 0) {
		if(env->conn_type == OPIE_CONN_NONE) {
			tmpfilemode = TT_DEBUG_CREATE;
		}
		else if (env->conn_type == OPIE_CONN_SCP) {
			tmpfilemode = TT_VISIBLE;
		}
		else {
			tmpfilemode = TT_STANDARD;
		}
	
		RemoteData *data = NULL;
		if(objtype != OPIE_OBJECT_TYPE_NOTE) {
			data = create_temp_file(remotefile, tmpfilemode);
			if(opie_xml_save_to_fd(doc, data->local_fd) == -1) {
				osync_trace(TRACE_EXIT_ERROR, "failed to write data to temporary file"); /* FIXME actually say what data we failed to write */
				goto error;
			}
			fsync(data->local_fd);
			lseek(data->local_fd, 0, SEEK_SET);
		}

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
				if(objtype == OPIE_OBJECT_TYPE_NOTE) {
					rc = ftp_put_notes(env, doc);
				}
				else {
					rc = ftp_put_file(env, data);
				}
				break;
				
			case OPIE_CONN_SCP:
				/* attempt and scp connection */
				OPIE_DEBUG("Attempting scp Put File.\n");
				if(objtype == OPIE_OBJECT_TYPE_NOTE) {
					/* FIXME support SCP for notes */
					OPIE_DEBUG("SCP not supported for notes.\n");
					rc = FALSE;
				}
				else
					rc = scp_put_file(env, data);
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
				char *basename = g_path_get_basename(data->remote_filename);
				char *backupfile = g_build_filename(env->backuppath, "upload_failures", basename, NULL);
				/* Run the backup */
				fprintf(stderr, "Error during upload to device, writing file to %s", backupfile); 
				rc = backup_file(backupfile, data->local_fd);
				g_free(backupfile);
				g_free(basename);
			}
		}
		
		if(data)
			cleanup_temp_file(data, tmpfilemode);
	}
	else {
		OPIE_DEBUG("OPIE: No address/todo/calendar changes to write\n");
	}
	
	osync_trace(TRACE_EXIT, "%s(%d)", __func__, rc );
	return rc;
	
error:
  return FALSE;
}


/*
 * ftp_put_file
 */
gboolean ftp_put_file(OpiePluginEnv* env, RemoteData *data) 
{
	gboolean rc = TRUE;
	struct stat file_info;
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	char* separator_path;
	
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
		
		char *ftpurl = g_strdup_printf("ftp://%s:%s@%s:%u%s%s",
		                          env->username,
		                          env->password,
		                          env->url,
		                          env->device_port,
		                          separator_path,
		                          data->remote_filename);

		/* get the file size of the local file */
		fstat(data->local_fd, &file_info);
	
		hd_src = fdopen(data->local_fd, "rb+");
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
 * scp_fetch_file
 */
gboolean scp_fetch_file(OpiePluginEnv* env, RemoteData *data)
{
	gboolean rc = TRUE;
	char* scpcommand = NULL;

	int scpretval;
	
	if(env->url && env->device_port && env->username) 
	{
		/* fetch each of the requested files */
		/* We have to close the temp file, because we want 
				sftp to be able to write to it */
		close(data->local_fd);
		
		/* not keeping it quiet for the moment */
		scpcommand = g_strdup_printf("sftp -o Port=%d -o BatchMode=yes %s@%s:%s %s",
		                            env->device_port,
		                            env->username,
		                            env->url,
		                            data->remote_filename,
		                            data->local_filename);
		
		scpretval = pclose(popen(scpcommand,"w"));
		
		if((scpretval == -1) || (WEXITSTATUS(scpretval) != 0))
		{
			OPIE_DEBUG("SFTP failed\n");
			rc = FALSE;
		}
		else 
		{
			OPIE_DEBUG("SFTP ok\n");
		}
		
		g_free(scpcommand);
		/* reopen the temp file */ 
		data->local_fd = open(data->local_filename, O_RDWR | O_EXCL);
	}  
	return rc;
}


/*
 * scp_put_file
 */
gboolean scp_put_file(OpiePluginEnv* env, RemoteData *data)
{
	gboolean rc = TRUE;
	char* scpcommand = NULL;
	int scpretval = 0;
	char batchfile[] = "/tmp/opie_syncXXXXXX";
	int batchfd = 0;

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
		g_string_append_printf(batchbuf, "put %s %s\n", data->local_filename, data->remote_filename);
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
