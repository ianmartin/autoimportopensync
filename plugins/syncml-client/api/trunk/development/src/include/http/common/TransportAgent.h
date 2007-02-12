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

#ifndef INCL_TRANSPORT_AGENT
#define INCL_TRANSPORT_AGENT

#ifdef LINUX
#include "http/linux/TransportAgent.h"
#else

#include "common/fscapi.h"
#include "wininet.h"

#include "http/common/URL.h"
#include "http/common/Proxy.h"
    
#define DEFAULT_MAX_TIMEOUT     300   //number of seconds of waiting response timeout.

/*
 * This class is the transport agent responsible for messages exchange
 * over an HTTP connection.
 * This is a generic abtract class, the common denominatot for the
 * win32/ppc transport agents.
 */

class TransportAgent {

    private:
        URL url;
        Proxy proxy;
        
        unsigned int responseTimeout;

        int init();
        void uninit();
        wchar_t* postHTTP(URL& url, const wchar_t* msg);
        void createHTTPRequest( char* request, const char*  resource, unsigned int length);

    public:

        TransportAgent(URL& url, Proxy& proxy, BOOL useCheckConnection, unsigned int maxResponseTimeout);
        ~TransportAgent();

        /*
         * Change the URL the subsequent calls to setMessage() should
         * use as target url.
         *
         * @param url the new target url
         */
        void setURL(URL& newURL);
        
        /*
         * Returns the url.
         */
        URL getURL();

        /*
         * Sends the given SyncML message to the server specified
         * by the instal property 'url'. Returns the response status code.
         * Use getResponse() to get the server response.
         */
        wchar_t* sendMessage(wchar_t* msg);

        /*
         * Returns the last response read from the server.
         * Use releaseResponseBuffer() to release the memory buffer.
         */

        wchar_t* getResponse();

        /*
         * Releases the response buffer allocated by sendMessage().
         */
        void releaseResponseBuffer();

        void setResponseTimeout(unsigned int responseTimeout);
        
        unsigned int getResponseTimeout();

    };

    //
    // TBR
    //
    wchar_t* sendMessage(URL& url, const wchar_t* msg);

#endif //LINUX

#endif  //INCL_TRANSPORT_AGENT


