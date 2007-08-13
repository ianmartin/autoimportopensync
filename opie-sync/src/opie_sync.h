/*

   Copyright 2005 Holger Hans Peter Freyther

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef OPIE_SYNC_H
#define OPIE_SYNC_H

#include <opensync/opensync.h>
#include <glib.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <string.h>

#include "opie_qcop.h"

typedef struct OpiePluginEnv OpiePluginEnv;
typedef struct OpieSinkEnv OpieSinkEnv;

typedef enum {
    OPIE_SYNC_QTOPIA_2,
    OPIE_SYNC_OPIE
} OPIE_DEVICE_TYPE;

typedef enum {
    OPIE_CONN_NONE,                          /* for debugging */
    OPIE_CONN_FTP,
    OPIE_CONN_SCP
} OPIE_CONN_TYPE;

typedef enum {
  OPIE_OBJECT_TYPE_UNKNOWN,
  OPIE_OBJECT_TYPE_CONTACT,
  OPIE_OBJECT_TYPE_EVENT,
  OPIE_OBJECT_TYPE_TODO,
  OPIE_OBJECT_TYPE_NOTE,
  OPIE_OBJECT_TYPE_CATEGORY
} OPIE_OBJECT_TYPE;


struct OpiePluginEnv {
	gchar*            username;
	gchar*            password;
	gchar*            url;
	unsigned int      device_port;
	OPIE_CONN_TYPE    conn_type;
	OPIE_DEVICE_TYPE  device_type;
	gchar*            backupdir;    /* location to create backup dirs */
	gchar*            backuppath;   /* the full path to the backup dir for this session */

	gboolean          use_qcop;
	qcop_conn*        qcopconn;
	gboolean          connected;

	xmlDoc*           categories_doc;

	OpieSinkEnv*      contact_env;
	OpieSinkEnv*      todo_env;
	OpieSinkEnv*      event_env;
	OpieSinkEnv*      note_env;
	
	GMutex*           plugin_mutex;
};

struct OpieSinkEnv {
	OpiePluginEnv*    plugin_env;
	OSyncObjTypeSink* sink;
	xmlDoc*           doc;
	OSyncHashTable*   hashtable;
	OSyncObjFormat*   objformat;
	const char*       listelement;
	const char*       itemelement;
	const char*       remotefile;
	OPIE_OBJECT_TYPE  objtype;
};

#define OPIE_FORMAT_XML_CONTACT "opie-xml-contact"
#define OPIE_FORMAT_XML_TODO    "opie-xml-todo"
#define OPIE_FORMAT_XML_EVENT   "opie-xml-event"
#define OPIE_FORMAT_XML_NOTE    "opie-xml-note"


#endif
