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
#include <stdio.h>
#include <string.h>

#include "ptypes/pport.h"

#if defined(WIN32) && !defined(NO_CRIT_MSGBOX)
#  include <windows.h>
#  define CRIT_MSGBOX
#endif


PTYPES_BEGIN


static void defhandler(int code, const char* msg)
{
#ifdef CRIT_MSGBOX
    char buf[2048];
    _snprintf(buf, sizeof(buf) - 1, "Fatal [%05x]: %s", code, msg);
    MessageBox(0, buf, "Internal error", MB_OK | MB_ICONSTOP);
#else
    fprintf(stderr, "\nInternal [%04x]: %s\n", code, msg);
#endif
}

static _pcrithandler crith = defhandler;


_pcrithandler getcrithandler()
{
    return crith;
}


_pcrithandler setcrithandler(_pcrithandler newh)
{
    _pcrithandler ret = crith;
    crith = newh;
    return ret;
}


void fatal(int code, const char* msg)
{
    if (crith != nil)
        (*crith)(code, msg);
    exit(code);
}


PTYPES_END


