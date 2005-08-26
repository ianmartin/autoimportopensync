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

#ifndef COBEXBFB_H
#define COBEXBFB_H

#include <stdint.h>
#include <bfb/bfb.h>
#include <openobex/obex.h>

#define	RECVSIZE 500	/* Recieve up to this much from socket */

enum cobex_type
{
        CT_SIEMENS,
        CT_ERICSSON
};

typedef struct {
	enum cobex_type type;	/* Type of connected mobile */
	uint8_t recv[RECVSIZE];	/* Buffer socket input */
	int recv_len;
	uint8_t seq;
	bfb_data_t *data_buf;	/* assembled obex frames */
	int data_size;		/* max buffer size */
	int data_len;		/* filled buffer length */
} cobex_t;

#define	COBEX_BFB_LOG_DOMAIN	"cobex-bfb"


/* callbacks */

int	cobex_connect (obex_t *self, void *data);
int	cobex_disconnect (obex_t *self, void *data);
int	cobex_write (obex_t *self, void *data, uint8_t *buffer, int length);
int	cobex_handleinput (obex_t *self, void *data, int timeout);


#endif /* COBEXBFB_H */
