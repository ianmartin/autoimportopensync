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
// @version $Id: SyncMLProcessor.cpp,v 1.14 2005/03/29 13:20:25 magi Exp $
//

#include "unixdefs.h"

#include <stdlib.h>
#include <stdio.h> // to be removed

#include "common/Log.h"

#include "spds/common/Utils.h"
#include "spds/common/Constants.h"
#include "spds/common/SyncMLProcessor.h"
#include "spds/common/SyncItemHolder.h"
#include "spds/common/SyncItemStatusHolder.h"


/*
 * This class is responsible for the processing of the incoming messages.
 */


USING_PTYPES

/*
 * Constructor
 */
SyncMLProcessor::SyncMLProcessor() { 
}

/*
 * Processes the initialization response. Returns 0 in case of success, an
 * error code in case of error.
 *
 * @param msg the response from the server
 */
int SyncMLProcessor::processInitResponse(SyncSource& source, wchar_t* msg) {
    wchar_t* syncBody = NULL;
    wchar_t* alert    = NULL;
    wchar_t* item     = NULL;
    wchar_t* target   = NULL;
    wchar_t* uri      = NULL;
    wchar_t* data     = NULL;
    wchar_t* dataNew  = NULL;

    unsigned int n = 0, pos = 0;
    int ret = -1;
    
    unsigned int startPos   = 0;
    unsigned int endPos     = 0;

    //
    // We have to look for the Alert element relative to the given source.
    // We will store the alert code in the SyncSource object.
    // If no valid Alert is found, we set lastErrorCode and lastErrorMsg
    // accordingly.
    //
    syncBody = getElementContentSaveMem(msg, TEXT("SyncBody"), NULL, &startPos, &endPos);
    if (syncBody == NULL) {
        setErrorMsg( ERR_REPRESENTATION,  TEXT("SyncBody not found!"));
        goto finally;
    }

    //
    // First of all check the status for the SyncHead
    //
    ret = getSyncHeaderStatusCode(syncBody);
    if ((ret <= 200) || (ret >=299)) {
        goto finally;
    }

    //
    // If the access is allowed
    ret = getAlertStatusCode(syncBody, source.getName(NULL, 0));
    if ((ret < 200) || (ret > 299 && ret != 508)) {
        goto finally;
    }

    ret = 0;

    pos = n = 0;
    do {
         alert = getElementContentSaveMem(syncBody, TEXT("Alert"), &n, &startPos, &endPos);
        if (alert == NULL) {
            break; // exit loop
        }
        pos += n;

        item = getElementContentSaveMem(alert, TEXT("Item"), NULL, &startPos, &endPos);
        if (item == NULL) {
            setErrorMsg( ERR_REPRESENTATION, 
                         TEXT("SyncBody/Alert/Item not found!"));

            goto finally;
        }

        target = getElementContentSaveMem(item, TEXT("Target"), NULL, &startPos, &endPos);
        if (item == NULL) {
            setErrorMsg( ERR_REPRESENTATION, 
                         TEXT("SyncBody/Alert/Item/Target not found!"));

            goto finally;
        }

        uri = getElementContentSaveMem(target, TEXT("LocURI"), NULL, &startPos, &endPos);
        if (uri == NULL) {
            setErrorMsg(ERR_REPRESENTATION,  
                        TEXT("SyncBody/Alert/Item/Target/LocURI not found!"));

            goto finally;
        }

        if (wcsncmp(uri, source.getName(NULL, 0), endPos - startPos) == 0) {
            data = getElementContentSaveMem(alert, TEXT("Data"), NULL, &startPos, &endPos);
            dataNew = getContent(data, startPos, endPos);
            if (data == NULL) {
                setErrorMsg( ERR_REPRESENTATION, 
                             TEXT("SyncBody/Alert/Data not found!"));

                goto finally;
            }

            source.setSyncMode((SyncMode)wcstol(dataNew, NULL, 10));
            ret = 0;
            break;
        }

        //
        // For the next iteration we will have new values
        //        
        safeDelete(&dataNew    );
    } while (alert != NULL);

    ret = 0;

    //
    // Finally ...
    //
finally:

    safeDelete(&dataNew    );

    return ret;
}

/*
 * Processes the synchronization response. Returns 0 in case of success, an
 * error code in case of error.
 * It feeds the given source with the server side modifications
 *
 * @param source the source
 * @param msg the response from the server
 */
int SyncMLProcessor::processSyncResponse(SyncSource& source, wchar_t* msg) {
    unsigned int n = 0;
    unsigned int pos1 = 0, pos2 = 0, pos3 = 0;
    unsigned int syncCount = 0, modCount = 0, itemCount = 0;
    unsigned int startPos = 0, endPos = 0;
    unsigned int startPosMod = 0, endPosMod = 0;
    unsigned int startPosSync = 0, endPosSync = 0;
    
    int ret = -1;

    wchar_t* sync   = NULL;
    wchar_t* mod    = NULL;
    wchar_t* target = NULL;
    wchar_t* uri    = NULL;
    wchar_t* item   = NULL;
    wchar_t* data   = NULL;
    wchar_t* id     = NULL;
    wchar_t* idNew  = NULL;
    
    wchar_t itemNumber[128]; itemNumber[0] = 0;

    SyncItem* syncItem              = NULL;
    SyncItemStatus* syncItemStatus  = NULL;

    tobjlist<SyncItem*> items = tobjlist<SyncItem*>();

    //
    // First of all check the status for the SyncHead
    //
    ret = getSyncHeaderStatusCode(msg);
    if ((ret < 200) || (ret >299)) {
        goto finally;
    }
    
    // 
    // check and write the status code into syncsource
    //
    
    ret = getItemStatusCode(source, msg);
    if ((ret < 200) || (ret >299)) {
        goto finally;
    }
    

    do {
        ++syncCount; pos2 = pos3 = 0;
        sync = getElementContentSaveMem(&msg[pos1], TEXT("Sync"), &n, &startPosSync, &endPosSync);
        if (sync == NULL) {
            //
            // No more to do
            //
            break;
        }
        pos1 += n;

        target = getElementContentSaveMem(sync, TEXT("Target"), NULL, &startPos, &endPos);
        if (target == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, 
                         TEXT("Missing Sync[%d]/Target element!"), syncCount);

            goto finally;
        }

        uri = getElementContentSaveMem(target, TEXT("LocURI"), NULL, &startPos, &endPos);
        if ((uri == NULL) || (wcsncmp(uri, source.getName(NULL, 0), endPos - startPos) != 0)) {
            //
            // This Sync is for another source
            //
            target  = NULL;
            uri     = NULL;
            continue;
        }

        //
        // Here the Sync is for the given source
        //
        target  = NULL;
        uri     = NULL;

        //
        // Search for all Add
        //
        clear(items); modCount = 0;
        do {
            ++modCount; pos3 = 0;

            item        = NULL;
            id          = NULL; 
            data        = NULL;
            mod         = NULL;

            mod = getElementContentSaveMem(&sync[pos2], TEXT("Add"), &n, &startPosMod, &endPosMod);
            if (mod == NULL || n + pos2 + startPosSync> endPosSync) {
                //
                // No more Add
                //
                break;
            }
            pos2 += n;
            
            //
            // Search for all Item
            //
            itemCount = 0;
            do {
                ++itemCount;

                item    = NULL;
                id      = NULL;
                data    = NULL;

                item = getElementContentSaveMem(&mod[pos3], TEXT("Item"), &n, &startPos, &endPos);
                if (item == NULL || n + pos3 + startPosMod> endPosMod) {
                    //
                    // No more Item
                    //
                    break;
                }
                pos3 += n;
                
                id = getElementContentSaveMem(item, TEXT("LocURI"), NULL, &startPos, &endPos);
                idNew = getContent(id, startPos, endPos);
                if (id == NULL) {
                    //
                    // Representation error
                    //
                    ret =  ERR_REPRESENTATION;
                    setErrorMsg( ERR_REPRESENTATION, 
                                 TEXT("Missing Sync[%d]/Add[%d]/Item[i]/Source/LocURI element!"), syncCount, modCount, itemCount);

                    goto finally;
                }

                data = getElementContentSaveMem(item, TEXT("Data"), NULL, &startPos, &endPos);
                syncItem = new SyncItem(idNew);
                                
                wchar_t ch = item[endPos];
                item[endPos] = 0;                
                
                syncItem->setData(data, (data == NULL) ? 0 : (endPos - startPos +1)*sizeof(wchar_t));                
                item[endPos] = ch;
                
                safeDelete(&idNew);

                if (lastErrorCode == ERR_NOT_ENOUGH_MEMORY) {
                    //
                    // Not enough memory
                    //
                    ret = ERR_NOT_ENOUGH_MEMORY;                    
                    goto finally;
                }

                add(items, new SyncItemHolder(syncItem));
            } while (item != NULL);
        } while (mod != NULL);
       
        source.setNewSyncItems(toSyncItemArray(items), length(items));

        //
        // Search for all Replace
        //
        clear(items); modCount = 0; pos2 = 0; startPosMod = 0; endPosMod = 0; 
        do {
            ++modCount; pos3 = 0;

            item        = NULL;
            id          = NULL; 
            data        = NULL;
            mod         = NULL;

            mod = getElementContentSaveMem(&sync[pos2], TEXT("Replace"), &n, &startPosMod, &endPosMod);
            if (mod == NULL  || n + pos2 + startPosSync > endPosSync) {
                //
                // No more Replace
                //
                break;
            }
            
            pos2 += n;
            
            //
            // Search for all Item
            //
            itemCount = 0;
            do {

                ++itemCount;

                item    = NULL;
                id      = NULL;
                data    = NULL;

                item = getElementContentSaveMem(&mod[pos3], TEXT("Item"), &n, &startPos, &endPos);
                if (item == NULL || n + pos3 + startPosMod> endPosMod) {
                    //
                    // No more Item
                    //
                    break;
                }
                pos3 += n;
                
                id = getElementContentSaveMem(item, TEXT("LocURI"), NULL, &startPos, &endPos);
                idNew = getContent(id, startPos, endPos);
                if (id == NULL) {
                    //
                    // Representation error
                    //
                    ret = ERR_REPRESENTATION;
                    setErrorMsg( ERR_REPRESENTATION,
                                 TEXT("Missing Sync[%d]/Replace[%d]/Item[i]/Source/LocURI element!"), syncCount, modCount, itemCount);

                    goto finally;
                }

                data = getElementContentSaveMem(item, TEXT("Data"), NULL, &startPos, &endPos);

                syncItem = new SyncItem(idNew);
                
                wchar_t ch = item[endPos];
                item[endPos] = 0;     
                syncItem->setData(data, (data == NULL) ? 0 : (endPos - startPos +1)*sizeof(wchar_t));                
                item[endPos] = ch;
                
                safeDelete(&idNew);

                if (lastErrorCode == ERR_NOT_ENOUGH_MEMORY) {
                    //
                    // Not enough memory
                    //
                    ret = ERR_NOT_ENOUGH_MEMORY;                    
                    goto finally;
                }
                add(items, new SyncItemHolder(syncItem));
            } while (item != NULL);
        } while (mod != NULL);
        
        source.setUpdatedSyncItems(toSyncItemArray(items), length(items));

        //
        // Search for all Delete
        //
        clear(items); modCount = 0; pos2 = 0; startPosMod = 0; endPosMod = 0; 
        do {
            ++modCount; pos3 = 0;

            item        = NULL;
            id          = NULL; 
            data        = NULL;
            mod         = NULL;


            mod = getElementContentSaveMem(&sync[pos2], TEXT("Delete"), &n, &startPosMod, &endPosMod);
            if (mod == NULL  || n + pos2 + startPosSync > endPosSync) {
                //
                // No more Delete
                //
                break;
            }
            pos2 += n;

            //
            // Search for all Item
            //
            itemCount = 0;
            do {
                ++itemCount;

                item    = NULL;
                id      = NULL;
                data    = NULL;

                item = getElementContentSaveMem(&mod[pos3], TEXT("Item"), &n, &startPos, &endPos);
                if (item == NULL || n + pos3 + startPosMod> endPosMod) {
                    //
                    // No more Item
                    //
                    break;
                }
                pos3 += n;

                id = getElementContentSaveMem(item, TEXT("LocURI"), NULL, &startPos, &endPos);
                idNew = getContent(id, startPos, endPos);
                if (id == NULL) {
                    //
                    // Representation error
                    //
                    ret = ERR_REPRESENTATION;
                    setErrorMsg( ERR_REPRESENTATION,
                                 TEXT("Missing Sync[%d]/Delete[%d]/Item[i]/Source/LocURI element!"), syncCount, modCount, itemCount);

                    goto finally;
                }

                syncItem = new SyncItem(idNew);
                syncItem->setData(NULL, 0);

                add(items, new SyncItemHolder(syncItem));
            } while (item != NULL);
        } while (mod != NULL);
        
        source.setDeletedSyncItems(toSyncItemArray(items), length(items));

    } while (sync != NULL);

    clear(items);  // releases the SyncItemHolders

    ret = 0;

finally:

    sync    = NULL;
    mod     = NULL;
    target  = NULL;
    uri     = NULL;
    item    = NULL;
    data    = NULL;
    id      = NULL;
    safeDelete( &idNew  );

    return ret;

}

/*
 * Processes the map message response. Returns 0 in case of success, an
 * error code in case of error.
 * It feeds the given source with the server side modifications
 *
 * @param source the source
 * @param msg the response from the server
 */
int SyncMLProcessor::processMapResponse(SyncSource& source, wchar_t* msg) {
    int ret = -1;

    //
    // for now it is always ok
    //
    //
    // First of all check the status for the SyncHead
    //
    ret = getSyncHeaderStatusCode(msg);
    if ((ret < 200) || (ret >299)) {
        goto finally;
    }

    ret = 0;

finally:

    return ret;
}

/*
 * Returns the SyncHeader/RespURI element of the given message. If the element is not
 * found it returns NULL. The returned respURI is allocated with the new operator
 * and must be discarded with delete by the caller.
 *
 * @param msg - the message - NOT NULL
 */
wchar_t* SyncMLProcessor::getRespURI(wchar_t* msg) {
    wchar_t* header  = NULL;
    wchar_t* respURI = NULL;

    header = getElementContent(msg, TEXT("SyncHdr"), NULL);
    if (header == NULL) {
        goto finally;
    }

    respURI = getElementContent(header, TEXT("RespURI"), NULL);

    finally:

    safeDelete(&header);

    return respURI;
}

/*
 * Check if the <Final> tag is included in the server message.
 *
 * @param msg - the message - NOT NULL
 */
BOOL SyncMLProcessor::checkTagFinal(wchar_t* msg) {
   
    wchar_t* final   = NULL;
    BOOL existsFinal = FALSE;

    final = getElementContent(msg, TEXT("Final"), NULL);
    if (final == NULL) {
        goto finally;
    }
    existsFinal = TRUE;

finally:

    safeDelete(&final);
    return existsFinal;
    
}

// ------------------------------------------------------------- Private methods

/*
* It's like getElementContent function but it doesn't allocate new memory.
* It handles the tag with attribute  
*
* @param xml the xml fragment
* @param tag the tag we want the content
* @param pos (OUTPUT) the position where the tag is found (ignored if NULL)
* @param startPos (OUTPUT) the start position of the tag content (ignored if NULL)
* @param pos (OUTPUT) the end position of the tag content (ignored if NULL)
*/

wchar_t* SyncMLProcessor::getElementContentSaveMem(wchar_t* xml, wchar_t* tag, unsigned int* pos,
                                                   unsigned int* startPos, unsigned int* endPos) {

    const int TagSize = 40;

    wchar_t* p1       = NULL;
    wchar_t* p2       = NULL;
    BOOL charFound    = FALSE;

    wchar_t openTag[TagSize];
    wchar_t closeTag[TagSize];
    
    unsigned int xmlLength = wcslen(xml);
    unsigned int l = wcslen(tag);

    if (pos != NULL) {
        *pos = 0;
    }

    swprintf(openTag, TagSize,  TEXT("<%ls>"), tag);
    swprintf(closeTag,  TagSize,  TEXT("</%ls>"), tag);

    p1 = wcsstr(xml, openTag);

    if (p1 == NULL) { // tag can have attributes
        //
        // This is abcxyz
        //
        // goto finally;

        // if p1 is null I try to discover the next '>' char to close the tag. If does not exist 
        // return NULL
        
        // try to find "<tagName/>". If found it return null.
        swprintf(openTag,   TagSize,  TEXT("<%ls/>"), tag);
        p1 = wcsstr(xml, openTag);
        
        if (p1 != NULL) {
            goto finally;
        }

        // try to find "<tagName"
        swprintf(openTag,  TagSize,  TEXT("<%ls"), tag);
        p1 = wcsstr(xml, openTag);

        if (p1 == NULL) {
            goto finally;
        }
        
        p1 = p1 + l + 1;   // <body_
        
        for (unsigned int k = 0; k < xmlLength; k++) { // Suppose max length as the xml string
            p1 = p1 + 1;
            if (*p1 == 0) {
                goto finally;
            }
            else if (*p1 == '>') {
                charFound = TRUE;
                p1 = p1 + 1;
                break;
            }            
        }
        if (!charFound)
            goto finally;


    } else {  // tag doesn't have attribute. Original version
    
        p1 = p1+l+2;
    
    }
    if (*p1 == 0) {
        //
        // This is abc<tag>\0
        //
        goto finally;
    }

    p2 = wcsstr(p1, closeTag);

    if (p2 == NULL) {
        //
        // This is abc<tag>xyz\0
        //
        p1 = NULL;
        goto finally;
    }
        
    if (pos != NULL) {
        *pos = p2-xml+l+3;
    }
    if (startPos != NULL) {
        *startPos = p1 - xml;
    }
    if (endPos != NULL) {
        *endPos = p2 - xml ;
    }   

    finally:

    return p1;
}


/*
* It return the content of the buffer specified by startPos (initial position)
* and and endPos (the end position)
* 
* It allocates new memory that has to be freed by caller.
*
* @param xml the xml fragment         
* @param startPos the start position of the tag content 
* @param endPos  the end position of the tag content 
* 
*/
        
wchar_t* SyncMLProcessor::getContent(wchar_t* xml, unsigned int startPos, unsigned int endPos) {
    
    wchar_t * ret = NULL;
    
    if (xml == NULL) {
        goto finally;
    }
    if (endPos <= startPos) {
        goto finally;
    }
    if (wcslen(xml) < endPos - startPos) {
        goto finally;
    }

    ret = new wchar_t[endPos - startPos + 1];

    wcsncpy(ret, xml, endPos - startPos);
    ret[endPos - startPos] = 0;

finally:

    return ret;
}

/*
 * Extracts the content of a tag into a SyncML message. It is supposed the
 * message is a valid SyncML message. It returns NULL in case the tag is not
 * found or the XML fragment is not in the expected form.
 * The returned pointer (if not NULL) is allocated with the new operator and
 * must be discarded with the operator delete.
 *
 * @param xml the xml fragment
 * @param tag the tag we want the content
 * @param pos (OUTPUT) the position just after the closing tag (ignored if NULL)
 *
 */

wchar_t* SyncMLProcessor::getElementContent(wchar_t* xml, wchar_t* tag, unsigned int* pos) {
    wchar_t* p1       = NULL;
    wchar_t* p2       = NULL;
    wchar_t* ret      = NULL;
    BOOL charFound    = FALSE;

    wchar_t openTag[40];
    wchar_t closeTag[40];
    
    unsigned int xmlLength = wcslen(xml);
    unsigned int l = wcslen(tag);

    if (pos != NULL) {
        *pos = 0;
    }

    swprintf(openTag,  sizeof(openTag) / sizeof(wchar_t), TEXT("<%ls>"), tag);
    swprintf(closeTag,  sizeof(closeTag) / sizeof(wchar_t), TEXT("</%ls>"), tag);

    p1 = wcsstr(xml, openTag);

    if (p1 == NULL) { // tag can have attributes
        //
        // This is abcxyz
        //
        // goto finally;
    
        // try to find "<tagName/>". If found it return null.
        swprintf(openTag,  sizeof(openTag) / sizeof(wchar_t), TEXT("<%ls/>"), tag);
        p1 = wcsstr(xml, openTag);

        if (p1 != NULL) {
            goto finally;
        }
        
        // try to find "<tagName"
        swprintf(openTag,  sizeof(openTag) / sizeof(wchar_t), TEXT("<%ls"), tag);
        p1 = wcsstr(xml, openTag);

        if (p1 == NULL) {
            goto finally;
        }
        
        p1 = p1 + l + 1;   // <body_ 

        for (unsigned int k = 0; k < xmlLength; k++) { // Suppose max length as the xml string
            p1 = p1 + 1;
            if (*p1 == 0) {
                goto finally;
            }
            else if (*p1 == '>') {
                charFound = TRUE;
                p1 = p1 + 1;
                break;
            }            
        }
        if (!charFound)
            goto finally;             
    
    } else {  // tag doesn't have attribute. Original version
        
        p1 = p1+l+2;

    }
    if (*p1 == 0) {
        //
        // This is abc<tag>\0
        //
        goto finally;
    }

    p2 = wcsstr(p1, closeTag);

    if (p2 == NULL) {
        //
        // This is abc<tag>xyz\0
        //
        goto finally;
    }

    ret = new wchar_t[p2-p1+1];
    
    if (ret != NULL) {
        wcsncpy(ret, p1, p2-p1);
        ret[p2-p1] = 0;
    } 
    // if no enough memory to instantiate the new object...
    else {
        ret = TEXT("");
    }

    if (pos != NULL) {
        *pos = p2-xml+l+3;
    }

    finally:

    return ret;

}

/*
 * Returns the status code for the SyncHeader command included
 * in the message sent by the client.
 *
 * @param syncBody - the SyncBody content
 */
int SyncMLProcessor::getSyncHeaderStatusCode(wchar_t* syncBody) {
    wchar_t* status = NULL;
    wchar_t* cmdRef = NULL;
    wchar_t* data   = NULL;
    wchar_t* tmp    = NULL;

    int ret = -1;
    unsigned int startPos = 0;
    unsigned int endPos   = 0;
    //
    // It must be the first one
    //
    status = getElementContentSaveMem(syncBody, TEXT("Status"), NULL, &startPos, &endPos);
    if (status == NULL) {
        //
        // It should not happen
        //
        setErrorMsg( ERR_REPRESENTATION,
                     TEXT("Status command for SyncHeader not found!"));
        goto finally;
    }

    cmdRef = getElementContentSaveMem(status, TEXT("CmdRef"), NULL, &startPos, &endPos);
    if ((cmdRef == NULL) || (wcsncmp(cmdRef, TEXT("0"), endPos - startPos))) {
        //
        // It should not happen
        //
        setErrorMsg( ERR_REPRESENTATION,
                     TEXT("Status/CmdRef either not found or not referring to SyncHeader!"));
        goto finally;
    }

    data = getElementContentSaveMem(status, TEXT("Data"), NULL, &startPos, &endPos);
    if (data == NULL) {
        //
        // It should not happen
        //
        setErrorMsg( ERR_REPRESENTATION, TEXT("Status/Data not found!"));
        goto finally;
    }

    tmp = getContent(data, startPos, endPos);
    ret = wcstoul(tmp, NULL, 10);

    finally:

    safeDelete(&tmp  );

    return ret;
}


/*
 * Returns the status code for the Alert relative to the given source.
 *
 * @param syncBody - the SyncBody content
 * @param sourceName - the name of the source
 */
int SyncMLProcessor::getAlertStatusCode(wchar_t* syncBody, wchar_t* sourceName) {
    unsigned int pos = 0;

    wchar_t* status         = NULL;
    wchar_t* cmd            = NULL;
    wchar_t* data           = NULL;
    wchar_t* sourceRef      = NULL;
    wchar_t* cmdNew         = NULL;
    wchar_t* sourceRefNew   = NULL;
    wchar_t* dataNew        = NULL;

    unsigned int startPos = 0;
    unsigned int endPos   = 0;

    int ret = -1;

    do {
 
        status = getElementContentSaveMem(&syncBody[pos], TEXT("Status"), &pos, &startPos, &endPos);
        if (status == NULL) {
            break;
        }

        cmd = getElementContentSaveMem(status, TEXT("Cmd"), NULL, &startPos, &endPos);
        if (cmd == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/Cmd not found!"));
            goto finally;
        }

        if (wcsncmp(TEXT("Alert"), cmd, endPos - startPos) != 0) {
            continue;
        }

         sourceRef = getElementContentSaveMem(status, TEXT("SourceRef"), NULL, &startPos, &endPos);
        if ((sourceRef != NULL) && (wcsncmp(sourceName, sourceRef, endPos - startPos) == 0)) {
            //
            // Found!
            //
            break;
        }

    } while(status != NULL);

    //
    // If status is not null, it is the good one!
    //
    if (status != NULL) {
        data = getElementContentSaveMem(status, TEXT("Data"), NULL, &startPos, &endPos);
        dataNew = getContent(data, startPos, endPos);
        if (data == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/Data not found!"));
            lastErrorCode = ERR_REPRESENTATION;
            goto finally;
        }
        ret = wcstoul(dataNew, NULL, 10);
        safeDelete(&dataNew);
    }

    finally:

    safeDelete(&dataNew);
    safeDelete(&cmdNew);
    safeDelete(&sourceRefNew);

    return ret;
}


/*
 * Get the response status code for every item.
 *
 * @param source - the source to hold the syncItemStatus array
 * @param msg - the msg
 */
int SyncMLProcessor::getItemStatusCode(SyncSource& source, wchar_t* msg) {
    
    unsigned int pos = 0;

    wchar_t* status         = NULL;
    wchar_t* cmdID          = NULL; wchar_t cmdIDBuffer [10]; cmdIDBuffer[0]  = 0;
    wchar_t* msgRef         = NULL; wchar_t msgRefBuffer[10]; msgRefBuffer[0] = 0;
    wchar_t* cmdRef         = NULL; wchar_t cmdRefBuffer[10]; cmdRefBuffer[0] = 0;
    wchar_t* cmd            = NULL; wchar_t cmdBuffer   [DIM_COMMAND_SYNC_ITEM_STATUS]; cmdBuffer[0] = 0;
    wchar_t* key            = NULL; wchar_t keyBuffer   [DIM_KEY_SYNC_ITEM_STATUS    ]; keyBuffer[0] = 0;
    wchar_t* data           = NULL; wchar_t dataBuffer  [10]; dataBuffer[0]   = 0;
    
    wchar_t* sourceRef      = NULL;  // or LocURI if there are more Item tag
    
    SyncItemStatus* syncItemStatus  = NULL;

    tobjlist<SyncItemStatus*> itemStatus = tobjlist<SyncItemStatus*>();


    unsigned int startPos = 0;
    unsigned int endPos   = 0;
    unsigned int startPosStatus = 0;
    unsigned int endPosStatus   = 0;
    unsigned int k = 0;

    int ret = -1;

    do {
 
        status = getElementContentSaveMem(&msg[pos], TEXT("Status"), &k, &startPosStatus, &endPosStatus);
        if (status == NULL) {
            break;
        } 
        pos += k;
        //
        // cmd
        // 
        cmd = getElementContentSaveMem(status, TEXT("Cmd"), NULL, &startPos, &endPos);
        if (cmd == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/Cmd not found!"));
            goto finally;
        } else {
            wcsncpy(cmdBuffer, cmd, endPos - startPos);
            cmdBuffer[endPos - startPos] = 0;
            
            if (wcscmp(cmdBuffer, TEXT("Replace")) != 0 &&
                wcscmp(cmdBuffer, TEXT("Add"))     != 0 &&
                wcscmp(cmdBuffer, TEXT("Delete"))  != 0   )
                
                continue;

        }
        
        //
        // CmdID
        //                
        cmdID = getElementContentSaveMem(status, TEXT("CmdID"), NULL, &startPos, &endPos);
        if (cmdID == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/CmdID not found!"));
            lastErrorCode = ERR_REPRESENTATION;
            goto finally;
        } else {
            wcsncpy(cmdIDBuffer, cmdID, endPos - startPos);
            cmdIDBuffer[endPos - startPos] = 0;
        }
        
        //
        // msgRef
        //                
        msgRef = getElementContentSaveMem(status, TEXT("MsgRef"), NULL, &startPos, &endPos);
        if (msgRef == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/MsgRef not found!"));
            goto finally;
        } else {
            wcsncpy(msgRefBuffer, msgRef, endPos - startPos);
            msgRefBuffer[endPos - startPos] = 0;
        }

        //
        // cmdRef
        //                
        cmdRef = getElementContentSaveMem(status, TEXT("CmdRef"), NULL, &startPos, &endPos);
        if (cmdRef == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/CmdRef not found!"));
            goto finally;
        } else {
            wcsncpy(cmdRefBuffer, cmdRef, endPos - startPos);
            cmdRefBuffer[endPos - startPos] = 0;
        }

        //
        // data
        //                
        data = getElementContentSaveMem(status, TEXT("Data"), NULL, &startPos, &endPos);
        if (data == NULL) {
            //
            // It should not happen
            //
            setErrorMsg( ERR_REPRESENTATION, TEXT("Status/Data not found!"));
            goto finally;
        } else {
            wcsncpy(dataBuffer, data, endPos - startPos);
            dataBuffer[endPos - startPos] = 0;
        }
        
        //
        // Looking for SourceRef tag. If found process the only ItemStatus else there are more ItemStatus 
        //
        unsigned int n = 0;
        sourceRef = getElementContentSaveMem(status, TEXT("SourceRef"), &n, &startPos, &endPos);        
        if (sourceRef != NULL && n < endPosStatus) {
            wcsncpy(keyBuffer, sourceRef, endPos - startPos);
            keyBuffer[endPos - startPos] = 0;
           
                syncItemStatus = new SyncItemStatus(keyBuffer);
            
                syncItemStatus->setCmd   (cmdBuffer                      );
                syncItemStatus->setCmdID (wcstoul(cmdIDBuffer,  NULL, 10));
                syncItemStatus->setMsgRef(wcstoul(msgRefBuffer, NULL, 10));
                syncItemStatus->setCmdRef(wcstoul(cmdRefBuffer, NULL, 10));
                syncItemStatus->setData  (wcstoul(dataBuffer,   NULL, 10));

                add(itemStatus, new SyncItemStatusHolder(syncItemStatus));
            
            
        } else { // it will find <Source><LocURI>
            unsigned int posLocURI = 0; 
            n = 0;
           
            do {
 
                sourceRef = getElementContentSaveMem(&status[posLocURI], TEXT("LocURI"), &n, &startPos, &endPos);
                if (sourceRef == NULL || n + posLocURI + startPosStatus> endPosStatus) {
                    break;
                }
                posLocURI += n;
                wcsncpy(keyBuffer, sourceRef, endPos - startPos);
                keyBuffer[endPos - startPos] = 0;

                syncItemStatus = new SyncItemStatus(keyBuffer);
                syncItemStatus->setCmd   (cmdBuffer                      );
                syncItemStatus->setCmdID (wcstoul(cmdIDBuffer,  NULL, 10));
                syncItemStatus->setMsgRef(wcstoul(msgRefBuffer, NULL, 10));
                syncItemStatus->setCmdRef(wcstoul(cmdRefBuffer, NULL, 10));
                syncItemStatus->setData  (wcstoul(dataBuffer,   NULL, 10));

                add(itemStatus, new SyncItemStatusHolder(syncItemStatus));
            } while(sourceRef != NULL);
        }
        

    } while(status != NULL);
    
    //
    // In this way the message from the server that contain valid Status about client item
    // are set. If no one exists no items are set (default is 0)
    // If the server message contains the server item to be processed by client and no status 
    // the arleady contents in the sources are not overwritten
    //
    if (length(itemStatus) > 0) {
        source.setSyncItemStatus(toSyncItemStatusArray(itemStatus), length(itemStatus));
    }
    
    ret = 200;    

 finally:

    return ret;
    
}

/*
 * Trnslates a PTypes object list into an array of SyncItemStatus*.
 *
 * @param items the item list
 */
SyncItemStatus** SyncMLProcessor::toSyncItemStatusArray(tobjlist<SyncItemStatus*>& items) {
    
    int l = length(items);

    SyncItemStatus** itemArrayStatus = new SyncItemStatus*[l];

    for (int i=0; i<l; ++i) {
        itemArrayStatus[i] = ((SyncItemStatusHolder*)items[i])->getItemStatus();
    }

    return itemArrayStatus;
}



/*
 * Trnslates a PTypes object list into an array of SyncItem*.
 *
 * @param items the item list
 */
SyncItem** SyncMLProcessor::toSyncItemArray(tobjlist<SyncItem*>& items) {
    int l = length(items);

    SyncItem** itemArray = new SyncItem*[l];

    for (int i=0; i<l; ++i) {
        itemArray[i] = ((SyncItemHolder*)items[i])->getItem();
    }

    return itemArray;
}

/*
TO BE REMOVED. It doesn't handle the tag with attribute...

wchar_t* SyncMLProcessor::getElementContentSaveMem(wchar_t* xml, wchar_t* tag, unsigned int* pos,
                                                   unsigned int* startPos, unsigned int* endPos) {

    wchar_t* p1       = NULL;
    wchar_t* p2       = NULL;

    wchar_t openTag[16];
    wchar_t closeTag[16];

    unsigned int l = wcslen(tag);

    if (pos != NULL) {
        *pos = 0;
    }

    wsprintf(openTag, TEXT("<%s>"), tag);
    wsprintf(closeTag, TEXT("</%s>"), tag);

    p1 = wcsstr(xml, openTag);

    if (p1 == NULL) {
        //
        // This is abcxyz
        //
        goto finally;
    }
    p1 = p1+l+2;

    if (*p1 == 0) {
        //
        // This is abc<tag>\0
        //
        goto finally;
    }

    p2 = wcsstr(p1, closeTag);

    if (p2 == NULL) {
        //
        // This is abc<tag>xyz\0
        //
        p1 = NULL;
        goto finally;
    }
        
    if (pos != NULL) {
        *pos = p2-xml+l+3;
    }
    if (startPos != NULL) {
        *startPos = p1 - xml;
    }
    if (endPos != NULL) {
        *endPos = p2 - xml ;
    }   

    finally:

    return p1;

}
*/

/*
TO BE REMOVED. It doesn't handle the tag with attribute...

wchar_t* SyncMLProcessor::getElementContent(wchar_t* xml, wchar_t* tag, unsigned int* pos) {
    wchar_t* p1       = NULL;
    wchar_t* p2       = NULL;
    wchar_t* ret      = NULL;

    wchar_t openTag[16];
    wchar_t closeTag[16];

    unsigned int l = wcslen(tag);

    if (pos != NULL) {
        *pos = 0;
    }

    wsprintf(openTag, TEXT("<%s>"), tag);
    wsprintf(closeTag, TEXT("</%s>"), tag);

    p1 = wcsstr(xml, openTag);

    if (p1 == NULL) {
        //
        // This is abcxyz
        //
        goto finally;
    }
    p1 = p1+l+2;

    if (*p1 == 0) {
        //
        // This is abc<tag>\0
        //
        goto finally;
    }

    p2 = wcsstr(p1, closeTag);

    if (p2 == NULL) {
        //
        // This is abc<tag>xyz\0
        //
        goto finally;
    }

    ret = new wchar_t[p2-p1+1];
    
    if (ret != NULL) {
        wcsncpy(ret, p1, p2-p1);
        ret[p2-p1] = 0;
    } 
    // if no enough memory to instantiate the new object...
    else {
        ret = TEXT("");
    }

    if (pos != NULL) {
        *pos = p2-xml+l+3;
    }

    finally:

    return ret;

}
*/
