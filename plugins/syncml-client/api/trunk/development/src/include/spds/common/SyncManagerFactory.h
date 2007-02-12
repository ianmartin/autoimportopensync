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

#ifndef INCL_SYNC_MANAGER_FACTORY
#define INCL_SYNC_MANAGER_FACTORY

#include "spds/common/SyncManager.h"

    /*
     * This class is a factory of SyncManager objects. It creates the new
     * SyncManager instance and configures it with the information
     * extracted from the DeviceManager layer.
     */
    class __declspec(dllexport) SyncManagerFactory {

    public:
        /*
         * Constructor.
         */
        SyncManagerFactory();

        /*
         * Creates and returns a new SyncManager. The SyncManager object is create
         * with the new operator and must be deleted by the caller with the operator
         * delete
         *
         * @param applicationURI the application specific initial context
         */
        SyncManager *getSyncManager(wchar_t* applicationURI);

    };

#endif



