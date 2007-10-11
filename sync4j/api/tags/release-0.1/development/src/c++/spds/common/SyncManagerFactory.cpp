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
// @version $Id: SyncManagerFactory.cpp,v 1.8 2005/03/29 13:20:25 magi Exp $
//

#include "unixdefs.h"
#include <stdio.h>

#include "common/fscapi.h"
#include "common/Log.h"

#include "spds/common/SyncManager.h"
#include "spds/common/SyncManagerFactory.h"
#include "spds/common/Config.h"

#include "spdm/common/DeviceManagerFactory.h"
#include "spdm/common/DeviceManager.h"

/*
 * Constructor.
 */
SyncManagerFactory::SyncManagerFactory() {};

/*
 * Creates and returns a new DeviceManager. The DeviceManager object is create
 * with the new operator and must be deleted by the caller with the operator
 * delete
 */
SyncManager* SyncManagerFactory::getSyncManager(wchar_t* applicationURI) {
    LOG.setLogPath( TEXT( "/tmp" ) ); 
    LOG.reset();
    LOG.infoEnabled = TRUE;
    LOG.debugEnabled = TRUE;

    LOG.info(INITIALIZING);
    //
    // Reads the configuration from the management node and fill
    // the config object. This will be passed to the SyncManager
    // constructor
    //
    Config* config = new Config( applicationURI );
    if (lastErrorCode != 0) {
        LOG.error(lastErrorMsg);
        return NULL;
    }
    
    //
    // At this point we can create the SyncManager
    //
    SyncManager* manager = new SyncManager( config );
    return manager;
}
