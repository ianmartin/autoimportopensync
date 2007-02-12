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
// @version $Id: SPDSUtils.cpp,v 1.4 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include "spds/common/Utils.h"

/*
 * Returns the SyncMode code corresponding to the given text version.
 * syncMode must be one of:
 *  . none
 *  . slow
 *  . two-way
 *  . one-way
 *  . refresh
 *
 * @param syncMode - the text representation of the sync mode
 *
 * NOTE: if the given sync mode text is not one of the above, SYNC_NONE
 * is returned;
 */
SyncMode syncModeCode(const wchar_t* syncMode) {

    if (wcscmp(syncMode, TEXT("slow")) == 0) {
        return SYNC_SLOW;
    } else if (wcscmp(syncMode, TEXT("two-way")) == 0) {
        return SYNC_TWO_WAY;
    } else if (wcscmp(syncMode, TEXT("one-way")) == 0) {
        return SYNC_ONE_WAY_FROM_SERVER;
    } else if (wcscmp(syncMode, TEXT("refresh")) == 0) {
        return SYNC_REFRESH_FROM_SERVER;
    }

    return SYNC_NONE;
}


/*
 * Deletes the given wchar_t[] buffer if it is not NULL
 * and sets the pointer to NULL
 *
 */
void safeDelete(wchar_t* p[]) {
    if (*p != NULL) {
        delete [] *p; *p = NULL;
    }
}

/*
 * Deletes each element of the given array of allocated objects/buffers. The
 * array pointer is not released.
 *
 */
void safeDelete(void* p[], unsigned int n) {
    unsigned int i;

    for (i = 0; i < n; ++i) {
        if (p[i] != NULL) {
            delete p[i];
        }
    }
}

