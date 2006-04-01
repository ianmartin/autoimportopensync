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

#ifndef INCL_DEVICE_MANAGER_UTIL_SPDS
#define INCL_DEVICE_MANAGER_UTIL_SPDS

#include "spds/common/Constants.h"
#include "spds/common/SyncItem.h"

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
    SyncMode syncModeCode(const wchar_t* syncMode);


    /*
     * Deletes the given wchar_t[] buffer if it is not NULL
     * and sets the pointer to NULL
     *
     */
    void safeDelete(wchar_t* p[]);

    /*
     * Deletes each element of the given array of allocated objects/buffers. The
     * array pointer is not released.
     *
     */
    void safeDelete(void* p[], unsigned int n);
#endif


