/*
 * Copyright (C) 2006 Michael Kolmodin
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

#ifndef  SMC_SYNC_SOURCE
#define  SMC_SYNC_SOURCE

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <vector>
#include <assert.h>

#include <opensync/opensync.h>
#include <syncml-client.h>

#include "SmcChangeFactory.h"
#include "ItemFactory.h"

typedef std::vector<SyncItem*> SyncItemVector_t ;

class SyncSourceException{
    protected: 
        char msg[ 256 ];

    public:
        SyncSourceException( char* why );
        char* getMsg();
};

class SmcSyncSource: public SyncSource
{

    protected:
        SyncItemVector_t     deletions;
        SyncItemVector_t     modifications;
        SyncItemVector_t     additions;

        void reportChangeItems( SyncItem**            items,
                                size_t                size,
                                OSyncChangeType       changeType,
                                SmcChangeFactory*     changeFactory,
                                OSyncContext*         ctx  );

        SyncItem** ItemVector2Array( SyncItemVector_t changes );


    public:

        SmcSyncSource( wchar_t* key );
        ~SmcSyncSource(); 

        void reportChanges( OSyncContext* ctx, SmcChangeFactory* factory );
        void commitChange( ItemFactory* itemFactory, OSyncChange* change)
              throw( SyncSourceException );
        void prepareForSync();

        void updateAnchors();
        char* getLastAnchor();
        char* getNextAnchor();

};

extern SmcSyncSource* createSyncSource( char* key ) 
    throw ( ConfigException );


#endif


