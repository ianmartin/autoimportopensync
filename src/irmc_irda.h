#ifndef IRMC_IRDA_H
#define IRMC_IRDA_H

#include <config.h>
#include <glib.h>
#include "irmc_obex.h"

typedef struct {
  char name[32]; // From sockaddr_irda
  char serial[128]; // From telecom/devinfo.txt
} irmc_ir_unit;

gint obex_irda_connect(obex_t *handle, gpointer ud);
gint obex_irda_disconnect(obex_t *handle, gpointer ud);
//GList* find_irda_units(irmc_connection *conn);


#endif
