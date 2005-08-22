#ifndef PALM_SYNC_H
#define PALM_SYNC_H

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
#include <string.h>

#define PILOT_DEVICE_SERIAL 0
#define PILOT_DEVICE_USB_VISOR 1
#define PILOT_DEVICE_IRDA 2
#define PILOT_DEVICE_NETWORK 4

typedef struct PSyncEnv {
	OSyncMember *member;
	char *statefile;
	char *username;
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
	char *databasename;
	char *codepage;
} PSyncEnv;

typedef struct {
	struct Address address;
	struct Appointment appointment;
	struct ToDo todo;
//	sync_object_type type;
	char uid[1024];
	char *category;
	int catID;
} palm_entry;

#endif //PALM_SYNC_H
