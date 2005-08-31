#ifndef IRMC_SYNC_H
#define IRMC_SYNC_H
#include <config.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/socket.h>
#if HAVE_IRDA
#include <linux/types.h>
#include <linux/irda.h>
#endif
#if HAVE_BLUETOOTH
#include <bluetooth/bluetooth.h>
#endif
#include <openobex/obex.h>
#include <opensync/opensync.h>

typedef enum {
  MEDIUM_BLUETOOTH=1,
  MEDIUM_IR=2,
  MEDIUM_CABLE=3
} connect_medium;

typedef enum {
  IRMC_CABLE_UNKNOWN = 0,
  IRMC_CABLE_ERICSSON,
  IRMC_CABLE_SIEMENS
} cable_type;

#include "irmc_obex.h"
#include "irmc_irda.h"

#define IRSYNC_APP_MAXEXPCOUNT 0x11
#define IRSYNC_APP_HARDDELETE 0x12
#define IRSYNC_APP_LUID 0x1
#define IRSYNC_APP_CHANGECOUNT 0x2
#define IRSYNC_APP_TIMESTAMP 0x3


typedef struct  {
  unsigned int calchangecounter;
  unsigned int pbchangecounter;
  obex_t obexhandle;
  char *calDID;
  char *pbDID;
  // Options
  connect_medium connectmedium;
  struct bt_unit btunit; // Bluetooth address
  int btchannel; // Bluetooth channel
  char cabledev[20]; // Cable device
  cable_type cabletype; // Type of device connected via cable
  irmc_ir_unit irunit; // IR device name and serial
#if HAVE_IRDA
  __u32 ir_addr; // Absolute address to IR device (used temporary)   
#endif
  gboolean fixdst;    // Fix T68i DST bug
  gboolean donttellsync; // Don't send IRMC-SYNC target to OBEX
                         // unless needed.
  gboolean onlyphonenumbers; // Accept only contacts with phone numbers
  gboolean dontacceptold; // Dont accept old contacts
  int maximumage; // old = this many days
  gboolean translatecharset;
  char *charset;
  gboolean alarmtoirmc, alarmfromirmc;
  gboolean convertade;
} irmc_connection;

typedef enum {
  PRESET_T68 = 1,
  PRESET_T39 = 2,
  PRESET_S55 = 3
} client_preset;


obex_t* irmc_obex_client(irmc_connection *conn);

irmc_connection* sync_connect(sync_pair *handle, connection_type type,
			      sync_object_type object_types);
gboolean sync_test_connection(irmc_connection *conn);
char* sync_connect_get_serial(irmc_connection *conn);
void irmc_disconnect(irmc_connection *conn);
void sync_disconnect(irmc_connection *conn);
void free_changes(GList *changes);
void get_changes(irmc_connection *conn, sync_object_type newdbs);
int cal_get_changes(irmc_connection *conn, int slowsync, GList **sync_changes);
int pb_get_changes(irmc_connection *conn, int slowsync, GList **sync_changes);

void safe_strcat(char *s1, const char *s2, int len);
void str_replace(char *in, char *out, int outbuflen, char *replfrom, char *replto);
void load_state(irmc_connection *conn);
void save_state(irmc_connection *conn);
void free_state(irmc_connection *conn);
void cal_modify_or_delete(irmc_connection *conn, 
			  char *event, char *inluid, 
			  char *outluid, int *outluidlen, int softdelete,
			  sync_object_type objtype);
void pb_modify_or_delete(irmc_connection *conn, 
			 char* event, char *inluid, 
			 char *outluid, int *outluidlen, int softdelete,
			 sync_object_type objtype);
void syncobj_modify(irmc_connection *conn, 
		    char* event, char *inluid,
		    sync_object_type objtype,
		    char *outluid, int *outluidlen);
void syncobj_delete(irmc_connection *conn, char *inluid,
		    sync_object_type objtype, int softdelete);

// Syncronous methods
gboolean always_connected(void);
char* short_name(void);
char* long_name(void);
sync_object_type object_types(void);
void plugin_init(void);

#endif
