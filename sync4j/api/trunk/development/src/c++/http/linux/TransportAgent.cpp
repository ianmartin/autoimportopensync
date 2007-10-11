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
// @author Stefano Fornari
// @author Michael Kolmodin
// @version $Id: TransportAgent.cpp,v 1.10 2005/03/29 13:20:25 magi Exp $
//
#include <locale.h>
#include "unixdefs.h"
#include "http/linux/TransportAgent.h"

/*
 * This is the Linux/libcurl  implementation of the TransportAgent object
 */
static int libcurlInited = FALSE;

/**
* The callback invoked from curl to store data from remote side. 
*
*/
extern "C" size_t writeFunc(void* ptr, size_t size, size_t nmemb, void* userp)
{
    GenericTransportAgent* transportAgent = (GenericTransportAgent*) userp;
    transportAgent->appendToResponse( (char*)ptr, size * nmemb );
    return  size * nmemb;
}

/**
* Debug callback invoked from curl
*/
extern "C" int debug_callback (CURL*         handle, 
                               curl_infotype info_type, 
                               char *        info, 
                               size_t        info_size, 
                               void*         data)
{
    char* buff = (char*) malloc( (info_size + 1 ) * sizeof( char ) );
    strncpy( buff, info, info_size );
    buff[ info_size ] = '\0';
    LOG.setMessage( L"Curldebug, info: %d, message: %s", info_type, buff );
    LOG.debug();
    free( buff );
}
    

/** Check curl return code, log messages if not OK. */
static void checkCurl( CURLcode code , char* hint )
{
    if( code != CURLE_OK ){
        LOG.setMessage( L"Curl error %d - \"%s\" doing %s",
                        code,
                        curl_easy_strerror( code ),
                        hint );
        LOG.info();
    }
    else{
        LOG.setMessage( L"CURL %s - OK", hint );
        LOG.debug();
    }
}

/** Set the url. */
void TransportAgent::setURL( URL& urlArg )
{
    GenericTransportAgent::setURL( urlArg );
    mbsUrl = new_wcstombs( urlArg.fullURL );
    checkCurl( curl_easy_setopt( curlHandle, CURLOPT_URL, mbsUrl ), "setUrl" );
}

/** Set the connection timeout. */
void TransportAgent::setResponseTimeout( unsigned int timeoutArg )
{
    GenericTransportAgent::setResponseTimeout( timeoutArg );
    checkCurl( curl_easy_setopt( curlHandle, CURLOPT_TIMEOUT, timeoutArg ),
               "setTimeout" );
}

void TransportAgent::init()
{
    CURLcode code;

    if( ! libcurlInited ){
        setlocale( LC_CTYPE, "" );
        libcurlInited = TRUE;
    }
    curlHandle = curl_easy_init();

    code = curl_easy_setopt( curlHandle, CURLOPT_ERRORBUFFER, errorMsgBuff );
    checkCurl( code, "errorBuffer" );

    code = curl_easy_setopt( curlHandle, CURLOPT_WRITEFUNCTION, &writeFunc );
    checkCurl( code, "writeFunc" );

    code = curl_easy_setopt( curlHandle, CURLOPT_WRITEDATA, this );
    checkCurl( code, "writeData" );

    code = curl_easy_setopt( curlHandle, CURLOPT_FORBID_REUSE, 1 );
    checkCurl( code, "forbidReuse" );

/**
    if( LOG.debugEnabled ){
        curl_easy_setopt( curlHandle, CURLOPT_VERBOSE, TRUE );
        checkCurl( code, "verbose" );

        code = curl_easy_setopt( curlHandle, 
                                 CURLOPT_DEBUGFUNCTION, 
                                 &debug_callback);
        checkCurl( code, "debugFunc" );
    }
**/

}

/**
*
* Default constructor. At least setURL must be invoked before
* sendMessage().
*
*/
TransportAgent::TransportAgent( ) :  GenericTransportAgent()
{
   init();
};

/*
* Constructor, this implementation ignores newProxy and useCheckConmn.
*
* @param url the url where messages will be sent with sendMessage()
* @param maxResponseTimeout Max nr of seconds to wait for reply.
*/
TransportAgent::TransportAgent(URL&            newURL, 
                               Proxy&          newProxy, 
                               BOOL            useCheckConn, 
                               unsigned int    responseTimeout) 
   : GenericTransportAgent( newURL, newProxy, useCheckConn, responseTimeout )

{
    init();
    setURL( newURL );
    setResponseTimeout( responseTimeout );
}

TransportAgent::~TransportAgent()
{
    curl_easy_cleanup( curlHandle );
    if( mbsUrl != NULL )
        delete[] mbsUrl;
}

/**
*
*  Update global error message and global error code, post log message. 
*
*  @param code global error code.
*  @param format printf style message template + args.
*
*/
void TransportAgent::setError( int code, wchar_t* format, ... )
{
    char mbsBuff[ DIM_ERROR_MESSAGE ];

    va_list ap;
    va_start( ap, format );
    vswprintf( lastErrorMsg, DIM_ERROR_MESSAGE, format, ap );
    va_end( ap );

    lastErrorCode = code;

    LOG.setMessage( lastErrorMsg );
    LOG.info( logmsg );
}


/**
 *
 * Sends the given SyncML message to the server specified
 * by the install property 'url'. Returns the response status code or -1
 * if it was not possible initialize the connection.
 *
 * Use getResponse() to get the server response.
 */
wchar_t* TransportAgent::sendMessage(wchar_t* msg) 
{
    CURLcode code;

    releaseResponseBuffer();

    LOG.setMessage( TEXT( "Connecting to %ls:%d"), url.host, url.port  );
    LOG.debug( logmsg );

    //
    // Prepare headers
    //
    size_t contentLength = wcslen( msg );
    char headerBuff[ 128 ];

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, SYNCML_CONTENT_TYPE_HDR );
    sprintf( headerBuff, "Content-Length: %d", contentLength  );
    headers = curl_slist_append(headers, headerBuff );
    headers = curl_slist_append(headers,  "Accept: text/*"  );

    code = curl_easy_setopt( curlHandle, CURLOPT_HTTPHEADER, headers ),
    checkCurl(  code, "headers" );

    curl_easy_setopt( curlHandle, CURLOPT_POSTFIELDSIZE, contentLength );
    checkCurl( code, "contentLength" ); 

    // Send a request to the HTTP server.
    toUTF( msg );
    code = curl_easy_setopt( curlHandle, CURLOPT_POSTFIELDS, msg );
    checkCurl( code, "contentLength" ); 

    code  = curl_easy_perform( curlHandle );
 
    curl_slist_free_all( headers ); 
    LOG.debug(MESSAGE_SENT);

    //
    // Check the status code, return
    //
    if (code != CURLE_OK) {
        setError( ERR_HTTP, 
                  L"HTTP request error: %d, (%s) msg:%s", 
                  code, 
                  curl_easy_strerror( code ),
                  errorMsgBuff );
        LOG.setMessage( lastErrorMsg );
        LOG.info();
        return( NULL );
    }
    else
        return (wchar_t*) getResponse();
}

/** C factories for dynamic loading. */
extern "C" TransportAgent* createTransportAgent()
{
    return new  TransportAgent;
}

extern "C" void deleteTransportAgent( TransportAgent* agent )
{
    delete agent;
}

