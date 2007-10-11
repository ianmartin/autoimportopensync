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

#ifndef INCL_SYNC_ITEM_STATUS
#define INCL_SYNC_ITEM_STATUS

#include "common/fscapi.h"
#include "common/Constants.h"
#include "spds/common/Constants.h"
        

    class /*__declspec(dllexport)*/ SyncItemStatus {

    private:

        int     cmdID;
        int     msgRef;
        int     cmdRef;
        wchar_t cmd[DIM_COMMAND_SYNC_ITEM_STATUS];
        wchar_t key[DIM_KEY_SYNC_ITEM_STATUS    ];
        int     data;

    public:
        /*
         * Default constructor
         */
        SyncItemStatus();

        /*
         * Constructs a new SyncItemStatus identified by the given key. The key must
         * not be longer than DIM_KEY_SYNC_ITEM_STATUS (see SPDS Constants).
         *
         * @param key - the key
         */
        SyncItemStatus(wchar_t* key);

        ~SyncItemStatus();

        /*
         * Returns the SyncItemStatus's key. If key is NULL, the internal buffer is
         * returned; if key is not NULL, the value is copied in the caller
         * allocated buffer and the given buffer pointer is returned.
         *
         * @param key - buffer where the key will be stored
         */
        wchar_t* getKey(wchar_t* key);

        /*
         * Changes the SyncItemStatus key. The key must not be longer than DIM_KEY_SYNC_ITEM_STATUS
         * (see SPDS Constants).
         *
         * @param key - the key
         */
        void setKey(wchar_t* key);

         /*
         * Returns the SyncItemStatus's command name. If cmd is NULL, the internal buffer is
         * returned; if cmd is not NULL, the value is copied in the caller
         * allocated buffer and the given buffer pointer is returned.
         *
         * @param cmd - buffer where the cmd will be stored
         */
        wchar_t* getCmd(wchar_t* cmd);

        /*
         * Changes the SyncItemStatus cmd. The cmd must not be longer than DIM_COMMAND_SYNC_ITEM_STATUS
         * (see SPDS Constants).
         *
         * @param cmd - the cmd
         */
        void setCmd(wchar_t* cmd);
       

        /*
         * Sets the SyncItemStatus data. The passed data are copied into an
         * internal variable.
         */
        void setData(int data);

        /*
         * Returns the SyncItemStatus data variable.
         */
        int getData();

        
        /*
         * Sets the SyncItemStatus command ID. The passed data are copied into an
         * internal variable.
         */
        void setCmdID(int cmdId);

        /*
         * Returns the SyncItemStatus command ID variable.
         */
        int getCmdID();

        /*
         * Sets the SyncItemStatus message referring. The passed data are copied into an
         * internal variable.
         */
        void setMsgRef(int msgRef);

        /*
         * Returns the SyncItemStatus message referring variable.
         */
        int getMsgRef();

        /*
         * Sets the SyncItemStatus command referring. The passed data are copied into an
         * internal variable.
         */
        void setCmdRef(int cmdRef);

        /*
         * Returns the SyncItemStatus command referring variable.
         */
        int getCmdRef();

    };

#endif


