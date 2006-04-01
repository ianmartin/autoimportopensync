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

/*
 * SyncMLBuilder
 *
 * This class is responsible for the construction of SyncML messages that
 * will be sent to the server.
 *
 */

#ifndef INCL_SYNCML_BUILDER
    #define INCL_SYNCML_BUILDER

    #include "ptypes/ptypes.h"

    #include "spds/common/SyncSource.h"


    USING_PTYPES


    class __declspec(dllexport) SyncMLBuilder {

    private:

        wchar_t target  [DIM_SERVERNAME];
        wchar_t username[DIM_USERNAME  ];
        wchar_t password[DIM_PASSWORD  ];
        wchar_t device  [DIM_DEVICE_ID ];

        void createAlert(SyncSource& source, string& alert);
        void createSync(SyncSource& source, SyncItem* items[], int n, string& syncTag);
        void createMap(SyncSource& source, string& map);

    public:


        /*
         * Constructor
         */
        SyncMLBuilder();

        SyncMLBuilder(wchar_t* t, wchar_t* u, wchar_t* p, wchar_t* d);

        /*
         * Creates and returns the initialization message for a given source.
         * The returned pointer points to a memory buffer allocated with
         * the new operator and must be discarded with the delete oprator.
         *
         * @param source - The SyncSource - NOT NULL
         *
         */
        wchar_t* SyncMLBuilder::prepareInitMsg(SyncSource& source);

        /*
         * Creates and returns the synchronization message for the given source.
         * The returned pointer points to a memory buffer allocated with
         * the new operator and must be discarded with the delete oprator.
         *
         * @param source the SyncSource to synchronize
         */
        wchar_t* SyncMLBuilder::prepareSyncMsg(SyncSource& source);

        /*
         * Creates and returns the mapping message for a given source.
         * The returned pointer points to a memory buffer allocated with
         * the new operator and must be discarded with the delete oprator.
         *
         * @param source - The SyncSource - NOT NULL
         *
         */
        wchar_t* SyncMLBuilder::prepareMapMsg(SyncSource& source);
		
        wchar_t* SyncMLBuilder::prepareAlertMessage(SyncSource& source);
        
        void SyncMLBuilder::concat(wchar_t* buf, wchar_t* map);


    };
#endif