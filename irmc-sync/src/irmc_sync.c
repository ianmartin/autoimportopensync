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

#define SYNC_OBJECT_TYPE_CALENDAR 0
#define SYNC_OBJECT_TYPE_TODO 1

#define IRSYNC_APP_MAXEXPCOUNT 0x11
#define IRSYNC_APP_HARDDELETE 0x12
#define IRSYNC_APP_LUID 0x1
#define IRSYNC_APP_CHANGECOUNT 0x2
#define IRSYNC_APP_TIMESTAMP 0x3

void safe_strcat(char *s1, const char *s2, int len) {
  s1[len-1] = 0;
  memcpy(s1+strlen(s1), s2,
   (strlen(s2)+1+strlen(s1)>len)?(len-strlen(s1)-1):strlen(s2)+1);
}

void str_replace(char *in, char *out, int outbuflen, char *replfrom, char *replto) {
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

// Unique function name so not other plugin is called accidentally.
void irmc_disconnect(irmc_config *config) {
  if (config->obexhandle) {
    OSyncError *error = NULL;
    irmc_obex_disconnect(config->obexhandle, &error);
    if (error)
      osync_error_free(&error);

    irmc_obex_cleanup(config->obexhandle);
  }

  config->obexhandle = 0;
}

// Return the serial number of an unconnected device
// The string must be freed with g_free()
char* sync_connect_get_serial(irmc_config *config) {
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


  doc = xmlParseMemory(data, size);

  if (!doc) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse settings");
    goto error;
  }

  cur = xmlDocGetRootElement(doc);

  if (!cur) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get root element of the settings");
    goto error_free_doc;
  }

  if (xmlStrcmp(cur->name, "config")) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Config valid is not valid");
    goto error_free_doc;
  }

  cur = cur->xmlChildrenNode;

  while (cur != NULL) {
    char *str = xmlNodeGetContent(cur);
    if (str) {
      if (!xmlStrcmp(cur->name, (const xmlChar *)"connectmedium")) {
        if (!strcmp(str, "bluetooth"))
          config->connectmedium = MEDIUM_BLUETOOTH;
        else if (!strcmp(str, "ir"))
          config->connectmedium = MEDIUM_IR;
        else if (!strcmp(str, "cable"))
          config->connectmedium = MEDIUM_CABLE;
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"btunit")) {
        baswap(&(config->btunit.bdaddr), strtoba(str));
      } else if (!xmlStrcmp(cur->name, (const xmlChar *)"btchannel")) {
        config->btchannel = atoi(str);
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

static void *irmcInitialize(OSyncMember *member, OSyncError **error)
{
  char *configdata;
  int configsize;

  //You need to specify the <some name>_environment somewhere with
  //all the members you need
  irmc_environment *env = malloc(sizeof(irmc_environment));
  assert(env != NULL);
  memset(env, 0, sizeof(irmc_environment));

  //now you can get the config file for this plugin
  if (!osync_member_get_config(member, &configdata, &configsize, error)) {
    osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
    free(env);
    return NULL;
  }

  if (!parse_settings( &(env->config), configdata, configsize, error)) {
    osync_error_update(error, "Unable to parse config data: %s", osync_error_print(error));
    free(env);
    return NULL;
  }

  //Process the configdata here and set the options on your environment
  free(configdata);
  env->member = member;

  //Now your return your struct.
  return (void *)env;
}

static void irmcConnect(OSyncContext *ctx)
{
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  env->config.obexhandle = irmc_obex_client(&(env->config));

  OSyncError *error = NULL;
  if (!irmc_obex_connect(env->config.obexhandle, env->config.donttellsync ? NULL : "IRMC-SYNC", &error)) {
    irmc_disconnect(&(env->config));
    osync_context_report_osyncerror(ctx, &error);
  } else
    osync_context_report_success(ctx);
/*
  //you can also use the anchor system to detect a device reset
  //or some parameter change here. Check the docs to see how it works
  char *lanchor = NULL;
  //Now you get the last stored anchor from the device
  if (!osync_anchor_compare(env->member, "lanchor", lanchor))
    osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
*/
}

static void irmcGetChangeinfo(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
  OSyncError *error = 0;

  if (!get_calendar_changeinfo(ctx, &error))
    goto error;

  if (!get_addressbook_changeinfo(ctx, &error))
    goto error;

  osync_context_report_success(ctx);
  osync_trace(TRACE_EXIT, "%s", __func__);
  return;

error:
  osync_context_report_osyncerror(ctx, &error);
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
}

#define DATABUFSIZE (65536*2)

gboolean get_calendar_changeinfo(OSyncContext *ctx, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);

  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int foo;
  int slowsync = 0;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  data = g_malloc(DATABUFSIZE);
  datap = data;

  len = DATABUFSIZE;
  filename = g_strdup_printf("telecom/cal/luid/%d.log", config->calchangecounter);

  if (!irmc_obex_get(config->obexhandle, filename, data, &len, error)) {
    g_free(filename);
    g_free(data);
    goto error;
  }

  g_free(filename);
  data[len] = 0;
  sscanf(datap, "SN:%256s\r\n", serial);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return 1;
  }

  datap+=2;
  sscanf(datap, "DID:%256s\r\n", did);
  if (!config->calDID || strcmp(config->calDID, did)) {
    // This is a new database
    if (config->calDID)
      g_free(config->calDID);
    config->calDID = g_strdup(did);
    slowsync = 1;
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "Total-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "Maximum-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  while(datap) {
    char type;
    int cc;
    char luid[256];
    datap+=2;
    if (sscanf(datap, "%c:%d::%256[^\r\n]", &type, &cc, luid) >= 3) {
      char *vcal = g_malloc(10240);
      char *converted_vcal = NULL;
      int vcal_size = 10240;

      filename = g_strdup_printf("telecom/cal/luid/%s.vcs", luid);
      if (!irmc_obex_get(config->obexhandle, filename, vcal, &vcal_size, error)){
        g_free(data);
        g_free(filename);
        g_free(vcal);
        goto error;
      }
      g_free(filename);

      // Add only if we handle this type of data
      OSyncChange *change = osync_change_new();
      osync_change_set_member(change, env->member);
      g_assert(change);

      if (vcal_size > 0) {
        if (!strstr(vcal, "BEGIN:VEVENT")) {
          if (strstr(vcal, "BEGIN:VTODO"))
            osync_change_set_objformat_string(change, "vtodo20");
        } else {
          osync_change_set_objformat_string(change, "vevent20");
        }
      }

      osync_change_set_uid(change, g_strdup(luid));

      converted_vcal = sync_vtype_convert(vcal, 0 | (config->fixdst ? VOPTION_FIXDSTFROMCLIENT : 0) |
                                                    (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                                    VOPTION_CALENDAR1TO2 |
                                                    (config->alarmfromirmc ? 0 : VOPTION_REMOVEALARM) |
                                                    VOPTION_CONVERTUTC, config->charset );
      vcal_size = strlen(converted_vcal);

      if (type == 'H')
        osync_change_set_changetype(change, CHANGE_DELETED);
      if (type == 'M' || vcal_size == 0) {
        osync_change_set_data(change, converted_vcal, vcal_size, TRUE);
        osync_change_set_changetype(change, CHANGE_MODIFIED);
      }

      osync_context_report_change(ctx, change);
    } else {
      if (datap[0] == '*')
        slowsync = 1;
    }
    datap = strstr(datap, "\r\n");
  }

  len = DATABUFSIZE;
  if (!irmc_obex_get(config->obexhandle, "telecom/cal/luid/cc.log", data, &len, error)) {
    g_free(data);
    goto error;
  } else {
    data[len] = 0;
    sscanf(data, "%d", &config->calchangecounter);
  }

  if (slowsync) {
    len = DATABUFSIZE;
    if (config->donttellsync) {
      // Reconnect with "IRMC-SYNC" to get X-IRMC-LUID
      irmc_obex_disconnect(config->obexhandle, error);
      sleep(1);
      if (irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
        sleep(2);
        if (!irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
          g_free(data);
          goto error;
        }
      }
    }
    if (!irmc_obex_get(config->obexhandle, "telecom/cal.vcs", data, &len, error))
      len = 0; // Continue anyway; Siemens models will fail this get if calendar is empty

    {
      char *event_end = NULL;
      char *event_start = data, *todo_start;
      char *event = NULL;
      char *converted_event = NULL;
      int objtype;

      data[len]=0;
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

          converted_event = sync_vtype_convert(event, 0 | (config->fixdst ? VOPTION_FIXDSTFROMCLIENT : 0) |
                                                          (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                                          VOPTION_CALENDAR1TO2 |
                                                          (config->alarmfromirmc ? 0 : VOPTION_REMOVEALARM) |
                                                          VOPTION_CONVERTUTC, config->charset);
          event_size = strlen(converted_event);
          osync_change_set_data(change, converted_event, event_size, TRUE);
          osync_change_set_changetype(change, CHANGE_ADDED);
          osync_context_report_change(ctx, change);
        }

        event_start = event_end;
      } while(event_start);
    }
  }
  g_free(data);
  return TRUE;

error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

gboolean get_addressbook_changeinfo(OSyncContext *ctx, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);

  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int foo;
  int slowsync = 0;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  data = g_malloc(DATABUFSIZE);
  datap = data;

  filename = g_strdup_printf("telecom/pb/luid/%d.log", config->pbchangecounter);

  if (!irmc_obex_get(config->obexhandle, filename, data, &len, error)) {
    g_free(filename);
    g_free(data);
    goto error;
  }

  g_free(filename);
  data[len] = 0;
  sscanf(datap, "SN:%256s\r\n", serial);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "DID:%256s\r\n", did);
  if (!config->pbDID || strcmp(config->pbDID, did)) {
    // This is a new database
    if (config->pbDID)
      g_free(config->pbDID);
    config->pbDID = g_strdup(did);
    slowsync = 1;
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "Total-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return TRUE;
  }
  datap+=2;
  sscanf(datap, "Maximum-Records:%d\r\n", &foo);
  datap = strstr(datap, "\r\n");
  while(datap) {
    char type;
    int cc;
    char luid[256];
    datap+=2;
    if (sscanf(datap, "%c:%d::%256[^\r\n]", &type, &cc, luid) >= 3) {
      char *vcard = g_malloc(65536);
      int vcard_size = 65536;
      char *converted_vcard = NULL;

      memset(vcard, 0, vcard_size);

      filename = g_strdup_printf("telecom/pb/luid/%s.vcf", luid);
      vcard_size = 10240;
      if (!irmc_obex_get(config->obexhandle, filename, vcard, &vcard_size, error)) {
        g_free(data);
        g_free(filename);
        g_free(vcard);
        goto error;
      }
      g_free(filename);

      OSyncChange *change = osync_change_new();
      osync_change_set_member(change, env->member);
      g_assert(change);

      osync_change_set_objformat_string(change, "vcard21");
      osync_change_set_uid(change, g_strdup(luid));

      if (type == 'H')
        osync_change_set_changetype(change, CHANGE_DELETED);
      if (type == 'M' || vcard_size == 0) {
        osync_change_set_changetype(change, CHANGE_MODIFIED);
        converted_vcard = sync_vtype_convert(vcard, 0 | (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                             VOPTION_FIXTELOTHER, config->charset);
        vcard_size = strlen(converted_vcard);
        osync_change_set_data(change, converted_vcard, vcard_size, TRUE);
      }

      osync_context_report_change(ctx, change);
    } else {
      if (datap[0] == '*')
        slowsync = 1;
    }
    datap = strstr(datap, "\r\n");
  }

  len = DATABUFSIZE;
  if (!irmc_obex_get(config->obexhandle, "telecom/pb/luid/cc.log", data, &len, error)) {
    goto error;
  } else {
    data[len] = 0;
    sscanf(data, "%d", &config->pbchangecounter);
  }

  if (slowsync) {
    len = DATABUFSIZE;
    if (config->donttellsync) {
      // Reconnect with "IRMC-SYNC" to get X-IRMC-LUID
      irmc_obex_disconnect(config->obexhandle, error);
      sleep(1);
      if (irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
        sleep(2);
        if (!irmc_obex_connect(config->obexhandle, "IRMC-SYNC", error)) {
          g_free(data);
          goto error;
        }
      }
    }
    if (!irmc_obex_get(config->obexhandle, "telecom/pb.vcf", data, &len, error))
      len = 0; // Continue anyway; Siemens models will fail this get if calendar is empty

    {
      char *vcard_end = NULL;
      char *vcard_start = data;
      char *vcard = NULL;
      char *converted_vcard = NULL;

      data[len]=0;
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

          converted_vcard = sync_vtype_convert(vcard, 0 | (config->translatecharset ? VOPTION_FIXCHARSET : 0) |
                                               VOPTION_FIXTELOTHER, config->charset);
          vcard_size = strlen(converted_vcard);
          osync_change_set_data(change, converted_vcard, vcard_size, TRUE);
          osync_change_set_changetype(change, CHANGE_ADDED);
          osync_context_report_change(ctx, change);
        }

        vcard_start = vcard_end;
      } while(vcard_start);
    }
  }
  g_free(data);
  return TRUE;

error:
  osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

static osync_bool irmcCalendarCommitChange(OSyncContext *ctx, OSyncChange *change)
{

  int res = 0;
  char name[256];
  char *vcal = NULL;
  char *converted_vcal = NULL;
  int vcal_size=0;
  char rspbuf[256];
  int rspbuflen=256;
  char apparambuf[256];
  char *apparam = apparambuf;
  OSyncError *error = NULL;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  strcpy(name, "telecom/cal/luid/");
  char *uid = osync_change_get_uid(change);
  if (uid) {
    safe_strcat(name, uid, 256);
  }
  safe_strcat(name, ".vcs", 256);

  vcal = osync_change_get_data(change);
  if (vcal) {
    converted_vcal = sync_vtype_convert(vcal, VOPTION_ADDUTF8CHARSET | 0 |
                                        (config->fixdst ? VOPTION_FIXDSTTOCLIENT : 0) |
                                        VOPTION_CALENDAR2TO1 | (config->alarmtoirmc ? 0 : VOPTION_REMOVEALARM) |
                                        (config->convertade ? VOPTION_CONVERTALLDAYEVENT : 0), NULL);
    vcal_size = strlen(converted_vcal);
  } else {
    vcal_size = 0;
  }

  config->calchangecounter++;
  sprintf(apparam+2, "%d", config->calchangecounter);
  apparam[0] = IRSYNC_APP_MAXEXPCOUNT;
  apparam[1] = strlen(apparam+2);
  apparam += strlen(apparam+2)+2;

  switch (osync_change_get_changetype(change)) {
    case CHANGE_DELETED:
      apparam[0] = IRSYNC_APP_HARDDELETE;
      apparam[1] = 0;
      apparam+=2;

      if (!irmc_obex_put(config->obexhandle, name, 0, vcal_size ? converted_vcal : NULL, vcal_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, &error)) {
        g_free(converted_vcal);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcal);
      break;
    case CHANGE_ADDED:
/*
      if ( bodylen > 0 ) {
        char *tmp;
        char *dtend = NULL;

        dtend = sync_get_key_data(event, "DTEND");
        if (config->dontacceptold && dtend) {
          time_t tend, now;
          tend = sync_dt_to_timet(dtend);
          now = time(NULL);
          if (now-tend > config->maximumage*3600*24) {
            g_free(dtend);
            sync_set_requestmsg(SYNC_MSG_USERDEFERRED, config->sync_pair);
            return;
          }
        }
        if (dtend)
          g_free(dtend);
      }
*/

      if (!irmc_obex_put(config->obexhandle, name, 0, vcal_size ? converted_vcal : NULL, vcal_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, &error)) {
        g_free(converted_vcal);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcal);
      break;
    case CHANGE_MODIFIED:
      if (!irmc_obex_put(config->obexhandle, name, 0, vcal_size ? converted_vcal : NULL, vcal_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, &error)) {
        g_free(converted_vcal);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcal);

      apparam = rspbuf;
      while (apparam < rspbuf+rspbuflen) {
        if (apparam[0] == IRSYNC_APP_CHANGECOUNT) {
            char tmpbuf[16];
            memcpy(tmpbuf, apparam+2, apparam[1] > 15 ? 15 : apparam[1]);
            tmpbuf[(int) apparam[1]] = 0;
            sscanf(tmpbuf, "%d", &config->calchangecounter);
        }

        apparam+=apparam[1]+2;
      }
      break;
    default:
      osync_debug("IRMC-SYNC", 0, "Unknown change type");
  }

  //Answer the call
  osync_context_report_success(ctx);
  return TRUE;
}

static osync_bool irmcContactCommitChange(OSyncContext *ctx, OSyncChange *change)
{
  int res = 0;
  char name[256];
  int vcard_size=0;
  char rspbuf[256];
  int rspbuflen=256;
  char apparambuf[256];
  char *apparam = apparambuf;
  char *vcard = NULL;
  char *converted_vcard = NULL;
  OSyncError *error = NULL;

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_config *config = &(env->config);

  char *uid = osync_change_get_uid(change);
  vcard = osync_change_get_data(change);

  strcpy(name, "telecom/pb/luid/");
  if (uid) {
    safe_strcat(name, uid, 256);
  }
  safe_strcat(name, ".vcf", 256);

  if (vcard) {
    converted_vcard = sync_vtype_convert(vcard, VOPTION_ADDUTF8CHARSET, NULL);
    vcard_size = strlen(converted_vcard);
  } else
    vcard_size = 0;

  config->pbchangecounter++;
  sprintf(apparam+2, "%d", config->pbchangecounter);
  apparam[0] = IRSYNC_APP_MAXEXPCOUNT;
  apparam[1] = strlen(apparam+2);
  apparam += strlen(apparam+2)+2;

  switch (osync_change_get_changetype(change)) {
    case CHANGE_DELETED:
      apparam[0] = IRSYNC_APP_HARDDELETE;
      apparam[1] = 0;
      apparam+=2;

      if (!irmc_obex_put(config->obexhandle, name, 0, converted_vcard, vcard_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, error)) {
        g_free(converted_vcard);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcard);
      break;
    case CHANGE_ADDED:
/*
      if (data) {
        char *tel = NULL;
        tel = sync_get_key_data(data, "TEL");
        if (config->onlyphonenumbers && !tel) {
          // If no phone number and user want only contacts with phone numbers
          return;
        }
      }
*/

      if (!irmc_obex_put(config->obexhandle, name, 0, converted_vcard, vcard_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, error)) {
        g_free(converted_vcard);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcard);
      break;
    case CHANGE_MODIFIED:
      if (!irmc_obex_put(config->obexhandle, name, 0, converted_vcard, vcard_size,
                         rspbuf, &rspbuflen, apparambuf, apparam - apparambuf, error)) {
        g_free(converted_vcard);
        osync_context_report_osyncerror(ctx, &error);
        return FALSE;
      }
      g_free(converted_vcard);

      apparam = rspbuf;
      while (apparam < rspbuf+rspbuflen) {
        if (apparam[0] == IRSYNC_APP_CHANGECOUNT) {
          char tmpbuf[16];
          memcpy(tmpbuf, apparam+2, apparam[1] > 15 ? 15 : apparam[1]);
          tmpbuf[(int) apparam[1]] = 0;
          sscanf(tmpbuf, "%d", &config->pbchangecounter);
        }

        apparam+=apparam[1]+2;
      }
      break;
    default:
      osync_debug("IRMC-SYNC", 0, "Unknown change type");
  }
  //Answer the call
  osync_context_report_success(ctx);
  return TRUE;
}

static void irmcSyncDone(OSyncContext *ctx)
{
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  /*
   * This function will only be called if the sync was successfull
   */

  //If we use anchors we have to update it now.
  char *lanchor = NULL;
  //Now you get/calculate the current anchor of the device
  osync_anchor_update(env->member, "lanchor", lanchor);

  //Answer the call
  osync_context_report_success(ctx);
}

static void irmcDisconnect(OSyncContext *ctx)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_disconnect(&(env->config));

  osync_context_report_success(ctx);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

static void irmcFinalize(void *data)
{
  osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

  irmc_environment *env = (irmc_environment *)data;
  g_free(env);

  osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
  OSyncPluginInfo *info = osync_plugin_new_info(env);

  info->name = "irmc-sync";
  info->longname = "IrMC Mobile Device";
  info->description = "Connects to IrMC compliant mobile devices,\nsuch as the SonyEricsson T39/T68/T610 or Siemens S55";
  info->version = 1;

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
  osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
  osync_plugin_set_commit_objformat(info, "event", "vevent20", irmcCalendarCommitChange);

  osync_plugin_accept_objtype(info, "todo");
  osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
  osync_plugin_set_commit_objformat(info, "todo", "vtodo20", irmcCalendarCommitChange);
}
