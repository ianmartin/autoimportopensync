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

 #ifndef INCL_SYNC_SOURCE
    #define INCL_SYNC_SOURCE

    #include "common/fscapi.h"
    #include "spds/common/Constants.h"
    #include "spds/common/SyncItem.h"
    #include "spds/common/SyncItemStatus.h"
    #include "spds/common/SyncMap.h"

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
    class /*__declspec(dllexport)*/ SyncSource {

    private:
        SyncMode   syncMode;
        unsigned long  lastSync;
        wchar_t name[DIM_SOURCE_NAME];
        wchar_t type[DIM_MIME_TYPE];
        wchar_t next[DIM_ANCHOR];
        wchar_t last[DIM_ANCHOR];
        wchar_t remoteURI[DIM_REMOTE_URI];

        unsigned int allItemsCount, updatedItemsCount, newItemsCount, deletedItemsCount, mapSize, itemStatusCount;
        SyncItem** allItems;
        SyncItem** updatedItems;
        SyncItem** deletedItems;
        SyncItem** newItems;
        SyncMap**  map;
        SyncItemStatus** itemStatus;

    public:

        SyncSource(wchar_t* name);

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
         wchar_t *getName(wchar_t* name, int dim);

        
        void setRemoteURI(wchar_t* uri);

        wchar_t* getRemoteURI(wchar_t* uri);

        /*
         * Sets the preferred synchronization mode for the SyncSource.
         *
         * @param syncMode - sync synchronization mode
         */
        void setPreferredSyncMode(SyncMode syncMode);

        /*
         * Returns the preferred synchronization mode for the SyncSource
         */
        SyncMode getPreferredSyncMode();

        /*
         * Sets the server imposed synchronization mode for the SyncSource.
         *
         * @param syncMode - sync synchronization mode
         */
        void setSyncMode(SyncMode syncMode);


        /*
         * Returns the synchronization mode.
         */
        SyncMode getSyncMode();

        /*
         * Sets the mime type standard for the source items
         *
         * @param type the mime type
         */
        void setType(wchar_t* type);

        /*
         * Returns the items data mime type. If type is NULL, the pointer to the
         * internal buffer is returned, otherwise the value is copied in the
         * given buffer, which is also returned to the caller.
         */
        wchar_t* getType(wchar_t* type);


        /*
         * Ends the synchronization of the specified source. If source contains
         * LUIG-GUID mapping this is sent to the server. It returns 0 in case
         * of success or an error code in case of error (see SPDS Error Codes).
         *
         * @param source - the SyncSource to sync
         */
        int endSync(SyncSource& source);

        /*
         * Returns the timestamp in milliseconds of the last synchronization.
         * The reference time of the timestamp is platform specific.
         */
        unsigned long getLastSync();

        /*
         * Sets the timestamp in millisencods of the lasr synchronization.
         * The reference time of the timestamp is platform specific.
         */
        void setLastSync(unsigned long timestamp);

        /*
         * Sets the last anchor associated to the source
         *
         * @param last last anchor
         */
        void setLastAnchor(wchar_t* last);

        /*
         * Gets the last anchor associated to the source. If last is NULL the
         * internal buffer address is returned, otherwise the value is copied
         * in the given buffer and the buffer address is returned.
         */
        wchar_t* getLastAnchor(wchar_t* last);

        /*
         * Sets the next anchor associated to the source
         *
         * @param next next anchor
         */
        void setNextAnchor(wchar_t* next);

        /*
         * Gets the next anchor associated to the source. If last is NULL the
         * internal buffer address is returned, otherwise the value is copied
         * in the given buffer and the buffer address is returned.
         */
        wchar_t* getNextAnchor(wchar_t* next);

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
        void setAllSyncItems(SyncItem* items[], unsigned int n);

        /*
         * Sets the items deleted after the last synchronization. The items
         * are passed to the SyncSource as an array of SyncItem objects.
         * The pointer to the array is stored internally, therefore that buffer must
         * stay available for all the lifespam of the SyncSource.
         *
         * @param items - the items to set
         * @param n - how many item
         */
        void setDeletedSyncItems(SyncItem* items[], unsigned int n);

        /*
         * Sets the items added after the last synchronization. The items
         * are passed to the SyncSource as an array of SyncItem objects.
         * The pointer to the array is stored internally, therefore that buffer must
         * stay available for all the lifespam of the SyncSource.
         *
         * @param items - the items to set
         * @param n - how many item
         */
        void setNewSyncItems(SyncItem* items[], unsigned int n);

        /*
         * Sets the items updated after the last synchronization. The items
         * are passed to the SyncSource as an array of SyncItem objects.
         * The pointer to the array is stored internally, therefore that buffer must
         * stay available for all the lifespam of the SyncSource.
         *
         * @param items - the items to set
         * @param n - how many item
         */
        void setUpdatedSyncItems(SyncItem* items[], unsigned int n);

        /*
         * Sets the items status object after the server response. The items status 
         * are passed to the SyncSource as an array of SyncItemStatus objects.
         * The pointer to the array is stored internally, therefore that buffer must
         * stay available for all the lifespam of the SyncSource.
         *
         * @param itemsStatus - the itemsStatus to set
         * @param n - how many itemStatus object
         */
        void setSyncItemStatus(SyncItemStatus* itemStatus[], unsigned int n);

        /*
         * Sets the LUID-GUID mapping of the last synchronization.
         *
         * @param mapping - the LUIDs-GUIDs mapping
         */
        void setLUIDGUIDMapping(SyncMap* mappings[], unsigned int n);


        /*
         * Returns the LUID-GUID mapping buffer.
         */
        SyncMap** getLUIDGUIDMapping();


        /*
         * Returns all items buffer.
         */
        SyncItem** getAllSyncItems();

        /*
         * Returns the number of items in allItems
         */
        unsigned int SyncSource::getAllSyncItemsCount();

        /*
         * Returns the 'new' items buffer.
         */
        SyncItem** getNewSyncItems();

        /*
         * Returns the number of items in NewItems
         */
        unsigned int SyncSource::getNewSyncItemsCount();

        /*
         * Returns the 'updated' items buffer.
         */
        SyncItem** getUpdatedSyncItems();

        /*
         * Returns the number of items in updatedItems
         */
        unsigned int SyncSource::getUpdatedSyncItemsCount();

        /*
         * Returns the 'deleted' items buffer.
         */
        SyncItem** getDeletedSyncItems();

        /*
         * Returns the number of items in deletedItems
         */
        unsigned int SyncSource::getDeletedSyncItemsCount();
        
        /*
         * Returns the SyncItemStatus buffer.
         */
        SyncItemStatus** getSyncItemStatus();

        /*
         * Returns the number of itemsStatus in itemsStatus
         */
        unsigned int SyncSource::getSyncItemStatusCount();

        /*
         * Returns the number of magging stored in the LUID-GUID mapping
         */
        unsigned int getMapSize();

        /*
         * Releases the items stored in all, deleted, updated, new and map arrays.
         */
        void clean();
    };

#endif