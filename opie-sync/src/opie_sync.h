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

typedef struct OpieSyncEnv OpieSyncEnv;

typedef enum {
    OPIE_SYNC_QTOPIA_2,
    OPIE_SYNC_OPIE
} OPIE_DEVICE_TYPE;

typedef enum {
    OPIE_CONN_NONE,                          /* for debugging */
    OPIE_CONN_FTP,
    OPIE_CONN_SCP
} OPIE_CONN_TYPE;

struct OpieSyncEnv {
    OSyncMember*      member;
    gchar*            username;
    gchar*            password;
    gchar*            url;
    unsigned int      device_port;
    OPIE_CONN_TYPE    conn_type;
    OPIE_DEVICE_TYPE  device_type;

    gboolean          use_qcop;
    qcop_conn*        qcopconn;

    GList*            calendar;
    GList*            contacts;
    GList*            todos;
    GList*            categories;

    OSyncHashTable*   hashtable;
};


#endif
