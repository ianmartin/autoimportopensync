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
#include "plugin.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    irmc_obex_disconnect(config->obexhandle);
    irmc_obex_cleanup(config->obexhandle);
  }

  config->obexhandle = 0;
}

// Return the serial number of an unconnected device
// The string must be freed with g_free()
char* sync_connect_get_serial(irmc_config *config) {
  char *sn = NULL;
  config->obexhandle = irmc_obex_client(config);
  
  if (irmc_obex_connect(config->obexhandle, 
			config->donttellsync ? NULL : "IRMC-SYNC") >= 0) {
    sn = irmc_obex_get_serial(config->obexhandle);
  }
  irmc_obex_disconnect(config->obexhandle);
  irmc_obex_cleanup(config->obexhandle);
  config->obexhandle = NULL;
  return(sn);
}


osync_bool parse_settings(irmc_config *config, const char *config, unsigned int size, OSyncError **error)
{
  osync_trace(TRACE_ENTRY, "%s(%p, %s, %p)", __func__, env, config, error);
  xmlDoc *doc = NULL;
  xmlNode *cur = NULL;

  // set defaults
  config->connectmedium = MEDIUM_BLUETOOTH;
  memset(config->btunit.address, 0, sizeof(config->btunit.address));
  config->btunit.channel = 0;
  memset(config->btunit.name, 0, sizeof(config->btunit.name));
  config->btchannel = 0;

  memset(config->cabledev, 0, sizeof(config->cabledev));
  config->cabletype = UNKNOWN;

  memset(config->irunit.name, 0, sizeof(config->irunit.name));
  memset(config->irunit.serial, 0, sizeof(config->irunit.serial));

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


  doc = xmlParseMemory(config, size);

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

static void *initialize(OSyncMember *member, OSyncError **error)
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

static void connect(OSyncContext *ctx)
{
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  env->config->obexhandle = irmc_obex_client(env->config);

  int ret;
  if ((ret = irmc_obex_connect(env->config->obexhandle, env->config->donttellsync ? NULL : "IRMC-SYNC")) < 0) {
    irmc_disconnect(env->config);
    osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to connect to mobile device");
  } else
    osync_context_report_success(ctx);

  //you can also use the anchor system to detect a device reset
  //or some parameter change here. Check the docs to see how it works
  char *lanchor = NULL;
  //Now you get the last stored anchor from the device
  if (!osync_anchor_compare(env->member, "lanchor", lanchor))
    osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
}

static void get_changeinfo(OSyncContext *ctx)
{
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  //If you use opensync hashtables you can detect if you need
  //to do a slow-sync and set this on the hastable directly
  //otherwise you have to make 2 function like "get_changes" and
  //"get_all" and decide which to use using
  //osync_member_get_slow_sync
  if (osync_member_get_slow_sync(env->member, "<object type>"))
    osync_hashtable_set_slow_sync(env->hashtable, "<object type>");

  /*
   * Now you can get the changes.
   * Loop over all changes you get and do the following:
   */
    char *data = NULL;
    //Now get the data of this change

    //Make the new change to report
    OSyncChange *change = osync_change_new();
    //Set the member
    osync_change_set_member(change, env->member);
    //Now set the uid of the object
    osync_change_set_uid(change, "<some uid>");
    //Set the object format
    osync_change_set_objformat_string(change, "<the format of the object>");
    //Set the hash of the object (optional, only required if you use hashtabled)
    osync_change_set_hash(change, "the calculated hash of the object");
    //Now you can set the data for the object
    //Set the last argument to FALSE if the real data
    //should be queried later in a "get_data" function

    osync_change_set_data(change, data, sizeof(data), TRUE);

    //If you use hashtables use these functions:
    if (osync_hashtable_detect_change(env->hashtable, change)) {
      osync_context_report_change(ctx, change);
      osync_hashtable_update_hash(env->hashtable, change);
    }
    //otherwise just report the change via
    //osync_context_report_change(ctx, change);

  //When you are done looping and if you are using hashtables
  osync_hashtable_report_deleted(env->hashtable, ctx, "data");

  //Now we need to answer the call
  osync_context_report_success(ctx);
}

static osync_bool commit_change(OSyncContext *ctx, OSyncChange *change)
{
  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);

  /*
   * Here you have to add, modify or delete a object
   *
   */
  switch (osync_change_get_changetype(change)) {
    case CHANGE_DELETED:
      //Delete the change
      //Dont forget to answer the call on error
      break;
    case CHANGE_ADDED:
      //Add the change
      //Dont forget to answer the call on error
      //If you are using hashtables you have to calculate the hash here:
      osync_change_set_hash(change, "new hash");
      break;
    case CHANGE_MODIFIED:
      //Modify the change
      //Dont forget to answer the call on error
      //If you are using hashtables you have to calculate the new hash here:
      osync_change_set_hash(change, "new hash");
      break;
    default:
      osync_debug("FILE-SYNC", 0, "Unknown change type");
  }
  //Answer the call
  osync_context_report_success(ctx);
  //if you use hashtable, update the hash now.
  osync_hashtable_update_hash(env->hashtable, change);
  return TRUE;
}

static void sync_done(OSyncContext *ctx)
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

static void disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

  irmc_environment *env = (irmc_environment *)osync_context_get_plugin_data(ctx);
  irmc_disconnect(env->config);

  osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);

  irmc_environment *env = (irmc_environment *)data;
	g_free(env);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

void get_info(OSyncEnv *env)
{
  OSyncPluginInfo *info = osync_plugin_new_info(env);

  info->name = "irmc_sync";
  info->longname = "IrMC Mobile Device";
  info->description = "Connects to IrMC compliant mobile devices,\nsuch as the SonyEricsson T39/T68/T610 or Siemens S55";
  info->version = 1;
  info->is_threadsafe = TRUE;

  info->functions.initialize = initialize;
  info->functions.connect = connect;
  info->functions.sync_done = sync_done;
  info->functions.disconnect = disconnect;
  info->functions.finalize = finalize;
  info->functions.get_changeinfo = get_changeinfo;

  info->timeouts.connect_timeout = 5;

  osync_plugin_accept_objtype(info, "contact");
  osync_plugin_accept_objformat(info, "contact", "vcard21", NULL);
  osync_plugin_set_commit_objformat(info, "contact", "vcard21", contact_commit_change);
  osync_plugin_set_access_objformat(info, "contact", "vcard21", contact_access);

  osync_plugin_accept_objtype(info, "event");
  osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
  osync_plugin_set_commit_objformat(info, "event", "vevent20", event_commit_change);
  osync_plugin_set_access_objformat(info, "event", "vevent20", event_access);

  osync_plugin_accept_objtype(info, "todo");
  osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
  osync_plugin_set_commit_objformat(info, "todo", "vtodo20", todo_commit_change);
  osync_plugin_set_access_objformat(info, "todo", "vtodo20", todo_access);
}
