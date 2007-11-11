#ifndef IRMC_OBEX_H
#define IRMC_OBEX_H
#include <glib.h>
#include <config.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef HAVE_IRDA
#include <linux/types.h>
#include <linux/irda.h>
#endif

#define IRMC_OBEX_OFFLINE 0
#define IRMC_OBEX_CONNECTING 1
#define IRMC_OBEX_PUTTING 2
#define IRMC_OBEX_GETTING 3
#define IRMC_OBEX_DISCONNECTING 4
#define IRMC_OBEX_REQDONE -1
#define IRMC_OBEX_REQFAILED -2

#ifdef HAVE_BLUETOOTH
#include <bluetooth/bluetooth.h>
#endif /* HAVE_BLUETOOTH */

#include <openobex/obex.h>
#include "cobex_bfb.h"

#include "irmc_sync.h"
#include "irmc_irda.h"

typedef struct  { 
  int fd;
  connect_medium connectmedium;
#ifdef HAVE_BLUETOOTH
  bdaddr_t bdaddr;
#endif
  char cabledev[20];
  cable_type cabletype;
  irmc_ir_unit irunit; // IR device name and serial  
#ifdef HAVE_IRDA
  __u32 ir_addr;
#endif
  int channel;
  int state;
  OSyncError **error;
  char *databuf;
  int *databuflen;
  int connected;
  struct termios oldtio;
  cobex_t cobex; // Data for OBEX over cable
  int busy;
} obexdata_t;


gboolean irmc_obex_connect(obex_t* handle, char* target, OSyncError **error);
gboolean irmc_obex_disconnect(obex_t* handle, OSyncError **error);
gboolean irmc_obex_put(obex_t* handle, char* name, char *type,
		 char *body, gint body_size, char *rspbuf, int *rspbuflen,
		 char *apparam, int apparamlen, OSyncError **error);
gboolean irmc_obex_get(obex_t *handle, char* name, char* buffer, int *buflen, OSyncError **error);
gint obex_cable_disconnect(obex_t *handle, gpointer ud);
gint obex_cable_write(obex_t *handle, gpointer ud,
		 guint8 *buf, gint buflen);
gint obex_cable_handleinput(obex_t *handle, gpointer ud, gint timeout);
gint irmc_obex_handleinput(obex_t* handle, int timeout);
void server_done(obex_t *handle, obex_object_t *object,
		 gint obex_cmd, gint obex_rsp);
void client_done(obex_t *handle, obex_object_t *object,
		 gint obex_cmd, gint obex_rsp);
void get_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp);
void put_client_done(obex_t *handle, obex_object_t *object, gint obex_rsp);

void obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp);

gint obex_bt_connect(obex_t *handle, gpointer ud);
gint obex_cable_connect(obex_t *handle, gpointer ud);
gint obex_irda_connect(obex_t *handle, gpointer ud);
gint obex_fd_disconnect(obex_t *handle, gpointer ud);
gint obex_fd_listen(obex_t *handle, gpointer ud);
gint obex_fd_write(obex_t *handle, gpointer ud,
		   guint8 *buf, gint buflen);
gint obex_fd_handleinput(obex_t *handle, gpointer ud, gint timeout);
void irmc_obex_init(void);
void irmc_obex_cleanup(obex_t* handle);
char* irmc_obex_get_serial(obex_t* handle);

#endif /* IRMC_OBEX_H */

