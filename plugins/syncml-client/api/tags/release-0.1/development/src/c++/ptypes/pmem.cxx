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

/*
 * This software is based on the C++ Portable Types Library (PTypes) by
 * Hovik Melikyan
 *
 *  C++ Portable Types Library (PTypes)
 *  Version 1.8.3  Released 25-Aug-2003
 *
 *  Copyright (c) 2001, 2002, 2003 Hovik Melikyan
 *
 *  http://www.melikyan.com/ptypes/
 *  http://ptypes.sourceforge.net/
 *
 */
#include "unixdefs.h"

#include <stdlib.h>

#include "ptypes/pport.h"


PTYPES_BEGIN


void memerror()
{
    fatal(CRIT_FIRST + 5, "Not enough memory");
}


void* memalloc(uint a)
{
    if (a == 0)
        return nil;
    else
    {
        void* p = calloc(1, a);
        if (p == nil)
            memerror();
        return p;
    }
}


void* memrealloc(void* p, uint a)
{
    if (a == 0)
    {
    memfree(p);
    return nil;
    }
    else if (p == nil)
    return memalloc(a);
    else
    {
    p = realloc(p, a);
        if (p == nil)
        memerror();
        return p;

    }
}


void memfree(void* p)
{
    if (p != nil)
        free(p);
}


PTYPES_END


