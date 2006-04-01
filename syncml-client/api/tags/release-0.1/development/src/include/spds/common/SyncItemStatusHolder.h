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

#ifndef INCL_SYNC_ITEM_STATUS_HOLDER
#define INCL_SYNC_ITEM_STATUS_HOLDER

#include "ptypes/ptypes.h"

#include "spds/common/SyncSource.h"

    /*
     * This class has the only scope to hold a SyncItemStatus looking as a PTypes
     * unknown object. It is used to being able to add SyncItemStatus into PTypes
     * collections.
     */


    USING_PTYPES

    class SyncItemStatusHolder : public unknown {

    public:
        SyncItemStatus* itemStatus;

        SyncItemStatusHolder(SyncItemStatus* itemStatus);
        SyncItemStatus* getItemStatus();
    };
#endif


