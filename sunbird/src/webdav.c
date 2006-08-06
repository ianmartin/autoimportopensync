/* 
   MultiSync Plugin for Mozilla Sunbird
   Copyright (C) 2005-2006 Markus Meyer <meyer@mesw.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License version 2.1 as
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

#include "webdav.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <neon/ne_socket.h>
#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include <neon/ne_auth.h>
#include <neon/ne_basic.h>

static char auth_username[100];
static char auth_password[100];
static int neon_initialized = 0;

static int webdav_server_auth(void* userdata, const char *realm, int attempts, char *username, char *password)
{
    strcpy(username, auth_username);
    strcpy(password, auth_password);
    return attempts;
}

static int init_neon()
{
    if (!neon_initialized)
        neon_initialized = (ne_sock_init() == 0);
    return neon_initialized;
}

static int webdav_spliturl(char* protocol, char* url, char* server, char* path)
{
    char *p1, *p2;
    
    p1 = strstr(url, "://");
    if (!p1)
        return 0;
    
    memcpy(protocol, url, sizeof(p1-url));
    protocol[p1-url] = 0;
    
    p1 += 3;
        
    p2 = strstr(p1, "/");
    if (!p2)
        return 0;
    
    strcpy(server, p1);
    server[p2-p1] = 0;
    strcpy(path, p2);
    
    return 1;
}

int webdav_download(char* filename, char* url, char* username, char* password)
{
    char protocol[256], server[256], path[256];
    int result;
    ne_session* sess;
    FILE* f;
    
    if (strlen(url) > 255 || strlen(username) > 99 || strlen(password) > 99)
        return WEBDAV_ERROR_INVALID_PARAMETER;

    if (!webdav_spliturl(protocol, url, server, path))
        return WEBDAV_ERROR_INVALID_PARAMETER;
    
    f = fopen(filename, "w");
    if (!f)
        return WEBDAV_ERROR_FILE_NOT_FOUND;

    strcpy(auth_username, username);
    strcpy(auth_password, password);
    
    if (!init_neon())
        return WEBDAV_ERROR_INIT;
        
    sess = ne_session_create(protocol, server, 80);
    if (!sess)
        return WEBDAV_ERROR_CONNECT;

    if (strcmp(protocol, "https") == 0)
        ne_ssl_trust_default_ca(sess);
    
    ne_set_server_auth(sess, webdav_server_auth, 0);
    result = ne_get(sess, path, fileno(f)) ? WEBDAV_ERROR_RESSOURCE : WEBDAV_SUCCESS;
    fclose(f);
    ne_session_destroy(sess);

    return result;
}

int webdav_upload(char* filename, char* url, char* username, char* password)
{
    char protocol[256], server[256], path[256], *buf;
    int result;
    int filesize;
    ne_session* sess;
    ne_request* req;
    FILE* f;
    
    if (strlen(url) > 255 || strlen(username) > 99 || strlen(password) > 99)
        return WEBDAV_ERROR_INVALID_PARAMETER;

    if (!webdav_spliturl(protocol, url, server, path))
        return WEBDAV_ERROR_INVALID_PARAMETER;
    
    f = fopen(filename, "r");
    if (!f)
        return WEBDAV_ERROR_FILE_NOT_FOUND;
    fseek(f, 0, SEEK_END);
    filesize = ftell(f);
    rewind(f);
    buf = (char*)malloc(filesize);
    if (!buf)
    {
        fclose(f);
        return WEBDAV_ERROR_OUT_OF_MEMORY;
    }
    if (fread(buf, 1, filesize, f) != 1) {
	    if (ferror(f)) {
		    fclose(f);
		    return WEBDAV_ERROR_RESSOURCE;
	    }
    }
    fclose(f);

    strcpy(auth_username, username);
    strcpy(auth_password, password);
    
    if (!init_neon())
        return WEBDAV_ERROR_INIT;
        
    sess = ne_session_create(protocol, server, 80);
    if (!sess)
    {
        free(buf);
        return WEBDAV_ERROR_CONNECT;
    }

    if (strcmp(protocol, "https") == 0)
        ne_ssl_trust_default_ca(sess);
    
    ne_set_server_auth(sess, webdav_server_auth, 0);

    req = ne_request_create(sess, "PUT", path);
    ne_set_request_body_buffer(req, buf, filesize);
    if (ne_request_dispatch(req) != 0)
    {
        ne_request_destroy(req);
        ne_session_destroy(sess);
        free(buf);
        return WEBDAV_ERROR_RESSOURCE;
    }

    result = ne_get_status(req)->code;
    
    ne_request_destroy(req);
    ne_session_destroy(sess);
    free(buf);
    
    if (result < 200 || result > 299)
        return WEBDAV_ERROR_RESSOURCE;
    else
        return WEBDAV_SUCCESS;
}
