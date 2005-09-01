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
  gboolean managedbsize;
  gboolean fake_recurring;
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
} irmc_config;

typedef struct irmc_environment {
	OSyncMember *member;

  irmc_config config;
} irmc_environment;

obex_t* irmc_obex_client(irmc_config *config);

#endif
