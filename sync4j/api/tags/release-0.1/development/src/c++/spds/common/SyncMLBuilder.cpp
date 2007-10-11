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
// @version $Id: SyncMLBuilder.cpp,v 1.21 2005/03/29 13:20:25 magi Exp $
//
#include "unixdefs.h"
#include <stdlib.h>

#include "common/Log.h"
#include "common/Constants.h"   // TBR
#include "spds/common/Constants.h"
#include "spds/common/SyncMLBuilder.h"

#include "spds/common/MessageTemplates.h"
#include "spdm/common/base64.h"
#include "spdm/common/Utils.h"
#include "spds/common/Utils.h"

USING_PTYPES

/*
 * Constructors
 */

SyncMLBuilder::SyncMLBuilder() {
    wcscpy(target,   TEXT(""));
    wcscpy(username, TEXT(""));
    wcscpy(password, TEXT(""));
    wcscpy(device,   TEXT(""));
}

SyncMLBuilder::SyncMLBuilder(wchar_t*      t  ,
                             wchar_t*      u,
                             wchar_t*      p,
                             wchar_t*      d) {
    wcsncpy(target,   t, DIM_SERVERNAME); target  [DIM_SERVERNAME-1] = 0;
    wcsncpy(username, u, DIM_USERNAME  ); username[DIM_USERNAME-1  ] = 0;
    wcsncpy(password, p, DIM_PASSWORD  ); password[DIM_PASSWORD-1  ] = 0;
    wcsncpy(device,   d, DIM_DEVICE_ID ); device  [DIM_DEVICE_ID-1 ] = 0;
}

/*
 * Creates and returns the initialization message for a given source.
 * The returned pointer points to a memory buffer allocated with
 * the new operator and must be discarded with the delete oprator.
 *
 * @param source - The SyncSource - NOT NULL
 *
 */
wchar_t* SyncMLBuilder::prepareInitMsg(SyncSource& source) {
    const unsigned int TmpBufSize = 512;
    unsigned int size;
    string alert = string();
    
    createAlert(source, alert);    

    //
    // Allocate anough memory for the entire message (alert
    // + xmlInit + info)
    //
    //---------- auth basic ------------//
    
    size = wcslen(xmlInitBasic)
         + length(alert)
         + 1024              // target + deviceid + username + password + source
         ;
    wchar_t* buf = new wchar_t[size];
    *buf = 0;
    
    wchar_t tmpBuf[TmpBufSize];
    swprintf(tmpBuf, TmpBufSize, TEXT("%ls:%ls"), username, password);

    // 
    // convert username:password into base64 
    // 

    char* loc = new_wcstombs(tmpBuf);
    int sizeArray = 4 * strlen(loc) + 1;
    char* base64  = new char[sizeArray];
    memset(base64, 0, sizeArray);

    encodeBase64(loc, base64, strlen(loc));    

    wchar_t *ptrContent = new_mbsrtowcs(base64); 
    swprintf(tmpBuf, TmpBufSize, TEXT("%ls"), ptrContent);
    
    swprintf(buf                     ,
             size                    ,
             xmlInitBasic            ,
             target                  ,
             device                  ,
             tmpBuf                  ,
             (const wchar_t*)alert   );
    
    delete[] loc; loc = NULL;
    delete[] base64 ; base64 = NULL;
    delete[] ptrContent; ptrContent = NULL;

    return buf;



    //----------- auth clear -----------//
    /*
    size = wcslen(xmlInit)
         + length(alert)
         + 1024              // target + deviceid + username + password + source
         ;
    wchar_t* buf = new wchar_t[size];
    *buf = 0;
    
    
    wsprintf(buf                     ,
             xmlInit                 ,
             target                  ,
             device                  ,
             username                ,
             password                ,
             (const wchar_t*)alert   );

    */
    
}

/*
 * Creates and returns the synchronization message for the given source.
 * The returned pointer points to a memory buffer allocated with
 * the new operator and must be discarded with the delete oprator.
 *
 * @param source the SyncSource to synchronize
 */
wchar_t* SyncMLBuilder::prepareSyncMsg(SyncSource& source) {
    wchar_t logmsg[512];

    SyncItem** items = NULL;
    unsigned int n = 0, i = 0;
    string statusTag = string();
    string syncTag = string();        
    wchar_t* buf      = NULL;
    unsigned int size = 0;

    switch (source.getSyncMode()) {
        case SYNC_SLOW:
            //
            // Slow Sync!
            //
            swprintf(logmsg, DIM_LOG_MESSAGE, 
                    TEXT("Preparing slow sync for %ls"), source.getName(NULL, 0) );
            LOG.debug(logmsg);

            n = source.getAllSyncItemsCount();
            
            items = source.getAllSyncItems();
            
            break;

        case SYNC_REFRESH_FROM_SERVER:
            //
            // Refresh from server
            //
            swprintf(logmsg,  DIM_LOG_MESSAGE,
                     TEXT("Preparing refresh for %ls"), source.getName(NULL, 0));
            LOG.debug(logmsg);
            items = new SyncItem*[0];

            break;

        case SYNC_ONE_WAY_FROM_SERVER:
            //
            // One-way sync from server: no needs of client modifications
            //
            swprintf(logmsg,  DIM_LOG_MESSAGE,
                     TEXT("Preparing one-way sync for %ls"), 
                     source.getName(NULL, 0));
            LOG.debug(logmsg);
            items = new SyncItem*[0];

            break;

        default:
            //
            // Fast Sync!
            //
            swprintf(logmsg, DIM_LOG_MESSAGE, 
                     TEXT("Preparing fast sync for %ls"), 
                     source.getName(NULL, 0));
            LOG.debug(logmsg);

            //
            // NOTE: filterRecordsForFastSync returns items in updated state
            //
            n = source.getNewSyncItemsCount()
              + source.getUpdatedSyncItemsCount()
              + source.getDeletedSyncItemsCount()
              ;

            items = new SyncItem*[n];

            i = source.getNewSyncItemsCount();
            memmove(items, source.getNewSyncItems(), i*sizeof(SyncItem*));
            n = i;
            i = source.getUpdatedSyncItemsCount();
            memmove(&items[n], source.getUpdatedSyncItems(), i*sizeof(SyncItem*));
            n += i;
            i = source.getDeletedSyncItemsCount();
            memmove(&items[n], source.getDeletedSyncItems(), i*sizeof(SyncItem*));
            n += i;

            break;
    }
    
    createSync(source, items, n, syncTag);
    if (lastErrorCode == ERR_NOT_ENOUGH_MEMORY) {            
        goto finally;
    }
    
    //
    // Calculate the size for the entire message
    //
    size = wcslen(xmlSync1)
                      + wcslen(xmlSync2)
                      + length(syncTag)
                      + 1024            // target + device + 2*source + timestamp
                      ;

    buf = new wchar_t[size];

    if (buf == NULL) {         
        lastErrorCode = ERR_NOT_ENOUGH_MEMORY;
        swprintf(lastErrorMsg, DIM_ERROR_MESSAGE, 
                 TEXT("Not enough memory (required %d bytes)"), size  );
        goto finally;
    }
    swprintf(buf                       ,
             size                      ,
             xmlSync1                  ,
             target                    ,
             device                    ,
             target                    ,
             device                    ,
             source.getRemoteURI(NULL)    , // source.getName(NULL, 0)   ,
             source.getName(NULL, 0)   ,
             source.getNextAnchor(NULL),
             source.getRemoteURI(NULL)    , //source.getName(NULL, 0)   ,
             source.getName(NULL, 0)   );
    wcscat(buf, (wchar_t*)syncTag);
    wcscat(buf, xmlSync2);
    
finally:    
    return buf;
}


/*
* Creates the message containing the alert 222 to request the other part of message.
*/
wchar_t* SyncMLBuilder::prepareAlertMessage(SyncSource& source) {
   
    wchar_t* ret = NULL;
    int size = 0;

    size = wcslen(xmlAlert) + 1024;
    ret = new wchar_t[size];
    swprintf(ret, size, 
                  xmlAlert, 
                  target,
                  device,
                  target,
                  device,
                  target,
                  device);
    return ret;
}






/*
 * Creates and returns the mapping message for a given source.
 * The returned pointer points to a memory buffer allocated with
 * the new operator and must be discarded with the delete oprator.
 *
 * @param source - The SyncSource - NOT NULL
 *
 */
wchar_t* SyncMLBuilder::prepareMapMsg(SyncSource& source) {
    string map = string();

    createMap(source, map);


    //
    // Calculate the size for the entire message
    //
    unsigned int size = wcslen(xmlMap)
                      + length(map)
                      + 1024            // target + device
                      ;

    wchar_t* buf = new wchar_t[size];
/*
    wsprintf( buf                 ,
              xmlMap              ,
              target              ,
              device              ,
              target              ,
              device              ,
              (const wchar_t*)map );
*/
	swprintf( buf             ,
              size                ,
              xmlMapStart1        ,
              target              ,
              device              ,
              target              ,
              device              );
	
	// work fine ***
	//wchar_t* tmp = new wchar_t[size];	
	//wsprintf(tmp, TEXT("%s%s"),(wchar_t*)map, xmlMapStart2);	
	//wcscat(buf, tmp);
	//delete tmp;
	//***
	
	//work fine
	//wcscat(buf, (wchar_t*)map);
	//wcscat(buf, xmlMapStart2);
	//****

	concat(buf, (wchar_t*)map);

	concat(buf, xmlMapStart2);


    return buf;
}


// ----------------------------------------------------------------- Private methods

void SyncMLBuilder::createAlert(SyncSource& source, string& alert) {
    wchar_t buf[ 32 ];
    swprintf( buf, 32, L"%d", source.getPreferredSyncMode() );

    alert = TEXT("<Alert>\n<CmdID>1</CmdID>\n<Data>");
    alert += buf;
    alert += TEXT("</Data>\n<Item>\n<Target><LocURI>");
    // alert += source.getName(NULL, 0);
    alert += source.getRemoteURI(NULL);
    alert += TEXT("</LocURI></Target>\n<Source><LocURI>");
    alert += source.getName(NULL, 0);
    alert += TEXT("</LocURI></Source>\n<Meta><Anchor xmlns=\"syncml:metinf\">\n");
    alert += TEXT("<Last>");
    alert += source.getLastAnchor(NULL);
    alert += TEXT("</Last>\n");
    alert += TEXT("<Next>");
    alert += source.getNextAnchor(NULL);
    alert += TEXT("</Next>\n");
    alert += TEXT("</Anchor></Meta>\n</Item>\n</Alert>\n");
}

void SyncMLBuilder::createSync(SyncSource& source, SyncItem* items[], int n, string& syncTag) {
    int i = 0;

    string addItems     = string();
    string deleteItems  = string();
    string replaceItems = string();
    wchar_t* checkMem   = NULL;
    SyncMode syncMode = source.getSyncMode();
    
    for(i=0; i < n; ++i) {
        
        switch (items[i]->getState()) {
            case SYNC_STATE_DELETED:
                deleteItems += TEXT("<Item>\n<Source><LocURI>");
                deleteItems += items[i]->getKey(NULL);
                deleteItems += TEXT("</LocURI></Source>\n</Item>\n");
                break;

            case SYNC_STATE_UPDATED:
            case SYNC_STATE_NONE:  // this is for slow sync
                
                replaceItems += TEXT("<Item>\n<Source><LocURI>");
                replaceItems += items[i]->getKey(NULL);
                replaceItems += TEXT("</LocURI></Source>\n<Data>");
                replaceItems += (wchar_t*)items[i]->getData();
                replaceItems += TEXT("</Data>\n</Item>\n");
                break;

            case SYNC_STATE_NEW:
                addItems += TEXT("<Item>\n<Source><LocURI>");
                addItems += items[i]->getKey(NULL);
                addItems += TEXT("</LocURI></Source>\n<Data>");
                addItems += (wchar_t*)items[i]->getData();
                addItems += TEXT("</Data>\n</Item>\n");
                break;
        } // end switch
    }
    
    wchar_t cmdID = '4';
    
    //
    // check if there is enough memory to instantiate the new array. If no return
    //

    checkMem = new wchar_t[length(deleteItems) + length(addItems) + length(replaceItems) + 1024];
    if (checkMem == NULL) {

        lastErrorCode = ERR_NOT_ENOUGH_MEMORY;
        swprintf(lastErrorMsg, DIM_ERROR_MESSAGE, 
                 TEXT("Not enough memory..."));
        goto finally;
    }
    safeDelete(&checkMem);
    
    if (length(addItems)>0) {
        syncTag += TEXT("\n<Add>\n<CmdID>");
        syncTag += cmdID;
        syncTag += TEXT("</CmdID>\n<Meta><Type xmlns='syncml:metinf'>");
        syncTag += source.getType(NULL);
        syncTag += TEXT("</Type></Meta>\n");
        syncTag += addItems;
        syncTag += TEXT("</Add>");

        ++cmdID;
    }
    
    if (length(replaceItems)>0) {
        syncTag += TEXT("\n<Replace>\n<CmdID>");
        syncTag += cmdID;
        syncTag += TEXT("</CmdID>\n<Meta><Type xmlns='syncml:metinf'>");
        syncTag += source.getType(NULL);
        syncTag += TEXT("</Type></Meta>\n");
        syncTag += replaceItems;
        syncTag += TEXT("</Replace>");

        ++cmdID;
    }
    
    if (length(deleteItems)>0) {
        syncTag += TEXT("\n<Delete>\n<CmdID>");
        syncTag += cmdID;
        syncTag += TEXT("</CmdID>\n");
        syncTag += deleteItems;
        syncTag += TEXT("</Delete>");

        ++cmdID;
    }
finally:
    safeDelete(&checkMem);
}

void SyncMLBuilder::createMap(SyncSource& source, string& map) {
    unsigned int n = 0;

    //
    // the MapItem element must be inserted only if there are
    // mappings
    //
    n = source.getMapSize();
    if (n == 0) {
        return;
    }

    map += TEXT("<Map>\n<CmdID>3</CmdID>\n<Target><LocURI>");
    // map += source.getName(NULL, 0);
    map += source.getRemoteURI(NULL);
    map += TEXT("</LocURI></Target>\n<Source><LocURI>");
    map += source.getName(NULL, 0);
    map += TEXT("</LocURI></Source>\n");

    SyncMap** mappings = source.getLUIDGUIDMapping();

    for (unsigned int i=0; i<n; ++i) {
        map += TEXT("<MapItem>\n<Target><LocURI>");
        map += mappings[i]->getGUID(NULL);
        map += TEXT("</LocURI></Target>\n");
        map += TEXT("<Source><LocURI>");
        map += mappings[i]->getLUID(NULL);
        map += TEXT("</LocURI></Source>\n</MapItem>\n");
    }

    map += TEXT("</Map>");
}

/* 
* For debugging the concat from buf with string...
*/

void SyncMLBuilder::concat(wchar_t* buf, wchar_t* map) {
    unsigned int n = 0, bufLen = 0;
	
    
	n = wcslen(map);
	bufLen = wcslen(buf);

    if (n == 0) {
        return;
    }		
	
    for (unsigned int i=0; i<n; ++i) {		
		if (map[i] != 0)
			buf[bufLen + i] = map[i];        
		else {
#ifndef LINUX
			wchar_t* localTemp = new wchar_t[bufLen + i + 256];
			wsprintf(localTemp, sizeof(localTemp) / sizeof(wchar_t), 
                                 TEXT("SC-API: %s - 0 at char num %i"), buf, i);
			MessageBox (NULL, localTemp, TEXT ("Debug eVC: Found 0"), MB_SETFOREGROUND |MB_OK);
			delete localTemp;
#else
			fwprintf( stdout,  TEXT("SC-API: %s - 0 at char num %i"), buf, i);
#endif
			break;
		}

    }
	
	buf[bufLen + n] = 0;


}	
