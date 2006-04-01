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

#ifndef INCL_SPDS_CONSTANTS
#define INCL_SPDS_CONSTANTS

#include "common/fscapi.h"

#define DIM_SOURCE_NAME      128
#define DIM_SOURCE_URI        64
#define DIM_KEY              256
#define DIM_MIME_TYPE         64
#define DIM_USERNAME         100
#define DIM_PASSWORD         100
#define DIM_DEVICE_ID         50
#define DIM_SERVERNAME       100
#define DIM_SYNC_MODES_LIST   64
#define DIM_SYNC_MODE         16
#define DIM_ANCHOR            32
#define DIM_DIR              256
#define DIM_REMOTE_URI        64
    
#define DIM_KEY_SYNC_ITEM_STATUS        64    
#define DIM_COMMAND_SYNC_ITEM_STATUS    128    


#define ERR_REPRESENTATION  700


#define TYPE_UNKNOWN TEXT("unknown")

    typedef enum {
        SYNC_NONE                          = 000,
        SYNC_TWO_WAY                       = 200,
        SYNC_SLOW                          = 201,
        SYNC_ONE_WAY_FROM_CLIENT           = 202,
        SYNC_REFRESH_FROM_CLIENT           = 203,
        SYNC_ONE_WAY_FROM_SERVER           = 204,
        SYNC_REFRESH_FROM_SERVER           = 205,
        SYNC_TWO_WAY_BY_SERVER             = 206,
        SYNC_ONE_WAY_FROM_CLIENT_BY_SERVER = 207,
        SYNC_REFRESH_FROM_CLIENT_BY_SERVER = 208,
        SYNC_ONE_WAY_FROM_SERVER_BY_SERVER = 209,
        SYNC_REFRESG_FROM_SERVER_BY_SERVER = 210,
    } SyncMode;

#define CONTEXT_SPDS_SYNCML  TEXT("spds/syncml")
#define CONTEXT_SPDS_SOURCES TEXT("spds/sources")

#define ERR_PROTOCOL_ERROR      400
#define ERR_AUTH_NOT_AUTHORIZED 401
#define ERR_AUTH_EXPIRED        402
#define ERR_NOT_FOUND           404
#define ERR_AUTH_REQUIRED       407
#define ERR_SRV_FAULT           500

#endif


