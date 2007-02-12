/*
 * Copyright (C) 2003-2005 Funambol
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

#ifndef INCL_GENERIC_TRANSPORT_AGENT
#define INCL_GENERIC_TRANSPORT_AGENT

#include <string>

#include "common/fscapi.h"

#include "http/common/URL.h"
#include "http/common/Proxy.h"
    
#define DEFAULT_MAX_TIMEOUT     300   //number of seconds of waiting response timeout.

using namespace std;
/*
 * This class is the transport agent responsible for messages exchange
 * over an HTTP connection. This class is a candidate to be a common
 * base class for all transport agents - the TransportAgent of the
 * original distribution is just not usable. The class handles the
 * attributes url, proxy and useCheckConnection. It also 
 * manages a buffer for the message received from the net. The
 * actual transfer to/from the server is delegated to derived
 * classes. Such classes must implement at least
 *
 *  - sendMessage() - send a message to server and read response.
 *  - strdup_mbsrtowcs() - convert an UTF string to wide char( Unicode ),
 * 
 */

class GenericTransportAgent 
{

    private:

    protected:
        struct{
            	char*  	   mbs;
		wchar_t*   wcs;
		size_t     size;
        } buffer;
	    

        URL                url;
        Proxy              proxy;
        unsigned int       responseTimeout;


        /**
         * This function translates a UNICODE string into a UTF string without
         * allocating additional memory. The translation is performed removing
         * the second byte of the UNICODE coding.
         *
         * @param s the string to translate
         */
        void toUTF(wchar_t* s);


    public:
        /** Append a chunk af data to the response buffer. */
        void  appendToResponse( char* data, size_t size );

        GenericTransportAgent();

        GenericTransportAgent( URL&          url, 
                               Proxy&        proxy, 
                               BOOL          useCheckConnection, 
                               unsigned int  maxResponseTimeout );

        virtual ~GenericTransportAgent();

        /*
         * Change the URL the subsequent calls to sendMessage() should
         * use as target url.
         *
         * @param url the new target url
         */
        virtual void setURL(URL& newURL);
        
        /*
         * Returns the url.
         */
        virtual URL getURL();

        /*
         * Sends the given SyncML message to the server specified
         * by the instal property 'url'. Returns the response status code.
         * Use getResponse() to get the server response.
         */
        virtual wchar_t* sendMessage(wchar_t* msg) = 0;

        /*
         * Returns the last response read from the server.
         * Use releaseResponseBuffer() to release the memory buffer.
         */
        const wchar_t* getResponse();

        /*
         * Releases the response buffer allocated by sendMessage().
         */
        void releaseResponseBuffer();

        void setResponseTimeout(unsigned int responseTimeout);
        
        virtual unsigned int getResponseTimeout();
};
extern size_t debugWriteFunc(void* ptr, size_t size, size_t nmemb, void* userp);

#endif


