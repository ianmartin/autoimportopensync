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

#include "ItemFactory.h"

/**
*
*   Used to convert OSyncChange items to SyncItem - that
*   is from opensync to SyncML.
*
*/
ItemFactory::ItemFactory( char* mimeType )
{
    size_t len =  strlen( mimeType );
    this->mimeType =  new wchar_t[ len + 1 ];
    swprintf( this->mimeType, len, L"%s", mimeType );
    this->mimeType[ len ] = L'\0';
}

ItemFactory::~ItemFactory()
{
    delete[] mimeType;
}

/**
*
*  Convert a OSyncChange* to a SyncItem*
*
*  @param change - input, not changed.
*  @return A SyncItem allocated with new.
*
*/
SyncItem* ItemFactory::getItem( OSyncChange*  change )
{
    wchar_t* wcsKey;

    wcsKey = new_mbsrtowcs( osync_change_get_uid( change ) );
    SyncItem* syncItem = new SyncItem( wcsKey );
    delete[] wcsKey;

    if( osync_change_has_data( change ) ){
        int len = osync_change_get_datasize( change );
        wchar_t* buff = new wchar_t[ len ];
        swprintf( buff, len, L"%s",
                  osync_change_get_data( change ) );
        syncItem->setData( buff,  len * sizeof( wchar_t ) );

        LOG.setMessage( L"Creating SyncItem, size: %ld, data:\n%ls\n",
                        syncItem->getDataSize(), syncItem->getData() ),
        LOG.debug();
        delete[] buff;
    }
    syncItem->setDataType( mimeType );
    return syncItem;
}


