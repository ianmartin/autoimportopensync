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
// @version $Id: HTTPHeader.h,v 1.3 2005/01/19 10:22:07 magi Exp $
//
// This class represents an HTTP header as read from a buffer or stream
//

#ifndef INCL_HTTP_HEADER
    #define INCL_HTTP_HEADER

    #define DIM_HEADERS 50
    #define MSG_BAD_PROTOCOL "BAD PROTOCOL"

    class HTTPHeader {
    private:
        char* version;
        unsigned int status;
        char* statusMessage;
        char* content;
        unsigned int headersCount;
        char* headers[DIM_HEADERS][2]; // up to DIM_HEADERS headers
        unsigned int size;

    public:
        HTTPHeader(char* buf);

        /*
         * Get the size in bytes of this HTTP header
         */
        unsigned int getSize();
        char* getVersion();
        unsigned int getStatus();
        char* getStatusMessage();
        char* getContent();
        unsigned int getHeadersCount();
        char** getHeader(unsigned int index);
        char* getHeaderValue(const char* header);

        /*
         * Returns the content lenght specified with the header Content-length.
         * If the header is not found, -1 is returned.
         *
         */
        int getContentLength();
    };
#endif