#ifndef _SYNC_ECAL_H_
#define _SYNC_ECAL_H_

/* 
   MultiSync Evolution2 Plugin
   Copyright (C) 2004 Tom Foottit <tom@foottit.com>
   Copyright (C) 2004 Bo Lincoln <lincoln@lysator.liu.se>

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
 *  $Id: sync_ecal.h,v 1.1.2.3 2004/04/17 01:44:37 irix Exp $
 */

#include <multisync.h>
#include <libecal/e-cal.h>

#include "evolution_sync.h"

sync_object_type evo2_cal_object_type_from_component(ECalComponent *comp);

ECal* evo2_cal_open(const char *uri);

int evo2_cal_get_changes(eds_conn_t *conn, sync_object_type newdbs, GList **changes);

gboolean evo2_cal_delete(eds_conn_t *conn, char* uid);

gboolean evo2_cal_create(eds_conn_t *conn, char* object, char* returnuid, int *returnuidlen);

gboolean evo2_cal_update(eds_conn_t *conn, char* uid, char* object);

#endif
