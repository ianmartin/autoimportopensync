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
// @version $Id: SyncMap.cpp,v 1.6 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include <stdio.h>

#include "spds/common/SyncMap.h"

SyncMap::SyncMap(wchar_t *g, wchar_t* l) {
    wcsncpy(guid, g, DIM_KEY); guid[DIM_KEY-1] = 0;
    wcsncpy(luid, l, DIM_KEY); luid[DIM_KEY-1] = 0;
}

/*
 * Returns the luid of this mapping. If luid is NULL, the internal
 * buffer address is returned, otherwise the value is copied into the
 * given buffer and its pointer is returned.
 *
 * @param luid - the buffer where the luid is copied to. It must be
 *               big enough
 */
wchar_t* SyncMap::getGUID(wchar_t* g) {
    if (g == NULL) {
        return guid;
    }

    return wcscpy(g, guid);
}


/*
 * Returns the luid of this mapping
 *
 * @param l - the buffer where the luid is copied to. It must be
 *               big enough
 */
wchar_t* SyncMap::getLUID(wchar_t* l) {
    if (l == NULL) {
            return luid;
        }

    return wcscpy(l, luid);
}
