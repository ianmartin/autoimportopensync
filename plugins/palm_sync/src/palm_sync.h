#ifndef PALM_PLUGIN_H
#define PALM_PLUGIN_H

#include <opensync/opensync.h>

#include <pi-socket.h>
#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-version.h>
#include <pi-address.h>
#include <pi-datebook.h>
#include <pi-todo.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <glib.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define PILOT_DEVICE_SERIAL 0
#define PILOT_DEVICE_USB_VISOR 1
#define PILOT_DEVICE_IRDA 2
#define PILOT_DEVICE_NETWORK 4

typedef struct {
	//client_connection commondata;
	//sync_pair *handle;
	//connection_type type;
	OSyncMember *member;
	char statefile[1024];
	char username[1024];
	int id;
	char *sockaddr;
	int timeout;
	int speed;
	int conntype;
	int debuglevel;
	int socket;
	int database;
	int popup;
	int mismatch;
	char databasename[1024];
	char codepage[1024];
} palm_connection;

typedef struct {
	struct Address address;
	struct Appointment appointment;
	struct ToDo todo;
//	sync_object_type type;
	char uid[1024];
	char *category;
	int catID;
} palm_entry;

GString *address2vcard(palm_connection *, struct Address, char *);
GString *calendar2vevent(palm_connection *, struct Appointment);
GString *todo2vcal(palm_connection *, struct ToDo, char *);
void palm_debug(palm_connection *conn, int level, char *message, ...);
int connectDevice(palm_connection *conn, gboolean block, gboolean popup);
int get_category_id_from_name(palm_connection *conn, char *name);
void VObjectOErrorHander(char *);

#endif
