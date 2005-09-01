/* 
   MultiSync IrMC Plugin - Synchronize IrMC (mobile) devices
   Copyright (C) 2002-2003 Bo Lincoln <lincoln@lysator.liu.se>

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
 *  $Id: irmc_bluetooth.c,v 1.21 2003/09/09 19:49:03 lincoln Exp $
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>
#include <config.h>

#include "irmc_obex.h"
#include "irmc_bluetooth.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <openobex/obex.h>

extern gboolean multisync_debug;

int rfcomm_connect(struct bt_unit *btu, int channel) {
  struct sockaddr_rc svr_addr, my_addr;
  int s;
  bdaddr_t tmp;

  svr_addr.rc_family = AF_BLUETOOTH;
  bacpy(&svr_addr.rc_bdaddr, &btu->bdaddr);
  svr_addr.rc_channel = channel;
  
  my_addr.rc_family = AF_BLUETOOTH;
  my_addr.rc_channel = channel;
  bacpy(&my_addr.rc_bdaddr, BDADDR_ANY);
  
  baswap(&tmp, &btu->bdaddr);
  osync_trace(TRACE_INTERNAL, "Trying to connect on to %s... ", batostr(&tmp));
  fflush(stdout);
  if( (s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM))==-1 ) {
    osync_trace(TRACE_INTERNAL, "Can't create socket. %s(%d)\n", strerror(errno), errno);
    return(-1);
  }

  if( !connect(s, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) ){
    osync_trace(TRACE_INTERNAL, "OK");
  } else {
    osync_trace(TRACE_INTERNAL, "Connect failed. %s(%d)\n", strerror(errno), errno);
    close(s);
    return(-1);
  }
  return(s);
}

void plugin_init(void) {
}


GList *find_bt_units() {
  GList *unitlist = NULL;
  inquiry_info ii[10];
  int numfound = 0;
  if (!sdp_general_inquiry(ii, 10, 10000, (uint8_t*)&numfound)) {
    int t;
    for (t = 0; t < numfound; t++) {
      bdaddr_t tmp;
      int retries = 3;
      irmc_bt_unit *irbt = g_malloc0(sizeof(irmc_bt_unit));
      int dd = hci_open_dev(0); // Open first HCI device
      sdp_list_t *services = NULL;
      sdp_list_t *attributes = NULL;
      sdp_list_t *reclist = NULL;
      uint16_t len = 0;
      uuid_t serviceuuid;
      uint32_t attributeid = 0x0000ffff;
      sdp_list_t *protodescs = NULL;
      sdp_session_t *sess = NULL;

      g_assert(irbt);
      baswap(&tmp, &ii[t].bdaddr);
      strncpy(irbt->address, batostr(&tmp), 20);
      irbt->channel = -1;
      irbt->name[0] = 0;
      if (dd >= 0) {
	hci_read_remote_name(dd, &ii[t].bdaddr, 32, irbt->name, 10000);
	hci_close_dev(dd);
      }
      
      // Now lookup the RFCOMM channel for IrMC Sync using SDP
	  
      while (retries-- > 0 &&
	     !(sess = sdp_connect(BDADDR_ANY, &ii[t].bdaddr, 0)))
	sleep(1);
      if (sess) {
	sdp_uuid16_create(&serviceuuid, IRMC_SYNC_SVCLASS_ID);
	services = sdp_list_append(0, &serviceuuid);
	attributes = sdp_list_append(0, &attributeid);
	
	sdp_service_search_attr_req(sess, services,
				    SDP_ATTR_REQ_RANGE,
				    attributes, &reclist);
	
	sdp_list_free(services, 0);
	sdp_list_free(attributes, 0);
	
	if (reclist) {
	  sdp_record_t *rec = (sdp_record_t*) reclist->data;
	  sdp_list_t *access = NULL;
	  sdp_get_access_protos(rec, &access);
	  
	  if (access) {
	    irbt->channel = sdp_get_proto_port(access, RFCOMM_UUID);
	  }
	}
	sdp_close(sess);
      } else {
      }
      unitlist = g_list_append(unitlist, irbt);
    }
  }
  return(unitlist);
}

gint obex_connect(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  userdata->fd = rfcomm_connect(&userdata->btu, userdata->channel);
  if (userdata->fd < 0) {
    return(-1);
  }
  fcntl(userdata->fd, F_SETFL, O_NONBLOCK);
  return(0);
}


gint obex_disconnect(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  close(userdata->fd);
  return(0);
}

gint obex_listen(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  return(0);
}

gint obex_write(obex_t *handle, gpointer ud, 
	       guint8 *buf, gint buflen) {
  int written = 0;
  int ret = 0;
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;

  while (ret >= 0 && written < buflen) {
    ret=write(userdata->fd,buf+written,buflen-written);
    if (ret >= 0)
      written+=ret;
  }
  return(written);
}

gint obex_handleinput(obex_t *handle, gpointer ud, gint timeout) {
  int n;
  struct timeval to;
  fd_set readfds;
  char buf[2048];
  int tot=0;
  int ret = 0;
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  n = userdata->fd + 1;
  FD_ZERO(&readfds);
  FD_SET(userdata->fd, &readfds);
  to.tv_sec = timeout;
  to.tv_usec = 0;
  while (userdata->state >= 0 &&
	 (ret = select(n, &readfds, NULL, NULL, &to)) > 0) {
    if((tot = read(userdata->fd, buf, 2048)) > 0) {
      OBEX_CustomDataFeed(handle, buf, tot);
    } else { 
      userdata->state = IRMC_OBEX_REQFAILED;
//      userdata->error = SYNC_MSG_CONNECTIONERROR;
    }
    to.tv_sec = timeout;
    to.tv_usec = 0;
  }
  if (userdata->state >= 0 && ret == 0) {
    userdata->state = IRMC_OBEX_REQFAILED;
//    userdata->error = SYNC_MSG_CONNECTIONERROR;
  }
  return(0);
}

void irmc_strtoba(bdaddr_t *bdaddr,char *str ) {
  baswap(bdaddr, strtoba(str));
}

char *irmc_batostr(bdaddr_t *bdaddr) {
  bdaddr_t tmp;
  
  baswap(&tmp, bdaddr);
  return(batostr(&tmp));
}
