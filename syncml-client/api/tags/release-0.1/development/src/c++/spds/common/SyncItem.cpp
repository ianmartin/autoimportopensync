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
// @version $Id: SyncItem.cpp,v 1.10 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include <stdlib.h>


#include "spds/common/SyncItem.h"

/*
 * Default constructor
 */
SyncItem::SyncItem() {
    wcscpy(key, TEXT(""));
}


/*
 * Constructs a new SyncItem identified by the given key. The key must
 * not be longer than DIM_KEY (see SPDS Constants).
 *
 * @param key - the key
 */
SyncItem::SyncItem(wchar_t* itemKey) {
    wcsncpy(key, itemKey, DIM_KEY);
    key[DIM_KEY-1] = 0;

    wcscpy(type, TYPE_UNKNOWN);
    data = NULL;
    size = -1;
    lastModificationTime = -1;
}

/*
 * Destructor. Free the allocated memory (if any)
 */
SyncItem::~SyncItem() {
    if (data != NULL) {
        free(data);
    }
}

/*
 * Returns the SyncItem's key. If key is NULL, the internal buffer is
 * returned; if key is not NULL, the value is copied in the caller
 * allocated buffer and the given buffer pointer is returned.
 *
 * @param key - buffer where the key will be stored
 */
wchar_t* SyncItem::getKey(wchar_t* itemKey) {
    if (itemKey == NULL) {
        return key;
    }

    return wcscpy(itemKey, key);
}

/*
 * Changes the SyncItem key. The key must not be longer than DIM_KEY
 * (see SPDS Constants).
 *
 * @param key - the key
 */
void SyncItem::setKey(wchar_t* itemKey) {
    wcsncpy(key, itemKey, DIM_KEY);
    key[DIM_KEY-1] = 0;
}

/*
 * Sets the SyncItem modification timestamp. timestamp is a milliseconds
 * timestamp since a reference time (which is platform specific).
 *
 * @param timestamp - last modification timestamp
 */
 void SyncItem::setModificationTime(long timestamp) {
     lastModificationTime = timestamp;
 }

/*
 * Returns the SyncItem modeification timestamp. The returned value
 * is a milliseconds timestamp since a reference time (which is
 * platform specific).
 */
long SyncItem::getModificationTime() {
    return lastModificationTime;
}

/*
 * Sets the SyncItem content data. The passed data are copied into an
 * internal buffer so that the caller can release the buffer after
 * calling setData(). The buffer is fred in the destructor.
 * If when calling setData, there was an existing allocated data block,
 * it is reused (shrinked or expanded as necessary).
 */
void* SyncItem::setData(void* itemData, long dataSize) {
    if (itemData == NULL) {   //FIXME!
        free(data); data = NULL;
    }
    data = realloc(data, dataSize);

    if (data == NULL) {
        lastErrorCode = ERR_NOT_ENOUGH_MEMORY;
        swprintf(lastErrorMsg, DIM_ERROR_MESSAGE,
                TEXT("Not enough memory (%d bytes required)"), dataSize);
        return data;

    }
    size = dataSize;
    memcpy(data, itemData, size);

    return data;
}

/*
 * Returns the SyncItem data buffer. The fred in the destructor.
 */
void* SyncItem::getData() {
    return data;
}

/*
 * Returns the SyncItem data size.
 */
long SyncItem::getDataSize() {
    return size;
}

/*
 * Sets the SyncItem data mime type
 *
 * @param - type the content mimetype
 */
void SyncItem::setDataType(wchar_t* mimeType) {
    wcsncpy(type, mimeType, DIM_MIME_TYPE);
    type[DIM_MIME_TYPE-1] = 0;
}

/*
 * Returns the SyncItem data mime type.
 *
 * @param type - buffer that will contain the mime type
 * @param n - size of the buffer
 */
void SyncItem::getDataType(wchar_t* mimeType, int n) {
    wcsncpy(mimeType, type, n);
    mimeType[n-1] = 0;
}

/*
 * Sets the SyncItem state
 *
 * @param state the new SyncItem state
 */
void SyncItem::setState(SyncState newState) {
    state = newState;
}

/*
 * Gets the SyncItem state
 */
SyncState SyncItem::getState() {
    return state;
}


