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

#include "syncml-client.h"

#ifdef LINUX
#define APPLICATION_URI TEXT("/apps/syncml-client")
#else
#define APPLICATION_URI TEXT("Funambol/examples/dummy")
#endif


#define MAP_SIZE            50
#define ALL_ITEMS_COUNT      4
#define DELETED_ITEMS_COUNT  1
#define UPDATED_ITEMS_COUNT  2
#define NEW_ITEMS_COUNT      1

static SyncMap*  mappings     [MAP_SIZE           ];
static SyncItem* newItems     [NEW_ITEMS_COUNT    ];
static SyncItem* updatedItems [UPDATED_ITEMS_COUNT];
static SyncItem* deletedItems [DELETED_ITEMS_COUNT];
static SyncItem* allItems     [ALL_ITEMS_COUNT    ];


void error() {
    swprintf(logmsg, DIM_LOG_MESSAGE, 
             TEXT("client side - Error!\nerror code: %d\nerror message: %ls"), 
             lastErrorCode, lastErrorMsg);
    LOG.error(logmsg);
}

void setAllItems(SyncSource& s) {
    allItems[0] = new SyncItem(TEXT("item1"));
    allItems[1] = new SyncItem(TEXT("item2"));
    allItems[2] = new SyncItem(TEXT("item3"));
    allItems[3] = new SyncItem(TEXT("item4"));

    //
    // NOTE: keep into account the terminator
    //
    allItems[0]->setData((void*)TEXT("This is item One")  , 17*sizeof(wchar_t));
    allItems[1]->setData((void*)TEXT("This is item Two")  , 17*sizeof(wchar_t));
    allItems[2]->setData((void*)TEXT("This is item Three"), 19*sizeof(wchar_t));
    allItems[3]->setData((void*)TEXT("This is item Four") , 18*sizeof(wchar_t));

    s.setAllSyncItems(allItems, 4);
}

void setModifiedItems(SyncSource& s) {
    newItems    [0] = new SyncItem(TEXT("item4"));
    deletedItems[0] = new SyncItem(TEXT("item5"));
    updatedItems[0] = new SyncItem(TEXT("item1"));
    updatedItems[1] = new SyncItem(TEXT("item3"));

    newItems[0]->setData((void*)TEXT("This is a new item Four")  , 24*sizeof(wchar_t));
    updatedItems[0]->setData((void*)TEXT("This is the updated item One")  , 29*sizeof(wchar_t));
    updatedItems[1]->setData((void*)TEXT("This is the updated item Three"), 31*sizeof(wchar_t));

    s.setNewSyncItems(newItems,         NEW_ITEMS_COUNT    );
    s.setDeletedSyncItems(deletedItems, DELETED_ITEMS_COUNT);
    s.setUpdatedSyncItems(updatedItems, UPDATED_ITEMS_COUNT);
}

void displayItems(SyncSource& s) {
    unsigned int n = 0, i = 0;
    SyncItem** items = NULL;

    n = s.getNewSyncItemsCount();
    items = s.getNewSyncItems();
        
    LOG.debug(TEXT("client side - New items"));
    LOG.debug(TEXT("client side - ========="));
    for (i=0; i<n; ++i) {
        swprintf(logmsg, DIM_LOG_MESSAGE, 
                 TEXT("client side - key: %ls, data: %ls"), items[i]->getKey(NULL), 
                 items[i]->getData() );
        LOG.info(logmsg);
    }
    

    n = s.getUpdatedSyncItemsCount();
    items = s.getUpdatedSyncItems();
    
    
    LOG.debug(TEXT("client side - Updated items"));
    LOG.debug(TEXT("client side - ============="));
    for (i=0; i<n; ++i) {
        swprintf(logmsg, DIM_LOG_MESSAGE,
                TEXT("client side - key: %ls, data: %ls"), 
                items[i]->getKey(NULL), items[i]->getData());
        LOG.info(logmsg);
    }
    
    n = s.getDeletedSyncItemsCount();
    items = s.getDeletedSyncItems();

    
    LOG.debug(TEXT("client side - Deleted items"));
    LOG.debug(TEXT("client side - ============="));
    for (i=0; i<n; ++i) {
        swprintf(logmsg, DIM_LOG_MESSAGE, 
                 TEXT("client side - key: %ls"), 
                 items[i]->getKey(NULL), items[i]->getData());
        LOG.info(logmsg);
    }
    

}

void displayMappings(SyncSource& s) {
    unsigned int n = 0;
    SyncMap** map = NULL;

    n = s.getMapSize();
    map = s.getLUIDGUIDMapping();
    
    LOG.debug(TEXT("client side - Mappings"));
    LOG.debug(TEXT("client side - ========"));
    for (unsigned int i=0; i<n; ++i) {
        swprintf(logmsg, DIM_LOG_MESSAGE,
                 TEXT("client side - luid: %ls, guid: %ls"), 
                 map[i]->getLUID(NULL), map[i]->getGUID(NULL));
        LOG.info(logmsg);
    }
    
}

void displayItemStatus(SyncSource& s) {
    unsigned int n = 0;
    SyncItemStatus** itemStatus = NULL;

    n = s.getSyncItemStatusCount();
    itemStatus = s.getSyncItemStatus();
    
    LOG.debug(TEXT("client side - Item Status"));
    LOG.debug(TEXT("client side - ========"));
    for (unsigned int i=0; i<n; ++i) {
        swprintf(logmsg, DIM_LOG_MESSAGE,
                 TEXT("client side - key: %ls, data: %i, cmd: %ls, cmdRef: %i, msgRef: %i, cmdID:%i"), 
                        itemStatus[i]->getKey(NULL), 
                        itemStatus[i]->getData  () ,
                        itemStatus[i]->getCmd(NULL), 
                        itemStatus[i]->getCmdRef() ,
                        itemStatus[i]->getMsgRef() , 
                        itemStatus[i]->getCmdID ());
        LOG.debug(logmsg);
    }
    
}

void setMappings(SyncSource& s) {
    //
    // For the purpose of this example, LUIDs are created prepending
    // the string "C - " to the GUIDs
    //
    unsigned int n = 0;
    wchar_t luid[DIM_KEY];
    SyncItem** items = NULL;

    n = s.getNewSyncItemsCount();
    items = s.getNewSyncItems();

    for (unsigned int i = 0; i<n; ++i) {
        swprintf(luid, DIM_KEY, 
                 TEXT("C - %ls"), items[i]->getKey(NULL));
        mappings[i] = new SyncMap(items[i]->getKey(NULL), luid);
    }

    s.setLUIDGUIDMapping(mappings, n);
}

#ifdef _WIN32_WCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd ) {
#else
int main(int argc, char** argv) {
#endif
    unsigned int n = 0, i = 0, ret = 0;

    lastErrorCode = 0;
    char* msg = "<init OK>";
    mbstowcs( lastErrorMsg, msg, strlen( msg ) + 1 );
        
    Log::debugEnabled = TRUE;
    Log::infoEnabled  = TRUE;
        
    SyncManagerFactory factory = SyncManagerFactory();
    SyncManager* syncManager = factory.getSyncManager(APPLICATION_URI);
    SyncSource source = SyncSource(TEXT("card"));    
       
    
    if (syncManager == NULL) {
        error();
        goto finally;
    }

    //
    // Initializations
    //
    for (i=0; i<10; ++i) {
        mappings[i] = NULL;
    }
        
    ret = syncManager->prepareSync(source);

    if (ret != 0) {
        switch (ret) {
            case ERR_PROTOCOL_ERROR:
                LOG.error(TEXT("Protocol error."));
                break;

            case ERR_AUTH_NOT_AUTHORIZED:
            case ERR_AUTH_REQUIRED:
                LOG.error(TEXT("Not authorized."));
                break;

            case ERR_AUTH_EXPIRED:
                LOG.error(TEXT("Account expired."));
                break;

            case ERR_SRV_FAULT:
                LOG.error(TEXT("Server error."));
                break;

            case ERR_NOT_FOUND:
                swprintf(logmsg, DIM_LOG_MESSAGE, 
                         TEXT("Server returned NOT FOUND for SyncSource %ls"), 
                         source.getName(NULL, 0));
                LOG.error(logmsg);
                break;

            default:
                error();
                break;
        }

        goto finally;
    }

    switch (source.getSyncMode()) {
        case SYNC_SLOW:
            setAllItems(source);
            break;

        case SYNC_TWO_WAY:
            setModifiedItems(source);
            break;

        default:
            break;
    }
    
    if (syncManager->sync(source) != 0) {
        error();
        goto finally;
    }
    displayItemStatus(source);
    displayItems(source);
    setMappings(source);
    displayMappings(source);
    
    if (syncManager->endSync(source) != 0) {
        error();
        goto finally;
    }

   lastErrorCode = 0;

finally:    

    //
    // Release initial items
    //
    safeDelete((void**)allItems, ALL_ITEMS_COUNT);
    safeDelete((void**)deletedItems, DELETED_ITEMS_COUNT);
    safeDelete((void**)updatedItems, UPDATED_ITEMS_COUNT);
    safeDelete((void**)newItems, NEW_ITEMS_COUNT);

    //
    // Release server items
    //

    SyncItem **p = NULL;
    p = source.getNewSyncItems();
    safeDelete((void**)p, source.getNewSyncItemsCount());
    if (p != NULL) {
        delete [] p;
    }
    p = source.getUpdatedSyncItems();
    safeDelete((void**)p, source.getUpdatedSyncItemsCount());
    if (p != NULL) {
        delete [] p;
    }
    p = source.getDeletedSyncItems();
    safeDelete((void**)p, source.getDeletedSyncItemsCount());
    if (p != NULL) {
        delete [] p;
    }
    
    n = source.getSyncItemStatusCount();
    safeDelete((void**)source.getSyncItemStatus(), n);

    //
    // Release mapping
    //
    n = source.getMapSize();
    safeDelete((void**)mappings, n);

    delete syncManager;    

    return lastErrorCode;
 }

