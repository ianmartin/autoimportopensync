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

#ifndef INCL_SYNC_CONFIG
#define INCL_SYNC_CONFIG

#include "http/common/Constants.h"
#include "spds/common/Constants.h"
#include "spds/common/SyncSourceConfig.h"

#include "spdm/common/Constants.h"

#define PROPERTY_USERNAME              TEXT("username"         )
#define PROPERTY_PASSWORD              TEXT("password"         )
#define PROPERTY_DEVICE_ID             TEXT("deviceId"         )
#define PROPERTY_FIRST_TIME_SYNC_MODE  TEXT("firstTimeSyncMode")
#define PROPERTY_USE_PROXY             TEXT("useProxy"         )
#define PROPERTY_PROXY_HOST            TEXT("proxyHost"        )
#define PROPERTY_PROXY_PORT            TEXT("proxyPort"        )
#define PROPERTY_SERVER_NAME           TEXT("serverName"       )
#define PROPERTY_SYNC_URL              TEXT("syncUrl"          )
#define PROPERTY_SYNC_BEGIN            TEXT("begin"            )
#define PROPERTY_SYNC_END              TEXT("end"              )
#define PROPERTY_SOURCE_NAME           TEXT("name"             )
#define PROPERTY_SOURCE_URI            TEXT("uri"              )
#define PROPERTY_SOURCE_SYNC_MODES     TEXT("syncModes"        )
#define PROPERTY_SOURCE_TYPE           TEXT("type"             )
#define PROPERTY_SOURCE_SYNC           TEXT("sync"             )
#define PROPERTY_SOURCE_LAST_SYNC      TEXT("last"             )
#define PROPERTY_SOURCE_DIR            TEXT("dir"              )
#define PROPERTY_CHECK_CONNECTION      TEXT("checkConn"        )
#define PROPERTY_RESPONSE_TIMEOUT      TEXT("responseTimeout"  )

#define DIRTY_USERNAME             0x0001
#define DIRTY_PASSWORD             0x0002
#define DIRTY_DEVICE_ID            0x0004
#define DIRTY_FIRST_TIME_SYNC_MODE 0x0008
#define DIRTY_USE_PROXY            0x0010
#define DIRTY_PROXY_HOST           0x0020
#define DIRTY_PROXY_PORT           0x0040
#define DIRTY_SERVER_NAME          0x0080
#define DIRTY_SYNC_URL             0x0100
#define DIRTY_SYNC_BEGIN           0x0200
#define DIRTY_SYNC_END             0x0400
#define DIRTY_SYNC_SOURCE          0x0800


#define DIM_SYNC_SOURCES 10

    class __declspec(dllexport) Config {

    private:

        wchar_t username   [DIM_USERNAME       ];
        wchar_t password   [DIM_PASSWORD       ];
        wchar_t deviceId   [DIM_DEVICE_ID      ];
        wchar_t proxyHost  [DIM_HOSTNAME       ];
        wchar_t serverName [DIM_SERVERNAME     ];
        wchar_t syncURL    [DIM_URL            ];
        wchar_t rootContext[DIM_MANAGEMENT_PATH];

        SyncSourceConfig sources[DIM_SYNC_SOURCES];

        BOOL useProxy, checkConn;

        int proxyPort;
        SyncMode firstTimeSyncMode;
        int sourceCount;
        unsigned long beginTimestamp;
        unsigned long endTimestamp;
        unsigned int dirty;
        unsigned int responseTimeout;

    public:
        Config();
        Config(wchar_t* root);

        wchar_t* getUsername();
        void setUsername(wchar_t* username);

        wchar_t* getPassword();
        void setPassword(wchar_t* password);

        wchar_t* getDeviceId();
        void setDeviceId(wchar_t* deviceId);

        SyncMode getFirstTimeSyncMode();
        void setFirstTimeSyncMode(SyncMode syncMode);

        BOOL getUseProxy();
        void setUseProxy(BOOL useProxy);

        wchar_t* getProxyHost();
        void setProxyHost(wchar_t* proxyHost);

        int getProxyPort();
        void setProxyPort(int proxyPort);

        wchar_t* getServerName();
        void setServerName(wchar_t* serverName);

        wchar_t* getSyncURL();
        void setSyncURL(wchar_t* url);

        void setSyncSourceCount(int n);
        int getSyncSourceCount();

        void setBeginSync(unsigned long timestamp);
        unsigned long getBeginSync();

        void setEndSync(unsigned long timestamp);
        unsigned long getEndSync();

        SyncSourceConfig& getSyncSourceConfig(int pos);
        BOOL getSyncSourceConfig(const wchar_t* name, SyncSourceConfig& sc);
        BOOL setSyncSourceConfig(SyncSourceConfig& sc);
        
        BOOL getCheckConn();
        void setCheckConn(BOOL checkConn);

        void setResponseTimeout(unsigned int responseTimeout);
        unsigned int getResponseTimeout();

        /*
         * Reads the configuration from the device manager
         */
        BOOL readFromDM();

        /*
         * Stores the changed information into the device manager.
         * It returns TRUE if the operation was successfull, FALSE otherwise.
         *
         * NOTE: this methods must be mainly developed. For now it stores the sources
         *       last timestamp only. Is TBD to trace which fields are changed and
         *       persists them in the DM as well.
         */
        BOOL saveToDM();

        /*
         * Is the dirty flag set?
         */
        BOOL isDirty();

    };
#endif


