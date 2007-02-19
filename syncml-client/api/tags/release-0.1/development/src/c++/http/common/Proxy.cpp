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
// @version $Id: Proxy.cpp,v 1.6 2005/01/19 10:22:07 magi Exp $
//
#include "unixdefs.h"
#include "common/fscapi.h"
#include "http/common/Proxy.h"


Proxy::Proxy() {
    setProxy(NULL, 0, NULL, NULL);
}

/*
 * Creates a new Proxy object
 *
 * @param host the proxy host
 * @param port the proxy port
 */
Proxy::Proxy(wchar_t* proxyHost, int proxyPort) {
    setProxy(proxyHost, proxyPort, NULL, NULL);
}

/*
 * Creates a new Proxy object
 *
 * @param host the proxy host
 * @param port the proxy port
 */
Proxy::Proxy(wchar_t* proxyHost, int proxyPort, wchar_t* proxyUser, wchar_t* proxyPassword){
    setProxy(proxyHost, proxyPort, proxyUser, proxyPassword);
}

void Proxy::setProxy(Proxy& newProxy) {
    setProxy(newProxy.host, newProxy.port, newProxy.user, newProxy.password);
}

/*
 * NULL values are translated into empty strings
 */
void Proxy::setProxy(wchar_t* proxyHost    ,
                     int      proxyPort    ,
                     wchar_t* proxyUser    ,
                     wchar_t* proxyPassword) {
    if (proxyHost != NULL) {
        wcsncpy(host, proxyHost, DIM_HOSTNAME);
        host[DIM_HOSTNAME-1] = 0;
    } else {
        wcscpy(host, TEXT(""));
    }

    if (proxyUser != NULL){
        wcsncpy(user, proxyUser, DIM_USERNAME);
        user[DIM_USERNAME-1] = 0;
    } else {
        wcscpy(user, TEXT(""));
    }

    if (proxyPassword != NULL){
        wcsncpy(password, proxyPassword, DIM_PASSWORD);
        password[DIM_PASSWORD-1] = 0;
    } else {
        wcscpy(password, TEXT(""));
    }
    port = proxyPort;
}

