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

#ifndef  SYNCML_CLIENT_PLUGIN
#define  SYNCML_CLIENT_PLUGIN

#include <opensync/opensync.h>
#include <syncml-client.h>
#include "SmcSyncSource.h"
#include "SmcConfig.h"

typedef struct{
    SyncManager*          syncManager;
    OSyncMember*          member;
    SmcConfig*            config;
    SmcSyncSource*        syncSource;
    char*                 lastAnchor;
    SyncMode              overallSyncMode;
} syncml_env;


class SyncManagerException{
    protected:
       char msg[ 256 ];
    public:
        const int error;
        char* getMsg();
        SyncManagerException( const int error , const char* msg );
};


#endif
