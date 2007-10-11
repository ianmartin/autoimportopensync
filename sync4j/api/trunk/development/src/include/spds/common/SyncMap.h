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

#ifndef INCL_SYNC_MAP
#define INCL_SYNC_MAP

#include "spds/common/Constants.h"

    class /*__declspec(dllexport)*/ SyncMap {

    private:
        wchar_t luid[DIM_KEY];
        wchar_t guid[DIM_KEY];


    public:
        SyncMap(wchar_t* guid, wchar_t* luid);

        /*
         * Returns the guid of this mapping. If guid is NULL, the internal
         * buffer address is returned, otherwise the value is copied into the
         * given buffer and its pointer is returned.
         *
         * @param guid - the buffer where the guid is copied to. It must be
         *               big enough
         */
        wchar_t* getGUID(wchar_t* guid);


        /*
         * Returns the luid of this mapping. If luid is NULL, the internal
         * buffer address is returned, otherwise the value is copied into the
         * given buffer and its pointer is returned.
         *
         * @param luid - the buffer where the luid is copied to. It must be
         *               big enough
         */
        wchar_t* getLUID(wchar_t* luid);
    };

#endif



