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

 #ifndef INCL_SYNC_ITEM
    #define INCL_SYNC_ITEM

    #include "common/fscapi.h"
    #include "common/Constants.h"
    #include "spds/common/Constants.h"

    typedef enum { SYNC_OK = 200, SYNC_ERR = 500 } SyncStatus;
    typedef enum {
        SYNC_STATE_NEW     = 'N',
        SYNC_STATE_UPDATED = 'U',
        SYNC_STATE_DELETED = 'D',
        SYNC_STATE_NONE    = ' '
    } SyncState;

    class /*__declspec(dllexport)*/ SyncItem {

    private:

        void* data;
        long size;

        wchar_t key[DIM_KEY];
        wchar_t type[DIM_MIME_TYPE];

        long lastModificationTime;
        SyncState state;

    public:
        /*
         * Default constructor
         */
        SyncItem();

        /*
         * Constructs a new SyncItem identified by the given key. The key must
         * not be longer than DIM_KEY (see SPDS Constants).
         *
         * @param key - the key
         */
        SyncItem(wchar_t* key);

        ~SyncItem();

        /*
         * Returns the SyncItem's key. If key is NULL, the internal buffer is
         * returned; if key is not NULL, the value is copied in the caller
         * allocated buffer and the given buffer pointer is returned.
         *
         * @param key - buffer where the key will be stored
         */
        wchar_t* getKey(wchar_t* key);

        /*
         * Changes the SyncItem key. The key must not be longer than DIM_KEY
         * (see SPDS Constants).
         *
         * @param key - the key
         */
        void setKey(wchar_t* key);

        /*
         * Sets the SyncItem modification timestamp. timestamp is a milliseconds
         * timestamp since a reference time (which is platform specific).
         *
         * @param timestamp - last modification timestamp
         */
        void setModificationTime(long timestamp);

        /*
         * Returns the SyncItem modeification timestamp. The returned value
         * is a milliseconds timestamp since a reference time (which is
         * platform specific).
         */
        long getModificationTime();

        /*
         * Sets the SyncItem content data. The passed data are copied into an
         * internal buffer so that the caller can release the buffer after
         * calling setData().
         */
        void* setData(void* data, long size);

        /*
         * Returns the SyncItem data buffer.
         */
        void* getData();

        /*
         * Returns the SyncItem data size.
         */
        long getDataSize();

        /*
         * Sets the SyncItem data mime type
         *
         * @param - type the content mimetype
         */
        void setDataType(wchar_t* type);

        /*
         * Returns the SyncItem data mime type.
         *
         * @param type - buffer that will contain the mime type
         * @param n - size of the buffer
         */
        void getDataType(wchar_t* type, int n);

        /*
         * Sets the SyncItem state
         *
         * @param state the new SyncItem state
         */
        void setState(SyncState newState);

        /*
         * Gets the SyncItem state
         */
        SyncState getState();

    };

#endif