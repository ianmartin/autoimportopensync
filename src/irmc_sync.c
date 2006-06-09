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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "sync_vtype.h"
#include "irmc_sync.h"
#include "irmc_bluetooth.h"

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

gboolean get_generic_changeinfo(OSyncContext *ctx, data_type_information *info, OSyncError **error);
void create_calendar_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type);
void create_addressbook_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type);
void create_notebook_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type);

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
    osync_error_free(&error);
    error = NULL;
  }

  irmc_obex_disconnect(config->obexhandle, &error);
  if (error)
    osync_error_free(&error);

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
      osync_error_free(&error);

    irmc_obex_cleanup(config->obexhandle);
  }

  config->obexhandle = 0;
}

/**
 * Parses the configuration of this plugin from a string in xml format.
 */
osync_bool parse_settings(irmc_config *config, const char *data, unsigned int size, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, config, data, error);
  xmlDoc *doc = NULL;
  xmlNode *cur = NULL;

  // set defaults
  config->fixdst = FALSE;
  config->donttellsync = FALSE;
  config->onlyphonenumbers = TRUE;
  config->dontacceptold = TRUE;
  config->maximumage = 7;
  config->translatecharset = FALSE;
  config->charset = g_strdup("ISO8859-1");
  config->alarmtoirmc = TRUE;
  config->alarmfromirmc = TRUE;
  config->convertade = FALSE;
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
#if HAVE_BLUETOOTH
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"btunit")) {
        baswap(&(config->btunit.bdaddr), strtoba(str));
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
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"managedbsize")) {
        if (!strcmp(str, "true"))
          config->managedbsize = TRUE;
        else
          config->managedbsize = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"fakerecur")) {
        if (!strcmp(str, "true"))
          config->fake_recurring = TRUE;
        else
          config->fake_recurring = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"fixedst")) {
        if (!strcmp(str, "true"))
          config->fixdst = TRUE;
        else
          config->fixdst = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"donttellsync")) {
        if (!strcmp(str, "true"))
          config->donttellsync = TRUE;
        else
          config->donttellsync = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"onlyphonenumbers")) {
        if (!strcmp(str, "true"))
          config->onlyphonenumbers = TRUE;
        else
          config->onlyphonenumbers = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"dontacceptold")) {
        if (!strcmp(str, "true"))
          config->dontacceptold = TRUE;
        else
          config->dontacceptold = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"maximumage")) {
        config->maximumage = atoi(str);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"translatecharset")) {
        if (!strcmp(str, "true"))
          config->translatecharset = TRUE;
        else
          config->translatecharset = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"charset")) {
        if (config->charset)
          g_free(config->charset);
        config->charset = g_strdup(str);
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"alarmfromirmc")) {
        if (!strcmp(str, "true"))
          config->alarmfromirmc = TRUE;
        else
          config->alarmfromirmc = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"alarmtoirmc")) {
        if (!strcmp(str, "true"))
          config->alarmtoirmc = TRUE;
        else
          config->alarmtoirmc = FALSE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"convertade")) {
        if (!strcmp(str, "true"))
          config->convertade = TRUE;
        else
          config->convertade = FALSE;
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
 * Load the synchronization anchors.
 */
void load_sync_anchors( OSyncMember *member, irmc_config *config )
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, config);			
  char *anchor = osync_anchor_retrieve(member, "event");
  if (!anchor) {
    config->calendar_changecounter = 0;
    config->calendar_dbid = NULL;
  } else {
    char data[ 256 ];
    memset( data, 0, sizeof( data ) );

    sscanf( anchor, "%d:%256s", &config->calendar_changecounter, data );
    config->calendar_dbid = g_strdup( data );
  }
  g_free( anchor );

  anchor = osync_anchor_retrieve(member, "contact");
  if (!anchor) {
    config->addressbook_changecounter = 0;
    config->addressbook_dbid = NULL;
  } else {
    char data[ 256 ];
    memset( data, 0, sizeof( data ) );
    sscanf( anchor, "%d:%256s", &config->addressbook_changecounter, data );
    config->addressbook_dbid = g_strdup( data );
  }
  g_free( anchor );

  anchor = osync_anchor_retrieve(member, "note");
  if (!anchor) {
    config->notebook_changecounter = 0;
    config->notebook_dbid = NULL;
  } else {
    char data[ 256 ];
    memset( data, 0, sizeof( data ) );

    sscanf( anchor, "%d:%256s", &config->notebook_changecounter, data );
    config->notebook_dbid = g_strdup( data );
  }
  g_free( anchor );

  anchor = osync_anchor_retrieve(member, "general");
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
void save_sync_anchors( OSyncMember *member, const irmc_config *config )
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, config);			
  char anchor[ 1024 ];

  snprintf( anchor, sizeof( anchor ), "%d:%s", config->calendar_changecounter, config->calendar_dbid );
  osync_anchor_update( member, "event", anchor );

  snprintf( anchor, sizeof( anchor ), "%d:%s", config->addressbook_changecounter, config->addressbook_dbid );
  osync_anchor_update( member, "contact", anchor );

  snprintf( anchor, sizeof( anchor ), "%d:%s", config->notebook_changecounter, config->notebook_dbid );
  osync_anchor_update( member, "note", anchor );

  snprintf( anchor, sizeof( anchor ), "%s", config->serial_number );
  osync_anchor_update( member, "general", anchor );
  osync_trace(TRACE_EXIT, "%s", __func__);
}

#if HAVE_BLUETOOTH
/**
 * Scan for available bluetooth devices and return a list of
 * found devices in xml structure.
 */
void *scan_devices( void *foo, const char *query, void *bar )
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, foo, query, bar);			
  xmlDoc *doc;
  xmlNode *node, *devices;
  xmlChar *data;
  int size;

  doc = xmlNewDoc((const xmlChar*)"1.0");
  devices = xmlNewDocNode(doc, NULL, (const xmlChar*)"devices", NULL);
  xmlDocSetRootElement(doc, devices);

  GList *unit_list = find_bt_units();
  for (; unit_list; unit_list = unit_list->next) {
    irmc_bt_unit *unit = unit_list->data;
    node = xmlNewTextChild(devices, NULL, (const xmlChar*)"device", NULL);
    xmlNewProp(node, (const xmlChar*)"address", (const xmlChar*)(unit->address));
    char *number = g_strdup_printf("%d", unit->channel);
    xmlNewProp(node, (const xmlChar*)"channel", (const xmlChar*)number);
    g_free(number);
    xmlNewProp(node, (const xmlChar*)"name", (const xmlChar*)(unit->name));
  }

  xmlDocDumpFormatMemory( doc, &data, &size, 0 );

  osync_trace(TRACE_EXIT, "%s: %p", __func__, data);
  return data;
}
#endif

/**
 * Tests whether a connection to a device with the given configuration can
 * be established.
 */
int *test_connection( void *foo, const char *configuration, void *bar )
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, foo, configuration, bar);			
  char data[10240];
  int size = 10240;
  OSyncError *error = NULL;
  irmc_config config;
  int *result = (int*)malloc(sizeof(int));

  // parse the configuration
  if (!parse_settings(&config, configuration, strlen(configuration), &error)) {
    osync_error_free(&error);
    *result = 0;
    osync_trace(TRACE_EXIT, "%s: %p", __func__, result);
    return result;
  }

  config.obexhandle = irmc_obex_client(&config);

  // connect to the device
  if (!irmc_obex_connect(config.obexhandle, config.donttellsync ? NULL : "IRMC-SYNC", &error)) {
    osync_error_free(&error);
    if (!irmc_obex_disconnect(config.obexhandle, &error))
      osync_error_free(&error);

    *result = 0;
    osync_trace(TRACE_EXIT, "%s: %p", __func__, result);
    return result;
  }

  memset( data, 0, sizeof( data ) );
  size = sizeof( data );

  // retrieve the 'telecom/devinfo.txt' file, which is available on all IrMC capable devices
  if (!irmc_obex_get(config.obexhandle, "telecom/devinfo.txt", data, &size, &error)) {
    osync_error_free(&error);
    if (!irmc_obex_disconnect(config.obexhandle, &error))
      osync_error_free(&error);
    irmc_obex_cleanup(config.obexhandle);

    *result = 0;
    osync_trace(TRACE_EXIT, "%s: %p", __func__, result);
    return result;
  }
  data[ size ] = '\0';

  // succeeded to connect and fetch data
  if (!irmc_obex_disconnect(config.obexhandle, &error))
    osync_error_free(&error);
  irmc_obex_cleanup(config.obexhandle);

  *result = 1;
  osync_trace(TRACE_EXIT, "%s: %p", __func__, result);
  return result;
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
  osync_trace(TRACE_ENTRY, "%s(%d, %s, %p, %p)", __func__, changecounter, object, obexhandle, error);

  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int foo;

  data = g_malloc(DATABUFSIZE);
  datap = data;

  len = DATABUFSIZE;
  filename = g_strdup_printf("telecom/%s/luid/%d.log", object, changecounter);

  memset(data, 0, DATABUFSIZE);
  len = DATABUFSIZE;
  if (!irmc_obex_get(obexhandle, filename, data, &len, error)) {
    g_free(filename);
    g_free(data);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return FALSE;
  }

  g_free(filename);
  data[len] = '\0';

  sscanf(datap, "SN:%256s\r\n", serial);
  if (!*serial_number || strcmp(*serial_number, serial)) {
    if (*serial_number)
      g_free(*serial_number);
    *serial_number = g_strdup(serial);
    *slowsync = TRUE;
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
  sscanf(datap, "Total-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "Maximum-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");

  if ( strstr(datap, "*") != NULL )
    *slowsync = TRUE;

  g_free(data);

  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
}


/**
 * Initialize the connection.
 */
static void *irmcInitialize(OSyncMember *member, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, member, error);			
  char *configdata;
  int configsize;

  // create new environment, where all connection information are stored in
  irmc_environment *env = malloc(sizeof(irmc_environment));
  assert(env != NULL);
  memset(env, 0, sizeof(irmc_environment));

  // retrieve the config data for this member
  if (!osync_member_get_config(member, &configdata, &configsize, error)) {
    osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
    free(env);
    osync_trace(TRACE_EXIT, "%s: NULL", __func__);
    return NULL;
  }

  // parse the config data of this member
  if (!parse_settings( &(env->config), configdata, configsize, error)) {
    osync_error_update(error, "Unable to parse config data: %s", osync_error_print(error));
    free(env);
    osync_trace(TRACE_EXIT, "%s: NULL", __func__);
    return NULL;
  }

  free(configdata);
  env->member = member;

  // return the environment
  osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
  return (void *)env;
}

/**
 * Establishes connection to the device.
 */
static void irmcConnect(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);			
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  config->obexhandle = irmc_obex_client(config);

  // connect to the device
  OSyncError *error = NULL;
  if (!irmc_obex_connect(config->obexhandle, config->donttellsync ? NULL : "IRMC-SYNC", &error)) {
    irmc_disconnect(config);
    osync_context_report_osyncerror(ctx, &error);
    osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
    return;
  }

  // load the synchronization anchors
  load_sync_anchors(env->member, config);

  // check whether a slowsync is necessary
  gboolean slowsync = FALSE;
  if ( !detect_slowsync( config->calendar_changecounter, "cal", &(config->calendar_dbid),
                         &(config->serial_number), &slowsync, config->obexhandle, &error ) )
  {
    irmc_disconnect(config);
    osync_context_report_osyncerror(ctx, &error);
    osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
    return;
  } else {
    osync_member_set_slow_sync(env->member, "event", slowsync);
  }

  slowsync = FALSE;
  if ( !detect_slowsync( config->addressbook_changecounter, "pb", &(config->addressbook_dbid),
                         &(config->serial_number), &slowsync, config->obexhandle, &error ) )
  {
    irmc_disconnect(config);
    osync_context_report_osyncerror(ctx, &error);
    osync_trace(TRACE_EXIT, "%s: %s", __func__, osync_error_print(&error));
    return;
  } else {
    osync_member_set_slow_sync(env->member, "contact", slowsync);
  }

  slowsync = FALSE;
  if ( !detect_slowsync( config->notebook_changecounter, "nt", &(config->notebook_dbid),
                         &(config->serial_number), &slowsync, config->obexhandle, &error ) )
  {
    irmc_disconnect(config);
    osync_context_report_osyncerror(ctx, &error);
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
    return;
  } else {
    osync_member_set_slow_sync(env->member, "note", slowsync);
  }

  osync_context_report_success(ctx);
}

/**
 * Requests all changeinfo from the device.
 *
 * This method calls get_generic_changeinfo() with
 * a different info structure for every object type
 * (calendar, addressbook, notebook).
 */
static void irmcGetChangeinfo(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
  OSyncError *error = 0;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  data_type_information info;

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "calendar");
  strcpy(info.identifier, "event");
  strcpy(info.path_identifier, "cal");
  strcpy(info.path_extension, "vcs");
  info.change_counter = &(config->calendar_changecounter);

  if (!get_generic_changeinfo(ctx, &info, &error))
    goto error;

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "addressbook");
  strcpy(info.identifier, "contact");
  strcpy(info.path_identifier, "pb");
  strcpy(info.path_extension, "vcf");
  info.change_counter = &(config->addressbook_changecounter);

  if (!get_generic_changeinfo(ctx, &info, &error))
    goto error;

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "notebook");
  strcpy(info.identifier, "note");
  strcpy(info.path_identifier, "nt");
  strcpy(info.path_extension, "vnt");
  info.change_counter = &(config->notebook_changecounter);

  if (!get_generic_changeinfo(ctx, &info, &error))
    goto error;

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return;

error:
  osync_context_report_osyncerror(ctx, &error);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

/**
 * Generic method for retrieving object independend change information.
 *
 * The object specific parts are handled by the following methods:
 *   - create_calendar_changeinfo()
 *   - create_addressbook_changeinfo()
 *   - create_notebook_changeinfo()
 */
gboolean get_generic_changeinfo(OSyncContext *ctx, data_type_information *info, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);

  char *buffer;
  char *buffer_pos;
  int buffer_length;

  char serial_number[256];
  char database_id[256];
  char *filename;
  int dummy;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  buffer = g_malloc(DATABUFSIZE);

  osync_trace(TRACE_INTERNAL, "syncing %s\n", info->name );
  memset(buffer, 0, DATABUFSIZE);

  // check whether we have to do a slowsync
  if (osync_member_get_slow_sync(env->member, info->identifier) == TRUE) {
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
      osync_error_free(error);
      *error = 0;
      buffer_length = 0;
    } else {
      g_free(filename);
      buffer[buffer_length] = '\0';
    }

    osync_trace(TRACE_INTERNAL, "obex get data: %s", buffer);

    // handle object specific part
    if ( strcmp( info->identifier, "event" ) == 0 )
      create_calendar_changeinfo( SLOW_SYNC, ctx, buffer, 0, 0 );
    else if ( strcmp( info->identifier, "contact" ) == 0 )
      create_addressbook_changeinfo( SLOW_SYNC, ctx, buffer, 0, 0 );
    else if ( strcmp( info->identifier, "note" ) == 0 )
      create_notebook_changeinfo( SLOW_SYNC, ctx, buffer, 0, 0 );

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

        // handle object specific part
        if ( strcmp( info->identifier, "event" ) == 0 )
          create_calendar_changeinfo(FAST_SYNC, ctx, data, luid, type);
        else if ( strcmp( info->identifier, "contact" ) == 0 )
          create_addressbook_changeinfo(FAST_SYNC, ctx, data, luid, type);
        else if ( strcmp( info->identifier, "note" ) == 0 )
          create_notebook_changeinfo(FAST_SYNC, ctx, data, luid, type);
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
 * Creates the calendar specific changeinfo for slow- and fastsync
 */
void create_calendar_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %s, %s, %i)", __func__, sync_type, ctx, data, luid, type);
  char *converted_event = NULL;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
//  irmc_config *config = &(env->config);

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

        OSyncChange *change = osync_change_new();
        osync_change_set_member(change, env->member);
        g_assert(change);

        if (objtype == SYNC_OBJECT_TYPE_CALENDAR)
          osync_change_set_objformat_string(change, "vevent20");
        else if (objtype == SYNC_OBJECT_TYPE_TODO)
          osync_change_set_objformat_string(change, "vtodo20");

        event_start = strstr(event, "X-IRMC-LUID:");
        if (event_start) {
          char luid[256];
          if (sscanf(event_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }

	/* XXX drop sync_vtype_* functions. 
        converted_event = sync_vtype_convert(event, 0 | (config->fixdst ? VOPTION_FIXDSTFROMCLIENT : 0) |
                                                        (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                                        VOPTION_CALENDAR1TO2 |
                                                        (config->alarmfromirmc ? 0 : VOPTION_REMOVEALARM) |
                                                        VOPTION_CONVERTUTC, config->charset);
	*/						
	converted_event = strdup(data);
        event_size = strlen(data);
	osync_change_set_data(change, converted_event, event_size, TRUE);
        osync_change_set_changetype(change, CHANGE_ADDED);
        osync_context_report_change(ctx, change);
      }

      event_start = event_end;
    } while(event_start);

  } else {
    OSyncChange *change = osync_change_new();
    osync_change_set_member(change, env->member);
    g_assert(change);

    osync_change_set_objformat_string(change, "plain");
    osync_change_set_uid(change, g_strdup(luid));

    int event_size = strlen(data);
    if (event_size > 0) {
      /* XXX drop sync_vtype_* functions	    
      converted_event = sync_vtype_convert(data, 0 | (config->fixdst ? VOPTION_FIXDSTFROMCLIENT : 0) |
                                          (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                          VOPTION_CALENDAR1TO2 |
                                          (config->alarmfromirmc ? 0 : VOPTION_REMOVEALARM) |
                                          VOPTION_CONVERTUTC, config->charset );
      */					  
      converted_event = strdup(data);
      event_size = strlen(data);
    } else {
      data = NULL;
      event_size = 0;
    }

    if (type == 'H')
      osync_change_set_changetype(change, CHANGE_DELETED);
    else if (type == 'M' || event_size == 0) {
      osync_change_set_data(change, converted_event, event_size, TRUE);
      osync_change_set_changetype(change, CHANGE_MODIFIED);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Creates the addressbook specific changeinfo for slow- and fastsync
 */
void create_addressbook_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %s, %s, %i)", __func__, sync_type, ctx, data, luid, type);			

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
//  irmc_config *config = &(env->config);

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
        int vcard_size = vcard_end - vcard_start+1;
        vcard = g_malloc(vcard_size);
        memcpy(vcard, vcard_start, vcard_end - vcard_start);
        vcard[vcard_end - vcard_start] = 0;

        OSyncChange *change = osync_change_new();
        osync_change_set_member(change, env->member);
        g_assert(change);

        osync_change_set_objformat_string(change, "vcard21");

        vcard_start = strstr(vcard, "X-IRMC-LUID:");
        if (vcard_start) {
          char luid[256];
          if (sscanf(vcard_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }
	vcard_size = strlen(vcard);
        osync_change_set_data(change, vcard, vcard_size, TRUE);
        osync_change_set_changetype(change, CHANGE_ADDED);
        osync_context_report_change(ctx, change);
      }

      vcard_start = vcard_end;
    } while(vcard_start);
  } else {
    OSyncChange *change = osync_change_new();
    osync_change_set_member(change, env->member);
    g_assert(change);

    osync_change_set_objformat_string(change, "vcard21");
    osync_change_set_uid(change, g_strdup(luid));

    int vcard_size = strlen(data);
    if (vcard_size > 0) {
      vcard_size = strlen(data);
    } else {
      vcard_size = 0;
    }

    if (type == 'H')
      osync_change_set_changetype(change, CHANGE_DELETED);
    else if (type == 'M' || vcard_size == 0) {
      osync_change_set_changetype(change, CHANGE_MODIFIED);
      osync_change_set_data(change, data, vcard_size, TRUE);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Creates the notebook specific changeinfo for slow- and fastsync
 */
void create_notebook_changeinfo(int sync_type, OSyncContext *ctx, char *data, char *luid, int type)
{
  osync_trace(TRACE_ENTRY, "%s(%i, %p, %s, %s, %i)", __func__, sync_type, ctx, data, luid, type);			
  char *converted_vnote = NULL;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

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

        OSyncChange *change = osync_change_new();
        osync_change_set_member(change, env->member);
        g_assert(change);

        osync_change_set_objformat_string(change, "vnote11");

        vnote_start = strstr(vnote, "X-IRMC-LUID:");
        if (vnote_start) {
          char luid[256];
          if (sscanf(vnote_start, "X-IRMC-LUID:%256s", luid)) {
            osync_change_set_uid(change, g_strdup(luid));
          }
        }

        converted_vnote = sync_vtype_convert(vnote, 0 | (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                             VOPTION_FIXTELOTHER, config->charset);
        vnote_size = strlen(converted_vnote);
        osync_change_set_data(change, converted_vnote, vnote_size, TRUE);
        osync_change_set_changetype(change, CHANGE_ADDED);
        osync_context_report_change(ctx, change);
      }

      vnote_start = vnote_end;
    } while(vnote_start);
  } else {
    OSyncChange *change = osync_change_new();
    osync_change_set_member(change, env->member);
    g_assert(change);

    osync_change_set_objformat_string(change, "vnote11");
    osync_change_set_uid(change, g_strdup(luid));

    int vnote_size = strlen(data);
    if (vnote_size > 0) {
      converted_vnote = sync_vtype_convert(data, 0 | (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                           VOPTION_FIXTELOTHER, config->charset);
      vnote_size = strlen(converted_vnote);
    } else {
      converted_vnote = NULL;
      vnote_size = 0;
    }

    if (type == 'H')
      osync_change_set_changetype(change, CHANGE_DELETED);
    else if (type == 'M' || vnote_size == 0) {
      osync_change_set_changetype(change, CHANGE_MODIFIED);
      osync_change_set_data(change, converted_vnote, vnote_size, TRUE);
    }

    osync_context_report_change(ctx, change);
  }
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Generic method to write back changeinfo to the device.
 */
osync_bool irmcGenericCommitChange(OSyncContext *ctx, data_type_information *info, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, ctx, info, change);			
  char name[256];
  char *data = NULL;
  char *converted_data = NULL;
  int data_size = 0;
  char rsp_buffer[256];
  int rsp_buffer_size = 256;
  char param_buffer[256];
  char *param_buffer_pos = param_buffer;
  char new_luid[256];
  OSyncError *error = NULL;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
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

  if (osync_change_get_changetype(change) != CHANGE_ADDED) {
    char *uid = (char *) osync_change_get_uid(change);
    if (uid) {
      safe_strcat(name, uid, 256);
    }
  }
  safe_strcat(name, ".", 256);
  safe_strcat(name, info->path_extension, 256);

  data = osync_change_get_data(change);

  // convert the data depending on the object type.
  if (data) {
    if (strcmp(info->identifier, "event") == 0) {
      converted_data = sync_vtype_convert(data, VOPTION_ADDUTF8CHARSET | 0 |
                                          (config->fixdst ? VOPTION_FIXDSTTOCLIENT : 0) |
                                          VOPTION_CALENDAR2TO1 | (config->alarmtoirmc ? 0 : VOPTION_REMOVEALARM) |
                                          (config->convertade ? VOPTION_CONVERTALLDAYEVENT : 0), NULL);
    } else if (strcmp(info->identifier, "contact") == 0) {
      converted_data = sync_vtype_convert(data, VOPTION_ADDUTF8CHARSET, NULL);
    } else if (strcmp(info->identifier, "note") == 0) {
      converted_data = sync_vtype_convert(data, VOPTION_ADDUTF8CHARSET, NULL);
    }

    data_size = strlen(converted_data);
  } else {
    data_size = 0;
  }
  converted_data = strdup(data);
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

    case CHANGE_DELETED:
      // modify obex header for hard delete request
      param_buffer_pos[0] = IRSYNC_APP_HARDDELETE;
      param_buffer_pos[1] = 0;
      param_buffer_pos += 2;

      // send the delete request
      if (!irmc_obex_put(config->obexhandle, name, 0, data_size ? converted_data : NULL, data_size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        g_free(converted_data);
        osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s FALSE: %s", __func__, osync_error_print(&error));
        return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s delete request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      g_free(converted_data);
      break;

    case CHANGE_ADDED:
      // send the add request
      if (!irmc_obex_put(config->obexhandle, name, 0, data_size ? converted_data : NULL, data_size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        g_free(converted_data);
        osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s FALSE: %s", __func__, osync_error_print(&error));
	return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the new luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s added request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      // set the returned luid for this change
      osync_change_set_uid(change, new_luid);

      g_free(converted_data);
      break;

    case CHANGE_MODIFIED:
      // send the modify request
      if (!irmc_obex_put(config->obexhandle, name, 0, data_size ? converted_data : NULL, data_size,
                         rsp_buffer, &rsp_buffer_size, param_buffer, param_buffer_pos - param_buffer, &error)) {
        g_free(converted_data);
        osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: FALSE: %s", __func__, osync_error_print(&error));
        return FALSE;
      }

      rsp_buffer[rsp_buffer_size] = '\0';

      // extract the luid and new change counter from the reply header
      parse_header_params(rsp_buffer, rsp_buffer_size, new_luid, sizeof(new_luid), info->change_counter);

      osync_trace(TRACE_INTERNAL, "%s modified request: resp=%s new_luid=%s cc=%d\n", info->identifier, rsp_buffer, new_luid, *(info->change_counter) );

      g_free(converted_data);
      break;

    default:
      osync_debug("IRMC-SYNC", 0, "Unknown change type");
  }

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
}

/**
 * Commits the calendar specific changeinfo to the device.
 */
static osync_bool irmcCalendarCommitChange(OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
  data_type_information info;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "calendar");
  strcpy(info.identifier, "event");
  strcpy(info.path_identifier, "cal");
  strcpy(info.path_extension, "vcs");
  info.change_counter = &(config->calendar_changecounter);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return irmcGenericCommitChange(ctx, &info, change);
}

/**
 * Commits the addressbook specific changeinfo to the device.
 */
static osync_bool irmcContactCommitChange(OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);			
  data_type_information info;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "addressbook");
  strcpy(info.identifier, "contact");
  strcpy(info.path_identifier, "pb");
  strcpy(info.path_extension, "vcf");
  info.change_counter = &(config->addressbook_changecounter);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return irmcGenericCommitChange(ctx, &info, change);
}

/**
 * Commits the notebook specific changeinfo to the device.
 */
static osync_bool irmcNoteCommitChange(OSyncContext *ctx, OSyncChange *change)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);			
  data_type_information info;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  memset(&info, 0, sizeof(info));
  strcpy(info.name, "notebook");
  strcpy(info.identifier, "note");
  strcpy(info.path_identifier, "nt");
  strcpy(info.path_extension, "vnt");
  info.change_counter = &(config->notebook_changecounter);

  osync_trace(TRACE_EXIT, "%s", __func__);
  return irmcGenericCommitChange(ctx, &info, change);
}

/**
 * This method is called when the sync was successfull.
 */
static void irmcSyncDone(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);			
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  // save synchronization anchors
  save_sync_anchors( env->member, &( env->config ) );

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * This method is called to disconnect from the device.
 */
static void irmcDisconnect(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  // disconnect from the device
  irmc_disconnect(&(env->config));

  osync_context_report_success(ctx);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * This method frees the member specific environment.
 */
static void irmcFinalize(void *data)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

  irmc_environment *env = (irmc_environment *)data;
  g_free(env);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * Returns all information about this plugin.
 */
void get_info(OSyncEnv *env)
{
  OSyncPluginInfo *info = osync_plugin_new_info(env);

  info->name = "irmc-sync";
  info->longname = "IrMC Mobile Device";
  info->description = "Connects to IrMC compliant mobile devices,\nsuch as the SonyEricsson T39/T68/T610 or Siemens S55";
  info->version = 1;

  info->timeouts.disconnect_timeout = 600;
  info->timeouts.get_changeinfo_timeout = 600;
  info->timeouts.get_data_timeout = 600;
  info->timeouts.read_change_timeout = 600;

  info->functions.initialize = irmcInitialize;
  info->functions.connect = irmcConnect;
  info->functions.sync_done = irmcSyncDone;
  info->functions.disconnect = irmcDisconnect;
  info->functions.finalize = irmcFinalize;
  info->functions.get_changeinfo = irmcGetChangeinfo;

  osync_plugin_accept_objtype(info, "contact");
  osync_plugin_accept_objformat(info, "contact", "vcard21", NULL);
  osync_plugin_set_commit_objformat(info, "contact", "vcard21", irmcContactCommitChange);

  osync_plugin_accept_objtype(info, "event");
  osync_plugin_accept_objformat(info, "event", "vevent10", NULL);
  osync_plugin_set_commit_objformat(info, "event", "vevent10", irmcCalendarCommitChange);

  osync_plugin_accept_objtype(info, "todo");
  osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
  osync_plugin_set_commit_objformat(info, "todo", "vtodo20", irmcCalendarCommitChange);

  osync_plugin_accept_objtype(info, "note");
  osync_plugin_accept_objformat(info, "note", "vnote11", NULL);
  osync_plugin_set_commit_objformat(info, "note", "vnote11", irmcNoteCommitChange);
}
