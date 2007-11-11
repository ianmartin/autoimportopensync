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
 *  $Id: irmc_irda.c,v 1.5 2003/07/17 19:14:11 lincoln Exp $ 
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>
#include <gmodule.h>
#ifdef HAVE_IRDA
#include <linux/types.h>
#include <linux/irda.h>
#endif
#include "irmc_obex.h"
#include "irmc_sync.h"

#include <openobex/obex.h>

#define DISC_BUF_LEN	sizeof(struct irda_device_list) + \
			sizeof(struct irda_device_info) * (10)

gint obex_irda_connect(obex_t *handle, gpointer ud) {
#ifdef HAVE_IRDA
  struct irda_device_list *list;
  unsigned char buf[DISC_BUF_LEN];
  int len;
  int i;
  unsigned char hints[4];	/* Hint be we filter on */
  obexdata_t *userdata;
  struct sockaddr_irda peer;
  userdata = (obexdata_t*) ud;

  userdata->fd = socket(AF_IRDA, SOCK_STREAM, 0);
  if(userdata->fd == -1) {
    osync_trace(TRACE_INTERNAL, "Can't create socket. %s(%d)\n", strerror(errno), errno);
    return(-1);
  }
  
  if (userdata->ir_addr) { 
    // We know the exact IR address (only for temporary use)
    peer.sir_family = AF_IRDA;
    peer.sir_lsap_sel = LSAP_ANY;
    peer.sir_addr = userdata->ir_addr;
    strcpy(peer.sir_name, "OBEX");
    if(!connect(userdata->fd, (struct sockaddr *) &peer, sizeof(peer)) ){
      fcntl(userdata->fd, F_SETFL, O_NONBLOCK);
      return(0);
    } else {
      close(userdata->fd);
      userdata->fd = -1;
      return(-1);
    }
  }
  
  hints[0] = HINT_EXTENSION;
  hints[1] = HINT_OBEX;
	
  /* Set the filter used for performing discovery */
  if (setsockopt(userdata->fd, SOL_IRLMP, IRLMP_HINT_MASK_SET,
		 hints, sizeof(hints))) {
    return(-1);
  }
	
  /* Set the list to point to the correct place */
  list = (struct irda_device_list *) buf;
  len = DISC_BUF_LEN;

  /* Perform a discovery and get device list */
  if (getsockopt(userdata->fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, (socklen_t *) &len)) {
    osync_trace(TRACE_INTERNAL, "Found no IR devices.\n");
    return(-1);
  }
	
  if (list->len <= 0) {
    osync_trace(TRACE_INTERNAL, "Found no IR devices.\n");
    return(-1);
  }

  for(i = 0; i < list->len; i++) {
    if (strcmp(list->dev[i].info, userdata->irunit.name) == 0) {
      peer.sir_family = AF_IRDA;
      peer.sir_lsap_sel = LSAP_ANY;
      peer.sir_addr = list->dev[i].daddr;
      strcpy(peer.sir_name, "OBEX");
      if(!connect(userdata->fd, (struct sockaddr *) &peer, sizeof(peer)) ){
	fcntl(userdata->fd, F_SETFL, O_NONBLOCK);
	return(0);
      }
    }
  }
  return(-1);
#else
  return(-1);
#endif
}

gint obex_irda_disconnect(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  if (userdata->fd >= 0) {
    close(userdata->fd);
  }
  return(0);
}

GList* find_irda_units(irmc_config *config) {
#ifdef HAVE_IRDA
  struct irda_device_list *list;
  unsigned char buf[DISC_BUF_LEN];
//int ret = -1; // unused!
//int err;  // unused!
  int len;
  int i;
  unsigned char hints[4];	/* Hint be we filter on */
  GList *found = NULL;
  int fd;

  fd = socket(AF_IRDA, SOCK_STREAM, 0);
  if(fd == -1) {
    osync_trace(TRACE_INTERNAL, "Can't create socket. %s(%d)\n", strerror(errno), errno);
    return(NULL);
  }
  if (fd < 0)
    return(NULL);
	
  hints[0] = HINT_EXTENSION;
  hints[1] = HINT_OBEX;
	
  /* Set the filter used for performing discovery */
  if (setsockopt(fd, SOL_IRLMP, IRLMP_HINT_MASK_SET,
		 hints, sizeof(hints))) {
    return(NULL);
  }
	
  /* Set the list to point to the correct place */
  list = (struct irda_device_list *) buf;
  len = DISC_BUF_LEN;

  /* Perform a discovery and get device list */
  if (getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, (socklen_t *) len)) {
    osync_trace(TRACE_INTERNAL, "Found no IR devices.\n");
    return(NULL);
  }
	
  if (list->len <= 0) {
    osync_trace(TRACE_INTERNAL, "Found no IR devices.\n");
    return(NULL);
  }

  for(i = 0; i < list->len; i++) {
    irmc_ir_unit *iru;
    char *sn;

    iru = g_malloc0(sizeof(irmc_ir_unit));
    g_assert(iru);
    strncpy(iru->name, list->dev[i].info, 32);
    config->ir_addr = list->dev[i].daddr; // Tell IrOBEX to use this abs address
    sn = sync_connect_get_serial(config);
    config->ir_addr = 0;
    if (sn) {
      strncpy(iru->serial, sn, 127);
      g_free(sn);
    }
    found = g_list_append(found, iru);
  }
  return(found);
#else
  return(NULL);
#endif
}
