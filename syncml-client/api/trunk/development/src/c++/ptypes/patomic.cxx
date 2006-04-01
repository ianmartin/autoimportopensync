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
#include "ptypes/ptypes.h"


PTYPES_BEGIN


int __PFASTCALL pexchange(int* target, int value)
{
    int r = *target;
    *target = value;
    return r;
}


void* __PFASTCALL pexchange(void** target, void* value)
{
    void* r = *target;
    *target = value;
    return r;
}


int __PFASTCALL pincrement(int* target)
{
    return ++(*target);
}


int __PFASTCALL pdecrement(int* target)
{
    return --(*target);
}

PTYPES_END


