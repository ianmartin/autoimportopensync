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

#ifndef INCL_FSCAPI_CONSTANTS
    #define INCL_FSCAPI_CONSTANTS

    #define DIM_ERROR_MESSAGE 512

    #define ERR_NOT_ENOUGH_MEMORY   1000
    #define ERR_PARAMETER_IS_EMPTY  1001
    #define ERR_PARAMETER_IS_NULL   1002
    #define ERR_WRONG_PARAMETERS    1003

    #define EMPTY_STRING TEXT("")
    
    // messages for the logging
    #define INITIALIZING                            TEXT("Initializing")
    #define INITIALIZATION_DONE                     TEXT("Initialization done")
    #define SERVER_ALERT_CODE                       TEXT("The server alert code for %s is %i")
    #define SYNCHRONIZING                           TEXT("Synchronizing %s")
    
    #define PREPARING_FAST_SYNC                     TEXT("Preparing fast sync for %s")
    #define PREPARING_SLOW_SYNC                     TEXT("Preparing slow sync for %s")
    #define PREPARING_SYNC_REFRESH_FROM_SERVER      TEXT("Preparing refresh from server sync for %s")
    #define PREPARING_SYNC_ONE_WAY_FROM_SERVER      TEXT("Preparing one way from server sync for %s")
    
    #define DETECTED_SLOW                           TEXT("Detected %i items")
    #define DETECTED_FAST                           TEXT("Detected %i new items, %i updated items, %i deleted items")
    
    #define SENDING_MODIFICATION                    TEXT("Sending modifications")
    #define SENDING_ALERT                           TEXT("Sending alert to get server modifications")
    #define RETURNED_NUM_ITEMS                      TEXT("Returned %i new items, %i updated items, %i deleted items for %s")
    
    #define MODIFICATION_DONE                       TEXT("Modification done")
    #define SENDING_MAPPING                         TEXT("Sending mapping")
    #define SYNCHRONIZATION_DONE                    TEXT("Synchronization done")

    #define RESPURI                                 TEXT("url from response to inizialization-message: %s")
    #define MESSAGE_SENT                            TEXT("Message sent")
    #define READING_RESPONSE                        TEXT("Reading response...")

#endif