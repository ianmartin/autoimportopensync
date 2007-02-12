/*
 * Copyright (C)  Michael Kolmodin
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
// @author Michael Kolmodin
// @version $Id: Error.cpp,v 1.7 2005/01/19 10:22:06 magi Exp $
//

#include <wchar.h>
#include <stdio.h>

#include "unixdefs.h"
#include "common/fscapi.h"
#include "common/Constants.h"

char* new_wcstombs( const wchar_t* wcs )
{
    if( wcs == 0 )
        return 0; 

    size_t size =  wcstombs(NULL, wcs,  0 ) ;
    char* buff = new char[ size + 1 ];
    wcstombs( buff, wcs, size + 1 );
    return( buff );
}

wchar_t* new_mbsrtowcs( const char* mbs, size_t maxLength )
{
    if( mbs == 0 )
       return 0; 

    mbstate_t mbstate;
    memset( &mbstate, 0, sizeof( mbstate_t ) );
    const char* src = mbs;

    size_t size = mbsrtowcs(NULL, &src, 0, &mbstate );
    assert( size >= 0 );
    size = ( maxLength < size ? maxLength : size );

    wchar_t* retBuffer = new wchar_t[ size + 1 ];
    src = mbs;
    memset( &mbstate, 0, sizeof( mbstate_t ) );
    mbsrtowcs( retBuffer, &src, size, &mbstate );
    retBuffer[ size ] = L'\0';

    return retBuffer;
}
