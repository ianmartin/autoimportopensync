/*
 * Copyright (C) 2003-2005 Funambol
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//
// @author Stefano Fornari
// @version $Id: URL.cpp,v 1.6 2005/03/30 10:12:51 magi Exp $
//
#include "unixdefs.h"
#include "common/fscapi.h"
#include "http/common/URL.h"


/*
 * Creates a new URL object from a string representation of the URL. The URL
 * must be in the form:
 *
 * <protocol>://<hostname>[:<port>]/<resource>
 *
 * If a parsing error occurs, fullURL is set to an empty string ("").
 *
 * @param url the URL
 */
URL::URL(wchar_t* url) {
    setURL(url);
}

/*
 * Default constructs
 */
URL::URL() {
    wcscpy(fullURL,  TEXT(""));
    wcscpy(host,     TEXT(""));
    wcscpy(protocol, TEXT(""));
    wcscpy(resource, TEXT(""));
    port = 0;
}

void URL::setURL(const URL& url) {
    wcscpy(fullURL,  url.fullURL );
    wcscpy(host,     url.host    );
    wcscpy(protocol, url.protocol);
    wcscpy(resource, url.resource);
    port = url.port;
}

void URL::setURL(const wchar_t* url) {
    int size;
    wcscpy(fullURL,  TEXT(""));
    wcscpy(host,     TEXT(""));
    wcscpy(protocol, TEXT(""));
    wcscpy(resource, TEXT(""));

    port = 0;

    if ((url == NULL) || (wcslen(url) == 0)) {
        return;
    }

    wchar_t* p = NULL;
    wchar_t* q = NULL;

    //
    // protocol (mandatory)
    //
    p = wcsstr(url, TEXT("://"));
    if ((p == NULL) || (p == url)) {
        return;
    }

    size = ( p-url < DIM_URL_PROTOCOL ? p-url : DIM_URL_PROTOCOL ); 
    wcsncpy(protocol, url, size);
    protocol[size] = 0;

    //
    // server (mandatory)
    // and
    // port (optional)
    //
    p += 3;
    q = wcsstr(p, TEXT("/"));
    if (q == NULL) {
        wcsncpy(host, p, DIM_HOSTNAME);
        host[DIM_HOSTNAME-1] = 0;
    } else {
        size = (q - p < DIM_HOSTNAME ? q - p : DIM_HOSTNAME );
        wcsncpy(host, p, size);
        host[size] = 0;
    }

    p = wcsstr(host, TEXT(":"));
    if (p == NULL) {
        port = 80;
    } else {
        port = wcstoul(p+1, NULL, 10);
        *p = 0;
    }

    //
    // resource
    //
    if (q != NULL) {
        wcsncpy(resource, q, DIM_URL);
        resource[DIM_URL-1] = 0;
    } else {
        resource[0] = 0;
    }

    //
    // fullURL
    //
    if (url != NULL) {
        wcsncpy(fullURL, url, DIM_URL);
        fullURL[DIM_URL-1] = 0;
    } else {
        fullURL[0] = 0;
    }
}


