#ifndef _OPIE_DEBUG_H_
#define _OPIE_DEBUG_H_

/* 
   MultiSync Opie Plugin - Synchronize Opie/Zaurus Devices
   Copyright (C) 2003 Tom Foottit <tom@foottit.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
   SOFTWARE IS DISCLAIMED.
*/

/*
 *  $Id: opie_debug.h,v 1.2.2.1 2004/04/12 20:22:37 irix Exp $
 */


#include <stdio.h>
#include <glib.h>

#ifndef OPIE_DEBUG
#define OPIE_DEBUG(x) (printf(x))
#endif

/* Use the OPIE_PRINT macro instead of printf.
 * For general purpose debug logging in the 
 * field, use OPIE_DEBUG from opie_log.h
 */

#ifdef _OPIE_PRINT_DEBUG
#define OPIE_PRINT(x) do { printf x; } while (0);
#else
#define OPIE_PRINT(x)
#endif

#endif
