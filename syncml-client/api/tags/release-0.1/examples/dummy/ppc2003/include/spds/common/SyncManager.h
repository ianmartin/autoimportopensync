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

 #ifndef INCL_SYNC_MANAGER
    #define INCL_SYNC_MANAGER

    #include "spds/common/Constants.h"
    #include "spds/common/Config.h"
    #include "spds/common/SyncSource.h"
    #include "spds/common/SyncMLBuilder.h"
    #include "spds/common/SyncMLProcessor.h"
    #include "http/common/TransportAgent.h"


    #define CONTEXT_SYNCAGENT TEXT("spds/syncml")

    class __declspec(dllexport) SyncManager {

    private:

        /*
         * The Config object containing the SyncManager configuration
         */
        Config* config;

        /*
         * The builder for SyncML messages
         */
        SyncMLBuilder* syncMLBuilder;

        /*
         * The SyncML processor
         */
        SyncMLProcessor* syncMLProcessor;

        /*
         * The transport agent to be used to send and receive messages.
         */
        TransportAgent* transportAgent;

        /*
         * Reads the definition of the given SyncSource from the DM.
         *
         * @param source the SyncSource
         */
        BOOL readSyncSourceDefinition(SyncSource& source);

        /*
         * Commits the configuration changes in the given SyncSource
         */
        BOOL commitChanges(SyncSource& source);

        /*
         * Commits the synchronization process
         */
        BOOL commitSync();


    public:
        SyncManager(Config* config);
        ~SyncManager();

        /*
         * Initializes a new synchronization session for the specified sync source.
         * It returns 0 in case of success, an error code in case of error
         * (see SPDS Error Codes).
         *
         * @param source - the SyncSource to synchronize
         */
        int prepareSync(SyncSource& source);


        /*
         * Synchronizes the specified source with the server. source should be
         * filled with the client-side modified items. At the end of the
         * process source will be fed with the items sent by the server.
         * It returns 0 in case of success or an error code in case of error
         * (see SPDS Error Codes).
         *
         * @param source - the SyncSource to sync
         */
        int sync(SyncSource& source);

        /*
         * Ends the synchronization of the specified source. If source contains
         * LUIG-GUID mapping this is sent to the server. It returns 0 in case
         * of success or an error code in case of error (see SPDS Error Codes).
         *
         * @param source - the SyncSource to sync
         */
        int endSync(SyncSource& source);

    };

#endif