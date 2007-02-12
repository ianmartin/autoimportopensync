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

#ifndef INCL_HTTP_CONSTANTS
#define INCL_HTTP_CONSTANTS

#define ERR_TPCIP_INIT                  2000
#define ERR_CONNECT                     2001
#define ERR_READING_CONTENT             2002
#define ERR_HTTP                        2003
#define ERR_HTTP_MISSING_CONTENT_LENGTH 2004
#define ERR_HTTP_TIME_OUT               2006
#define ERR_RESP_URI_DIFFERS_FROM_URL   2008

#define METHOD_GET          TEXT("GET"                       )
#define METHOD_POST         TEXT("POST"                      )
#define USER_AGENT          "Funambol SyncML Client"
#define SYNCML_CONTENT_TYPE "application/vnd.syncml+xml"

#define HTTP_OK           = 200,
#define HTTP_SERVER_ERROR = 500,
#define HTTP_UNAUTHORIZED = 401,
#define HTTP_ERROR        = 400


#define DIM_URL_PROTOCOL       10
#define DIM_URL              2048
#define DIM_HOSTNAME           50
#define DIM_USERNAME          100
#define DIM_PASSWORD          100
#define DIM_BUFFER           4096
#define DIM_HEADER             64

#define STATUS_OK   200

#endif


