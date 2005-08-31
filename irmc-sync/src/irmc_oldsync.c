/* 
   MultiSync IrMC Plugin - Synchronize IrMC (mobile) devices
   Copyright (C) 2002-2003 Bo Lincoln <lincoln@lysator.liu.se>

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

/*
 *  $Id: irmc_sync.c,v 1.57 2004/04/06 09:47:31 lincoln Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include "irmc_sync.h"
#include <glib.h>
#include <gmodule.h>
#include <multisync.h>
#include <iconv.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "gui.h"


#define IRMCFILE "irmc"

extern GModule *bluetoothplugin;
gpointer (*plugin_function)();
#define CALL_PLUGIN(mod, name, args) (g_module_symbol(mod,name,(gpointer*)&plugin_function)?(*plugin_function)args:NULL)

extern long timezone;

extern gboolean multisync_debug;

void safe_strcat(char *s1, const char *s2, int len) {
  s1[len-1] = 0;
  memcpy(s1+strlen(s1), s2, 
	 (strlen(s2)+1+strlen(s1)>len)?(len-strlen(s1)-1):strlen(s2)+1);
}

void str_replace(char *in, char *out, int outbuflen, char *replfrom, char *replto){
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

void load_state(irmc_connection *conn) {
  char *filename;
  FILE *f;
  char line[256];

  conn->onlyphonenumbers = TRUE;
  conn->dontacceptold = TRUE;
  conn->maximumage = 7;
  conn->charset = g_strdup("ISO8859-1");
  conn->translatecharset = FALSE;
  conn->alarmfromirmc = TRUE;
  conn->alarmtoirmc = TRUE;
  filename = g_strdup_printf("%s/%s%s", sync_get_datapath(conn->sync_pair),
			     (conn->conntype==CONNECTION_TYPE_LOCAL?"local":"remote"), IRMCFILE);
  if ((f = fopen(filename, "r"))) {
    while (fgets(line, 256, f)) {
      char prop[128], data[256];
      if (sscanf(line, "%128s = %256[^\n]", prop, data) == 2) {
	if (!strcmp(prop, "calchangecounter")) {
	  sscanf(data, "%d", &conn->calchangecounter);
	  dd(printf("Loaded change counter %d\n", conn->calchangecounter));
	}
	if (!strcmp(prop, "pbchangecounter")) {
	  sscanf(data, "%d", &conn->pbchangecounter);
	  dd(printf("Loaded change counter %d\n", conn->pbchangecounter));
	}
	if (!strcmp(prop, "calDID")) {
	  conn->calDID = g_strdup(data);
	}
	if (!strcmp(prop, "pbDID")) {
	  conn->pbDID = g_strdup(data);
	}
	if (!strcmp(prop, "connectmedium")) {
	  if (!strcmp(data, "bluetooth"))
	    conn->connectmedium = MEDIUM_BLUETOOTH;
	  else if (!strcmp(data, "ir"))
	    conn->connectmedium = MEDIUM_IR;
	  else if (!strcmp(data, "cable"))
	    conn->connectmedium = MEDIUM_CABLE;
	}	  
	if (!strcmp(prop, "btunit")) {
	  if (bluetoothplugin) {
	    CALL_PLUGIN(bluetoothplugin, "irmc_strtoba", 
			(&conn->btunit.bdaddr, data));
	  }
	}
	if (!strcmp(prop, "btchannel")) {
	  sscanf(data, "%d", &conn->btchannel);
	}
	if (!strcmp(prop, "irname")) {
	  strncpy(conn->irunit.name, data, 31);
	}
	if (!strcmp(prop, "irserial")) {
	  strncpy(conn->irunit.serial, data, 127);
	}
	if (!strcmp(prop, "cabledev")) {
	  strncpy(conn->cabledev, data, 19);
	}
	if (!strcmp(prop, "cabletype")) {
	  sscanf(data, "%d", &conn->cabletype);
	}
	if (!strcmp(prop, "managedbsize")) {
	  if (!strcmp(data, "yes"))
	    conn->commondata.managedbsize = TRUE;
	  else
	    conn->commondata.managedbsize = FALSE;
	}
	if (!strcmp(prop, "fakerecur")) {
	  if (!strcmp(data, "yes"))
	    conn->commondata.fake_recurring = TRUE;
	  else
	    conn->commondata.fake_recurring = FALSE;
	}
	if (!strcmp(prop, "fixdst")) {
	  if (!strcmp(data, "yes"))
	    conn->fixdst = TRUE;
	  else
	    conn->fixdst = FALSE;
	}
	if (!strcmp(prop, "donttellsync")) {
	  if (!strcmp(data, "yes"))
	    conn->donttellsync = TRUE;
	  else
	    conn->donttellsync = FALSE;
	}
	if (!strcmp(prop, "onlyphonenumbers")) {
	  if (!strcmp(data, "yes"))
	    conn->onlyphonenumbers = TRUE;
	  else
	    conn->onlyphonenumbers = FALSE;
	}
	if (!strcmp(prop, "dontacceptold")) {
	  if (!strcmp(data, "yes"))
	    conn->dontacceptold = TRUE;
	  else
	    conn->dontacceptold = FALSE;
	}
	if (!strcmp(prop, "maximumage")) {
	  sscanf(data, "%d", &conn->maximumage);
	}
	if (!strcmp(prop, "translatecharset")) {
	  if (!strcmp(data, "yes"))
	    conn->translatecharset = TRUE;
	  else
	    conn->translatecharset = FALSE;
	}
	if (!strcmp(prop, "charset")) {
	  if (conn->charset)
	    g_free(conn->charset);
	  conn->charset = g_strdup(data);
	}
	if (!strcmp(prop, "alarmfromirmc")) {
	  if (!strcmp(data, "yes"))
	    conn->alarmfromirmc = TRUE;
	  else
	    conn->alarmfromirmc = FALSE;
	}
	if (!strcmp(prop, "alarmtoirmc")) {
	  if (!strcmp(data, "yes"))
	    conn->alarmtoirmc = TRUE;
	  else
	    conn->alarmtoirmc = FALSE;
	}
	if (!strcmp(prop, "convertade")) {
	  if (!strcmp(data, "yes"))
	    conn->convertade = TRUE;
	  else
	    conn->convertade = FALSE;
	}
      }
    }
    fclose(f);
  }
  g_free(filename);
}

void save_state(irmc_connection *conn) {
  FILE *f;
  char *filename;
  
  filename = g_strdup_printf("%s/%s%s", sync_get_datapath(conn->sync_pair), 
			     (conn->conntype==CONNECTION_TYPE_LOCAL?"local":"remote"), IRMCFILE);
  if ((f = fopen(filename, "w"))) {

    fprintf(f, "calchangecounter = %d\n", conn->calchangecounter);
    fprintf(f, "pbchangecounter = %d\n", conn->pbchangecounter);
    fprintf(f, "connectmedium = %s\n", conn->connectmedium==MEDIUM_BLUETOOTH?"bluetooth":(conn->connectmedium==MEDIUM_IR?"ir":"cable"));
    if (bluetoothplugin)
      fprintf(f, "btunit = %s\n", CALL_PLUGIN(bluetoothplugin, "irmc_batostr",
					      (&conn->btunit.bdaddr)));
    fprintf(f, "btchannel = %d\n", conn->btchannel);
    fprintf(f, "irname = %s\n", conn->irunit.name);
    fprintf(f, "irserial = %s\n", conn->irunit.serial);
    fprintf(f, "cabledev = %s\n", conn->cabledev);
    fprintf(f, "cabletype = %d\n", conn->cabletype);
    if (conn->calDID)
      fprintf(f, "calDID = %s\n", conn->calDID);
    if (conn->pbDID)
      fprintf(f, "pbDID = %s\n", conn->pbDID);
    fprintf(f, "managedbsize = %s\n", conn->commondata.managedbsize?"yes":"no");
    fprintf(f, "fakerecur = %s\n", conn->commondata.fake_recurring?"yes":"no");
    fprintf(f, "fixdst = %s\n", conn->fixdst?"yes":"no");
    fprintf(f, "donttellsync = %s\n", conn->donttellsync?"yes":"no");
    fprintf(f, "onlyphonenumbers = %s\n", conn->onlyphonenumbers?"yes":"no");
    fprintf(f, "dontacceptold = %s\n", conn->dontacceptold?"yes":"no");
    fprintf(f, "maximumage = %d\n", conn->maximumage);
    fprintf(f, "translatecharset = %s\n", conn->translatecharset?"yes":"no");
    fprintf(f, "charset = %s\n", conn->charset);
    fprintf(f, "alarmfromirmc = %s\n", conn->alarmfromirmc?"yes":"no");
    fprintf(f, "alarmtoirmc = %s\n", conn->alarmtoirmc?"yes":"no");
    fprintf(f, "convertade = %s\n", conn->convertade?"yes":"no");
    fclose(f);
  }
  g_free(filename);
}

void free_state(irmc_connection *conn) {
  if (conn->calDID)
    g_free(conn->calDID);
  if (conn->charset)
    g_free(conn->charset);
  g_free(conn);
}





irmc_connection* sync_connect(sync_pair *handle, connection_type type,
			      sync_object_type object_types) {
  irmc_connection* conn;
  int ret;

  conn = g_malloc0(sizeof(irmc_connection));
  g_assert(conn);
  conn->sync_pair = handle;
  conn->conntype = type;
  conn->commondata.object_types = object_types;
  load_state(conn);


  conn->obexhandle = irmc_obex_client(conn);
  
  if ((ret = irmc_obex_connect(conn->obexhandle, conn->donttellsync?NULL:"IRMC-SYNC"))<0) {
    irmc_disconnect(conn);
    sync_set_requestmsg(ret, handle);
    return(NULL);
  }
  sync_set_requestdone(conn->sync_pair);
  return(conn);
}

gboolean sync_test_connection(irmc_connection *conn) {
  char data[10240];
  int len = 10240;
  conn->obexhandle = irmc_obex_client(conn);
  
  if (irmc_obex_connect(conn->obexhandle, 
			conn->donttellsync?NULL:"IRMC-SYNC")) {
    irmc_obex_disconnect(conn->obexhandle);
    irmc_obex_cleanup(conn->obexhandle);
    conn->obexhandle = NULL;
    return(FALSE);
  }
  len = 10240;
  if (irmc_obex_get(conn->obexhandle, "telecom/devinfo.txt", data, &len)) {
    irmc_obex_disconnect(conn->obexhandle);
    irmc_obex_cleanup(conn->obexhandle);
    conn->obexhandle = NULL;
    return(FALSE);
  }
  // Succeeded to connect and fetch data.
  irmc_obex_disconnect(conn->obexhandle);
  irmc_obex_cleanup(conn->obexhandle);
  conn->obexhandle = NULL;
  return(TRUE);
}


// Return the serial number of an unconnected device
// The string must be freed with g_free()
char* sync_connect_get_serial(irmc_connection *conn) {
  char *sn = NULL;
  conn->obexhandle = irmc_obex_client(conn);
  
  if (irmc_obex_connect(conn->obexhandle, 
			conn->donttellsync?NULL:"IRMC-SYNC") >= 0) {
    sn = irmc_obex_get_serial(conn->obexhandle);
  }
  irmc_obex_disconnect(conn->obexhandle);
  irmc_obex_cleanup(conn->obexhandle);
  conn->obexhandle = NULL;
  return(sn);
}

#define DATABUFSIZE (65536*2)

void free_changes(GList *changes) {
  while (changes) {
    GList *change;
    changed_object *obj;

    change = g_list_first(changes);
    if (change->data) {
      obj = change->data;
      if (obj->comp) {
	g_free(obj->comp);
      }
      if (obj->removepriority)
	g_free(obj->removepriority);
      if (obj->uid)
	g_free(obj->uid);
    }
    changes = g_list_remove_link(changes, change);
  }
}


void get_changes(irmc_connection *conn, sync_object_type newdbs) {
  GList *changes = NULL;
  int ret;
  sync_object_type retnewdbs = 0;
  change_info *chinfo;

  if (conn->commondata.object_types & (SYNC_OBJECT_TYPE_CALENDAR |
				       SYNC_OBJECT_TYPE_TODO)) {
    ret = cal_get_changes(conn, newdbs&(SYNC_OBJECT_TYPE_CALENDAR |
					SYNC_OBJECT_TYPE_TODO), &changes);
    if (ret == SYNC_MSG_SLOWSYNC)
      retnewdbs |= (SYNC_OBJECT_TYPE_CALENDAR|SYNC_OBJECT_TYPE_TODO);
    if (ret < 0)
      goto err;
  }
  if (conn->commondata.object_types & SYNC_OBJECT_TYPE_PHONEBOOK) {
    ret = pb_get_changes(conn, newdbs&SYNC_OBJECT_TYPE_PHONEBOOK, &changes);
    if (ret == SYNC_MSG_SLOWSYNC)
      retnewdbs |= SYNC_OBJECT_TYPE_PHONEBOOK;
    if (ret < 0)
      goto err;
  }
  
  chinfo = g_malloc0(sizeof(change_info));
  chinfo->changes = changes;
  chinfo->newdbs = retnewdbs;
  sync_set_requestdata(chinfo, conn->sync_pair);
  return;
  
err:
  if (changes)
    free_changes(changes);     
  sync_set_requestmsg(ret, conn->sync_pair);
}

int cal_get_changes(irmc_connection *conn, int slowsync, GList **sync_changes) {
  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int ret;
  int newdb = 0;
  
  data = g_malloc(DATABUFSIZE);
  datap = data;

  len = DATABUFSIZE;
  filename = g_strdup_printf("telecom/cal/luid/%d.log", conn->calchangecounter);

  if ((ret = irmc_obex_get(conn->obexhandle, filename, data, &len)) < 0) {
    dd(printf("Get log failed.\n"));
    g_free(filename);
    g_free(data);
    return(ret);
  }
  g_free(filename);
  data[len] = 0;
  dd(printf("Change log: \n%s\n", data));
  sscanf(datap, "SN:%256s\r\n", serial);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "DID:%256s\r\n", did);
  if (!conn->calDID || strcmp(conn->calDID, did)) {
    // This is a new database
    if (conn->calDID)
      g_free(conn->calDID);
    conn->calDID = g_strdup(did);
    slowsync = 1;
    newdb = 1;
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "Total-Records:%d\r\n", &conn->commondata.calendarrecords);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "Maximum-Records:%d\r\n", &conn->commondata.maxcalendarrecords);
  datap = strstr(datap, "\r\n");
  while(datap) {
    char type;
    int cc;
    char luid[256];
    datap+=2;
    if (sscanf(datap, "%c:%d::%256[^\r\n]", &type, &cc, luid) >= 3) {
      changed_object* change;
      char *event_end = NULL;
      char *event_start = NULL;
      char event[10240];
      char objdata[10240];
      int objlen = 10240;
      sync_object_type objtype;      

      filename = g_strdup_printf("telecom/cal/luid/%s.vcs", luid);
      objlen = 10240;
      if ((ret=irmc_obex_get(conn->obexhandle, filename, objdata, &objlen))<0){
	objdata[0] = 0;
	objlen = 0;
	//printf("Get object %s failed.\n", luid);
      }
      g_free(filename);
      
      objtype = SYNC_OBJECT_TYPE_CALENDAR;
      if (objlen > 0) {
	if (!strstr(objdata, "BEGIN:VEVENT"))
	  if (strstr(objdata, "BEGIN:VTODO"))
	    objtype = SYNC_OBJECT_TYPE_TODO;
      }
      
      // Add only if we handle this type of data
      if (conn->commondata.object_types & objtype) {
	change = g_malloc0(sizeof(changed_object));
	g_assert(change);
	change->uid = g_strdup(luid);
	if (objlen > 0) {
	  dd(printf("Original data:\n%s\n", objdata));
	  change->comp = sync_vtype_convert(objdata, 0|(conn->fixdst?VOPTION_FIXDSTFROMCLIENT:0)|(conn->translatecharset?VOPTION_FIXCHARSET:0)|VOPTION_CALENDAR1TO2|(conn->alarmfromirmc?0:VOPTION_REMOVEALARM)|VOPTION_CONVERTUTC, conn->charset);
	  change->removepriority = sync_get_key_data(change->comp, "DTEND");
	}
	
	if (type == 'D')
	  change->change_type = SYNC_OBJ_SOFTDELETED;
	if (type == 'H')
	  change->change_type = SYNC_OBJ_HARDDELETED;
	if (type == 'M' || objlen == 0)
	  change->change_type = SYNC_OBJ_MODIFIED;
	change->object_type = objtype;
	*sync_changes = g_list_append(*sync_changes, change);
      }
    } else {
      if (datap[0] == '*')
	slowsync = 1;
    }
    datap = strstr(datap, "\r\n");
  }
  
  len = DATABUFSIZE;
  if ((ret = irmc_obex_get(conn->obexhandle, "telecom/cal/luid/cc.log", data, &len))<0) {
    dd(printf("Get cc failed.\n"));
  } else {
    data[len] = 0;
    sscanf(data, "%d", &conn->calchangecounter);
    dd(printf("Changecounter: %d\n", conn->calchangecounter));
  }

  if (slowsync) {
    len = DATABUFSIZE;
    if (conn->donttellsync) {
      // Reconnect with "IRMC-SYNC" to get X-IRMC-LUID
      irmc_obex_disconnect(conn->obexhandle);
      sleep(1);
      if (irmc_obex_connect(conn->obexhandle, "IRMC-SYNC")) {
	sleep(2);
	if ((ret = irmc_obex_connect(conn->obexhandle, "IRMC-SYNC")) < 0) {
	  g_free(data);
	  return(ret);
	}
      }
    }
    irmc_async_slowsync_msg(SYNC_OBJECT_TYPE_CALENDAR);
    if ((ret = irmc_obex_get(conn->obexhandle, "telecom/cal.vcs", data, &len))<0) 
      len = 0; // Continue anyway; Siemens models will fail this get if calendar is empty
    { 
      char *event_end = NULL;
      char *event_start = data, *todo_start;
      char *event = NULL;
      changed_object* change;
      sync_object_type objtype;      
      
      irmc_async_close_infodialog();
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
	  event = g_malloc(event_end - event_start + 256);
	  sprintf(event, "BEGIN:VCALENDAR\r\nVERSION:1.0\r\n");
	  pos = strlen(event);
	  memcpy(event+pos,event_start,event_end-event_start);
	  sprintf(event+(pos+event_end-event_start), "\r\nEND:VCALENDAR\r\n");
	  change = g_malloc0(sizeof(changed_object));
	  g_assert(change);
	  change->comp = sync_vtype_convert(event, 0|(conn->fixdst?VOPTION_FIXDSTFROMCLIENT:0)|(conn->translatecharset?VOPTION_FIXCHARSET:0)|VOPTION_CALENDAR1TO2|(conn->alarmfromirmc?0:VOPTION_REMOVEALARM)|VOPTION_CONVERTUTC, conn->charset);
	  change->removepriority = sync_get_key_data(change->comp, "DTEND");
	  
	  event_start = strstr(event, "X-IRMC-LUID:");
	  if (event_start) {
	    char luid[256];
	    if (sscanf(event_start, "X-IRMC-LUID:%256s", luid)) {
	      change->uid = g_strdup(luid);
	    }
	  }
	  change->change_type = SYNC_OBJ_MODIFIED;
	  change->object_type = objtype;
	  *sync_changes = g_list_append(*sync_changes, change);
	  g_free(event);
	}
	event_start = event_end;
      } while(event_start);
    }
  }
  g_free(data);
  return(newdb?SYNC_MSG_SLOWSYNC:0);
}

int pb_get_changes(irmc_connection *conn, int slowsync, GList **sync_changes) {
  char *data;
  char *datap;
  int len = DATABUFSIZE;
  char serial[256];
  char did[256] = "";
  char *filename;
  int ret;
  int newdb = 0;

  data = g_malloc(DATABUFSIZE);
  datap = data;

  filename = g_strdup_printf("telecom/pb/luid/%d.log", conn->pbchangecounter);

  if ((ret=irmc_obex_get(conn->obexhandle, filename, data, &len))<0) {
    dd(printf("Get log failed.\n"));
    g_free(filename);
    g_free(data);
    return(ret);
  }
  g_free(filename);
  data[len] = 0;
  dd(printf("Change log: \n%s\n", data));
  sscanf(datap, "SN:%256s\r\n", serial);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "DID:%256s\r\n", did);
  if (!conn->pbDID || strcmp(conn->pbDID, did)) {
    // This is a new database
    if (conn->pbDID)
      g_free(conn->pbDID);
    conn->pbDID = g_strdup(did);
    slowsync = 1;
    newdb = 1;
  }
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "Total-Records:%d\r\n", &conn->commondata.phonebookrecords);
  datap = strstr(datap, "\r\n");
  if (!datap) {
    g_free(data);
    return(0);
  }
  datap+=2;
  sscanf(datap, "Maximum-Records:%d\r\n", &conn->commondata.maxphonebookrecords);
  datap = strstr(datap, "\r\n");
  while(datap) {
    char type;
    int cc;
    char luid[256];
    datap+=2;
    if (sscanf(datap, "%c:%d::%256[^\r\n]", &type, &cc, luid) >= 3) {
      changed_object* change;
      char objdata[65536];
      int objlen = 65536;
      sync_object_type objtype;      

      filename = g_strdup_printf("telecom/pb/luid/%s.vcf", luid);
      objlen = 10240;
      if ((ret=irmc_obex_get(conn->obexhandle, filename, objdata, &objlen))<0) {
	objdata[0] = 0;
	objlen = 0;
	//printf("Get object %s failed.\n", luid);
      }
      g_free(filename);
      objdata[objlen]=0;
      
      objtype = SYNC_OBJECT_TYPE_PHONEBOOK;

      // Add only if we handle this type of data
      if (conn->commondata.object_types & objtype) {
	change = g_malloc0(sizeof(changed_object));
	g_assert(change);
	change->uid = g_strdup(luid);
	if (objlen > 0)
	  change->comp = sync_vtype_convert(objdata, 0|(conn->translatecharset?VOPTION_FIXCHARSET:0)|VOPTION_FIXTELOTHER, conn->charset);
	if (type == 'D')
	  change->change_type = SYNC_OBJ_SOFTDELETED;
	if (type == 'H')
	  change->change_type = SYNC_OBJ_HARDDELETED;
	if (type == 'M' || objlen == 0)
	  change->change_type = SYNC_OBJ_MODIFIED;
	change->object_type = objtype;
	*sync_changes = g_list_append(*sync_changes, change);
      }
    } else {
      if (datap[0] == '*')
	slowsync = 1;
    }
    datap = strstr(datap, "\r\n");
  }
    
  len = DATABUFSIZE;
  if ((ret=irmc_obex_get(conn->obexhandle, "telecom/pb/luid/cc.log", data, &len))<0) {
    dd(printf("Get cc failed.\n"));
  } else {
    data[len] = 0;
    sscanf(data, "%d", &conn->pbchangecounter);
    dd(printf("Phonebook changecounter: %d\n", conn->pbchangecounter));
  }

  if (slowsync) {
    len = DATABUFSIZE;
    if (conn->donttellsync) {
      // Reconnect with "IRMC-SYNC" to get X-IRMC-LUID
      irmc_obex_disconnect(conn->obexhandle);
      sleep(1);
      if (irmc_obex_connect(conn->obexhandle, "IRMC-SYNC")) {
	sleep(2);
	if ((ret=irmc_obex_connect(conn->obexhandle, "IRMC-SYNC"))<0) {
	  g_free(data);
	  return(ret);
	}
      }
    }
    irmc_async_slowsync_msg(SYNC_OBJECT_TYPE_PHONEBOOK);
    if ((ret=irmc_obex_get(conn->obexhandle, "telecom/pb.vcf", data, &len))<0)
      len = 0; // Continue anyway; Siemens models will fail this get if calendar is empty
    {
      char *event_end = NULL;
      char *event_start = data, *todo_start;
      char *event = NULL;
      changed_object* change;
      sync_object_type objtype;      
      
      irmc_async_close_infodialog();
      data[len]=0;
      do {
	char *start = event_start;
	objtype = SYNC_OBJECT_TYPE_PHONEBOOK;
	event_start = strstr(start, "BEGIN:VCARD");
	if ((event_end = strstr(start, "END:VCARD")))
	  event_end += strlen("END:VCARD");
	
	if (event_start && event_end) {
	  event = g_malloc(event_end-event_start+1);
	  memcpy(event, event_start, event_end-event_start);
	  event[event_end-event_start] = 0;
	  change = g_malloc0(sizeof(changed_object));
	  g_assert(change);
	  change->comp = sync_vtype_convert(event,0|(conn->translatecharset?VOPTION_FIXCHARSET:0)|VOPTION_FIXTELOTHER, conn->charset);
	  event_start = strstr(event, "X-IRMC-LUID:");
	  if (event_start) {
	    char luid[256];
	    if (sscanf(event_start, "X-IRMC-LUID:%256s", luid)) {
	      change->uid = g_strdup(luid);
	    }
	  }
	  g_free(event);
	  change->change_type = SYNC_OBJ_MODIFIED;
	  change->object_type = objtype;
	  *sync_changes = g_list_append(*sync_changes, change);
	}
	event_start = event_end;
      } while(event_start);
    }
  }
  g_free(data);
  return(newdb?SYNC_MSG_SLOWSYNC:0);
}


// Unique function name so not other plugin is called accidentally.
void irmc_disconnect(irmc_connection *conn) {
  if (!conn)
    return;
  if (conn->obexhandle) {
    irmc_obex_disconnect(conn->obexhandle);
    irmc_obex_cleanup(conn->obexhandle);
  }
  conn->obexhandle = 0;
  free_state(conn);
}


void sync_disconnect(irmc_connection *conn) {
  sync_pair *sync_pair = conn->sync_pair;
  irmc_disconnect(conn);
  sync_set_requestdone(sync_pair);
}




// Add, modify, or delete events in mobile calendar
void cal_modify_or_delete(irmc_connection *conn, 
			  char *event, char *inluid, 
			  char *outluid, int *outluidlen, int softdelete,
			  sync_object_type objtype) {
  int res = 0;
  char name[256];
  char *body = NULL;
  int bodylen=0;
  char rspbuf[256];
  int rspbuflen=256;
  char apparambuf[256];
  char *apparam = apparambuf;
  
  strcpy(name, "telecom/cal/luid/");
  if (inluid) {
    safe_strcat(name, inluid, 256);
  }
  safe_strcat(name, ".vcs", 256);
  if (event) {
    char *tmp;
    char *dtend = NULL;

    dtend = sync_get_key_data(event, "DTEND");
    if (conn->dontacceptold && dtend) {
      time_t tend, now;
      tend = sync_dt_to_timet(dtend);
      now = time(NULL);
      if (now-tend > conn->maximumage*3600*24) {
	g_free(dtend);
	sync_set_requestmsg(SYNC_MSG_USERDEFERRED, conn->sync_pair);
	return;
      }
    }
    if (dtend)
      g_free(dtend);
    
    body = sync_vtype_convert(event, VOPTION_ADDUTF8CHARSET|0|(conn->fixdst?VOPTION_FIXDSTTOCLIENT:0)|VOPTION_CALENDAR2TO1|(conn->alarmtoirmc?0:VOPTION_REMOVEALARM)|(conn->convertade?VOPTION_CONVERTALLDAYEVENT:0),NULL);
    bodylen = strlen(body);
    dd(printf("Converted body:\n%s\n", body));
  } else {
    bodylen = 0;
  }
  conn->calchangecounter++;
  sprintf(apparam+2, "%d", conn->calchangecounter);
  apparam[0] = IRSYNC_APP_MAXEXPCOUNT;
  apparam[1] = strlen(apparam+2);
  apparam += strlen(apparam+2)+2;
  if (!event && !softdelete) {
    // Delete object => Mark as hard delete
    apparam[0] = IRSYNC_APP_HARDDELETE;
    apparam[1] = 0;
    apparam+=2;
  }
  res = irmc_obex_put(conn->obexhandle, name, 0, bodylen?body:NULL, bodylen,
  		    rspbuf, &rspbuflen, apparambuf, apparam-apparambuf);
  g_free(body);
  if (res < 0) {
    sync_set_requestmsg(res,conn->sync_pair);
    return;
  }

  if (bodylen > 0) {
    if (!inluid)
      conn->commondata.calendarrecords++;
  } else {
    conn->commondata.calendarrecords--;
  }

  if (outluidlen)
    *outluidlen = 0;
  apparam = rspbuf;
  while (apparam < rspbuf+rspbuflen) {
    switch(apparam[0]) {
    case IRSYNC_APP_LUID:
      if (outluid && outluidlen) {
	memcpy(outluid, apparam+2, apparam[1]);
	*outluidlen = apparam[1];
	outluid[*outluidlen] = 0; // End string
      }
      break;
    case IRSYNC_APP_CHANGECOUNT: {
      char tmpbuf[16];
      memcpy(tmpbuf, apparam+2, apparam[1]>15?15:apparam[1]);
      tmpbuf[(int) apparam[1]] = 0;
      sscanf(tmpbuf, "%d", &conn->calchangecounter);
      dd(printf("New change counter: %d\n", conn->calchangecounter));
    }
      break;
    default:
      dd(printf("irmc_cal_modify: Received unknown APPARAM\n"));
    }
    apparam+=apparam[1]+2;
  }
  // save_state(conn);
  sync_set_requestdone(conn->sync_pair);

}

// Add, modify, or delete events in mobile phonebook
void pb_modify_or_delete(irmc_connection *conn, 
			 char* event, char *inluid, 
			 char *outluid, int *outluidlen, int softdelete,
			 sync_object_type objtype) {
  int res = 0;
  char name[256];
  int bodylen=0;
  char rspbuf[256];
  int rspbuflen=256;
  char apparambuf[256];
  char *apparam = apparambuf;
  char *body = NULL;

  strcpy(name, "telecom/pb/luid/");
  if (inluid) {
    safe_strcat(name, inluid, 256);
  }
  safe_strcat(name, ".vcf", 256);
  if (event) {
    char *tel = NULL;
    tel = sync_get_key_data(event, "TEL");
    if (conn->onlyphonenumbers && !tel) {
      // If no phone number and user want only contacts with phone numbers
      sync_set_requestmsg(SYNC_MSG_USERDEFERRED, conn->sync_pair);
      return;
    }
    g_free(tel);
    body = sync_vtype_convert(event, VOPTION_ADDUTF8CHARSET,NULL);
    bodylen = strlen(body);
    dd(printf("Body: %s\n", body));
  } else {
    bodylen = 0;
  }
  conn->pbchangecounter++;
  sprintf(apparam+2, "%d", conn->pbchangecounter);
  apparam[0] = IRSYNC_APP_MAXEXPCOUNT;
  apparam[1] = strlen(apparam+2);
  apparam += strlen(apparam+2)+2;
  if (!event && !softdelete) {
    // Delete object => Mark as hard delete
    apparam[0] = IRSYNC_APP_HARDDELETE;
    apparam[1] = 0;
    apparam+=2;
  }
  res = irmc_obex_put(conn->obexhandle, name, 0, body, bodylen,
		      rspbuf, &rspbuflen, apparambuf, apparam-apparambuf);
  if (body)
    g_free(body);
  if (res) {
    sync_set_requestmsg(res, conn->sync_pair);
    return;
  }

  if (bodylen > 0) {
    if (!inluid)
      conn->commondata.phonebookrecords++;
  } else
    conn->commondata.phonebookrecords--;

  if (outluidlen)
    *outluidlen = 0;
  apparam = rspbuf;
  while (apparam < rspbuf+rspbuflen) {
    switch(apparam[0]) {
    case IRSYNC_APP_LUID:
      if (outluid && outluidlen) {
	memcpy(outluid, apparam+2, apparam[1]);
	*outluidlen = apparam[1];
	outluid[*outluidlen] = 0; // End string
      }
      break;
    case IRSYNC_APP_CHANGECOUNT: {
      char tmpbuf[16];
      memcpy(tmpbuf, apparam+2, apparam[1]>15?15:apparam[1]);
      tmpbuf[(int) apparam[1]] = 0;
      sscanf(tmpbuf, "%d", &conn->pbchangecounter);
      dd(printf("New change counter: %d\n", conn->pbchangecounter));
    }
      break;
    default:
      dd(printf("irmc_pb_modify: Received unknown APPARAM\n"));
    }
    apparam+=apparam[1]+2;
  }
  // Not a problem to remove?
  //save_state(conn);
  sync_set_requestdone(conn->sync_pair);
}

void syncobj_modify(irmc_connection *conn, 
		    char* event, char *inluid,
		    sync_object_type objtype,
		    char *outluid, int *outluidlen) {
  if (objtype == SYNC_OBJECT_TYPE_CALENDAR || 
      objtype == SYNC_OBJECT_TYPE_TODO)
    cal_modify_or_delete(conn, event, inluid, outluid, outluidlen, 0, objtype);
  else if (objtype == SYNC_OBJECT_TYPE_PHONEBOOK)
    pb_modify_or_delete(conn, event, inluid, outluid, outluidlen, 0, objtype);
  else
    sync_set_requestfailed(conn->sync_pair);
}

void syncobj_delete(irmc_connection *conn, char *inluid,
		    sync_object_type objtype, int softdelete) {
  if (objtype == SYNC_OBJECT_TYPE_CALENDAR || 
      objtype == SYNC_OBJECT_TYPE_TODO)
    cal_modify_or_delete(conn, NULL, inluid, NULL, NULL, softdelete, objtype);
  else if (objtype == SYNC_OBJECT_TYPE_PHONEBOOK)
    pb_modify_or_delete(conn, NULL, inluid, NULL, NULL, softdelete, objtype);
  else
    sync_set_requestfailed(conn->sync_pair);
}

// Called by the syncengine after synchronization 
// (if success, we can discard our internal change list)
void sync_done(irmc_connection *conn, gboolean success) {
  if (success) {
    // No need to do it asyncronously
    save_state(conn);
  }
  sync_set_requestdone(conn->sync_pair);
}


// Return TRUE if this client does not have to be polled 
// (i.e. can be constantly connected)
gboolean always_connected() {
  return(FALSE);
}

char* short_name() {
  return("irmc-sync");
}

char* long_name() {
  return("IrMC Mobile Device");
}

// Return the types of objects that this client handle
sync_object_type object_types() {
  return(SYNC_OBJECT_TYPE_CALENDAR | SYNC_OBJECT_TYPE_TODO | 
	 SYNC_OBJECT_TYPE_PHONEBOOK);
}

void plugin_init(void) {
  irmc_obex_init();
}

char* plugin_info(void) {
  return("Connects to IrMC compliant mobile devices,\nsuch as the SonyEricsson T39/T68/T610 or Siemens S55.");
}

int plugin_API_version(void) {
  return(3);
}
