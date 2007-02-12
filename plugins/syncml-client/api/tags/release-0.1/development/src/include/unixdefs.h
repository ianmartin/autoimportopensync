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

#ifndef UNIXDEFS
#define UNIXDEFS

#include    <stdint.h>
#include    <iconv.h>
#include    <string.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <stdarg.h>
#include    <wchar.h>
#include    <wctype.h>
#include    <math.h>
#include    <assert.h>
#include    <values.h>
#include    <time.h>
#include    <sys/time.h>

#define LINUX

#define __cdecl
#define __declspec(arg)
#define __declspec(arg)

#define TEXT(x)     L##x
//#define min(x,y)    ( x < y ? x : y )

#ifndef FALSE
#define FALSE       0
#define TRUE        1
#endif

#define BOOL        int
#define LPBOOL      int*
#define DWORD       int32_t
#define LPCSTR      const char*
#define LPSTR       char*
#define LPCWSTR     const wchar_t*
#define LPWSTR      wchar_t*
#define UINT        unsigned int


#define CP_ACP      0


extern FILE*    _wfopen( wchar_t* filename, wchar_t* mode );

/**
*
* Return a wchar_t* (Unicode)  heap allocated string from multibyte 
* (UTF) argument.  Caller should invoke delete[] after using return value.
*/
extern wchar_t*  new_mbsrtowcs( const char* input, size_t maxlen = INT_MAX );

/** 
*
* Return a UTF string allocated on heap from Unicode input 
* Caller should invoke delete[] after using return value.
*
*/
extern char* new_wcstombs( const wchar_t* wcs );


#endif
