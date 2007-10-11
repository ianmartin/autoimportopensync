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

#ifndef INCL_COMMON_WMEM
    #define INCL_COMMON_WMEM

    #include <stdlib.h>

    inline void* __cdecl wmemmove(void *d, const void *s, size_t l)       { return memmove(d, s, l*sizeof(wchar_t)); };
    inline void* __cdecl wmemcpy (void *d, const void *s, size_t l)       { return memcpy (d, s, l*sizeof(wchar_t)); };
    inline void* __cdecl wmemset (void *d, wchar_t c, size_t l)           { return memset (d, c, l*sizeof(wchar_t)); };
    inline int   __cdecl wmemcmp (const void *d, const void *s, size_t l) { return memcmp (d, s, l*sizeof(wchar_t)); };
#endif