#ifndef IRMC_BLUETOOTH_H
#define IRMC_BLUETOOTH_H

#include <config.h>
#include "irmc_obex.h"

typedef struct {
  char address[20];
  int channel;
  char name[32];
} irmc_bt_unit;

GList *find_bt_units(void);
int rfcomm_connect(struct bt_unit *btu, int channel);


#endif
