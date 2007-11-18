/*
   MultiSync IrMC Plugin - Synchronize IrMC (mobile) devices
   Copyright (C) 2002-2003 Bo Lincoln <lincoln@lysator.liu.se>
                      2005 Tobias Koenig <tokoe@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*/

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-group.h>
#include <opensync/opensync-version.h>

#include <assert.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "irmc_sync.h"

#define SYNC_OBJECT_TYPE_CALENDAR 0
#define SYNC_OBJECT_TYPE_TODO 1

#define SLOW_SYNC 0
#define FAST_SYNC 1

#define IRSYNC_APP_MAXEXPCOUNT 0x11
#define IRSYNC_APP_HARDDELETE 0x12
#define IRSYNC_APP_LUID 0x1
#define IRSYNC_APP_CHANGECOUNT 0x2
#define IRSYNC_APP_TIMESTAMP 0x3

#define DATABUFSIZE (65536*2)

typedef struct {
  char name[256];
  char identifier[256];
  char path_identifier[20];
  char path_extension[20];
  unsigned int *change_counter;
} data_type_information;

void safe_strcat(char *s1, const char *s2, int len)
{
  s1[len-1] = 0;
  memcpy(s1+strlen(s1), s2,
   (strlen(s2)+1+strlen(s1)>len)?(len-strlen(s1)-1):strlen(s2)+1);
}

void str_replace(char *in, char *out, int outbuflen, char *replfrom, char *replto)
{
  char *pos=in, *oldpos = in;
  out[0] = 0;
  while((pos=strstr(pos, replfrom))) {
    strncat(out, oldpos, pos-oldpos);
    safe_strcat(out, replto, outbuflen);
    pos+=strlen(replfrom);
    oldpos = pos;
  }
  safe_strcat(out, oldpos, outbuflen);
}

void parse_header_params(char *input, int input_size, char *luid, int luid_size, unsigned int *change_counter)
{
  int offset = 0;
  memset(luid, 0, luid_size);

  if (input_size < 2)
    return;

  int len = input[ 1 ];
  memcpy(luid, input + 2, (luid_size < len ? luid_size : len));

  if (luid_size < (2 + len + 2))
    return;

  offset = 2 + len + 2;
  len = input[ 2 + len + 2 ];

  char cc[ 11 ];
  memset(cc, 0, sizeof(cc));

  memcpy(cc, input + offset, (10 < len ? 10 : len));
  if (sscanf( cc, "%d", change_counter) != 1)
    *change_counter = 0;
}

// Return the serial number of an unconnected device
// The string must be freed with g_free()
char* sync_connect_get_serial(irmc_config *config)
{
  char *sn = NULL;
  config->obexhandle = irmc_obex_client(config);
  OSyncError *error = NULL;

  if (irmc_obex_connect(config->obexhandle,
      config->donttellsync ? NULL : "IRMC-SYNC", &error)) {
    sn = irmc_obex_get_serial(config->obexhandle);
  } else {
    osync_error_unref(&error);
    error = NULL;
  }

  irmc_obex_disconnect(config->obexhandle, &error);
  if (error)
    osync_error_unref(&error);

  irmc_obex_cleanup(config->obexhandle);
  config->obexhandle = NULL;
  return(sn);
}

// Unique function name so not other plugin is called accidentally.
void irmc_disconnect(irmc_config *config)
{
  if (config->obexhandle) {
    OSyncError *error = NULL;
    irmc_obex_disconnect(config->obexhandle, &error);
    if (error)
      osync_error_unref(&error);

    irmc_obex_cleanup(config->obexhandle);
  }

  config->obexhandle = 0;
}

/**
 * Parses the configuration of this plugin from a string in xml format.
 */
osync_bool parse_settings(irmc_config *config, const char *data, unsigned int size, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, data, error);
  osync_trace(TRACE_SENSITIVE, "Content of data:\n%s", data);

  xmlDoc *doc = NULL;
  xmlNode *cur = NULL;

  // set defaults
  config->donttellsync = FALSE;
  config->serial_number = NULL;


  doc = xmlParseMemory(data, size);

  if (!doc) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
    goto error;
  }

  cur = (xmlNode *) xmlDocGetRootElement(doc);

  if (!cur) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
    goto error_free_doc;
  }

  if (xmlStrcmp(cur->name, (const xmlChar*) "config")) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
    goto error_free_doc;
  }

  cur = cur->xmlChildrenNode;

  while (cur != NULL) {
    char *str = (char *) xmlNodeGetContent(cur);
    if (str) {
      if (!xmlStrcmp(cur->name, (const xmlChar *)"connectmedium")) {
        if (!strcmp(str, "bluetooth"))
          config->connectmedium = MEDIUM_BLUETOOTH;
        else if (!strcmp(str, "ir"))
          config->connectmedium = MEDIUM_IR;
        else if (!strcmp(str, "cable"))
          config->connectmedium = MEDIUM_CABLE;
#ifdef HAVE_BLUETOOTH
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"btunit")) {
        baswap(&(config->bdaddr), strtoba(str));
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"btchannel")) {
        config->btchannel = atoi(str);
#endif
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"irname")) {
        strncpy(config->irunit.name, str, 31);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"irserial")) {
        strncpy(config->irunit.serial, str, 127);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"cabledev")) {
        strncpy(config->cabledev, str, 19);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"cabletype")) {
        config->cabletype = atoi(str);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"donttellsync")) {
        if (!strcmp(str, "true"))
          config->donttellsync = TRUE;
        else
          config->donttellsync = FALSE;
      }
      xmlFree(str);
    }
    cur = cur->next;
  }

  xmlFreeDoc(doc);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;

error_free_doc:
    xmlFreeDoc(doc);
error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

/**
 * Load the general synchronization anchor.
 */
void load_sync_anchors( irmc_environment *env )
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

  irmc_config *config = &(env->config);

  char *anchor = osync_anchor_retrieve(env->anchor_path, "general");
  if (!anchor) {
    config->serial_number = NULL;
  } else {
    char data[ 256 ];
    memset( data, 0, sizeof( data ) );

    sscanf( anchor, "%s", data );
    config->serial_number = g_strdup( data );
  }
  g_free( anchor );
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Save synchronization anchors.
 */
void save_sync_anchors( const irmc_environment *env )
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);

  const irmc_config *config = &(env->config);
   
  osync_anchor_update( env->anchor_path, "general", config->serial_number );
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Creates the calendar specific changeinfo for slow- and fastsync
 */
void create_calendar_changeinfo(int sync_type, OSyncObjTypeSink *sink, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %s, %i)", __func__, sync_type, ctx, data, luid, type);
  osync_trace(TRACE_SENSITIVE, "Content of data:\n%s", data);

  OSyncError *error = NULL;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);
  
  if (sync_type == SLOW_SYNC) {
    char *event_start = data, *todo_start;
    char *event_end = NULL;
    char *event = NULL;
    int objtype;


    do {
      char *start = event_start;
      objtype = SYNC_OBJECT_TYPE_CALENDAR;
      event_start = strstr(start, "BEGIN:VEVENT");
      todo_start = strstr(start, "BEGIN:VTODO");
      if (!event_start || (todo_start && (todo_start < event_start))) {
        event_start = todo_start;
        objtype = SYNC_OBJECT_TYPE_TODO;
      }
      if (objtype == SYNC_OBJECT_TYPE_CALENDAR)
        if ((event_end = strstr(start, "END:VEVENT")))
          event_end += strlen("END:VEVENT");

      if (objtype == SYNC_OBJECT_TYPE_TODO)
        if ((event_end = strstr(start, "END:VTODO")))
          event_end += strlen("END:VTODO");

      if (event_start && event_end) {
        int pos = 0;
        int event_size = event_end - event_start + 256;
        event = g_malloc(event_size);
        memset(event, 0, event_size);

        sprintf(event, "BEGIN:VCALENDAR\r\nVERSION:1.0\r\n");
        pos = strlen(event);
        memcpy(event+pos,event_start,event_end-event_start);
        sprintf(event+(pos+event_end-event_start), "\r\nEND:VCALENDAR\r\n");

        OSyncChange *change = osync_change_new(&error);
        g_assert(change);


	OSyncData *odata = NULL;
	//TODO: strlen() + 1
        event_size = strlen(event);

        if (objtype == SYNC_OBJECT_TYPE_CALENDAR) {
	  odata = osync_data_new(event, event_size, database->objformat, &error);
        } else if (objtype == SYNC_OBJECT_TYPE_TODO) {
	  odata = osync_data_new(event, event_size, database->objformat, &error);
	}

        event_start = strstr(event, "X-IRMC-LUID:");
        if (event_start) {
          char luid[256];
          if (sscanf(event_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }

	osync_change_set_data(change, odata);
        osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
        osync_context_report_change(ctx, change);
      }

      event_start = event_end;
    } while(event_start);

  } else {
    OSyncChange *change = osync_change_new(&error);
    g_assert(change);

    osync_change_set_uid(change, g_strdup(luid));

    int event_size;

    if (!data) {
      data = NULL;
      event_size = 0;
    } else {
      event_size = strlen(data);
    }

    /* H stands for hard delete. D stands for delete. */
    if (type == 'H' || type == 'D')
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
    else if (type == 'M' || event_size == 0) {
      OSyncData *odata = osync_data_new(data, event_size, plain, &error);


      osync_change_set_data(change, odata);
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Creates the addressbook specific changeinfo for slow- and fastsync
 */
void create_addressbook_changeinfo(int sync_type, OSyncObjTypeSink *sink, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %s, %i)", __func__, sync_type, ctx, data, luid, type);			
  osync_trace(TRACE_SENSITIVE, "Content of data:\n%s", data);

  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  OSyncError *error = NULL;
  int vcard_size = 0;

  
  if (sync_type == SLOW_SYNC) {
    char *vcard_start = data;
    char *vcard_end = NULL;
    char *vcard = NULL;

    do {
      char *start = vcard_start;
      vcard_start = strstr(start, "BEGIN:VCARD");
      if ((vcard_end = strstr(start, "END:VCARD")))
        vcard_end += strlen("END:VCARD");

      if (vcard_start && vcard_end) {
        vcard_size = vcard_end - vcard_start+1;
        vcard = g_malloc(vcard_size);
        memcpy(vcard, vcard_start, vcard_end - vcard_start);
        vcard[vcard_end - vcard_start] = 0;

        OSyncChange *change = osync_change_new(&error);
        g_assert(change);

        vcard_start = strstr(vcard, "X-IRMC-LUID:");
        if (vcard_start) {
          char luid[256];
          if (sscanf(vcard_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }

        OSyncData *odata = osync_data_new(vcard, vcard_size, database->objformat, &error);

        osync_change_set_data(change, odata);

        osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
        osync_context_report_change(ctx, change);
      }

      vcard_start = vcard_end;
    } while(vcard_start);
  } else {
    OSyncChange *change = osync_change_new(&error);
    g_assert(change);

    osync_change_set_uid(change, g_strdup(luid));

    if ((data != NULL) && (strlen(data) > 0)) {
      vcard_size = strlen(data)+1;
    } else {
      vcard_size = 0;
    }

    /* H stands for hard delete. D stands for delete. */
    if (type == 'H' || type == 'D') {
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
      OSyncData *odata = osync_data_new(NULL, 0, database->objformat, &error);
      osync_change_set_data(change, odata);
    } else if (type == 'M' || vcard_size == 0) {
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
      OSyncData *odata = osync_data_new(data, vcard_size, database->objformat, &error);
      osync_change_set_data(change, odata);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Creates the notebook specific changeinfo for slow- and fastsync
 */
void create_notebook_changeinfo(int sync_type, OSyncObjTypeSink *sink, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %p, %s, %i)", __func__, sync_type, ctx, data, luid, type);			
  osync_trace(TRACE_SENSITIVE, "Content of data:\n%s", data);

  irmc_database *database = osync_objtype_sink_get_userdata(sink);
  
  OSyncError *error = NULL;

  if (sync_type == SLOW_SYNC) {
    char *vnote_start = data;
    char *vnote_end = NULL;
    char *vnote = NULL;

    do {
      char *start = vnote_start;
      vnote_start = strstr(start, "BEGIN:VNOTE");
      if ((vnote_end = strstr(start, "END:VNOTE")))
        vnote_end += strlen("END:VNOTE");

      if (vnote_start && vnote_end) {
        int vnote_size = vnote_end - vnote_start+1;
        vnote = g_malloc(vnote_size);
        memcpy(vnote, vnote_start, vnote_end - vnote_start);
        vnote[vnote_end - vnote_start] = 0;

        OSyncChange *change = osync_change_new(&error);
        g_assert(change);

        //osync_change_set_objformat_string(change, "vnote11");

        vnote_start = strstr(vnote, "X-IRMC-LUID:");
        if (vnote_start) {
          char luid[256];
          if (sscanf(vnote_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }

	// TODO: strlen() + 1
        vnote_size = strlen(vnote);
        OSyncData *data = osync_data_new(vnote, vnote_size, database->objformat, &error);

        osync_change_set_data(change, data);


        osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
        osync_context_report_change(ctx, change);
      }

      vnote_start = vnote_end;
    } while(vnote_start);
  } else {
    OSyncChange *change = osync_change_new(&error);
    g_assert(change);

    //osync_change_set_objformat_string(change, "vnote11");
    osync_change_set_uid(change, g_strdup(luid));

    int vnote_size;
    if (!data) {
      data = NULL;
      vnote_size = 0;
    } else {
      vnote_size = strlen(data);
    }

    /* H stands for hard delete. D stands for delete. */
    if (type == 'H' || type == 'D')
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
    else if (type == 'M' || vnote_size == 0) {
      osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
      OSyncData *odata = osync_data_new(data, vnote_size, database->objformat, &error);

      osync_change_set_data(change, odata);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Generic method for retrieving object independend change information.
 *
 * The object specific parts are handled by the following methods:
 *   - create_calendar_changeinfo()
 *   - create_addressbook_changeinfo()
 *   - create_notebook_changeinfo()
 */
gboolean get_generic_changeinfo(irmc_environment *env, OSyncPluginInfo *oinfo, OSyncContext *ctx, data_type_information *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, env, info, ctx, info, error);

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(oinfo);

  char *buffer;
  char *buffer_pos;
  int buffer_length;

  char serial_number[256];
  char database_id[256];
  char *filename;
  int dummy;

  irmc_config *config = &(env->config);


  buffer = g_malloc(DATABUFSIZE);

  osync_trace(TRACE_INTERNAL, "syncing %s\n", info->name );
  memset(buffer, 0, DATABUFSIZE);

  // check whether we have to do a slowsync
  if (osync_objtype_sink_get_slowsync(sink) == TRUE) {
    osync_trace(TRACE_INTERNAL, "slowsync %s\n", info->name );
    buffer_length = DATABUFSIZE;
    if (config->donttellsync) {
      // reconnect with "IRMC-SYNC" to get X-IRMC-LUID
      irmc_obex_disconnect(config->obexhandle, error);
      sleep(1);
      if (!irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
        sleep(2);
        if (!irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
          g_free(buffer);
          goto error;
        }
      }
    }

    // get $objecttype/info.log for debugging
    memset(buffer, 0, DATABUFSIZE);
    buffer_length = DATABUFSIZE;

    filename = g_strdup_printf("telecom/%s/info.log", info->path_identifier);
    if (!irmc_obex_get(config->obexhandle, filename, buffer, &buffer_length, error)) {
      g_free(buffer);
      g_free(filename);
      goto error;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
      osync_trace(TRACE_INTERNAL, "info.log of object type \"%s\"\n%s\n",
		      info->path_identifier, buffer);
    }

    // read change counter from device
    memset(buffer, 0, DATABUFSIZE);
    buffer_length = DATABUFSIZE;

    filename = g_strdup_printf("telecom/%s/luid/cc.log", info->path_identifier);
    if (!irmc_obex_get(config->obexhandle, filename, buffer, &buffer_length, error)) {
      g_free(buffer);
      g_free(filename);
      goto error;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
      sscanf(buffer, "%d", info->change_counter);
    }

    memset(buffer, 0, DATABUFSIZE);
    buffer_length = DATABUFSIZE;

    // read complete data from device
    filename = g_strdup_printf("telecom/%s.%s", info->path_identifier, info->path_extension);
    if (!irmc_obex_get(config->obexhandle, filename, buffer, &buffer_length, error)) {
      // continue anyway; Siemens models will fail this get if document is empty
      g_free(filename);
      osync_error_unref(error);
      *error = 0;
      buffer_length = 0;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
    }

    osync_trace(TRACE_SENSITIVE, "OBEX-IN:\n%s\n", buffer);

    // handle object specific part
    if ( strcmp( info->identifier, "event" ) == 0 )
      create_calendar_changeinfo( SLOW_SYNC, sink, ctx, buffer, 0, 0 );
    else if ( strcmp( info->identifier, "contact" ) == 0 )
      create_addressbook_changeinfo( SLOW_SYNC, sink, ctx, buffer, 0, 0 );
    else if ( strcmp( info->identifier, "note" ) == 0 )
     create_notebook_changeinfo( SLOW_SYNC, sink, ctx, buffer, 0, 0 );

  } else {
    osync_trace(TRACE_INTERNAL, "fastsync %s\n", info->name );

    memset(buffer, 0, DATABUFSIZE);
    buffer_length = DATABUFSIZE;

    osync_trace(TRACE_INTERNAL, "retrieving 'telecom/%s/luid/%d.log'\n", info->path_identifier, *( info->change_counter ) );

    // retrieve change log for current change counter
    filename = g_strdup_printf("telecom/%s/luid/%d.log", info->path_identifier, *(info->change_counter));
    if (!irmc_obex_get(config->obexhandle, filename, buffer, &buffer_length, error)) {
      g_free(filename);
      g_free(buffer);
      goto error;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
    }

    // initialize buffer_pos with the begin of buffer
    buffer_pos = buffer;

    // skip the change log header
    sscanf(buffer_pos, "SN:%256s\r\n", serial_number);
    buffer_pos = strstr(buffer_pos, "\r\n");
    if (!buffer_pos) {
      g_free(buffer);
      osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
      return TRUE;
    }

    buffer_pos += 2;
    sscanf(buffer_pos, "DID:%256s\r\n", database_id);
    buffer_pos = strstr(buffer_pos, "\r\n");
    if (!buffer_pos) {
      g_free(buffer);
      osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
      return TRUE;
    }
    buffer_pos += 2;
    sscanf(buffer_pos, "Total-Records:%d\r\n", &dummy);
    buffer_pos = strstr(buffer_pos, "\r\n");
    if (!buffer_pos) {
      g_free(buffer);
      osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
      return TRUE;
    }
    buffer_pos += 2;
    sscanf(buffer_pos, "Maximum-Records:%d\r\n", &dummy);
    buffer_pos = strstr(buffer_pos, "\r\n");

    // parse all change log entries
    while (buffer_pos) {
      char type;
      int cc;
      char luid[256];

      buffer_pos += 2;
      if (sscanf(buffer_pos, "%c:%d::%256[^\r\n]", &type, &cc, luid) == 3) {
        int data_size = 10240;
        char *data = g_malloc(data_size);

        memset(data, 0, data_size);

	// Don't try do get removed entries!
	if (type != 'H') {
		// retrieve data for the specific change log entry
		filename = g_strdup_printf("telecom/%s/luid/%s.%s", info->path_identifier, luid, info->path_extension);
		if (!irmc_obex_get(config->obexhandle, filename, data, &data_size, error)){
		  g_free(buffer);
		  g_free(filename);
		  g_free(data);
		  goto error;
		} else {
		  g_free(filename);
		  data[data_size] = '\0';
		}
	}

        // handle object specific part
        if ( strcmp( info->identifier, "event" ) == 0 )
          create_calendar_changeinfo(FAST_SYNC, sink, ctx, data, luid, type);
        else if ( strcmp( info->identifier, "contact" ) == 0 )
          create_addressbook_changeinfo(FAST_SYNC, sink, ctx, data, luid, type);
        else if ( strcmp( info->identifier, "note" ) == 0 )
          create_notebook_changeinfo(FAST_SYNC, sink, ctx, data, luid, type);
      }

      buffer_pos = strstr(buffer_pos, "\r\n");
    }

    memset(buffer, 0, DATABUFSIZE);
    buffer_length = DATABUFSIZE;

    // update change counter
    filename = g_strdup_printf("telecom/%s/luid/cc.log", info->path_identifier);
    if (!irmc_obex_get(config->obexhandle, filename, buffer, &buffer_length, error)) {
      g_free(filename);
      g_free(buffer);
      goto error;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
      sscanf(buffer, "%d", info->change_counter);
    }
  }

  g_free(buffer);
  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;

error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}





/**
 * Checks whether a slowsync is necessary.
 *
 * That's the case if one of the following conditions matches:
 *   - the device has an unknown serial number
 *   - the database id differs from our saved one
 *   - the changelog contains a '*' in the last line
 */
gboolean detect_slowsync(int changecounter, char *object, char **dbid, char **serial_number,
                         gboolean *slowsync, obex_t obexhandle, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%d, %s, %s, %s, %p, %p)", __func__, changecounter, object, *dbid, *serial_number, obexhandle, error);

  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int ret;

  data = g_malloc(DATABUFSIZE);
  datap = data;

  filename = g_strdup_printf("telecom/%s/luid/%d.log", object, changecounter);

  memset(data, 0, DATABUFSIZE);
  len = DATABUFSIZE - 1;
  if (!irmc_obex_get(obexhandle, filename, data, &len, error)) {
    g_free(filename);
    g_free(data);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return FALSE;
  }

  g_free(filename);
  data[len] = '\0';

  ret = sscanf(datap, "SN:%256s\r\n", serial);
 if (ret > 0) {
    if (!*serial_number || strcmp(*serial_number, serial)) {
      if (*serial_number)
        g_free(*serial_number);
      *serial_number = g_strdup(serial);
      *slowsync = TRUE;
    }
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
    return TRUE;
  }

  datap+=2;
  sscanf(datap, "DID:%256s\r\n", did);
  if (!*dbid || strcmp(*dbid, did)) {
    // This is a new database
    if (*dbid)
      g_free(*dbid);
    *dbid = g_strdup(did);
    *slowsync = TRUE;
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
    return TRUE;
  }
  datap+=2;
  //sscanf(datap, "Total-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
    return TRUE;
  }
  datap+=2;
  //sscanf(datap, "Maximum-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");

  if ( strstr(datap, "*") != NULL ) {
    *slowsync = TRUE;
  }

  g_free(data);

  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
}

/* Establish connection to the device */
static void irmcConnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);			

  irmc_environment *env = (irmc_environment *)data;
  irmc_config *config = &(env->config);
  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  const char *objtype = osync_objtype_sink_get_name(sink);

  char *anchor = osync_anchor_retrieve(env->anchor_path, objtype);
  if (!anchor) {
    database->changecounter = 0;
    database->dbid = NULL;
  } else {
    char data[ 256 ];
    memset( data, 0, sizeof( data ) );

    sscanf( anchor, "%d:%256s", &(database->changecounter), data );
    database->dbid = g_strdup( data );
  }
  g_free( anchor );

  OSyncError *error = NULL;
  if (! env->isConnected) {

	  config->obexhandle = irmc_obex_client(config);
	  // connect to the device
	  if (!irmc_obex_connect(config->obexhandle, config->donttellsync ? NULL : "IRMC-SYNC", &error)) {
	    irmc_disconnect(config);
	    osync_context_report_osyncerror(ctx, error);
	    osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
	   return;
	  }
  }

  // load the general synchronization anchors
  load_sync_anchors(env);

  // check whether a slowsync is necessary
  gboolean slowsync = FALSE;

  if (!detect_slowsync(database->changecounter, database->obex_db, &(database->dbid),
                         &(config->serial_number), &slowsync, config->obexhandle, &error)) {
    irmc_disconnect(config);
    osync_context_report_osyncerror(ctx, error);
    osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
   return;
  }

  if (slowsync == TRUE) {
	  osync_trace(TRACE_INTERNAL, "Have to do a slowsync for objtype %s", objtype);
	  osync_objtype_sink_set_slowsync(sink, TRUE);
  } else {
    osync_trace(TRACE_INTERNAL, "No slowsync required");
  }   

  osync_context_report_success(ctx);
}

/**
 * This method is called to disconnect from the device.
 */
static void irmcDisconnect(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);

  irmc_environment *env = (irmc_environment *)data;

  // TODO: handle disconnect of several sink engines..
  // disconnect from the device
  irmc_disconnect(&(env->config));

  save_sync_anchors(env);

  osync_context_report_success(ctx);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * This method is called when the sync was successfull.
 */
static void irmcSyncDone(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);			

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  const char *objtype = osync_objtype_sink_get_name(sink);

  if (database->changecounter >= 0 && strcmp(database->dbid, "FFFFFF")) {
    char *anchor = g_strdup_printf("%d:%s", database->changecounter, database->dbid);
    osync_anchor_update( env->anchor_path, objtype, anchor );
    g_free(anchor);
  } else {
    osync_trace(TRACE_INTERNAL, "ERROR: Invalid values for event anchor detected.");
  }

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Requests all changeinfo from the device.
 *
 * This method calls get_generic_changeinfo() for objtype event
 */
static void irmcCalendarGetChangeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%p, %p, %p)", __func__, data, info, ctx);
  OSyncError *error = 0;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  data_type_information datainfo;

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "calendar");
  strcpy(datainfo.identifier, "event");
  strcpy(datainfo.path_identifier, "cal");
  strcpy(datainfo.path_extension, "vcs");
  datainfo.change_counter = &(database->changecounter);


  if (!get_generic_changeinfo(env, info, ctx, &datainfo, &error))
    goto error;

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return;

error:
  osync_context_report_osyncerror(ctx, error);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/**
 * Requests all changeinfo from the device.
 *
 * This method calls get_generic_changeinfo() for objtype contact 
 */
static void irmcContactGetChangeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p,%p,%p)", __func__, data, info, ctx);
  OSyncError *error = 0;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  data_type_information datainfo;

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "addressbook");
  strcpy(datainfo.identifier, "contact");
  strcpy(datainfo.path_identifier, "pb");
  strcpy(datainfo.path_extension, "vcf");
  datainfo.change_counter = &(database->changecounter);

  if (!get_generic_changeinfo(env, info, ctx, &datainfo, &error))
    goto error;

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return;

error:
  osync_context_report_osyncerror(ctx, error);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/**
 * Requests all changeinfo from the device.
 *
 * This method calls get_generic_changeinfo() for objtype note 
 */
static void irmcNoteGetChangeinfo(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%p, %p, %p)", __func__, data, info, ctx);
  OSyncError *error = 0;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  data_type_information datainfo;

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "notebook");
  strcpy(datainfo.identifier, "note");
  strcpy(datainfo.path_identifier, "nt");
  strcpy(datainfo.path_extension, "vnt");
  datainfo.change_counter = &(database->changecounter);

  if (!get_generic_changeinfo(env, info, ctx, &datainfo, &error))
    goto error;

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return;

error:
  osync_context_report_osyncerror(ctx, error);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}


/**
 * Generic method to write back changeinfo to the device.
 */
osync_bool irmcGenericCommitChange(irmc_environment *env, OSyncObjTypeSink *sink, OSyncContext *ctx, data_type_information *info, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, sink, ctx, info, change);
  char name[256];
  OSyncData *data = NULL;
  char rsp_buffer[256];
  int rsp_buffer_size = 256;
  char param_buffer[256];
  char *param_buffer_pos = param_buffer;
  char new_luid[256];
  char *buf = NULL;
  unsigned int size;

  OSyncError *error = NULL;

  irmc_config *config = &(env->config);

  /**
   * If we add a new object the passed path name looks like the following
   *
   *  telecom/<type>/luid/.<extension>
   *
   * but when we change or delete an existing object, the luid must be appended
   *
   *  telecom/<type>/luid/<luid>.<extension>
   */

  // synthesize path name of the changed object
  snprintf(name, sizeof(name), "telecom/%s/luid/", info->path_identifier);

  if (osync_change_get_changetype(change) != OSYNC_CHANGE_TYPE_ADDED) {
    char *uid = (char *) osync_change_get_uid(change);
    if (uid) {
      safe_strcat(name, uid, 256);
    }
  }
  safe_strcat(name, ".", 256);
  safe_strcat(name, info->path_extension, 256);

  data = osync_change_get_data(change);


  osync_data_get_data(data, &buf, &size);

  // increase change counter
  (*(info->change_counter))++;

  memset(param_buffer, 0, sizeof(param_buffer));

  // synthesize obex header
  sprintf(param_buffer_pos + 2, "%d", *(info->change_counter));
  param_buffer_pos[0] = IRSYNC_APP_MAXEXPCOUNT;
  param_buffer_pos[1] = strlen(param_buffer_pos + 2);
  param_buffer_pos += strlen(param_buffer_pos + 2) + 2;

  memset(rsp_buffer, 0, sizeof(rsp_buffer));

  osync_trace(TRACE_INTERNAL, "change on object %s\n", name );
  switch (osync_change_get_changetype(change)) {

    case OSYNC_CHANGE_TYPE_DELETED:
      // modify obex header for hard delete request
      param_buffer_pos[0] = IRSYNC_APP_HARDDELETE;
      param_buffer_pos[1] = 0;
      param_buffer_pos += 2;

      // send the delete request
      if (!irmc_obex_put(config->obexhandle, name, 0, size ? buf : NULL, size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        g_free(buf);
        osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s FALSE: %s", __func__, osync_error_print(&error));
        return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s delete request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      break;

    case OSYNC_CHANGE_TYPE_ADDED:
      // send the add request
      if (!irmc_obex_put(config->obexhandle, name, 0, size ? buf : NULL, size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        g_free(buf);
        osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s FALSE: %s", __func__, osync_error_print(&error));
	return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the new luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s added request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      // set the returned luid for this change
      osync_change_set_uid(change, new_luid);

      break;

    case OSYNC_CHANGE_TYPE_MODIFIED:
      // send the modify request
      if (!irmc_obex_put(config->obexhandle, name, 0, size ? buf : NULL, size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: FALSE: %s", __func__, osync_error_print(&error));
        return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s modified request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      break;

    default:
      osync_trace(TRACE_INTERNAL, "Unknown change type");
  }

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
}

/**
 * Commits the calendar specific changeinfo to the device.
 */
static void irmcCalendarCommitChange(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, data, info, ctx, change);
  data_type_information datainfo;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "calendar");
  strcpy(datainfo.identifier, "event");
  strcpy(datainfo.path_identifier, "cal");
  strcpy(datainfo.path_extension, "vcs");
  datainfo.change_counter = &(database->changecounter);

  irmcGenericCommitChange(env, sink, ctx, &datainfo, change);
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Commits the addressbook specific changeinfo to the device.
 */
static void irmcContactCommitChange(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);			
  data_type_information datainfo;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "addressbook");
  strcpy(datainfo.identifier, "contact");
  strcpy(datainfo.path_identifier, "pb");
  strcpy(datainfo.path_extension, "vcf");
  datainfo.change_counter = &(database->changecounter);

  irmcGenericCommitChange(env, sink, ctx, &datainfo, change);
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Commits the notebook specific changeinfo to the device.
 */
static void irmcNoteCommitChange(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);			
  data_type_information datainfo;

  OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
  irmc_environment *env = (irmc_environment *)data;
  irmc_database *database = osync_objtype_sink_get_userdata(sink);

  memset(&datainfo, 0, sizeof(datainfo));
  strcpy(datainfo.name, "notebook");
  strcpy(datainfo.identifier, "note");
  strcpy(datainfo.path_identifier, "nt");
  strcpy(datainfo.path_extension, "vnt");
  datainfo.change_counter = &(database->changecounter);

  irmcGenericCommitChange(env, sink, ctx, &datainfo, change);
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * This method frees the member specific environment.
 */
static void irmcFinalize(void *data)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

  irmc_environment *env = (irmc_environment *)data;

  g_free(env->anchor_path);

  while (env->databases) {
    irmc_database *db = env->databases->data;
    g_free(db);
    env->databases = g_list_remove(env->databases, db);
  }

  g_free(env);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Discover the capabilities of the IrMC mobile.
 */
static osync_bool irmcDiscover(void *data, OSyncPluginInfo *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);

  irmc_environment *env = (irmc_environment *)data;

  GList *s = NULL;
  for (s = env->databases; s; s = s->next) {
  	irmc_database *sinkenv = s->data;
  	osync_objtype_sink_set_available(sinkenv->sink, TRUE);
  }

  OSyncVersion *version = osync_version_new(error);
  osync_version_set_plugin(version, "irmc-sync");
  //osync_version_set_modelversion(version, "version");
  //osync_version_set_firmwareversion(version, "firmwareversion");
  //osync_version_set_softwareversion(version, "softwareversion");
  //osync_version_set_hardwareversion(version, "hardwareversion");
  osync_plugin_info_set_version(info, version);
  osync_version_unref(version);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return TRUE;
}

static irmc_database *create_database(OSyncPluginInfo *info, const char *objtype, const char *format, char *obex_db, OSyncSinkGetChangesFn getchanges, OSyncSinkCommitFn commit, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p)", __func__, info, objtype, format, error);	

  OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);

  irmc_database *database = osync_try_malloc0(sizeof(irmc_database), error); 
  if (!database)
    goto error;

  database->sink = osync_objtype_sink_new(objtype, error);
  if (!database->sink)
    goto error_free_db;

  OSyncObjTypeSinkFunctions functions;
  memset(&functions, 0, sizeof(functions));
  functions.connect = irmcConnect;
  functions.disconnect = irmcDisconnect;
  functions.sync_done = irmcSyncDone;
  functions.get_changes = getchanges;
  functions.commit = commit;

  osync_objtype_sink_set_functions(database->sink, functions, database);

  database->objformat = osync_format_env_find_objformat(formatenv, format);
  if (!database->objformat) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Can't find object format \"%s\" for object type \"%s\"! "
                                           "Is the vformat plugin correctly installed?", format, objtype);
    goto error_free_db;
  }
  osync_objtype_sink_add_objformat(database->sink, format);

  osync_plugin_info_add_objtype(info, database->sink);

  database->obex_db = obex_db;

  osync_trace(TRACE_EXIT, "%s: %p", __func__, database);
  return database;

  error_free_db:
  g_free(database);

  error:    
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

/* Initialize connection */
static void *irmcInitialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugin, info, error);			
  const char *configdata = NULL;

  // create new environment, where all connection information are stored in
  irmc_environment *env = osync_try_malloc0(sizeof(irmc_environment), error);
  if (!env)
    goto error;

  env->isConnected = FALSE;

  // retrieve the config data
  configdata = osync_plugin_info_get_config(info); 
  if (!configdata) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to get config data.");
    goto error_free_env;
  }

  // parse the config data
  if (!parse_settings( &(env->config), configdata, strlen(configdata), error)) {
    osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse config data: %s", osync_error_print(error));
    goto error_free_env;
  }

  // set default irmc anchor path
  env->anchor_path = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));

  irmc_database *contactdb = create_database(info, "contact", "vcard21", "pb", irmcContactGetChangeinfo, irmcContactCommitChange, error); 
  irmc_database *eventdb = create_database(info, "event", "vevent10", "cal", irmcCalendarGetChangeinfo, irmcCalendarCommitChange, error);
  irmc_database *tododb = create_database(info, "todo", "vtodo10", "cal", irmcCalendarGetChangeinfo, irmcCalendarCommitChange, error);
  irmc_database *notedb = create_database(info, "note", "vnote11", "nt", irmcNoteGetChangeinfo, irmcNoteCommitChange, error);

  if (!contactdb || !eventdb || !tododb || !notedb)
    goto error_free_env;

  env->databases = g_list_append(env->databases, contactdb); 
  env->databases = g_list_append(env->databases, eventdb); 
  env->databases = g_list_append(env->databases, tododb); 
  env->databases = g_list_append(env->databases, notedb); 

  // return the environment
  osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
  return (void *)env;

error_free_env:
  free(env);

error:    
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return NULL;
}

/* Return all plugin information */
osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
  OSyncPlugin *plugin = osync_plugin_new(error);
  if (!plugin)
    goto error;

  osync_plugin_set_name(plugin, "irmc-sync");
  osync_plugin_set_longname(plugin, "IrMC Mobile Device");
  osync_plugin_set_description(plugin, "IrMC Protocl based Mobiles like older Sony Ericsson, Siemens or other modles");

  osync_plugin_set_initialize(plugin, irmcInitialize);
  osync_plugin_set_finalize(plugin, irmcFinalize);
  osync_plugin_set_discover(plugin, irmcDiscover);

  osync_plugin_env_register_plugin(env, plugin);
  osync_plugin_unref(plugin);

  return TRUE;

error:  
  osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
  osync_error_unref(error);
  return FALSE;
}

int get_version(void)
{
  return 1;
}
