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

#ifndef INCL_SYNCML_PROCESSOR
    #define INCL_SYNCML_PROCESSOR

    #include "ptypes/ptypes.h"
    #include "spds/common/SyncSource.h"
    #include "spds/common/SyncItem.h"
    #include "spds/common/SyncItemStatus.h"

    USING_PTYPES


    /*
     * This class is responsible for the processing of the incoming messages.
     */

    class __declspec(dllexport) SyncMLProcessor {

    private:
        /*
         * Extracts the content of a tag into a SyncML message. It is supposed the
         * message is a valid SyncML message. It returns NULL in case the tag is not
         * found or the XML fragment is not in the expected form.
         * The returned pointer (if not NULL) is allocated with the new operator and
         * must be discarded with the operator delete.
         *
         * @param xml the xml fragment
         * @param tag the tag we want the content
         * @param pos (OUTPUT) the position where the tag is found (ignored if NULL)
         *
         */
        wchar_t* SyncMLProcessor::getElementContent(wchar_t* xml, wchar_t* tag, unsigned int* pos);
        
        /*
         * It's like getElementContent function but it doesn't allocate new memory.
         * 
         * @param xml the xml fragment
         * @param tag the tag we want the content
         * @param pos (OUTPUT) the position where the tag is found (ignored if NULL)
         * @param startPos (OUTPUT) the start position of the tag content (ignored if NULL)
         * @param pos (OUTPUT) the end position of the tag content (ignored if NULL)
         */
        wchar_t* SyncMLProcessor::getElementContentSaveMem(wchar_t* xml, wchar_t* tag, unsigned int* pos, unsigned int* startPos, unsigned int* endPos);
        
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
        
        wchar_t* SyncMLProcessor::getContent(wchar_t* xml, unsigned int startPos, unsigned int endPos);
        
        /*
         * Returns the status code for the SyncHeader command included
         * in the message sent by the client.
         *
         * @param syncBody - the SyncBody content
         */
        int SyncMLProcessor::getSyncHeaderStatusCode(wchar_t* syncBody);

        /*
         * Trnslates a PTypes object list into an array of SyncItem*.
         *
         * @param items the item list
         */
        SyncItem** toSyncItemArray(tobjlist<SyncItem*>& items);

        /*
         * Returns the status code for the Alert relative to the given source.
         *
         * @param syncBody - the SyncBody content
         * @param sourceName - the name of the source
         */
        int getAlertStatusCode(wchar_t* syncBody, wchar_t* sourceName);

        /*
         * Get the response status code for every item.
         *
         * @param source - the source to hold the syncItemStatus array
         * @param msg - the msg
         */
        int getItemStatusCode(SyncSource& source, wchar_t* msg);

        /*
         * Trnslates a PTypes object list into an array of SyncItemStatus*.
         *
         * @param items the item list
         */
        SyncItemStatus** toSyncItemStatusArray(tobjlist<SyncItemStatus*>& items);        

    public:

        /*
         * Constructor
         */
        SyncMLProcessor();

        /*
         * Processes the initialization response. Returns 0 in case of success, an
         * error code in case of error.
         *
         * @param msg the response from the server
         */
        int processInitResponse(SyncSource& source, wchar_t* msg);

        /*
         * Processes the synchronization response. Returns 0 in case of success, an
         * error code in case of error.
         * It feeds the given source with the server side modifications
         *
         * @param source the source
         * @param msg the response from the server
         */
        int processSyncResponse(SyncSource& source, wchar_t* msg);

        /*
         * Processes the map message response. Returns 0 in case of success, an
         * error code in case of error.
         * It feeds the given source with the server side modifications
         *
         * @param source the source
         * @param msg the response from the server
         */
        int processMapResponse(SyncSource& source, wchar_t* msg);

        /*
         * Returns the SyncHeader/RespURI element of the given message. If the element is not
         * found it returns NULL. The returned respURI is allocated with the new operator
         * and must be discarded with delete by the caller.
         *
         * @param msg - the message - NOT NULL
         */
        wchar_t* getRespURI(wchar_t* msg);

        /*
         * Check if the <Final> tag is included in the server message.
         *
         * @param msg - the message - NOT NULL
         */
        BOOL SyncMLProcessor::checkTagFinal(wchar_t* msg);

    };

#endif