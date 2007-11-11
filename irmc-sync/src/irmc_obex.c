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
 *  $Id: irmc_obex.c,v 1.30 2004/02/09 18:53:31 lincoln Exp $
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
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <glib.h>
#include <gmodule.h>
#ifdef HAVE_IRDA
#include <linux/types.h>
#include <linux/irda.h>
#endif
#include "irmc_obex.h"
#include "irmc_sync.h"
#include "irmc_irda.h"

#include <openobex/obex.h>

gpointer (*plugin_function)();
#define CALL_PLUGIN(mod, name, args) (g_module_symbol(mod,name,(gpointer*)&plugin_function)?(*plugin_function)args:NULL)
GModule *bluetoothplugin = NULL;

extern gboolean multisync_debug;

void obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
{
  osync_trace(TRACE_ENTRY, "%s(%p,%p,%i,%i,%i, %i)", __func__, handle, object, mode, event, obex_cmd, obex_rsp);
  obexdata_t *userdata;
  osync_trace(TRACE_INTERNAL, "obex event: %i", event);

  userdata = (obexdata_t*) OBEX_GetUserData(handle);
  switch (event)	{
  case OBEX_EV_PROGRESS:
    break;
  case OBEX_EV_REQDONE:
    userdata->busy = 0;
    /* we should act as client all the day. */
    client_done(handle, object, obex_cmd, obex_rsp);
    break;
  case OBEX_EV_REQHINT:
    /* Comes BEFORE the lib parses anything. */
    switch(obex_cmd) {
    case OBEX_CMD_PUT:
    case OBEX_CMD_CONNECT:
    case OBEX_CMD_DISCONNECT:
      OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
      break;
    default:
      OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
      break;
    }
    break;
  case OBEX_EV_REQ:
    /* Comes when a server-request has been received. */
    //server_request(object, event, obex_cmd);
    break;
  case OBEX_EV_LINKERR:
  case OBEX_EV_PARSEERR:
  case OBEX_EV_ABORT:
    userdata->state = IRMC_OBEX_REQFAILED;
    osync_error_set(userdata->error, OSYNC_ERROR_NO_CONNECTION, "Request failed." );
    break;
  default:
    g_print("Unknown event!\n");
    break;
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

void client_done(obex_t *handle, obex_object_t *object, 
		 gint obex_cmd, gint obex_rsp)
{
  obexdata_t *ud;
  ud = OBEX_GetUserData(handle);
  
  ud->state = IRMC_OBEX_REQDONE;

  switch(obex_cmd)	{
  case OBEX_CMD_CONNECT:
    break;
  case OBEX_CMD_DISCONNECT:
    break;
  case OBEX_CMD_PUT:
    put_client_done(handle, object, obex_rsp);
    break;
  case OBEX_CMD_GET:
    get_client_done(handle, object, obex_rsp);
    break;
  case OBEX_CMD_SETPATH:
    break;
  }
}

void put_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp) {
  obexdata_t *ud;
  obex_headerdata_t hv;
  guint8 hi;
  gint hlen;
  const char *body = NULL;
  int body_len = 0;

  ud = OBEX_GetUserData(handle);
  if(obex_rsp != OBEX_RSP_SUCCESS) {
    ud->state = IRMC_OBEX_REQFAILED;
    return;
  }

  while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, (uint32_t *) &hlen))	{
    if (hi == OBEX_HDR_APPARAM) {
      body = (char*) hv.bs;
      body_len = (int) hlen;
    }
  }
  if (body && ud->databuf && ud->databuflen && *(ud->databuflen) >= body_len) {
    // Copy received data to buffer
    memcpy(ud->databuf, body, body_len);
    *(ud->databuflen) = body_len;
  }
  if (!body)
    *(ud->databuflen) = 0;

}

void get_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp) {
  obex_headerdata_t hv;
  guint8 hi;
  gint hlen;
  obexdata_t *ud;
  const char *body = NULL;
  int body_len = 0;
  
  ud = OBEX_GetUserData(handle);

  if(obex_rsp != OBEX_RSP_SUCCESS) {
    ud->state = IRMC_OBEX_REQFAILED;
    return;
  }
  
  while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, (uint32_t *) &hlen))	{
    if(hi == OBEX_HDR_BODY)	{
      body = (char*) hv.bs;
      body_len = (int) hlen;
      break;
    }
  }
  
  if(!body) {
    ud->state = IRMC_OBEX_REQFAILED;
    osync_error_set(ud->error, OSYNC_ERROR_GENERIC, "Obex protocol error");
    return;
  }	
  if (ud->databuf && ud->databuflen && *(ud->databuflen) >= body_len) {
    // Copy received data to buffer
    memcpy(ud->databuf, body, body_len);
    *(ud->databuflen) = body_len;
  } else
    ud->state = IRMC_OBEX_REQFAILED;
}

//
// Send an AT-command and return back one line of answer if any.
// Code stolen from open-obex-apps-0.98
//

gint obex_cable_at(obexdata_t *userdata, gchar *cmd, gchar *rspbuf, gint rspbuflen, gint timeout)
{
  fd_set ttyset;
  struct timeval tv;
  gint fd;
  
  char *answer = NULL;
  char *answer_end = NULL;
  unsigned int answer_size;
  
  char tmpbuf[100] = {0,};
  int actual;
  int total = 0;
  int done = 0;
  
  fd = userdata->fd;
  rspbuf[0] = 0;
  if(fd < 0)
    return -1;
  
  if(cmd != NULL) {
    // Write command
    gint cmdlen;
    
    cmdlen = strlen(cmd);
    if(write(fd, cmd, cmdlen) < cmdlen)	{
      perror("Error writing to port");
      return -1;
    }
  }
  
  while(!done)	{
    FD_ZERO(&ttyset);
    FD_SET(fd, &ttyset);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
      actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
      if(actual < 0)
	return actual;
      total += actual;
      
      /* Answer didn't come within the length of the buffer. Cancel! */
      if(total == sizeof(tmpbuf))
	return -1;
      
      if( (answer = index(tmpbuf, '\n')) )	{
	// Remove first line (echo)
	if( (answer_end = index(answer+1, '\n')) )	{
	  // Found end of answer
	  done = 1;
	}
      }
    }
    else	{
      /* Anser didn't come in time. Cancel */
      return -1;
    }
  }
  //	printf("Answer: %s\n", answer);
  
  // Remove heading and trailing \r
  if((*answer_end == '\r') || (*answer_end == '\n'))
    answer_end--;
  if((*answer_end == '\r') || (*answer_end == '\n'))
    answer_end--;
  if((*answer == '\r') || (*answer == '\n'))
    answer++;
  if((*answer == '\r') || (*answer == '\n'))
    answer++;

  answer_size = (answer_end) - answer +1;
  
  //	printf("Answer size=%d\n", answer_size);
  if( (answer_size) >= rspbuflen )
    return -1;
  
  
  strncpy(rspbuf, answer, answer_size);
  rspbuf[answer_size] = 0;
  return 0;
}

gint obex_cable_connect(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;
  struct termios newtio;
  char rspbuf[200];

  userdata = (obexdata_t*) ud;
  userdata->fd = open(userdata->cabledev, O_RDWR|O_NONBLOCK|O_NOCTTY);
  if (userdata->fd < 0) {
//    return(SYNC_MSG_CONNECTIONERROR);
    return -1;
  }

  tcgetattr(userdata->fd, &userdata->oldtio);
  bzero(&newtio, sizeof(struct termios));
  newtio.c_cflag = B115200 | CLOCAL | CS8 | CREAD | CRTSCTS;
  newtio.c_cc[VMIN] = 1;
  newtio.c_cc[VTIME] = 0;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  tcflush(userdata->fd, TCIFLUSH);
  tcsetattr(userdata->fd, TCSANOW, &newtio);

#ifdef HAVE_COBEX  
  if (userdata->cabletype == IRMC_CABLE_SIEMENS)
    return(cobex_connect(handle, ud));
#endif  
  if (userdata->cabletype != IRMC_CABLE_ERICSSON)
    goto err;

  /*if(obex_cable_at(userdata, "ATZ\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
    dd(printf("Comm-error sending ATZ\n"));
  }
  
  if(strcasecmp("OK", rspbuf) != 0)	{
    dd(printf("Error doing ATZ (%s)\n", rspbuf));
    goto err;
  }
  
  if(obex_cable_at(userdata, "AT+GMI\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
    dd(printf("Comm-error sending AT+GMI\n"));
  }
  
  if(!strcasecmp("ERICSSON", rspbuf) != 0)	{
    dd(printf("Detected Ericsson.\n"));
    userdata->cabletype = IRMC_CABLE_ERICSSON;
    // Remove extra "OK" on e.g. Ericsson 520
    obex_cable_at(userdata, NULL, rspbuf, sizeof(rspbuf), 1);
  }
  if(!strcasecmp("SONYERICSSON", rspbuf) != 0)	{
    dd(printf("Detected SonyEricsson.\n"));
    userdata->cabletype = IRMC_CABLE_ERICSSON;
  }
  if(!strcasecmp("SIEMENS", rspbuf) != 0)	{
    dd(printf("Detected Siemens.\n"));
    userdata->cabletype = IRMC_CABLE_SIEMENS;
    close(userdata->fd);
    return(cobex_connect(handle, ud));
  }
  if (userdata->cabletype == IRMC_CABLE_UNKNOWN)
    goto err;
  */

  // Set up Ericsson phone in OBEX mode.
  if(obex_cable_at(userdata, "ATZ\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
    osync_trace(TRACE_INTERNAL, "Comm-error sending ATZ\n");
  }
  
  if(strcasecmp("OK", rspbuf) != 0)	{
    osync_trace(TRACE_INTERNAL, "Error doing ATZ (%s)\n", rspbuf);
    goto err;
  }
  if(obex_cable_at(userdata, 
		   "AT*EOBEX\r", rspbuf, sizeof(rspbuf), 1) < 0)	{
    osync_trace(TRACE_INTERNAL, "Comm-error sending AT*EOBEX\n");
    goto err;
  }
  if(strcasecmp("CONNECT", rspbuf) != 0)	{
    osync_trace(TRACE_INTERNAL, "Error doing AT*EOBEX (%s)\n", rspbuf);
    goto err;
  }
  fcntl(userdata->fd, F_SETFL, O_NONBLOCK);
  return(0);
  
 err:
  obex_cable_disconnect(handle, ud);
//  return (SYNC_MSG_CONNECTIONERROR);
  return -1;
}


gint obex_cable_disconnect(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;
  
  userdata = (obexdata_t*) ud;
  if (userdata->fd >= 0) {
    // Send a break to get out of OBEX-mode
    if(ioctl(userdata->fd, TCSBRKP, 0) < 0) {
      osync_trace(TRACE_INTERNAL, "Unable to send break!\n");
    }
    tcsetattr(userdata->fd, TCSANOW, &userdata->oldtio);
    close(userdata->fd);
  }
  return(0);
}

gint obex_cable_listen(obex_t *handle, gpointer ud) {
  obexdata_t *userdata;

  userdata = (obexdata_t*) ud;
  return(0);
}

gint obex_cable_write(obex_t *handle, gpointer ud, 
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

gint obex_cable_handleinput(obex_t *handle, gpointer ud, gint timeout) {
  int n;
  struct timeval to;
  fd_set readfds;
  char buf[2048];
  int tot=0;
  obexdata_t *userdata;
  int ret = 0;

  userdata = (obexdata_t*) ud;
  n = userdata->fd + 1;
  FD_ZERO(&readfds);
  FD_SET(userdata->fd, &readfds);
  to.tv_sec = timeout;
  to.tv_usec = 0;
  while (userdata->state >= 0 &&
	 (ret=select(n, &readfds, NULL, NULL, &to)) > 0) {
    if((tot = read(userdata->fd, buf, 2048)) > 0) {
      OBEX_CustomDataFeed(handle, (uint8_t *) buf, tot);
    } else { 
      userdata->state = IRMC_OBEX_REQFAILED;
      osync_error_set(userdata->error, OSYNC_ERROR_NO_CONNECTION, NULL);
    }
    to.tv_sec = timeout;
    to.tv_usec = 0;
  }
  if (userdata->state >= 0 && ret == 0) {
    userdata->state = IRMC_OBEX_REQFAILED;
    osync_error_set(userdata->error, OSYNC_ERROR_NO_CONNECTION, NULL);
  }
  return(0);
}

obex_t* irmc_obex_client(irmc_config *config) {
  obex_ctrans_t bttrans;
#ifdef HAVE_COBEX
  obex_ctrans_t cabletrans = { obex_cable_connect, cobex_disconnect,
			       NULL, cobex_write, 
			       cobex_handleinput, 0 };
#endif

#ifdef HAVE_IRDA
  obex_ctrans_t irdatrans = { obex_irda_connect, obex_irda_disconnect,
			      obex_cable_listen, obex_cable_write,
			      obex_cable_handleinput, 0 };
#endif
  obexdata_t *userdata;
  obex_t *handle = NULL;

  if (bluetoothplugin) {
    g_module_symbol(bluetoothplugin,"obex_connect",
		    (gpointer) &bttrans.connect);
    g_module_symbol(bluetoothplugin,"obex_disconnect",
		    (gpointer) &bttrans.disconnect);
    g_module_symbol(bluetoothplugin,"obex_write",(gpointer) &bttrans.write);
    g_module_symbol(bluetoothplugin,"obex_listen",(gpointer) &bttrans.listen);
    g_module_symbol(bluetoothplugin,"obex_handleinput",
		    (gpointer) &bttrans.handleinput);
  }
  userdata = (obexdata_t*) g_malloc0(sizeof(obexdata_t));
#if 1 //OBEX_CUSTOMDATA
  bttrans.customdata = userdata;
#ifdef HAVE_IRDA
  irdatrans.customdata = userdata;
#endif

#ifdef HAVE_COBEX
  cabletrans.customdata = userdata;
#endif

#endif
#ifdef OBEX_USERDATA
  bttrans.userdata = userdata;
#ifdef HAVE_IRDA
  irdatrans.userdata = userdata;
#endif

#ifdef HAVE_COBEX
  cabletrans.userdata = userdata;
#endif

#endif
#ifdef HAVE_BLUETOOTH
  memcpy(&userdata->bdaddr, &config->bdaddr, sizeof(bdaddr_t));
#endif
  userdata->channel = config->btchannel;
  strncpy(userdata->cabledev, config->cabledev, 19);
  userdata->cabletype = config->cabletype;
  memcpy(&userdata->irunit, &config->irunit, sizeof(irmc_ir_unit));
#ifdef HAVE_IRDA
  userdata->ir_addr = config->ir_addr; // Temp absolute IR address
#endif
  userdata->connectmedium = config->connectmedium;
  userdata->state = IRMC_OBEX_OFFLINE;
  userdata->connected = 0;
  userdata->busy = 0;

  switch(userdata->connectmedium) {
  case MEDIUM_BLUETOOTH:
#ifdef HAVE_BLUETOOTH
    if (!(handle = OBEX_Init(OBEX_TRANS_BLUETOOTH, obex_event, 0)))
      return(0);
#endif
    break;
  case MEDIUM_IR:
#ifdef HAVE_IRDA
    if (!(handle = OBEX_Init(OBEX_TRANS_CUST, obex_event, 0)))
      return(0);
    OBEX_RegisterCTransport(handle, &irdatrans);
#endif
    break;
  case MEDIUM_CABLE:
#ifdef HAVE_COBEX    
    if (!(handle = OBEX_Init(OBEX_TRANS_CUST, obex_event, 0)))
      return(0);
    OBEX_RegisterCTransport(handle, &cabletrans);
#endif    
    break;
  }
  OBEX_SetUserData(handle, (gpointer) userdata);
  return(handle);
}

gboolean irmc_obex_connect(obex_t* handle, char* target, OSyncError **error) {
  osync_trace(TRACE_ENTRY, "%s(%p,%p,%p)", __func__, handle, target, error);
  int ret = -1;
  obex_object_t *object; 
  obex_headerdata_t hd; 
  obexdata_t *userdata;
  struct sockaddr_un addr;
  

  userdata = (obexdata_t*) OBEX_GetUserData(handle);
  userdata->connected = 0;
  switch(userdata->connectmedium) {
  case MEDIUM_BLUETOOTH:
#ifdef HAVE_BLUETOOTH
    ret=BtOBEX_TransportConnect(handle, NULL, &userdata->bdaddr, userdata->channel);
#endif
    break;
  case MEDIUM_IR:
    // ret = -1;
    ret=OBEX_TransportConnect (handle, (struct sockaddr*) &addr, 0);
    break;
  case MEDIUM_CABLE:
    ret=OBEX_TransportConnect (handle, (struct sockaddr*) &addr, 0);
    break;
  }
  if (ret < 0) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot connect via OBEX.");
    return FALSE;
  }
  userdata->connected = 1;
  if ((object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT))) {
    if (target) {
      hd.bs = (const uint8_t *) target; 
      OBEX_ObjectAddHeader(handle, object, OBEX_HDR_TARGET, hd, 
			   strlen(target), 0); 
    }
    userdata->busy = 1;
    ret = OBEX_Request(handle, object);
    if (ret < 0) {
      osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot connect via OBEX.");
      return FALSE;
    }
  }
  userdata->state = IRMC_OBEX_CONNECTING;
  while ( userdata->busy ) {
    irmc_obex_handleinput(handle, 10);
  }
  if (userdata->state == IRMC_OBEX_REQDONE) {
    if (strlen(userdata->irunit.serial) > 0) {
#ifdef HAVE_IRDA
      if (!userdata->ir_addr) {
#endif
	char *serial = irmc_obex_get_serial(handle);
	if (!serial || strcmp(serial, userdata->irunit.serial)) {
    osync_trace(TRACE_INTERNAL, "Device serial number is not correct.\n");
	  if (serial)
	    g_free(serial);
	  irmc_obex_disconnect(handle, error);
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot connect via OBEX.");
    return FALSE;
	}
	g_free(serial);
#ifdef HAVE_IRDA
      }
#endif
    }
    // Connected and serial number is OK!
    osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;
  }
  osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot connect via OBEX.");
  osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
  return FALSE;
}

gboolean irmc_obex_disconnect(obex_t* handle, OSyncError **error) {
  obex_object_t *object; 
  obexdata_t *userdata;

  userdata = (obexdata_t*) OBEX_GetUserData(handle);  
  if (userdata->connected) {
    if ((object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT))) {
      userdata->busy = 1;
      if(OBEX_Request(handle, object) < 0)  {
        osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot disconnect from OBEX.");
        return FALSE;
      }
    }
    userdata->state = IRMC_OBEX_DISCONNECTING;
    while ( userdata->busy ) {
      irmc_obex_handleinput(handle, 10);
    }
    OBEX_TransportDisconnect(handle);
    userdata->connected = 0;
  }
  return TRUE;
}

void irmc_obex_cleanup(obex_t* handle) {
  if (handle) {
    obexdata_t *userdata;
    
    userdata = (obexdata_t*) OBEX_GetUserData(handle);  
    OBEX_Cleanup(handle);
    g_free(userdata);
  }
}

// Return the serial number of an already connected device.
// The string must be freed with g_free()
char* irmc_obex_get_serial(obex_t* handle) {
  char data[10240];
  int len = 10240;
  char *sn;
  sn = g_malloc(128);
  len = 10240;
  OSyncError *error = NULL;
  if (irmc_obex_get(handle, "telecom/devinfo.txt", data, &len, &error)>=0){
    char *p = data;
    while (p && p < data+len) {
      if (sscanf(p, "SN:%127s", sn) >= 1) {
	return(sn);
      } else {
	if (sscanf(p, "SN;%*[^:]%s", sn) >= 1) {
	  return(sn);
	} else {
	  p = strstr(p, "\n");
	  if (p)
	    p++;
	}
      }
    } 
  } else {
    osync_error_unref(&error);
  }
  return(NULL);
}

gboolean irmc_obex_put(obex_t* handle, char* name, char *type,
		 char *body, gint body_size, char *rspbuf, int *rspbuflen,
		 char *apparam, int apparamlen, OSyncError **error) {
  
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %i, %p, %p, %s, %i, %p)",
	__func__, handle, name, type, body, body_size, rspbuf, rspbuflen,
	apparam, apparamlen, error);

  obex_object_t *object; 
  obex_headerdata_t hd; 
  char unicodename[1024];
  obexdata_t *userdata;
  int namesize = 0;

  userdata = (obexdata_t*) OBEX_GetUserData(handle);  
  if ((object = OBEX_ObjectNew(handle, OBEX_CMD_PUT))) {
    hd.bq4 = body_size; 
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hd, 4, 0); 
    /* Add unicode name header*/ 
    namesize = OBEX_CharToUnicode((uint8_t *) unicodename, (const uint8_t *) name, 1024);
    hd.bs = (const uint8_t *) unicodename; 
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd, 
			 namesize, 0); 
    if (type) {
      hd.bs = (const uint8_t *) type;
      OBEX_ObjectAddHeader(handle, object, OBEX_HDR_TYPE, hd, 
			 strlen(type), 0); 
    }
    if (apparam) {
      hd.bs = (const uint8_t *) apparam;
      OBEX_ObjectAddHeader(handle, object, OBEX_HDR_APPARAM, hd, 
			   apparamlen, 0); 
    }
    if (body) {
      /* Add body header*/ 
      hd.bs = (const uint8_t *) body;
      osync_trace(TRACE_SENSITIVE, "OBEX-OUT:\n%s\n", hd.bs); 
      OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hd, body_size, 0); 
    }
    userdata->busy = 1;
    if(OBEX_Request(handle, object) < 0) {
      osync_error_set(error, OSYNC_ERROR_GENERIC, "Cannot put a item on mobile device");
      osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
      return FALSE;
    }
    userdata->state = IRMC_OBEX_PUTTING;
    userdata->databuf = rspbuf;
    userdata->databuflen = rspbuflen;
    while ( userdata->busy ) {
      irmc_obex_handleinput(handle, 30);
    }
    if (userdata->state == IRMC_OBEX_REQDONE) {
      osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
      return TRUE;
    }

    if ( userdata->error ) {
      error = userdata->error;
      osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
      return FALSE;
    } else {
      osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
      return TRUE;
    }
  }

  osync_error_set(error, OSYNC_ERROR_GENERIC, "Request failed");
  osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
  return FALSE;
}

gboolean irmc_obex_get(obex_t *handle, char* name, char* buffer, int *buflen, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, %p, %p)", __func__, handle, name, buffer, buflen, error);

  obex_object_t *object;
  obex_headerdata_t hd;
  char unicodename[1024];
  obexdata_t *userdata;
  int namesize = 0;

  userdata = (obexdata_t*) OBEX_GetUserData(handle);  
  if((object = OBEX_ObjectNew(handle, OBEX_CMD_GET))) {
    /* Add unicode name header*/ 
    namesize = OBEX_CharToUnicode((uint8_t *) unicodename, (const uint8_t *) name, 1024);
    hd.bs = (const uint8_t *) unicodename; 
    OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hd, 
			 namesize, 0); 
    userdata->databuf = buffer;
    userdata->databuflen = buflen;
    userdata->busy = 1;
    userdata->error = error;
    OBEX_Request(handle, object);
    userdata->state = IRMC_OBEX_GETTING;
    while ( userdata->busy ) {
      irmc_obex_handleinput(handle, 30);
    }
    if (userdata->state == IRMC_OBEX_REQDONE) {
      osync_trace(TRACE_EXIT, "%s : TRUE", __func__);
      return TRUE;
    }

    if (userdata->error) {
      error = userdata->error;
      goto error;
    } else
      osync_trace(TRACE_EXIT, "%s : TRUE", __func__);
      return TRUE;
  }
error:
  osync_error_set(error, OSYNC_ERROR_GENERIC, "Request failed");
  osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
  return FALSE;
}

gint irmc_obex_handleinput(obex_t* handle, int timeout) {
  return(OBEX_HandleInput(handle, timeout));
}

void irmc_obex_init() {
}

