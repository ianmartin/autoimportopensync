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

#ifndef INCL_HTTP_PROXY
#define INCL_HTTP_PROXY

#include "common/Constants.h"
#include "http/common/Constants.h"

    class __declspec(dllexport) Proxy {

    public:
        wchar_t host    [DIM_HOSTNAME];
        wchar_t user    [DIM_USERNAME];
        wchar_t password[DIM_PASSWORD];
        int     port;

        Proxy();
        Proxy(wchar_t* host, int port);

        Proxy(wchar_t* host, int port, wchar_t* user, wchar_t* password);

        void setProxy(Proxy& proxy);
        void Proxy::setProxy(wchar_t* proxyHost, int proxyPort, wchar_t* proxyUser, wchar_t* proxyPassword);

        Proxy& operator= (Proxy& proxy) { setProxy(proxy); return *this;}
    };
#endif


