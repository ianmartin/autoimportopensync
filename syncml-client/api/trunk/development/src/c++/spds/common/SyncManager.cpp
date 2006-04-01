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
// @version $Id: SyncManager.cpp,v 1.21 2005/03/30 10:12:51 magi Exp $
//

#include "unixdefs.h"
#include "common/fscapi.h"
#include "common/Log.h"

#include "spds/common/Config.h"
#include "spds/common/Constants.h"
#include "spds/common/SyncManager.h"
#include "spds/common/SyncMLBuilder.h"
#include "spds/common/SyncMLProcessor.h"
#include "spds/common/Utils.h"
#include "http/common/TransportAgent.h"

extern const wchar_t* xmlInit; // TBR

/*
 * Constructor
 */
SyncManager::SyncManager(Config* configuration) {
        
    config = configuration;

    syncMLBuilder = new SyncMLBuilder(config->getServerName(),
                                      config->getUsername  (),
                                      config->getPassword  (),
                                      config->getDeviceId  ());

    syncMLProcessor = new SyncMLProcessor();

    transportAgent = NULL;

}

/*
 * Destructor
 */
SyncManager::~SyncManager() {
    if (syncMLBuilder != NULL) {
        delete syncMLBuilder;
    }

    if (syncMLProcessor != NULL) {
        delete syncMLProcessor;
    }

    if (transportAgent != NULL) {
        delete transportAgent;
    }

    if (config != NULL) {
        delete config;
    }
}

/*
 * Initializes a new synchronization session for the specified sync source.
 * It returns 0 in case of success, an error code in case of error
 * (see SPDS Error Codes).
 *
 * @param source - the SyncSource to synchronize - NOT NULLS
 */
int SyncManager::prepareSync(SyncSource& source) {
    wchar_t* initMsg     = NULL;
    wchar_t* responseMsg = NULL;
    wchar_t* respURI     = NULL;

    int ret = 0;

    URL url = URL(config->getSyncURL());
    Proxy proxy = Proxy();

    //
    // Set the begin timestamp
    //
    config->setBeginSync(time(NULL));

    if (readSyncSourceDefinition(source) == FALSE) {
        ret = lastErrorCode = ERR_SOURCE_DEFINITION_NOT_FOUND;
        swprintf(
            lastErrorMsg,
            DIM_ERROR_MESSAGE, 
            TEXT("The source definition for the source named %ls was not found"),
            source.getName(NULL, 0)
        );
        LOG.error(lastErrorMsg);
        goto finally;
    } 
    
    if (source.getSyncMode() == SYNC_NONE) {
        
        LOG.setMessage( TEXT("No source %s to synchronize"), source.getName(NULL, 0) );
        LOG.info(logmsg);
        goto finally;
    }

    //
    // Creates the initialization message
    //
    initMsg = syncMLBuilder->prepareInitMsg(source);
    if (initMsg == NULL) {
        ret = lastErrorCode;
        goto finally;
    }

    LOG.debug(TEXT("initialization-message:"));
    LOG.debug(initMsg);

    //
    // Creates the transport agent. This is cached in the instance variable
    // transportAgent so that the next phases of the sync can use the same
    // agent. Plus, the agent status is in this way preserved between the
    // multiple SyncML messages.
    //
///    transportAgent = new TransportAgent(url, proxy, config->getCheckConn(), config->getResponseTimeout());
    transportAgent = new TransportAgent(url, proxy, config->getCheckConn(), config->getResponseTimeout());

    if (lastErrorCode != 0) {
        ret = lastErrorCode;
        goto finally;
    }

    responseMsg = transportAgent->sendMessage(initMsg);
    if (responseMsg == NULL) {
        ret = lastErrorCode;
        LOG.error(lastErrorMsg);
        goto finally;
    }
    LOG.info(INITIALIZATION_DONE);

    LOG.debug(TEXT("response to inizialization-message:"));
    LOG.debug(responseMsg);

    ret = syncMLProcessor->processInitResponse(source, responseMsg);
    if (ret == -1) {
        ret = lastErrorCode;
        goto finally;
    }
    
    LOG.setMessage( SERVER_ALERT_CODE, source.getRemoteURI(NULL), source.getSyncMode() );
    LOG.info(logmsg); 

    respURI = syncMLProcessor->getRespURI(responseMsg);
    if (respURI != NULL) {
        url = URL(respURI);
        transportAgent->setURL(url);
    }    
    LOG.setMessage( RESPURI, respURI); 
    LOG.debug(logmsg); 

finally:

    safeDelete(&initMsg    );
    transportAgent->releaseResponseBuffer();
//    safeDelete(&responseMsg);
    safeDelete(&respURI    );

    return ret;
}


/*
 * Synchronizes the specified source with the server. source should be
 * filled with the client-side modified items. At the end of the
 * process source will be fed with the items sent by the server.
 * It returns 0 in case of success or an error code in case of error
 * (see SPDS Error Codes).
 *
 * @param source - the SyncSource to sync
 */
int SyncManager::sync(SyncSource& source) {
    wchar_t* syncMsg     = NULL;    
    wchar_t* responseMsg = NULL;    
    int ret = 0;    

    LOG.setMessage( SYNCHRONIZING, source.getRemoteURI(NULL)); 
    LOG.info(logmsg); 
    
    switch (source.getSyncMode()) {
        case SYNC_SLOW:
            LOG.setMessage( PREPARING_SLOW_SYNC, source.getRemoteURI(NULL));  
            LOG.info(logmsg);
            LOG.setMessage( DETECTED_SLOW, source.getAllSyncItemsCount());  
            LOG.info(logmsg);
            
            break;

        case SYNC_REFRESH_FROM_SERVER:
            LOG.setMessage( PREPARING_SYNC_REFRESH_FROM_SERVER, source.getRemoteURI(NULL)); 
            LOG.info(logmsg);
            break;

        case SYNC_ONE_WAY_FROM_SERVER:
            LOG.setMessage( PREPARING_SLOW_SYNC, source.getRemoteURI(NULL)); 
            LOG.info(logmsg);
            break;

        default: // fast
            LOG.setMessage( PREPARING_FAST_SYNC, source.getRemoteURI(NULL)); 
            LOG.info(logmsg);
            LOG.setMessage( DETECTED_FAST, source.getNewSyncItemsCount(), source.getUpdatedSyncItemsCount(), source.getDeletedSyncItemsCount());    
            LOG.info(logmsg);
            break;
    }
    

    syncMsg = syncMLBuilder->prepareSyncMsg(source);    
    if (syncMsg == NULL) {                 
        ret = lastErrorCode;       
        goto finally;
    }
     
    LOG.debug(TEXT("modification-message:"));
    LOG.debug(syncMsg);
      
    LOG.info(SENDING_MODIFICATION);

    responseMsg = transportAgent->sendMessage(syncMsg);
    if (responseMsg == NULL) {
        ret = lastErrorCode;
        URL url = URL(config->getSyncURL()); 
        URL respURI = transportAgent->getURL(); 
        
        LOG.debug(url.host);
        LOG.debug(respURI.host);

        if (wcscmp(url.host, respURI.host) != 0 ||
            url.port != respURI.port) {
                wchar_t port [10];
               respURI.port = 0 ? 
                       swprintf(port, sizeof(port)/sizeof(wchar_t), TEXT("")) : 
                       swprintf(port, sizeof(port)/sizeof(wchar_t), 
                                TEXT(":%d"), respURI.port );
                ret = ERR_RESP_URI_DIFFERS_FROM_URL;
                swprintf(lastErrorMsg, 
                         DIM_ERROR_MESSAGE,
                         TEXT("Error connecting to: %ls://%ls%ls"), 
                         respURI.protocol, respURI.host, port );
        }        

        goto finally;
    }
    
    LOG.debug(TEXT("response to modification-message:"));
    LOG.debug(responseMsg);    

    ret = syncMLProcessor->processSyncResponse(source, responseMsg);
    if (ret == -1) {
        ret = lastErrorCode;
        goto finally;
    }
  
        
    if (syncMLProcessor->checkTagFinal(responseMsg)) {
        //
        // Final tag exists so every modification is sent by server into this message. 
        //
        LOG.setMessage( RETURNED_NUM_ITEMS, source.getNewSyncItemsCount(), 
                                            source.getUpdatedSyncItemsCount(), 
                                            source.getDeletedSyncItemsCount(),
                                            source.getRemoteURI(NULL));    
        LOG.info(logmsg);
        goto finally;
    }
    //
    // Send the alert 222 to the server to keep its modification
    //
    safeDelete(&syncMsg    );   
//    safeDelete(&responseMsg);
    transportAgent->releaseResponseBuffer();
    
    syncMsg = syncMLBuilder->prepareAlertMessage(source);    
    if (syncMsg == NULL) {                 
        ret = lastErrorCode;       
        goto finally;
    }
    
    LOG.info(SENDING_ALERT);
    LOG.debug(TEXT("alert-message:"));
    LOG.debug(syncMsg);
    
    responseMsg = transportAgent->sendMessage(syncMsg);
    
    //
    // process the response to get the item
    //
    
     if (responseMsg == NULL) {
        ret = lastErrorCode;
        swprintf(lastErrorMsg, sizeof( lastErrorMsg ) / sizeof( wchar_t ), TEXT("Error"));
        goto finally;
    }       

    LOG.debug(TEXT("response to alert-message:"));
    LOG.debug(responseMsg);
    
    ret = syncMLProcessor->processSyncResponse(source, responseMsg);
    if (ret == -1) {
        ret = lastErrorCode;
        goto finally;
    }
    LOG.setMessage( RETURNED_NUM_ITEMS, source.getNewSyncItemsCount(), 
                                            source.getUpdatedSyncItemsCount(), 
                                            source.getDeletedSyncItemsCount(),
                                            source.getRemoteURI(NULL));    
    LOG.info(logmsg);

finally:
    
    safeDelete(&syncMsg    );
    transportAgent->releaseResponseBuffer();
//    safeDelete(&responseMsg);    
    return ret;

}

/*
 * Ends the synchronization of the specified source. If source contains
 * LUIG-GUID mapping this is sent to the server. It returns 0 in case
 * of success or an error code in case of error (see SPDS Error Codes).
 *
 * @param source - the SyncSource to sync
 */
int SyncManager::endSync(SyncSource& source) {
   
    wchar_t* mapMsg      = NULL;
    wchar_t* responseMsg = NULL;
    int ret = 0;

    LOG.info(MODIFICATION_DONE);

    /*
    if (source.getMapSize() <= 0) {
        goto finally;
    }
    */
    mapMsg = syncMLBuilder->prepareMapMsg(source);
    if (mapMsg == NULL) {
        ret = lastErrorCode;
        goto finally;
    }

    LOG.debug(TEXT("mapping-message:"));
    LOG.debug(mapMsg);
    LOG.info(SENDING_MAPPING);
    
    responseMsg = transportAgent->sendMessage(mapMsg);
    if (responseMsg == NULL) {
        ret = lastErrorCode;
        goto finally;
    }

    LOG.debug(TEXT("response to mapping-message:"));
    LOG.debug(responseMsg);

    ret = syncMLProcessor->processMapResponse(source, responseMsg);
    
    if (ret == -1) {
        ret = lastErrorCode;
        goto finally;
    }

    commitChanges(source); // source level commit
    
    LOG.info(SYNCHRONIZATION_DONE);

finally:

	//commitChanges(source); // source level commit
    //
    // Sets the begin timestamp
    //
    config->setEndSync(time(NULL));
    commitSync(); // sync level commit
    safeDelete(&mapMsg     );
    transportAgent->releaseResponseBuffer();
//    safeDelete(&responseMsg);

    return ret;
}

// ------------------------------------------------------------- Private methids

BOOL SyncManager::readSyncSourceDefinition(SyncSource& source) {
    SyncSourceConfig ssc;
    wchar_t anchor[DIM_ANCHOR];

    if (config->getSyncSourceConfig(source.getName(NULL, 0), ssc) == FALSE) {
        return FALSE;
    }
    
    // syncMode setted only if value has no good value...
    if (source.getPreferredSyncMode() == (SyncMode)0 ||
        source.getPreferredSyncMode() < (SyncMode)0    ||
        source.getPreferredSyncMode() > (SyncMode)210      )  

        source.setPreferredSyncMode(syncModeCode(ssc.getSync(NULL)));


    source.setType(ssc.getType(NULL));

    unsigned long last = ssc.getLast();
    source.setLastSync( last );

    swprintf(anchor, sizeof( anchor ) / sizeof( wchar_t ), L"%u", last  );
    source.setLastAnchor(anchor);

    swprintf(anchor, sizeof( anchor ) / sizeof( wchar_t ), L"%lu", time(NULL) );
    source.setNextAnchor(anchor);

    source.setRemoteURI(ssc.getURI(NULL));

    return TRUE;
}

/*
 * Commits the configuration changes in the given SyncSource
 */
BOOL SyncManager::commitChanges(SyncSource& source) {
    SyncSourceConfig ssc;

    if (config->getSyncSourceConfig(source.getName(NULL, 0), ssc) == FALSE) {
        return FALSE;
    }

    ssc.setLast(config->getBeginSync());
    config->setSyncSourceConfig(ssc);

    return TRUE;
}

/*
 * Commits the synchronization process
 */
BOOL SyncManager::commitSync() {
    return config->saveToDM();
}
