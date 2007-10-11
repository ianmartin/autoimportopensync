/*
 * Copyright (C) 2006 Michael Kolmodin
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

#ifndef INCL_CURL_TRANSPORT_AGENT
#define INCL_CURL_TRANSPORT_AGENT

#include <string>

#include "curl.h"

#include "common/Log.h"
#include "common/fscapi.h"

#include "http/common/Constants.h"
#include "http/common/URL.h"
#include "http/common/Proxy.h"
#include "http/linux/GenericTransportAgent.h"
    
#define DEFAULT_MAX_TIMEOUT     300   //number of seconds of waiting response timeout.
#define SYNCML_CONTENT_TYPE_HDR "Content-Type: application/vnd.syncml+xml"

using namespace std;

/*
 * This class is the transport agent responsible for messages exchange
 * over an HTTP connection.
 * This is the Linux/Curl concrete class.
 */

class TransportAgent:  public GenericTransportAgent 
{

    private:

    protected:
        CURL*      curlHandle;
        char*      mbsUrl;
        char       errorMsgBuff[ CURL_ERROR_SIZE ];

        void       init();

        void       setError( int code, wchar_t* format, ... );

    public:

        TransportAgent( );

        TransportAgent( URL&          url, 
                        Proxy&        proxy, 
                        BOOL          useCheckConnection, 
                        unsigned int  maxResponseTimeout );

        ~TransportAgent();


        /*
         * Sends the given SyncML message to the server specified
         * by the install property 'url'. Returns the response.
         * FIXME: Use getResponse() to get the server response.
         */
        wchar_t* sendMessage(wchar_t* msg);

        /** Callback invoked from Curl */
        size_t writeData( void *buffer, size_t size, size_t nmemb, void *userp );
        void setURL( URL& url );
        void setResponseTimeout( unsigned int timeoutArg );

    };

#endif


