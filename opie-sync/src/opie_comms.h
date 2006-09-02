#ifndef _OPIE_COMMS_H_
#define _OPIE_COMMS_H_

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
 *  $Id: opie_comms.h,v 1.5 2004/02/20 15:55:14 irix Exp $
 */


#include "opie_sync.h"

typedef enum {
  OPIE_OBJECT_TYPE_UNKNOWN = 0x00,
  OPIE_OBJECT_TYPE_CALENDAR = 0x01,
  OPIE_OBJECT_TYPE_PHONEBOOK = 0x02,
  OPIE_OBJECT_TYPE_TODO = 0x04,
  OPIE_OBJECT_TYPE_ANY = 0xff
} opie_object_type;


/* initialize and cleanup the comms layer - call only once per plugin */
void comms_init();
void comms_shutdown();


/* connect to the device and pull down the data */
gboolean opie_connect_and_fetch(OpieSyncEnv* env, opie_object_type object_types);


/* connect to the device and push the files back */
gboolean opie_connect_and_put( OpieSyncEnv* env, opie_object_type object_types);



#endif
