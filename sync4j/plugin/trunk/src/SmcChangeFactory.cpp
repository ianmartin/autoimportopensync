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

#include "SmcChangeFactory.h"
/**
*
*  A  SyncItem* -> OSyncChange* converter.
*
*/

const char* ContactPattern =  "*BEGIN:VCARD*VERSION*";
const char* EventPattern   =  "*BEGIN:VCALENDAR*VERSION:*BEGIN:VEVENT*";
const char* NotePattern    =  "*BEGIN:VNOTE*VERSION:*";
const char* TodoPattern    =  "*BEGIN:VCALENDAR*VERSION:*BEGIN:VTODO*";

SmcChangeFactory::SmcChangeFactory( OSyncMember* member )
{
    this->member = member;
}

SmcChangeFactory::~SmcChangeFactory()
{
}

char* SmcChangeFactory::wcstombs_dup( const wchar_t* wcs )
{
    if( wcs == 0 )
        return 0;

    size_t size =  wcstombs(NULL, wcs,  0 ) ;
    char* buff = (char*) g_malloc(( size + 1 ) * sizeof(char) );
    wcstombs( buff, wcs, size + 1 );
    return( buff );
}

char* getObjectType( const char* data )
{
    if( data == NULL )
        return NULL;

    if(  g_pattern_match_simple( ContactPattern, data ) )
        return "contact";
    else if( g_pattern_match_simple( TodoPattern, data ) )
         return "todo";
    else if( g_pattern_match_simple( EventPattern, data ))
         return "event";
    else if( g_pattern_match_simple( NotePattern, data ) )
         return "note";
    else
         return NULL;
}


/**
*
*  Convert a SyncItem to a OSyncChange.
*
*  @param SyncItem - Input, not changed.
*  @param changeType - governs the change's type.
*
*  @return A new OsyncChange* object, allocated through osync_change_new().
*
*/
OSyncChange* SmcChangeFactory::getChange( SyncItem*        item, 
                                          OSyncChangeType  changeType )
{
    OSyncChange *change = osync_change_new();
    char* objType = NULL;

    if( changeType != CHANGE_DELETED ){
        size_t charCnt  = item->getDataSize() / sizeof( wchar_t );
        char* data = (char*) g_malloc( (charCnt + 1) * sizeof( char ) );
        wcstombs( data, (wchar_t*) item->getData(), charCnt );
        data[ charCnt ] = '\0';

        objType =  getObjectType( data );
        if( objType == NULL )
            return( NULL );
        else
            osync_change_set_data( change, data, charCnt, TRUE );

        ObjectTypeConfig config;
        for( config.readFirst(); config.hasNext(); config.readNext() ) {
           if( strcmp( objType,  config.objType ) == 0 ) {
               osync_change_set_objformat_string( change,  config.objFormat );
               break;
           }
        }
    }
    
    osync_change_set_member( change, member );
    char* mbsKey = wcstombs_dup(  item->getKey( (wchar_t*) 0 ) );
    osync_change_set_uid( change, mbsKey );
    free( mbsKey );
    osync_change_set_objtype_string( change, objType );
    osync_change_set_changetype( change, changeType );


    return change;
}


