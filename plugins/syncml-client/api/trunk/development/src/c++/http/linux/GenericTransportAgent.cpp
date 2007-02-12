/*
 * Copyright (C) 2005 Michael Kolmodin
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
// @author Michael Kolmodin
// @version $Id: TransportAgent.cpp,v 1.10 2005/03/29 13:20:25 magi Exp $
//
#include <locale.h>
#include <string>
#include "unixdefs.h"

#include "http/linux/GenericTransportAgent.h"
#include "common/Log.h"

using namespace std;

/*
 * This function translate a UNICODE string into a UTF string without
 * allocating additional memory. The translation is performed removing
 * the second byte of the UNICODE coding.
 *
 * @param s the string to translate
 */
void GenericTransportAgent::toUTF(wchar_t* s) {
    int l = wcslen(s);
    wchar_t* w = s;
    char*    c = (char*)s;

    while (l--) {
        *c = (char)*w;
        ++c; ++w;
    }

    *c = 0;
}

/*
 * Constructor, this implementation ignores newProxy and useCheckConmn.
 *
 * @param url the url where messages will be sent with sendMessage()
 * @param maxResponseTimeout Max nr of seconds to wait for reply.
 */
GenericTransportAgent::GenericTransportAgent(URL&            newURL, 
                                             Proxy&          newProxy, 
                                             BOOL            useCheckConn, 
                                             unsigned int    maxResponseTimeout) 
{ 
    buffer.size     = 0;
    buffer.wcs      = NULL;
    buffer.mbs      = NULL;
    url             = newURL;
    responseTimeout = maxResponseTimeout;
}

GenericTransportAgent::GenericTransportAgent()
{
    url             = NULL;
    responseTimeout = DEFAULT_MAX_TIMEOUT;
}

GenericTransportAgent::~GenericTransportAgent()
{
    releaseResponseBuffer();
}

/** Append a chunk of data to response buffer. */
void GenericTransportAgent::appendToResponse( char* data, size_t dataSize )
{
    buffer.mbs = (char*) realloc( buffer.mbs, buffer.size + dataSize + 1 );
    memcpy( &(buffer.mbs[ buffer.size ]), data, dataSize );
    buffer.size += dataSize;
    buffer.mbs[ buffer.size ] = '\0';

    assert( strlen( buffer.mbs ) == buffer.size );
}

void GenericTransportAgent::setURL( URL& urlArg )
{
    url = urlArg;
}

URL GenericTransportAgent::getURL()
{
   return url;
}

void GenericTransportAgent::setResponseTimeout(unsigned int responseTimeoutArg )
{
    responseTimeout = responseTimeoutArg;
}

unsigned int GenericTransportAgent::getResponseTimeout()
{
    return responseTimeout;
}

/*
 * Returns the last response read from the server.
 * Use releaseResponseBuffer() to release the memory buffer.
 */
const wchar_t* GenericTransportAgent::getResponse()
{
    if( buffer.wcs == NULL ){
        buffer.wcs = new_mbsrtowcs( buffer.mbs );
    }
    return( buffer.wcs );
}

/*
 * Releases the response buffer allocated by sendMessage().
 */
void GenericTransportAgent::releaseResponseBuffer()
{
   delete[] buffer.wcs;
   buffer.wcs = NULL;

   free( buffer.mbs );
   buffer.mbs = NULL;

   buffer.size = 0;
}
