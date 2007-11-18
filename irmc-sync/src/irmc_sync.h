#ifndef IRMC_SYNC_H
#define IRMC_SYNC_H
#include <config.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/socket.h>
#ifdef HAVE_IRDA
#include <linux/types.h>
#include <linux/irda.h>
#endif
#ifdef HAVE_BLUETOOTH
#include <bluetooth/bluetooth.h>
#endif
#include <openobex/obex.h>
#include <opensync/opensync.h>

//Specify any structs etc here.

typedef enum {
  MEDIUM_BLUETOOTH = 1,
  MEDIUM_IR = 2,
  MEDIUM_CABLE = 3
} connect_medium;

typedef enum {
  IRMC_CABLE_UNKNOWN = 0,
  IRMC_CABLE_ERICSSON,
  IRMC_CABLE_SIEMENS
} cable_type;

#include "irmc_obex.h"
#include "irmc_irda.h"

typedef struct  {
  char *serial_number;

  obex_t obexhandle;             // Handle to the obex connection

  // connection options
  connect_medium connectmedium;  // The connection type

  // bluetooth specific
  bdaddr_t bdaddr;               // Bluetooth address
  int btchannel;                 // Bluetooth channel

  // cable specific
  char cabledev[20];             // Cable device
  cable_type cabletype;          // Type of device connected via cable

  // irda specific
  irmc_ir_unit irunit;           // IR device name and serial
#ifdef HAVE_IRDA
  __u32 ir_addr;                 // Absolute address to IR device (used temporary)
#endif

  // filter options (should be moved to filter objects)
  gboolean donttellsync;         // Don't send IRMC-SYNC target to OBEX unless needed
} irmc_config;

typedef struct irmc_environment {
  char *anchor_path;             // The path of the irmc anchor
  irmc_config config;            // The configuration
  GList *databases;		 // List of irmc_datase
  osync_bool isConnected;
} irmc_environment;

typedef struct irmc_database {
  OSyncObjFormat *objformat;     // The configured objformat for this database
  OSyncObjTypeSink *sink;
  unsigned int changecounter;   
  char *dbid;
  char *obex_db;
} irmc_database;

obex_t* irmc_obex_client(irmc_config *config);
char* sync_connect_get_serial(irmc_config *config);
gboolean detect_slowsync(int changecounter, char *object, char **dbid, char **serial_number,
                         gboolean *slowsync, obex_t obexhandle, OSyncError **error);

#endif
