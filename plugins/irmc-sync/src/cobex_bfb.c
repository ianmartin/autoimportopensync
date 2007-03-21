/*
 *   MultiSync IrMC Plugin - Synchronize IrMC (mobile) devices
 *   cobex_bfb: Talk OBEX over a serial port (Siemens specific)
 *
 *   Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 *   Modified for the MultiSync project by Bo Lincoln <lincoln@lysator.liu.se>
 *  
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

/*
 *  $Id: cobex_bfb.c,v 1.11 2004/02/09 18:53:31 lincoln Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "irmc_obex.h"
#include <openobex/obex.h>
#include "cobex_bfb.h"
#include "bfb/bfb.h"
#include "bfb/bfb_io.h"
#include "bfb/common.h"

static void cobex_cleanup(obexdata_t *c, int force)
{
  return_if_fail (c != NULL);
#ifdef _WIN32
  return_if_fail (c->fd != INVALID_HANDLE_VALUE);
#else
  return_if_fail (c->fd > 0);
#endif

  bfb_io_close(c->fd, force);

#ifdef _WIN32
	c->fd = INVALID_HANDLE_VALUE;
#else
  c->fd = -1;
#endif
}

int cobex_connect(obex_t *self, void *data)
{
	obexdata_t *c;
	int typeinfo;
  return_val_if_fail (self != NULL, -1);
  return_val_if_fail (data != NULL, -1);
	c = (obexdata_t *) data;

  DEBUG(3, "%s() \n", __func__);

	c->fd = bfb_io_open(c->cabledev, &typeinfo);
	DEBUG(3, "%s() bfb_io_open returned %d, %d\n", __func__, c->fd, typeinfo);
	if(typeinfo == 2) {
		c->cobex.type = CT_ERICSSON;
    c->cabletype = IRMC_CABLE_ERICSSON;
	} else {
		c->cobex.type = CT_SIEMENS;
    c->cabletype = IRMC_CABLE_SIEMENS;
  }

#ifdef _WIN32
	if(c->fd == INVALID_HANDLE_VALUE)
#else
  if(c->fd == -1)
#endif
    return -1;

  return 1;
}

int cobex_disconnect(obex_t *self, void *data)
{
	obexdata_t *c;
  return_val_if_fail (self != NULL, -1);
  return_val_if_fail (data != NULL, -1);

	c = (obexdata_t *) data;

  DEBUG(3, "%s() \n", __func__);
  cobex_cleanup(c, FALSE);
  return 1;
}

// FIXME: All of this is quite ugly, since I borrowed the Siemens bfb 
// code and tried to incorporate it with the Ericsson code. 
// Hard to debug without hardware...

/* Called from OBEX-lib when data needs to be written */
int cobex_write(obex_t *self, void *data, uint8_t *buffer, int length)
{
  int actual;
	obexdata_t *c;
  return_val_if_fail (self != NULL, -1);
  return_val_if_fail (data != NULL, -1);
	c = (obexdata_t *) data;
	
  DEBUG(3, "%s() \n", __func__);

  DEBUG(3, "%s() Data %d bytes\n", __func__, length);

	if (c->cobex.type == CT_ERICSSON) {
		actual = write(c->fd, buffer, length);
		if (actual < length)	{
			DEBUG(1, "Error writing to port (%d expected %d)\n", actual, length);
			return actual; /* or -1? */
		}
		return actual;
	}

  if (c->cobex.seq == 0){
    actual = bfb_send_first(c->fd, buffer, length);
    DEBUG(2, "%s() Wrote %d first packets (%d bytes)\n", __func__, actual, length);
  } else {
    actual = bfb_send_next(c->fd, buffer, length, c->cobex.seq);
    DEBUG(2, "%s() Wrote %d packets (%d bytes)\n", __func__, actual, length);
  }
  c->cobex.seq++;

  return actual;
}

/* Called when input data is needed */
int cobex_handleinput(obex_t *self, void *data, int timeout)
{
#ifdef _WIN32
	DWORD actual;
#else
  struct timeval time;
  fd_set fdset;
  int actual = 0;
#endif
  bfb_frame_t *frame;

  obexdata_t *c;

  return_val_if_fail (self != NULL, -1);
  return_val_if_fail (data != NULL, -1);
  c = (obexdata_t *) data;
  
#ifdef _WIN32
	if (!ReadFile(c->fd, &(c->recv[c->cobex.recv_len]), sizeof(c->cobex.recv) - c->cobex.recv_len, &actual, NULL))
		DEBUG(2, "%s() Read error: %ld\n", __func__, actual);

	DEBUG(2, "%s() Read %ld bytes (%d bytes already buffered)\n", __func__, actual, c->cobex.recv_len);
	/* FIXME ... */
#else
  time.tv_sec = timeout;
  time.tv_usec = 0;

  /* Add the fd's to the set. */
  FD_ZERO(&fdset);
  FD_SET(c->fd, &fdset);

  /* Wait for input */
  DEBUG(2, "%s() Waiting for data.\n", __func__);
	actual = select(c->fd + 1, &fdset, NULL, NULL, &time);

  DEBUG(2, "%s() There is something (%d)\n", __func__, actual);
	  
  /* Check if this is a timeout (0) or error (-1) */
  if(actual <= 0) {
    c->state = IRMC_OBEX_REQFAILED;
    return actual;
  }
	  
  actual = read(c->fd, &(c->cobex.recv[c->cobex.recv_len]), sizeof(c->cobex.recv) - c->cobex.recv_len);
  DEBUG(2, "%s() Read %d bytes (%d bytes already buffered)\n", __func__, actual, c->cobex.recv_len);
#endif
	  
  if (c->cobex.type == CT_ERICSSON) {
    if (actual > 0) {
      OBEX_CustomDataFeed(self, c->cobex.recv, actual);
      return 1;
    }
    c->state = IRMC_OBEX_REQFAILED;
    return actual;
  }
	  
  if ((c->cobex.data_buf == NULL) || (c->cobex.data_size == 0)) {
    c->cobex.data_size = 1024;
    c->cobex.data_buf = malloc(c->cobex.data_size);
  }
	  
  if (actual > 0) {
    c->cobex.recv_len += actual;
    DEBUGBUFFER(c->cobex.recv, c->cobex.recv_len);
	    
    while ((frame = bfb_read_packets(c->cobex.recv, &(c->cobex.recv_len)))) {
      DEBUG(2, "%s() Parsed %x (%d bytes remaining)\n", __func__, frame->type, c->cobex.recv_len);
	      
      bfb_assemble_data(&c->cobex.data_buf, &c->cobex.data_size, &c->cobex.data_len, frame);
	      
      if (bfb_check_data(c->cobex.data_buf, c->cobex.data_len) == 1) {
        actual = bfb_send_ack(c->fd);
        DEBUG(2, "%s() Wrote ack packet (%d)\n", __func__, actual);
		
        OBEX_CustomDataFeed(self, c->cobex.data_buf->data, c->cobex.data_len-7);
    	  /*
	        free(c->cobex.data);
    	    c->cobex.data = NULL;
	       */
        c->cobex.data_len = 0;
		
    	  if (c->cobex.recv_len > 0) {
	        DEBUG(2, "%s() Data remaining after feed, this can't be good.\n", __func__);
  	      DEBUGBUFFER(c->cobex.recv, c->cobex.recv_len);
    	  }

        return 1;
      }
    }
  }

  return actual;
}