#ifndef _OPIE_QCOP_H_
#define _OPIE_QCOP_H_

/* 
   MultiSync Opie Plugin - Synchronize Opie/Zaurus Devices
   Copyright (C) 2003 Tom Foottit <tom@foottit.com>
                      Eike M. Lang <mail@elang.de> 

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
 *  $Id: opie_qcop.h,v 1.2 2003/09/22 21:35:29 irix Exp $
 */

/* #include "opie_comms.h" */

typedef struct 
{
  gboolean result;                /* result of the previous operation */
  char* resultmsg;                /* string associated with the result (error msg, etc) */  
  int socket;                     /* socket open to the QCopBridge server */
  gboolean syncing;               /* Tells us whether we are currently syncing */
  pthread_mutex_t access_mutex;   /* At times we have two threads sharing access, so we need this */
} qcop_conn;


typedef struct 
{
  qcop_conn* qconn;            /* the socket */
  void (*cancel_routine)();    /* routine to call if the user cancels the sync */
} qcop_monitor_data;


qcop_conn* qcop_connect(gchar* addr, gchar* username, gchar* password);
void qcop_disconnect(qcop_conn* qconn);
void qcop_freeqconn(qcop_conn* qconn);
char* qcop_get_root(qcop_conn* qconn);
void qcop_start_sync(qcop_conn* qconn, void (*cancel_routine)());
void qcop_stop_sync(qcop_conn* qconn);


#endif
