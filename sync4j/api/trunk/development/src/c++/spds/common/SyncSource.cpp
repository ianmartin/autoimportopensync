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
// @version $Id: SyncSource.cpp,v 1.11 2005/01/19 10:22:07 magi Exp $
//
#include "unixdefs.h"

#include "common/fscapi.h"
#include "common/Log.h"
#include "spds/common/SyncSource.h"

/*
 * A SyncSource represents the database under sync and the state of its
 * synchronization. The SyncSource object is fed from the client
 * application with the needed sync mode and the client modified items
 * and from the synchronization engine with the negotiated sync mode and
 * server modifified items. At the end of a synchronization process,
 * the client application has the opportunity to set the LUID-GUID mapping
 * so that it can be sent during sync finalization.
 *
 * Here is an example of how to use the SyncSource:
 *
 * SyncItem* deletedItems[];
 * SyncItem* updatedItems[];
 * SyncItem* newItems[];
 * SyncItem* allItems[];
 *
 * SyncSource s = SyncSource("testdb");
 *
 * SyncManager sm = SyncManager();
 *
 * s.setSyncMode(SYNC_TWO_WAY);
 *
 * sm.prepareSync(s);
 * if (s.getSyncMode() == SYNC_TWO_WAY) {
 *
 *     ... fill deletedItems, updatedItems, newItems ...
 *
 *     s.setDeletedItems(deletedItems, deletedCount);
 *     s.setUpdatedItems(updatedItems, updatedCount);
 *     s.setNewItems(newItems, newCount);
 * } else if (s.getSyncMode() == SYNC_SLOW) {
 *
 *     ... fill allItems ...
 *
 *     s.setAllItems(allItems, itemCount)
 * }
 *
 * sm.sync(s);
 *
 * s.getDeletedItems(deletedItems, &n);
 * ... process deleted items ...
 * s.getUpdatedItems(updatedItems, &n);
 * ... process updated items ...
 * s.getNewItems(newItems, &n);
 * ... process new items ...
 *
 * ... creating the mapping ...
 *
 * s.setLUIDGUIDMapping(mappings, nMap);
 *
 * sm.endSync(s);
 *
 */

SyncSource::SyncSource(wchar_t* sourceName) {
    if ((sourceName == NULL) || (*sourceName == 0)) {
        setErrorMsg( ERR_PARAMETER_IS_EMPTY,
                     TEXT("name cannot be empty (NULL or 0-length)"));
        goto finally;
    }

    wcsncpy(name, sourceName, DIM_SOURCE_NAME);
    name[DIM_SOURCE_NAME-1] = 0;
    *type = *next = *last = 0;

    finally:

    syncMode = SYNC_NONE;
    lastSync = -1;

    allItems     = NULL; allItemsCount     = 0;
    updatedItems = NULL; updatedItemsCount = 0;
    deletedItems = NULL; deletedItemsCount = 0;
    newItems     = NULL; newItemsCount     = 0;
    itemStatus   = NULL; itemStatusCount   = 0;
    msgID        = 0;


    map = NULL; mapSize = 0;
}

/*
 * Returns the source name. If sourceName is <> NULL, the source name is copied
 * in it. If sourceName is <> NULL the returned value is sourceName. Otherwise,
 * the returned value is the internal buffer pointer. Note that this will be
 * released at object automatic destruction.
 *
 * @param sourceName - the buffer where the name will be copied (if != NULL) - NULL
 * @param dim - buffer size
 *
 */
 wchar_t *SyncSource::getName(wchar_t* sourceName, int dim) {
     if (sourceName != NULL) {
         wcsncpy(sourceName, name, dim);
         sourceName[dim-1] = 0;

         return sourceName;
     }

     return name;
 }

/*
 * Sets the synchronization mode required for the
 * SyncSource.
 *
 * @param syncMode - sync synchronization mode
 */
void SyncSource::setPreferredSyncMode(SyncMode sourceSyncMode) {
    syncMode = sourceSyncMode;
}

/*
 * Returns the preferred synchronization mode for the SyncSource
 */
SyncMode SyncSource::getPreferredSyncMode() {
    return syncMode;
}

/*
 * Sets the synchronization mode required for the
 * SyncSource.
 *
 * @param syncMode - sync synchronization mode
 */
void SyncSource::setRemoteURI(wchar_t* uri) {
    wcsncpy(remoteURI, (uri != NULL) ? uri : TEXT(""), DIM_REMOTE_URI);
    type[DIM_REMOTE_URI-1] = 0;
}

/*
 * Returns the preferred synchronization mode for the SyncSource
 */
wchar_t* SyncSource::getRemoteURI(wchar_t* uri) {
   if (uri == NULL) {
        return remoteURI;
    }

    return wcscpy(uri, remoteURI);
}



/*
 * Sets the mime type standard for the source items
 *
 * @param mimeType the mime type
 */
void SyncSource::setType(wchar_t* mimeType) {
    wcsncpy(type, (mimeType == NULL) ? TEXT("") : mimeType, DIM_MIME_TYPE);
    type[DIM_MIME_TYPE-1] = 0;
}

/*
 * Returns the items data mime type. If type is NULL, the pointer to the
 * internal buffer is returned, otherwise the value is copied in the
 * given buffer, which is also returned to the caller.
 *
 * @param mimeType the buffer where to copy the mime type value
 */
wchar_t* SyncSource::getType(wchar_t* mimeType) {
    if (mimeType == NULL) {
        return type;
    }

    return wcscpy(mimeType, type);
}


/*
 * Sets the server imposed synchronization mode for the SyncSource.
 *
 * @param syncMode - sync synchronization mode
 */
void SyncSource::setSyncMode(SyncMode mode) {
    syncMode = mode;
}

/*
 * Returns the synchronization mode.
 */
SyncMode SyncSource::getSyncMode() {
    return syncMode;
}

/*
 * Ends the synchronization of the specified source. If source contains
 * LUIG-GUID mapping this is sent to the server. It returns 0 in case
 * of success or an error code in case of error (see SPDS Error Codes).
 *
 * @param source - the SyncSource to sync
 */
int SyncSource::endSync(SyncSource& source) {
    //
    // TBD
    //
    return 0;
}

/*
 * Returns the timestamp in milliseconds of the last synchronization.
 * The reference time of the timestamp is platform specific.
 */
unsigned long SyncSource::getLastSync() {
    return lastSync;
}

/*
 * Sets the timestamp in millisencods of the lasr synchronization.
 * The reference time of the timestamp is platform specific.
 */
void SyncSource::setLastSync(unsigned long timestamp) {
    lastSync = timestamp;
}

/*
 * Sets the last anchor associated to the source
 *
 * @param lastAnchor last anchor
 */
void SyncSource::setLastAnchor(wchar_t* lastAnchor) {
    wcsncpy(last, (lastAnchor != NULL) ? lastAnchor : TEXT(""), DIM_ANCHOR);
}

/*
 * Gets the last anchor associated to the source. If last is NULL the
 * internal buffer address is returned, otherwise the value is copied
 * in the given buffer and the buffer address is returned.
 */
wchar_t* SyncSource::getLastAnchor(wchar_t* lastAnchor) {
    if (lastAnchor == NULL) {
        return last;
    }

    return wcscpy(lastAnchor, last);
}

/*
 * Sets the next anchor associated to the source
 *
 * @param next next anchor
 */
void SyncSource::setNextAnchor(wchar_t* nextAnchor) {
    wcsncpy(next, (nextAnchor != NULL) ? nextAnchor : TEXT(""), DIM_ANCHOR);
    next[DIM_ANCHOR-1] = 0;
}

/*
 * Gets the next anchor associated to the source. If last is NULL the
 * internal buffer address is returned, otherwise the value is copied
 * in the given buffer and the buffer address is returned.
 */
wchar_t* SyncSource::getNextAnchor(wchar_t* nextAnchor) {
    if (nextAnchor == NULL) {
        return next;
    }

    return wcscpy(nextAnchor, next);
}

/*
 * Sets all the items stored in the data source. For performance
 * reasons, this should only be set when required (for example in case
 * of slow or refresh sync). The items are passed to the SyncSource as
 * an array of SyncItem objects.
 * The pointer to the array is stored internally, therefore that buffer must
 * stay available for all the lifespam of the SyncSource.
 *
 * @param items - the items to set
 * @param n - how many item
 */
void SyncSource::setAllSyncItems(SyncItem* items[], unsigned int n) {
    if ((items == NULL) || (n < 0)) {
        lastErrorCode = ERR_WRONG_PARAMETERS;
        setErrorMsg( ERR_WRONG_PARAMETERS, 
                     TEXT("Wrong parameters - items: %p, n: %d"), items, n);
        return;
    }

    allItems      = items;
    allItemsCount =     n;

    //
    // We set the state to synchronized, just in case they are not
    //
    for (unsigned int i=0; i<n; ++i) {
        allItems[i]->setState(SYNC_STATE_NONE);
    }
}


/*
 * Sets the items deleted after the last synchronization. The items
 * are passed to the SyncSource as an array of SyncItem objects.
 * The pointer to the array is stored internally, therefore that buffer must
 * stay available for all the lifespam of the SyncSource.
 *
 * @param items - the items to set
 * @param n - how many item
 */
void SyncSource::setDeletedSyncItems(SyncItem* items[], unsigned int n) {
    if ((items == NULL) || (n < 0)) {
        lastErrorCode = ERR_WRONG_PARAMETERS;
        setErrorMsg( ERR_WRONG_PARAMETERS,
                     TEXT("Wrong parameters - items: %p, n: %d"), items, n);
        return;
    }

    deletedItems      = items;
    deletedItemsCount =     n;

    //
    // We set the state to deleted, just in case they are not
    //
    for (unsigned int i=0; i<n; ++i) {
        deletedItems[i]->setState(SYNC_STATE_DELETED);
    }
}

/*
 * Sets the items added after the last synchronization. The items
 * are passed to the SyncSource as an array of SyncItem objects.
 * The pointer to the array is stored internally, therefore that buffer must
 * stay available for all the lifespam of the SyncSource.
 *
 * @param items - the items to set
 * @param n - how many item
 */
void SyncSource::setNewSyncItems(SyncItem* items[], unsigned int n) {
    if ((items == NULL) || (n < 0)) {
        setErrorMsg( ERR_WRONG_PARAMETERS, 
                     TEXT("Wrong parameters - items: %p, n: %d"), items, n);
        return;
    }

    newItems      = items;
    newItemsCount =     n;

    //
    // We set the state to new, just in case they are not
    //
    for (unsigned int i=0; i<n; ++i) {
        newItems[i]->setState(SYNC_STATE_NEW);
    }
}

/*
 * Sets the items updated after the last synchronization. The items
 * are passed to the SyncSource as an array of SyncItem objects.
 * The pointer to the array is stored internally, therefore that buffer must
 * stay available for all the lifespam of the SyncSource.
 *
 * @param items - the items to set
 * @param n - how many item
 */
void SyncSource::setUpdatedSyncItems(SyncItem* items[], unsigned int n){
    if ((items == NULL) || (n < 0)) {
        setErrorMsg( ERR_WRONG_PARAMETERS,
                     TEXT("Wrong parameters - items: %p, n: %d"), items, n);
        return;
    }

    updatedItems      = items;
    updatedItemsCount =     n;


    //
    // We set the state to updated, just in case they are not
    //
    for (unsigned int i=0; i<n; ++i) {
        updatedItems[i]->setState(SYNC_STATE_UPDATED);
    }
}

/*
 * Sets the LUID-GUID mapping of the last synchronization.
 * The pointer to the array is stored internally, therefore that buffer must
 * stay available for all the lifespam of the SyncSource.
 *
 * @param mapping - the LUIDs-GUIDs mapping
 */
void SyncSource::setLUIDGUIDMapping(SyncMap* mappings[], unsigned int n){
    if ((mappings == NULL) || (n < 0)) {
        setErrorMsg( ERR_WRONG_PARAMETERS,
                     TEXT("Wrong parameters - items: %p, n: %d"), mappings, n);
        return;
    }

    map     = mappings;
    mapSize = n       ;
}


void SyncSource::setSyncItemStatus(SyncItemStatus* items[], unsigned int n){
    if ((items == NULL) || (n < 0)) {
        setErrorMsg( ERR_WRONG_PARAMETERS,
                     TEXT("Wrong parameters - items: %p, n: %d"), items, n);
        return;
    }

    itemStatus      = items;
    itemStatusCount =     n;
  
}


/*
 * Returns all items stored in the data source.
 */
SyncItem** SyncSource::getAllSyncItems() {
    return allItems;
}

/*
 * Returns the number of items in allItems
 */
unsigned int SyncSource::getAllSyncItemsCount() {
    return allItemsCount;
}

/*
 * Returns the new items buffer.
 */
SyncItem** SyncSource::getNewSyncItems() {
    return newItems;
}

/*
 * Returns the number of items in newItems
 */
unsigned int SyncSource::getNewSyncItemsCount() {
    return newItemsCount;
}


/*
 * Returns the updated items buffer.
 */
SyncItem** SyncSource::getUpdatedSyncItems() {
    return updatedItems;
}

/*
 * Returns the number of items in updatedItems
 */
unsigned int SyncSource::getUpdatedSyncItemsCount() {
    return updatedItemsCount;
}

/*
 * Returns the deleted items buffer.
 */
SyncItem** SyncSource::getDeletedSyncItems() {
    return deletedItems;
}

/*
 * Returns the number of items in deletedItems
 */
unsigned int SyncSource::getDeletedSyncItemsCount() {
    return deletedItemsCount;
}

/*
 * Returns the LUID-GUID mapping buffer
 */
SyncMap** SyncSource::getLUIDGUIDMapping() {
    return map;
}

/*
 * Returns the number of magging stored in the LUID-GUID mapping
 */
unsigned int SyncSource::getMapSize() {
    return mapSize;
}

/*
 * Returns the SyncItemStatus buffer.
 */
SyncItemStatus**SyncSource:: getSyncItemStatus() {
    return itemStatus;
}

/*
 * Returns the number of itemsStatus in itemsStatus
 */
unsigned int SyncSource::getSyncItemStatusCount(){
    return itemStatusCount;
}

