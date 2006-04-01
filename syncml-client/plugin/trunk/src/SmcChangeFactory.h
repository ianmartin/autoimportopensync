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

#ifndef  SMC_CHANGE_FACTORY
#define  SMC_CHANGE_FACTORY

#include <opensync/opensync.h>

#include <syncml-client.h>
//#include "syncml-client-plugin.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "SmcConfig.h"

class SmcChangeFactory {

    protected:

       char*         objFormat;
       char*         objType;
       OSyncMember*  member;

       char*         wcstombs_dup( const wchar_t* wcs );


    public:

        SmcChangeFactory( OSyncMember* member );
        ~SmcChangeFactory(); 

        OSyncChange* getChange( SyncItem* item, OSyncChangeType  changeType );


};

#endif


