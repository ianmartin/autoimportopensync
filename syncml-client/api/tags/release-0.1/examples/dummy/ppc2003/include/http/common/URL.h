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
#ifndef INCL_HTTP_URL
    #define INCL_HTTP_URL

    #include "common/Constants.h"
    #include "http/common/Constants.h"

    class __declspec(dllexport) URL {

    public:
        wchar_t fullURL [DIM_URL         ];
        wchar_t protocol[DIM_URL_PROTOCOL];
        wchar_t host    [DIM_HOSTNAME    ];
        wchar_t resource[DIM_URL         ];
        int     port;

        URL();
        URL(wchar_t* url);

        void setURL(URL& url);
        void setURL(wchar_t* url);

        URL& operator= (URL& url) { setURL(url); return *this;}
        URL& operator= (wchar_t* url) { setURL(url); return *this;}
    };
#endif