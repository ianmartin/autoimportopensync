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
// @version $Id: Config.cpp,v 1.13 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include "spds/common/Config.h"
#include "common/Constants.h"

#include "spdm/common/DeviceManager.h"
#include "spdm/common/DeviceManagerFactory.h"

Config::Config() {
    useProxy = FALSE;
    proxyPort = 8080;
    
    wcscpy(rootContext, TEXT(""));
    wcscpy(username   , TEXT(""));
    wcscpy(password   , TEXT(""));
    wcscpy(deviceId   , TEXT(""));
    wcscpy(proxyHost  , TEXT(""));
    wcscpy(serverName , TEXT(""));
    wcscpy(syncURL    , TEXT(""));

    firstTimeSyncMode = SYNC_SLOW;

    dirty = 0;
    sourceCount = 0;
}

Config::Config(wchar_t* root) {
    wcsncpy(rootContext, root, DIM_MANAGEMENT_PATH);
    rootContext[DIM_MANAGEMENT_PATH-1] = 0;

    readFromDM();

    dirty = 0;
}

wchar_t* Config::getUsername() {
    return username;
}

void Config::setUsername(wchar_t* u){
    wcsncpy(username, u, DIM_USERNAME);
    username[DIM_USERNAME-1] = 0;
    dirty |= DIRTY_USERNAME;
}

wchar_t* Config::getPassword() {
    return password;
}

void Config::setPassword(wchar_t* p) {
    wcsncpy(password, p, DIM_PASSWORD);
    password[DIM_PASSWORD-1] = 0;
    dirty |= DIRTY_PASSWORD;
}


wchar_t* Config::getDeviceId() {
    return deviceId;
}

void Config::setDeviceId(wchar_t* d) {
    wcsncpy(deviceId, d, DIM_DEVICE_ID);
    deviceId[DIM_DEVICE_ID-1] = 0;
    dirty |= DIRTY_DEVICE_ID;
}

SyncMode Config::getFirstTimeSyncMode() {
    return firstTimeSyncMode;
}

void Config::setFirstTimeSyncMode(SyncMode s) {
    firstTimeSyncMode = s;
    dirty |= DIRTY_FIRST_TIME_SYNC_MODE;
}

BOOL Config::getUseProxy() {
    return useProxy;
}

void Config::setUseProxy(BOOL p) {
    useProxy = p;
    dirty |= DIRTY_USE_PROXY;
}

wchar_t* Config::getProxyHost() {
    return proxyHost;
}

void Config::setProxyHost(wchar_t* h) {
    wcsncpy(proxyHost, h, DIM_HOSTNAME);
    proxyHost[DIM_HOSTNAME-1] = 0;
    dirty |= DIRTY_PROXY_HOST;
}

int Config::getProxyPort() {
    return proxyPort;
}

void Config::setProxyPort(int p) {
    proxyPort = p;
    dirty |= DIRTY_PROXY_PORT;
}

wchar_t* Config::getServerName() {
    return serverName;
}

void Config::setServerName(wchar_t* s) {
    wcsncpy(serverName, s, DIM_SERVERNAME);
    serverName[DIM_SERVERNAME-1] = 0;
    dirty |= DIRTY_SERVER_NAME;
}

wchar_t* Config::getSyncURL() {
    return syncURL;
}

void Config::setSyncURL(wchar_t* u) {
    wcsncpy(syncURL, u, DIM_URL);
    syncURL[DIM_URL-1] = 0;
    dirty |= DIRTY_SYNC_URL;
}

void Config::setSyncSourceCount(int n) {
    sourceCount = n;
}

int Config::getSyncSourceCount() {
    return sourceCount;
}

void Config::setBeginSync(unsigned long timestamp) {
    beginTimestamp = timestamp;
    dirty |= DIRTY_SYNC_BEGIN;
}

unsigned long Config::getBeginSync() {
    return beginTimestamp;
}

void Config::setEndSync(unsigned long timestamp) {
    endTimestamp = timestamp;
    dirty |= DIRTY_SYNC_END;
}

unsigned long Config::getEndSync() {
    return endTimestamp;
}

BOOL Config::getCheckConn() {
    return checkConn;
}
void Config::setCheckConn(BOOL p) {
    checkConn = p;
}

void Config::setResponseTimeout(unsigned int resTimeout) {
    responseTimeout = resTimeout;
}

unsigned int Config::getResponseTimeout() {
    return responseTimeout;
}


SyncSourceConfig& Config::getSyncSourceConfig(int pos) {
    return sources[pos];
}

BOOL Config::getSyncSourceConfig(const wchar_t* name, SyncSourceConfig& sc) {
    int i=0;
    for (i=0; i<sourceCount; ++i) {
        if (wcsncmp(name, 
                    getSyncSourceConfig(i).getName(NULL), 
                    DIM_SOURCE_NAME) == 0) 
        {
            break;
        }
    }

    if (i >= sourceCount) {
        //
        // Not found!
        //
        return FALSE;
    }

    sc = sources[i];

    return TRUE;
}

BOOL Config::setSyncSourceConfig(SyncSourceConfig& sc) {
    int i=0;
    for (i=0; i<sourceCount; ++i) {
        if (wcsncmp(sc.getName(NULL), getSyncSourceConfig(i).getName(NULL), DIM_SOURCE_NAME) == 0) {
            break;
        }
    }

    if (i >= sourceCount) {
        //
        // Not found!
        //
        return FALSE;
    }

    sources[i] = sc;

    dirty |= DIRTY_SYNC_SOURCE;

    return TRUE;
}


BOOL Config::readFromDM() {
    const int VALUESIZE = 512;
    wchar_t context[DIM_MANAGEMENT_PATH];
    int nc = 0, ret = FALSE;

    //
    // Creates the DeviceManager object and extracts the relevant configuration
    // information.
    //
    DeviceManagerFactory factory = DeviceManagerFactory();
    DeviceManager* dm = factory.getDeviceManager();

    swprintf(context, DIM_MANAGEMENT_PATH,
             TEXT("%ls/%ls"), rootContext, CONTEXT_SPDS_SYNCML);
    ManagementNode* spdsConfig = dm->getManagementNode(context);
    if (spdsConfig == NULL) {
        lastErrorCode = ERR_INVALID_CONTEXT;
        swprintf(lastErrorMsg, DIM_ERROR_MESSAGE,
                 TEXT("SyncManager configuration not found: %ls"), context);
        goto finally;
    }

    wchar_t buf[VALUESIZE]; // big enough for everything

    //
    // Reads the configuration from the management node and fill
    // the config object. This will be passed to the SyncManager
    // constructor
    //
    spdsConfig->getPropertyValue(PROPERTY_USERNAME, buf, VALUESIZE);
    setUsername(buf); swprintf(buf, VALUESIZE, TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_PASSWORD, buf, VALUESIZE);
    setPassword(buf); swprintf(buf,VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_DEVICE_ID, buf, VALUESIZE);
    setDeviceId(buf); swprintf(buf, VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_FIRST_TIME_SYNC_MODE, buf, VALUESIZE);
    setFirstTimeSyncMode((SyncMode)wcstol(buf, NULL, 10)); 
    swprintf(buf, VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_USE_PROXY, buf, VALUESIZE);
    setUseProxy(wcstol(buf, NULL, 10)); 
    swprintf(buf, VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_PROXY_HOST, buf, VALUESIZE);
    setProxyHost(buf); 
    swprintf(buf,VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_PROXY_PORT, buf, VALUESIZE);
    setProxyPort(wcstol(buf, NULL, 10)); 
    swprintf(buf,  VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_SERVER_NAME, buf, VALUESIZE);
    setServerName(buf); 
    swprintf(buf, VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_SYNC_URL, buf, VALUESIZE);
    setSyncURL(buf); 
    swprintf(buf,  VALUESIZE,  TEXT(""));
    
    spdsConfig->getPropertyValue(PROPERTY_CHECK_CONNECTION, buf, VALUESIZE);
    setCheckConn(wcstol(buf, NULL, 10)); 
    swprintf(buf,  VALUESIZE,  TEXT(""));

    spdsConfig->getPropertyValue(PROPERTY_RESPONSE_TIMEOUT, buf, VALUESIZE);
    setResponseTimeout(wcstol(buf, NULL, 10)); 
    swprintf(buf,  VALUESIZE,  TEXT(""));

    delete spdsConfig; spdsConfig = NULL; 

    //
    // Now reads the SyncSource definitions. They are under the context
    // <appURI>/CONTEXT_SPDS_SOURCES
    //
    swprintf(context, DIM_MANAGEMENT_PATH,
             TEXT("%ls/%ls"), rootContext, CONTEXT_SPDS_SOURCES);
    spdsConfig = dm->getManagementNode(context);

    if (spdsConfig == NULL) {
        lastErrorCode = ERR_INVALID_CONTEXT;
        swprintf(lastErrorMsg, DIM_ERROR_MESSAGE,
                 TEXT("SyncManager configuration not found: %ls"), context);
        goto finally;
    }

    nc = spdsConfig->getChildrenCount();
    if (nc > 0) {
		lastErrorCode = 0;
        ManagementNode** children = new ManagementNode*[nc];
        memset( children, 0, nc * sizeof(  ManagementNode* ) );

        spdsConfig->getChildren(children, &nc);
		if (lastErrorCode) {
			goto finally;
		}

        setSyncSourceCount(nc);
        for (int i = 0; i<nc; ++i) {
            swprintf(buf, VALUESIZE,  
                     TEXT("%u"), getSyncSourceConfig(i).getLast());
            children[i]->getPropertyValue(PROPERTY_SOURCE_NAME, buf, VALUESIZE);
            getSyncSourceConfig(i).setName(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));                        

            children[i]->getPropertyValue(PROPERTY_SOURCE_URI, buf, VALUESIZE);
            getSyncSourceConfig(i).setURI(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));

            children[i]->getPropertyValue(PROPERTY_SOURCE_SYNC_MODES, buf, VALUESIZE);
            getSyncSourceConfig(i).setSyncModes(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));

            children[i]->getPropertyValue(PROPERTY_SOURCE_TYPE, buf, VALUESIZE);
            getSyncSourceConfig(i).setType(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));

            children[i]->getPropertyValue(PROPERTY_SOURCE_SYNC, buf, VALUESIZE);
            getSyncSourceConfig(i).setSync(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));

            children[i]->getPropertyValue(PROPERTY_SOURCE_DIR, buf, VALUESIZE);
            getSyncSourceConfig(i).setDir(buf); 
            swprintf(buf, VALUESIZE, TEXT(""));

            children[i]->getPropertyValue(PROPERTY_SOURCE_LAST_SYNC, buf, VALUESIZE);
            getSyncSourceConfig(i).setLast(wcstoul(buf, NULL, 10)); 
            swprintf(buf, VALUESIZE, TEXT(""));

            delete children[i];
        }
        delete[] children;
    }
    ret = TRUE;

finally:

	if (dm != NULL) {
		delete dm;
	}

	if (spdsConfig != NULL) {
		delete spdsConfig;
	}
	

    return ret;
}


/*
 * Stores the changed information into the device manager.
 * It returns TRUE if the operation was successfull, FALSE otherwise.
 *
 * NOTE: this methods must be mainly developed. For now it stores the sources
 *       last timestamp only. Is TBD to trace which fields are changed and
 *       persists them in the DM as well.
 */
BOOL Config::saveToDM() {
    const int VALUESIZE = 512;
    wchar_t context[DIM_MANAGEMENT_PATH];
    wchar_t buf[VALUESIZE]; // big enough for everything
    int nc = 0, ret = FALSE;

    //
    // If there is nothing to save, just do nothing
    //
    if (isDirty() == 0) {
        return TRUE;
    }

    //
    // Creates the DeviceManager object and extracts the relevant configuration
    // information.
    //
    DeviceManagerFactory factory = DeviceManagerFactory();
    DeviceManager* dm = factory.getDeviceManager();

    //
    // Writes the modified SyncMnager configuration fields
    //
    swprintf(context, sizeof( context )/sizeof( wchar_t ),
             TEXT("%ls/%ls"), rootContext, CONTEXT_SPDS_SYNCML);
    ManagementNode* spdsConfig = dm->getManagementNode(context);
    if (spdsConfig == NULL) {
        lastErrorCode = ERR_INVALID_CONTEXT;
        swprintf(lastErrorMsg, sizeof( lastErrorMsg ) / sizeof( wchar_t ),
                 TEXT("SyncManager configuration not found: %ls"), context);
        goto finally;
    }

    //
    // TBD: all the other properties
    //

    //
    // The synchronization bengin and end timestamps
    //
    if (dirty & DIRTY_SYNC_BEGIN) {
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),TEXT("%u"), getBeginSync());
        spdsConfig->setPropertyValue(PROPERTY_SYNC_BEGIN, buf);
    }

    if (dirty & DIRTY_SYNC_END) {
        swprintf(buf, sizeof(buf)/sizeof(wchar_t),TEXT("%u"), getEndSync());
        spdsConfig->setPropertyValue(PROPERTY_SYNC_END, buf);
    }

    delete spdsConfig; spdsConfig = NULL;
    
    //
    // Writes the last timestamps in the source definitions under
    // <appURI>/CONTEXT_SPDS_SOURCES
    //
    swprintf(context, sizeof(context)/sizeof(wchar_t),
            TEXT("%ls/%ls"), rootContext, CONTEXT_SPDS_SOURCES);
    spdsConfig = dm->getManagementNode(context);

    if (spdsConfig == NULL) {
        lastErrorCode = ERR_INVALID_CONTEXT;
        swprintf(lastErrorMsg, sizeof(lastErrorMsg)/sizeof(wchar_t),
                TEXT("SyncManager configuration not found: %ls"), context);
        goto finally;
    }

    nc = spdsConfig->getChildrenCount();
    if ((nc > 0) && (dirty & DIRTY_SYNC_SOURCE)) {
        ManagementNode** children = new ManagementNode*[nc];

        spdsConfig->getChildren(children, &nc);
        setSyncSourceCount(nc);

        for (int i = 0; i<nc; ++i) {
            swprintf(buf, sizeof(buf)/sizeof(wchar_t), 
                     TEXT("%u"), getSyncSourceConfig(i).getLast());
            children[i]->setPropertyValue(PROPERTY_SOURCE_LAST_SYNC, buf);

            swprintf(buf,  sizeof(buf)/sizeof(wchar_t), 
                     TEXT("%ls"), getSyncSourceConfig(i).getSync(NULL));
            children[i]->setPropertyValue(PROPERTY_SOURCE_SYNC, buf);
			
            swprintf(buf,sizeof(buf)/sizeof(wchar_t),  
                      TEXT("%ls"), getSyncSourceConfig(i).getSyncModes(NULL));
            children[i]->setPropertyValue(PROPERTY_SOURCE_SYNC_MODES, buf);

	    swprintf(buf, sizeof(buf)/sizeof(wchar_t), 
                     TEXT("%ls"), getSyncSourceConfig(i).getType(NULL));
            children[i]->setPropertyValue(PROPERTY_SOURCE_TYPE, buf);

            swprintf(buf, sizeof(buf)/sizeof(wchar_t), 
                     TEXT("%ls"), getSyncSourceConfig(i).getDir(NULL));
            children[i]->setPropertyValue(PROPERTY_SOURCE_DIR, buf);

            swprintf(buf,  sizeof(buf)/sizeof(wchar_t), 
                     TEXT("%ls"), getSyncSourceConfig(i).getURI(NULL));
            children[i]->setPropertyValue(PROPERTY_SOURCE_URI, buf);

            delete children[i];
        }

        delete [] children;
    }

    ret = TRUE;

finally:

	if (dm != NULL) {
		delete dm;
	}

	if (spdsConfig != NULL) {
		delete spdsConfig;
	}

    return ret;
}

/*
 * Is the dirty flag set?
 */
BOOL Config::isDirty() {
    return (dirty != 0);
}
