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
// @version $Id: SyncItemHolder.cpp,v 1.5 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include "spds/common/SyncSource.h"
#include "spds/common/SyncItemHolder.h"

/*
 * This class has the only scope to hold a SyncItem looking as a PTypes
 * unknown object. It is used to being able to add SyncItems into PTypes
 * collections.
 */

SyncItemHolder::SyncItemHolder(SyncItem* i) : unknown() {
    item = i;
}

/*
 * Returns the hold SyncItem object.
 */
SyncItem* SyncItemHolder::getItem() {
    return item;
}
