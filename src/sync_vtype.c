/* 
   MultiSync - A PIM data synchronization program
   Copyright (C) 2002-2003 Bo Lincoln <lincoln@lysator.liu.se>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   In addition, as a special exception, Bo Lincoln <lincoln@lysator.liu.se>
   gives permission to link the code of this program with
   the OpenSSL library (or with modified versions of OpenSSL that use the
   same license as OpenSSL), and distribute linked combinations including
   the two.  You must obey the GNU General Public License in all
   respects for all of the code used other than OpenSSL.  If you modify
   this file, you may extend this exception to your version of the
   file, but you are not obligated to do so.  If you do not wish to
   do so, delete this exception statement from your version.

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
 *  $Id: sync_vtype.c,v 1.28 2004/04/06 10:34:07 lincoln Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <iconv.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "sync_vtype.h"

typedef enum {
  ALARM_ACTION_AUDIO,
  ALARM_ACTION_DISPLAY,
  ALARM_ACTION_EMAIL,
  ALARM_ACTION_PROCEDURE
} vtype_alarm_action;

 
typedef enum {
  VTYPE_UNKNOWN = 0,
  VTYPE_VCALENDAR10 = 0x1,
  VTYPE_VCALENDAR20 = 0x2,
  VTYPE_VCALENDAR = 0x3,
  VTYPE_VCARD21 = 0x4,
  VTYPE_VCARD30 = 0x8,
  VTYPE_VCARD = 0xc
} vtype_type;

// Parse and correct a number of errors in the VCARD/VTODO data
// The "opts" parameter decides which errors/features that should be corrected
char* sync_vtype_convert(char *card, sync_voption opts, char* charset) {
  GString *outcard;
  char *incard = g_strdup(card), *ret = NULL;
  gboolean alarmmode = FALSE; // If in alarm clause of vCAL 2.0
  char *alarmdescription = NULL;
  time_t alarmtime = 0, alarmduration = 0;
  vtype_alarm_action alarmaction = ALARM_ACTION_DISPLAY;
  int alarmrepeat = 0;
  gboolean output = TRUE; // If feedthrough output is enabled
  vtype_type datatype = VTYPE_UNKNOWN;

  outcard = g_string_new("");
  while (incard) {
    char* endln;
    char head[256];

    endln = strstr(incard, "\n");
    if (endln)
      endln[0] = 0; // End string at end of line
    if (endln && endln > incard && endln[-1] == '\r')
      endln[-1] = 0;
    if (strlen(incard) > 1 && 
	sscanf(incard, "%255[^:]:", head) >= 1 &&
	incard[strlen(head)] == ':') {
      char *line = head;
      char *endent;
      char name[256];
      char *data = strstr(incard, ":");
      gboolean qp = FALSE;
      gboolean fixdst = FALSE;
      gboolean nocategorytel = FALSE;
      gboolean trigger = FALSE;
      gboolean triggervaluedatetime = FALSE, triggerrelatedend = FALSE;
      gboolean ver2rrule = FALSE, ver1rrule = FALSE;
      gboolean addfn = FALSE;
      gboolean addlabel = FALSE;
      gboolean adr = FALSE;
      gboolean fixlocaltime = FALSE;
      gboolean fixutc = FALSE;
      gboolean dt2to1 = FALSE, dt1to2 = FALSE;
      gboolean dtend = FALSE, alldayevent = FALSE;
      char adrtype[256] = "";
      char linecharset[256] = "";
      unsigned char *value = NULL;

      if (data)
	value = (unsigned char *) g_strdup(data+1);
      else
	value = (unsigned char *) g_strdup("");
      
      output = TRUE;
      endent = strstr(line, ";");
      if (strstr(line, "QUOTED-PRINTABLE"))
      // Sloppy detection of QP data, means that =\n is followed by more
	qp = TRUE; 
      while (endln && (endln[1] == ' ' || 
		       (qp && value[strlen((const char *) value)-1] == '='))) {
	// Extend line-broken data
	char *tmp;
	incard = endln+1;
	endln = strstr(incard, "\n");
	if (endln)
	  endln[0] = 0; // End string at end of line
	if (endln && endln > incard && endln[-1] == '\r')
	  endln[-1] = 0;
	if (qp)
	  tmp = g_strdup_printf("%s\r\n%s", value, incard);
	else
	  tmp = g_strdup_printf("%s%s", value, incard+1);
	g_free(value);
	value = (unsigned char *) tmp;
      }
      qp = FALSE; // Do a proper QP detection
      if (endent)
	endent[0] = 0;
      strncpy(name, line, 255);
      if (!g_strcasecmp(name, "BEGIN")) {
	if (!g_strcasecmp((const gchar *) value, "VCALENDAR"))
	  datatype = VTYPE_VCALENDAR;
	if (!g_strcasecmp((const gchar *) value, "VCARD"))
	  datatype = VTYPE_VCARD;
      }
      if (!g_strcasecmp(name, "VERSION")) {
	if (!g_strcasecmp((const gchar *) value,"1.0") && (datatype&VTYPE_VCALENDAR))
	  datatype = VTYPE_VCALENDAR10;
	if (!g_strcasecmp((const gchar *) value,"2.0") && (datatype&VTYPE_VCALENDAR))
	  datatype = VTYPE_VCALENDAR20;
	if (!g_strcasecmp((const gchar *) value,"2.1") && (datatype&VTYPE_VCARD))
	  datatype = VTYPE_VCARD21;
	if (!g_strcasecmp((const gchar *) value,"3.0") && (datatype&VTYPE_VCARD))
	  datatype = VTYPE_VCARD30;
      }

      if ((opts & (VOPTION_FIXDSTTOCLIENT|VOPTION_FIXDSTFROMCLIENT)) &&
	  (!g_strcasecmp(name, "DTSTART") || !g_strcasecmp(name, "DTEND")))
	fixdst = TRUE;
      if ((opts & (VOPTION_CONVERTUTC)) &&
	  (!g_strcasecmp(name, "DTSTART") || !g_strcasecmp(name, "DTEND")))
	fixlocaltime = TRUE;
      if ((opts & (VOPTION_REMOVEUTC)) &&
	  (!g_strcasecmp(name, "DTSTART") || !g_strcasecmp(name, "DTEND")))
	fixutc = TRUE;
      if ((opts & (VOPTION_CALENDAR2TO1)) &&
	  (!g_strcasecmp(name, "DTSTART") || !g_strcasecmp(name, "DTEND")))
	dt2to1 = TRUE;
      if (!g_strcasecmp(name, "DTEND"))
	dtend = TRUE;
      if ((opts & (VOPTION_CALENDAR1TO2)) &&
	  (!g_strcasecmp(name, "DTSTART") || !g_strcasecmp(name, "DTEND")))
	dt1to2 = TRUE;
      if (!g_strcasecmp(name, "N")) {
	char *fn = sync_get_key_data(card, "FN");
	if (!fn) {
	  addfn = TRUE;
	} else
	  g_free(fn);
      }
      if (!g_strcasecmp(name, "ADR")) {
	char *label = sync_get_key_data(card, "LABEL");
	adr = TRUE;
	if (!label) {
	  addlabel = TRUE;
	} else
	  g_free(label);
      }
      if ((opts & VOPTION_FIXTELOTHER) && !g_strcasecmp(name, "TEL"))
	nocategorytel = TRUE;
      if (!g_strcasecmp(name, "BEGIN") && !g_strcasecmp((const gchar *) value,"VALARM")) {
	if (opts & VOPTION_CALENDAR2TO1 || (opts & VOPTION_REMOVEALARM)) {
	  alarmmode = TRUE;
	  alarmrepeat = 0;
	  alarmtime = 0;
	  alarmduration = 0;
	}
      }
      if ((opts & VOPTION_REMOVEPHOTO) && !g_strcasecmp(name, "PHOTO"))
	output = FALSE;
      if ((opts & VOPTION_CALENDAR2TO1) && !g_strcasecmp(name, "RRULE"))
	ver2rrule = TRUE;
      if ((opts & VOPTION_CALENDAR1TO2) && !g_strcasecmp(name, "RRULE"))
	ver1rrule = TRUE;
      if (alarmmode && !g_strcasecmp(name, "TRIGGER"))
	trigger = TRUE;
      if (alarmmode)
	output = FALSE;
      if (alarmmode &&
	  !g_strcasecmp(name, "END") && !g_strcasecmp((const gchar *) value,"VALARM")) {
	// The end of a vCAL 2.0 alarm clause
	char *dtalarm = sync_timet_to_dt(alarmtime);
	char *dur = NULL;
	alarmmode = FALSE;
	if (!(opts & VOPTION_REMOVEALARM)) {
	  if (alarmduration != 0)
	    dur = sync_timet_to_dur(alarmduration);
	  if (alarmaction == ALARM_ACTION_AUDIO || 
	      alarmaction == ALARM_ACTION_DISPLAY) {
	    // Add both AALARM and DALARM so that most devices will be happy.
	    g_string_sprintfa(outcard, "AALARM:%s;%s;%d;\r\n", dtalarm, dur?dur:"",
			      alarmrepeat);
	    g_string_sprintfa(outcard, "DALARM:%s;%s;%d;%s\r\n", dtalarm, 
			      dur?dur:"", alarmrepeat, 
			      alarmdescription?alarmdescription:"");
	  }
	  if (dur)
	    g_free(dur);
	}
	if (dtalarm)
	  g_free(dtalarm);
	if (alarmdescription)
	  g_free(alarmdescription);
	alarmdescription = NULL;
      }
      if ((opts & VOPTION_CALENDAR1TO2) && 
	  !g_strcasecmp(name, "VERSION") && !g_strcasecmp((const gchar *) value,"1.0")) {
	output = FALSE;
	g_string_append(outcard, "VERSION:2.0\r\n");
      }
      if ((opts & VOPTION_CALENDAR2TO1) && 
	  !g_strcasecmp(name, "VERSION") && !g_strcasecmp((const gchar *) value,"2.0")) {
	output = FALSE;
	g_string_append(outcard, "VERSION:1.0\r\n");
      }
      if ((!g_strcasecmp(name, "AALARM") || !g_strcasecmp(name, "DALARM"))) {
	// Write vCAL 1.0 AALARM or DALARM as 2.0 VALARM clause
	char dtalarm[256], dur[256], descr[256];
	int repeat = 0;
	char *trigdur, *dtstart, *summary;
	
	
	if (opts & VOPTION_CALENDAR1TO2 || (opts & VOPTION_REMOVEALARM))
	  output = FALSE;
	if ((opts & VOPTION_CALENDAR1TO2) && !(opts & VOPTION_REMOVEALARM)) {
	  dur[0] = 0;
	  dtalarm[0] = 0;
	  descr[0] = 0;
	  sscanf((const char *) value, "%255[^;];%255[^;];%d;%255s", dtalarm, dur, &repeat,
		 descr);
	  g_string_append(outcard, "BEGIN:VALARM\r\n");
	  dtstart = sync_get_key_data(card, "DTSTART");
	  alarmtime = sync_dt_to_timet(dtalarm)-sync_dt_to_timet(dtstart);
	  if (dtstart)
	    g_free(dtstart);
	  trigdur = sync_timet_to_dur(alarmtime);
	  g_string_sprintfa(outcard, "TRIGGER;VALUE=DURATION;RELATED=START:%s\r\n", trigdur?trigdur:"");
	  if (trigdur)
	    g_free(trigdur);
	  g_string_append(outcard, "ACTION:DISPLAY\r\n");
	  if (repeat > 0)
	    g_string_sprintfa(outcard, "REPEAT:%d\r\n", repeat);
	  if (strlen(dur) > 0)
	    g_string_sprintfa(outcard, "DURATION:%s\r\n", dur);
	  if (!g_strcasecmp(name, "DALARM") && strlen(descr) > 0) {
	    g_string_sprintfa(outcard, "DESCRIPTION:%s\r\n", descr);
	  } else {
	    summary = sync_get_key_data(card, "SUMMARY");
	    if (summary) {
	      g_string_sprintfa(outcard, "DESCRIPTION:%s\r\n", summary);
	      g_free(summary);
	    }
	  }
	  g_string_append(outcard, "END:VALARM\r\n");
	}
      }
      if (output)
	g_string_append(outcard, name);
      line = endent;
      if (line)
	line = line+1;
      while (line) {
	char propname[256];
	char propdata[256];
	gboolean outputprop = TRUE;
	int ret = 0;
	endent = strstr(line, ";");
	if (endent)
	  endent[0] = 0;
	ret = sscanf(line, "%255[^=]=%255s", propname, propdata);
	if (ret > 0) {
	  if (nocategorytel)
	    nocategorytel = FALSE;
	  if (trigger && 
	      !g_strcasecmp(propname, "VALUE") &&
	      !g_strcasecmp(propdata, "DATE-TIME"))
	    triggervaluedatetime = TRUE;
	  if (trigger && 
	      !g_strcasecmp(propname, "RELATED") &&
	      !g_strcasecmp(propdata, "END"))
	    triggerrelatedend = TRUE;
	  if (!g_strcasecmp(propname, "CHARSET"))
	    strncpy(linecharset, propdata, 255);
	  if (adr && 
	      (!g_strcasecmp(propname, "HOME") || 
	      !g_strcasecmp(propname, "WORK"))) {
	    strncpy(adrtype, propname, 255);
	  }

	  if ((!g_strcasecmp(propname, "ENCODING") &&
	       !g_strcasecmp(propdata, "QUOTED-PRINTABLE")) ||
	      !g_strcasecmp(propname, "QUOTED-PRINTABLE")) {
	    // Convert
	    qp = TRUE;
	    outputprop = FALSE;
	  }
	  if (((opts & VOPTION_CALENDAR2TO1) &&
	       !g_strcasecmp(propname, "TZID")) ||
	      (((opts & VOPTION_CALENDAR1TO2) || 
		(datatype == VTYPE_VCALENDAR20)) &&
	       !g_strcasecmp(propname, "CHARSET"))) {
	    // Discard unknown properties
	    outputprop = FALSE;
	  }
	  if (dt2to1 && 
	      !g_strcasecmp(propname, "VALUE") &&
	      !g_strcasecmp(propdata, "DATE")) {
	    outputprop = FALSE;
	    alldayevent = TRUE;
	  }
	      
	  if (outputprop) {
	    if (output) {
	      g_string_append(outcard, ";");
	      g_string_append(outcard, propname);
	      if (ret >= 2) {
		g_string_append(outcard, "=");
		g_string_append(outcard, propdata);
	      }
	    }
	  }
	}
	line = endent;
	if (line)
	  line = line+1;
      }
      if (qp) { // Temporarily decode QP
	char *tmp;
	tmp = (char *) value;
	value = (unsigned char*) sync_vtype_decode_qp((char *) value);
	g_free(tmp);
      }
      if ((opts & VOPTION_FIXCHARSET) && value && charset) {
	iconv_t ic;
	int t;
	gboolean highchar = FALSE;
	for (t = 0; t < strlen((const char *) value); t++)
	  if (value[t] > 127)
	    highchar = TRUE;
	if (highchar) {
	  ic = iconv_open("UTF-8", charset);
	  if (ic >= 0) {
	    char *utfvalue = g_malloc0(65536);
	    size_t inbytes = strlen((const char *) value);
	    size_t outbytes = 65536;
	    char *inbuf = (char *) value, *outbuf = utfvalue;
	    iconv(ic, &inbuf, &inbytes, &outbuf, &outbytes);
	    g_free(value);
	    value = (unsigned char *) utfvalue;
	    iconv_close(ic);
	  }
	}
      }

      if ((opts & VOPTION_ADDUTF8CHARSET) &&
	  (datatype != VTYPE_VCALENDAR20 || (opts & VOPTION_CALENDAR2TO1)) && 
	  value && output &&
	  strlen(linecharset) == 0) {
	int t;
	gboolean highchar = FALSE;
	for (t = 0; t < strlen((const char *) value); t++)
	  if (value[t] > 127)
	    highchar = TRUE;
	if (highchar)
	  g_string_append(outcard, ";CHARSET=UTF-8");
      }
      if (value && alldayevent) {
	int y, m, d, h, min, s, res;
	char utc = ' ';
	res = sscanf((const char *) value, 
		     "%4d%2d%2dT%2d%2d%2d%c", &y, &m, &d, &h, &min, &s, &utc);
	if (res >= 3) {
	  if ((opts & VOPTION_CONVERTALLDAYEVENT)) {
	    // Date only, convert to 00:00
	    if (!dtend) { // DTSTART
	      g_free(value);
	      value = (unsigned char *) g_strdup_printf("%04d%02d%02dT000000Z", y, m, d);
	    } else { // DTEND
	      time_t t = sync_dt_to_timet((char *) value);
	      char *tpos;
	      t -= 24*3600; // Move one day backwards
	      g_free(value);
	      value = (unsigned char *) sync_timet_to_dt(t);
	      tpos = strstr((const char *) value, "T");
	      if (tpos) {
		char *tmp;
		tpos[0] = 0;
		tmp = g_strdup_printf("%sT240000Z", value);
		g_free(value);
		value = (unsigned char *) tmp;
	      }
	    }
	  } else {
	    g_free(value);
	    value = (unsigned char *) g_strdup_printf("%04d%02d%02dT000000", y, m, d);
	  }
	}
      }
      if (value && dt1to2) {
	int y, m, d, h, min, s, res;
	char utc = ' ';
	res = sscanf((const char *) value, 
		     "%4d%2d%2dT%2d%2d%2d%c", &y, &m, &d, &h, &min, &s, &utc);
	if (res == 7 && (h == 0|| h == 24) && min == 0 && s == 0) {
	  // All day event?
	  g_free(value);
	  
	  g_string_append(outcard, ";VALUE=DATE");
	  value = (unsigned char *) g_strdup_printf("%04d%02d%02d", y, m, d);
	  if (h == 24) {
	    time_t t = sync_dt_to_timet((char *) value);
	    char *tpos;
	    t += 24*3600; // Move one day forwards
	    g_free(value);
	    value = (unsigned char *) sync_timet_to_dt(t);
	    tpos = strstr((const char *) value, "T");
	    if (tpos)
	      tpos[0] = 0; // End string after date
	  }	    
	}
      }
      if (fixdst && value && strlen((const char *) value) > 0 && 
	  value[strlen((const char *) value)-1] == 'Z') {
	// Fix broken DST handling on Ericsson phones by modifying
	// times of an event by the DST at present.
	time_t now, dt;
	struct tm *nowinfo, *dtinfo;
	int nowdst, dtdst;
	
	time(&now);
	nowinfo = localtime(&now); 
	nowdst = nowinfo->tm_isdst;
	dt = sync_dt_to_timet((char *) value);
	dtinfo = localtime(&dt); 
	dtdst = dtinfo->tm_isdst;
	if (nowdst && !dtdst) {
	  g_free(value);
	  value = (unsigned char*) sync_timet_to_dt((opts&VOPTION_FIXDSTFROMCLIENT)?dt+3600:dt-3600);
	}
	if (!nowdst && dtdst) {
	  g_free(value);
	  value = (unsigned char*) sync_timet_to_dt((opts&VOPTION_FIXDSTFROMCLIENT)?dt-3600:dt+3600);
	}	  
      }
      if (fixlocaltime && value && strlen((const char *) value) > 0 &&
	  value[strlen((const char *) value)-1] == 'Z') {
	time_t dt;
	dt = sync_dt_to_timet((char *) value);
	g_free(value);
	value = (unsigned char*) sync_timet_to_dt(dt);
      }
      if (fixutc && value && strlen((const char *) value) > 0 &&
	  value[strlen((const char *) value)-1] == 'Z') {
	// Interpret UTC as localtime instead
	value[strlen((const char *) value)-1] = 0;
      }
      if (nocategorytel) {
	// Add VOICE as category if the phone number has no category
	g_string_append(outcard, ";");
	g_string_append(outcard, "VOICE");
      }
      if (ver1rrule && value) {
	// Convert a vCAL 1.0 RRULE to 2.0
	char* rrule = sync_vtype_vcal1_to_vcal2((char *) value);
	g_free(value);
	value = (unsigned char*) rrule;
      }
      if (ver2rrule && value) {
	// Convert a vCAL 2.0 RRULE to 1.0
	char* rrule = sync_vtype_vcal2_to_vcal1((char *) value);
	g_free(value);
	value = (unsigned char*) rrule;
      }
      if (trigger) {
	// A trigger in an alarm clause
	time_t dur = sync_dur_to_timet((char *)value);
	char *dtstart = sync_get_key_data(card, "DTSTART");
	alarmtime = sync_dt_to_timet(dtstart)+dur;
	if (dtstart)
	  g_free(dtstart);
      }
      if (alarmmode && !g_strcasecmp(name, "ACTION")) {
	if (!g_strcasecmp((const gchar *) value,"AUDIO"))
	  alarmaction = ALARM_ACTION_AUDIO;
	if (!g_strcasecmp((const gchar *) value,"DISPLAY"))
	  alarmaction = ALARM_ACTION_DISPLAY;
	if (!g_strcasecmp((const gchar *) value,"EMAIL"))
	  alarmaction = ALARM_ACTION_EMAIL;
	if (!g_strcasecmp((const gchar *) value,"PROCEDURE"))
	  alarmaction = ALARM_ACTION_PROCEDURE;
      }
      if (alarmmode && !g_strcasecmp(name, "REPEAT"))
	sscanf((const char *) value, "%d", &alarmrepeat);
      if (alarmmode && !g_strcasecmp(name, "DURATION"))
	alarmduration = sync_dur_to_timet((char *) value);
      if (alarmmode && !g_strcasecmp(name, "DESCRIPTION"))
	alarmdescription = g_strdup((const gchar *) value);
      if (value && (opts & VOPTION_CALENDAR1TO2)) {
	// Convert "," to "\,"
	char **tmp = g_strsplit((const gchar *) value, ",", 255);
	g_free(value);
	value = (unsigned char*) g_strjoinv("\\,", tmp);
	g_strfreev(tmp);
      }
      if (value && (opts & VOPTION_CALENDAR2TO1)) {
	// Convert "\," to ","
	char **tmp = g_strsplit((const gchar *) value, "\\,", 255);
	g_free(value);
	value = (unsigned char*) g_strjoinv(",", tmp);
	g_strfreev(tmp);
      }
      if (adr && strlen(adrtype) == 0) {
	// If no address type specified, add WORK (mostly for Evolution)
	g_string_append(outcard, ";WORK");
      }

      if (output) {
	// Write the value
	if ((qp && value && strstr((const char *) value, "\r")) || strstr((const char *) value,"\n")) {
	  // Re-encode if necessary
	  char *tmp = sync_vtype_encode_qp((char *) value);
	  g_string_append(outcard, ";ENCODING=QUOTED-PRINTABLE:");
	  g_string_append(outcard, tmp);
	  g_free(tmp);
	} else {
	  g_string_append(outcard, ":");
	  g_string_append(outcard, (const gchar *) value);
	}
	g_string_append(outcard, "\r\n");
      }

      if (addfn && value) {
	char last[256]="", first[256]="";
	if (sscanf((const char *) value, "%255[^;];%255[^;]", last, first) > 0 ||
	    sscanf((const char *) value, ";%255[^;]", first) > 0) {
	  g_string_append(outcard, "FN:");
	  if (strlen(first) > 0)
	    g_string_append(outcard, first);
	  if (strlen(first) > 0 && strlen(last) > 0)
	    g_string_append(outcard, " ");
	  if (strlen(last) > 0)
	    g_string_append(outcard, last);
	  g_string_append(outcard, "\r\n");
	}
      }
      if (addlabel && value) {
	char *label, *tmp; 
//	char *pos = (char *) value, *oldpos = (char *) value; // unused!
//	int t;  // unused!
	gchar** data = g_strsplit((const gchar *) value, ";", 256);
	if (data[0] && data[1] && data[2] && data[3] && data[4] &&
	    data[5] && data[6]) {
	  tmp = g_strdup_printf("%s%s%s\n%s\n%s, %s%s%s\n%s",
				data[2], data[2][0]?" ":"", data[0], 
				data[1], data[3], data[4], data[4][0]?" ":"",
				data[5], data[6]);
	  label = sync_vtype_encode_qp(tmp);
	  g_free(tmp);
	  g_string_append(outcard, "LABEL;");
	  if (strlen(adrtype) > 0) {
	    g_string_append(outcard, adrtype);
	  } else
	    g_string_append(outcard, "WORK");
	  g_string_sprintfa(outcard, ";ENCODING=QUOTED-PRINTABLE:%s\r\n",
			    label);
	  g_free(label);
	}
	g_strfreev(data);
      }
      
      g_free(value);
    }
    incard = endln;
    if (incard)
      incard += 1;
  }
  ret = outcard->str;
  g_string_free(outcard, FALSE);
  g_free(incard);
  if (alarmdescription)
    g_free(alarmdescription);
  return(ret);
}

// Decode QUOTED-PRINTABLE
char* sync_vtype_decode_qp(char *in) {
  char *pos = in, *oldpos = in, *out;
  GString *newvalue = g_string_new("");
  
  if (!in)
    return NULL;
  while((pos=strstr(pos, "="))) {
    int c;
    pos[0] = 0;
    g_string_append(newvalue, oldpos);
    if (pos[1] == '\r' || pos[1] == '\n') {
      if (pos[1] == '\r')
	pos++;
      pos+=2;
    } else {
      if (sscanf(pos+1, "%2x", &c)) {
	char s[2] = " ";
	s[0] = c;
	g_string_append(newvalue, s);
	pos+=3;
      }
    }
    oldpos = pos;
  }
  g_string_append(newvalue, oldpos);
  out = newvalue->str;
  g_string_free(newvalue, FALSE);
  return(out);
}

char *sync_vtype_encode_qp(char* in) {
  char *out;
  GString *newvalue = g_string_new("");
  int t = 0;
  int count = 0;
  if (!in)
    return NULL;

  while (in[t]) {
    char hex[] = "0123456789ABCDEF";
    if (in[t] > 'z' || in[t] < ' ' || in[t] == '=') {
      g_string_sprintfa(newvalue, "=%c%c", 
			hex[(((unsigned char) in[t])>>4)&0xf],
			hex[in[t]&0xf]);
      count+=3;
    } else {
      g_string_sprintfa(newvalue, "%c", in[t]);
      count++;
    }
    if (count >= 76) {
      g_string_sprintfa(newvalue, "=\r\n");
      count=0;
    }
    t++;
  }
  out = newvalue->str;
  g_string_free(newvalue, FALSE);
  return(out);
}


// Convert vCAL 1.0 RRULEto vCAL 2.0
char* sync_vtype_vcal1_to_vcal2(char* in) {
  gchar** bits = g_strsplit(in, " ", 256);
  int i;
  int last_bit;
  char frequency[256];
  char* freqstring = NULL;
  int interval;
  gchar** outbits = g_malloc0(sizeof(gchar*) * 256);
  int outbit = 0;
  int count = 0;
  char* byday=NULL;
  char* bymonthday=NULL;
  char* byyearday=NULL;
  char* bymonth=NULL;
  gchar* out = NULL;
  time_t untiltime = 0;
   
  for(i=0; bits[i]; i++);
  
  last_bit = i - 1;
  
  /* get frequency and interval */
  if (sscanf(bits[0], "%[A-Z]%d", frequency, &interval) < 2) {
    out = g_strdup(in);
    goto err;
  }
  
  /* get count */
  if (sscanf(bits[last_bit], "#%d", &count) < 1) {
    // Not a count, assume "until" value
    untiltime = sync_dt_to_timet(bits[last_bit]);
  }
  
  if(last_bit > 1) {
    gchar* middlebits[last_bit];
    gchar* foo;
    for(i=1; i < last_bit; i++) {
      middlebits[i-1] = bits[i];
    }
    middlebits[last_bit-1] = NULL;
    foo = sync_vtype_vcal1_list_to_vcal2(middlebits);

    if (!strcmp(frequency, "MD"))
      bymonthday = foo;
    else if (!strcmp(frequency, "MP"))
      byday = foo;
    else if (!strcmp(frequency, "YD"))
      byyearday = foo;
    else if (!strcmp(frequency, "YM"))
      bymonth = foo;
    else 
      byday = foo;
  }
  /* format frequency */
  if(!strcmp(frequency, "D")) {
    freqstring = "DAILY";
  } else if(!strcmp(frequency, "W")) {
    freqstring = "WEEKLY";
  } else if(!strcmp(frequency, "MD") || !strcmp(frequency, "MP")) {
    freqstring = "MONTHLY";
  } else if(!strcmp(frequency, "YD") || !strcmp(frequency, "YM")) {
    freqstring = "YEARLY";
  }
  if (!freqstring) { // Not parsable
    g_strfreev(outbits);
    g_strfreev(bits);
    return(g_strdup(in));
  }

  outbits[outbit] = g_strdup_printf("FREQ=%s", freqstring);
  outbit++;
  
  /* interval */
  outbits[outbit] = g_strdup_printf("INTERVAL=%d", interval);
  outbit++;
  
  /* count */
  if(count > 0) {
    outbits[outbit] = g_strdup_printf("COUNT=%d", count);
    outbit++;
  } else if(untiltime > 0) {
    /* until */
    char *dt = sync_timet_to_dt_utc(untiltime);
    outbits[outbit] = g_strdup_printf("UNTIL=%s", dt);
    g_free(dt);
    outbit++;
  }
  
  if(byday) {
    outbits[outbit] = g_strdup_printf("BYDAY=%s", byday);
    g_free(byday);
    outbit++;
  }
  if(byyearday) {
    outbits[outbit] = g_strdup_printf("BYYEARDAY=%s", byyearday);
    g_free(byyearday);
    outbit++;
  }
  if(bymonth) {
    outbits[outbit] = g_strdup_printf("BYMONTH=%s", bymonth);
    g_free(bymonth);
    outbit++;
  }  
  if(bymonthday) {
    outbits[outbit] = g_strdup_printf("BYMONTHDAY=%s", bymonthday);
    g_free(bymonthday);
    outbit++;
  }
  
  outbits[outbit] = NULL;
  out = g_strjoinv(";", outbits);

 err:  
  g_strfreev(outbits);
  g_strfreev(bits);
  return out;
}

// Convert vCAL 2.0 RRULEto vCAL 1.0
char* sync_vtype_vcal2_to_vcal1(char* value) {
  char freq[256], *pos;
  if (sscanf((const char *) value, "FREQ=%255[^;]", freq) >= 1) {
    time_t untiltime = 0;
    int count = 0, interval = 1;
    char bywdaylist[256] = "";
    char bymodaylist[256] = "";
    char byyrdaylist[256] = "";
    char bymolist[256] = "";
    gboolean converted = FALSE;
    GString *newvalue = g_string_new("");
    pos = strstr((const char *) value, ";");
    if (pos)
      pos++;
    while(pos) {
      char name[256], data[256];
      if (sscanf(pos, "%255[^=]=%255[^;]", name, data) == 2) {
	// Not complete, only what can be converted to 1.0
	if (!g_strcasecmp(name, "UNTIL"))
	  untiltime = sync_dt_to_timet(data);
	if (!g_strcasecmp(name, "COUNT"))
	  sscanf(data, "%d", &count);
	if (!g_strcasecmp(name, "INTERVAL"))
	  sscanf(data, "%d", &interval);
	if (!g_strcasecmp(name, "BYDAY"))
	  strncpy(bywdaylist, data, 256);
	if (!g_strcasecmp(name, "BYMONTHDAY"))
	  strncpy(bymodaylist, data, 256);
	if (!g_strcasecmp(name, "BYYEARDAY"))
	  strncpy(byyrdaylist, data, 256);
	if (!g_strcasecmp(name, "BYMONTH"))
	  strncpy(bymolist, data, 256);
      }
      pos = strstr(pos, ";");
      if (pos)
	pos++;
    }
    if (!g_strcasecmp(freq, "DAILY")) {
      g_string_sprintfa(newvalue, "D%d", interval);
      converted = TRUE;
    }
    if (strlen(bywdaylist)) {
      // Convert bywdaylist to 1.0
    }
    if (strlen(bywdaylist)) {
      char *tmp = sync_vtype_vcal2_list_to_vcal1(bywdaylist);
      strncpy(bywdaylist, tmp, 255);
      g_free(tmp);
    }
    if (strlen(bymodaylist)) {
      char *tmp = sync_vtype_vcal2_list_to_vcal1(bymodaylist);
      strncpy(bymodaylist, tmp, 255);
      g_free(tmp);
    }
    if (strlen(byyrdaylist)) {
      char *tmp = sync_vtype_vcal2_list_to_vcal1(byyrdaylist);
      strncpy(byyrdaylist, tmp, 255);
      g_free(tmp);
    }
    if (strlen(bymolist)) {
      char *tmp = sync_vtype_vcal2_list_to_vcal1(bymolist);
      strncpy(bymolist, tmp, 255);
      g_free(tmp);
    }
    if (!g_strcasecmp(freq, "WEEKLY")) {
      g_string_sprintfa(newvalue, "W%d", interval);
      if (strlen(bywdaylist)) {
	g_string_append(newvalue, " ");
	g_string_append(newvalue, bywdaylist);
      }
      converted = TRUE;
    }
    if (!g_strcasecmp(freq, "MONTHLY")) {
      if (strlen(bywdaylist)) {
	g_string_sprintfa(newvalue, "MP%d ", interval);
	g_string_append(newvalue, bywdaylist);
	converted = TRUE;
      } else if (strlen(bymodaylist)) {
	g_string_sprintfa(newvalue, "MD%d ", interval);
	g_string_append(newvalue, bymodaylist);
	converted = TRUE;
      }
    }    
    if (!g_strcasecmp(freq, "YEARLY")) {
      if (strlen(bymolist)) {
	g_string_sprintfa(newvalue, "YM%d ", interval);
	g_string_append(newvalue, bymolist);
	converted = TRUE;
      } else if (strlen(byyrdaylist)) {
	g_string_sprintfa(newvalue, "YD%d ", interval);
	g_string_append(newvalue, byyrdaylist);
	converted = TRUE;
      }
    }    
    if (untiltime > 0) {
      // Add until or count
      char *dt = sync_timet_to_dt_utc(untiltime);
      g_string_sprintfa(newvalue, " %s", dt);
      g_free(dt);
    } else {
      g_string_sprintfa(newvalue, " #%d", count);
    }
    if (converted) {
      // Conversion successful
      value = newvalue->str;
      g_string_free(newvalue, FALSE);
    } else {
      value = g_strdup(value);
      g_string_free(newvalue, TRUE);
    }
  } else {
    value = g_strdup(value);
  }
  return(value);
}    

char* sync_vtype_vcal1_list_to_vcal2(char **bits) {
  GString *buf = g_string_new("");
  char *value;
  int i;
  for (i = 0; bits[i]; i++) {
    int pos;
    char sign;
    if (i > 0)
      g_string_append(buf, ",");
    if (sscanf(bits[i], "%d%c" , &pos, &sign) == 2) {
      if (sign == '-')
	pos = -pos;
      g_string_sprintfa(buf, "%d", pos);
      if (bits[i+1] && !sscanf(bits[i+1], "%d", &pos)) {
	// A day follows
	g_string_sprintfa(buf, " %s", bits[i+1]);
	i++;
      }
    } else {
      g_string_append(buf, bits[i]);
    }
  }
  value = buf->str;
  g_string_free(buf, FALSE);
  return(value);
}

char* sync_vtype_vcal2_list_to_vcal1(char *in) {
  GString *buf = g_string_new("");
  gchar** bits = g_strsplit(in, ",", 256);
  char *value;
  int i;
  for (i = 0; bits[i]; i++) {
    int pos = 0;
    char day[16] = "";
    int ret = 0;
    if (i > 0)
      g_string_append(buf, " ");
    if ((ret = sscanf(bits[i], "%d %15s", &pos, day)) > 0) {
      if (pos > 0)
	g_string_sprintfa(buf, "%d+", pos);
      else
	g_string_sprintfa(buf, "%d-", -pos);
      if (ret >= 2)
	g_string_sprintfa(buf, " %s", day);
    } else {
      g_string_append(buf, bits[i]);
    }
  }
  g_strfreev(bits);
  value = buf->str;
  g_string_free(buf, FALSE);
  return(value);
}

char* sync_timet_to_dt(time_t t){
  struct tm *data;
  data = localtime(&t);
  return (g_strdup_printf("%04d%02d%02dT%02d%02d%02d", data->tm_year+1900,
			  data->tm_mon+1, data->tm_mday, data->tm_hour, 
			  data->tm_min, data->tm_sec));
}

char* sync_timet_to_dt_utc(time_t t){
  struct tm *data;
  data = gmtime(&t);
  return (g_strdup_printf("%04d%02d%02dT%02d%02d%02dZ", data->tm_year+1900,
			  data->tm_mon+1, data->tm_mday, data->tm_hour, 
			  data->tm_min, data->tm_sec));
}


time_t sync_dt_to_timet(char *str) {
  struct tm data;
  time_t dt;
  char z = ' ';
  tzset();
  data.tm_hour = 0;
  data.tm_min = 0;
  data.tm_sec = 0;
  if (!str)
    return 0;
  if (sscanf(str, "%4d%2d%2dT%2d%2d%2d%c", &data.tm_year,
	     &data.tm_mon, &data.tm_mday, &data.tm_hour, &data.tm_min,
	     &data.tm_sec, &z) >= 3) {
    data.tm_year -= 1900;
    data.tm_mon -= 1;
    data.tm_wday = 0;
    data.tm_yday = 0;
    // Removed by Bo Lincoln 2004-03-31. Bug or something that changed in
    // recent glibc?
    //if (z == 'Z')
    // data.tm_isdst = 0;
    //else
    data.tm_isdst = -1;
    dt = mktime(&data);
    if (z == 'Z') {
      struct tm *local;
      local = localtime(&dt);
      dt += local->tm_gmtoff; 
    }
  }
  return(dt);
} 

// Convert RFC2445 duration to seconds
time_t sync_dur_to_timet(char *str) {
  char *pos = str;
  char *end = NULL;
  char typestr[] = "D";
  char type = 0;
  int amt = 0;
  int sign = 1, dur = 0;
  

  if (!str)
    return(0);
  end = str+strlen(str);
  if (pos < end && pos[0] == '-') {
    sign = -1;
    pos++;
  } else if (pos < end && pos[0] == '+') {
    pos++;
  }
  if (pos < end && pos[0] == 'P') {
    pos++;
  } else
    return(sign*dur);
  
  if (pos < end && sscanf(pos, "%d%c", &amt, &type) == 2) {
    if (type == 'W')
      dur += 3600*24*7*amt;
    if (type == 'D')
      dur += 3600*24*amt;
    typestr[0] = type;
    pos = strstr(pos, typestr)+1;
  }
  if (pos < end && pos[0] == 'T') {
    pos++;
  } else
    return(sign*dur);
  while (pos < end && sscanf(pos, "%d%c", &amt, &type) == 2) {
    if (type == 'H')
      dur += 3600*amt;
    if (type == 'M')
      dur += 60*amt;
    if (type == 'S')
      dur += amt;
    typestr[0] = type;
    pos = strstr(pos, typestr)+1;
  }
  return(sign*dur);
}

// Convert seconds to RFC2445 DURATION
char* sync_timet_to_dur(time_t dur) {
  char *value;
  GString *ret = g_string_new("");

  if (dur < 0) {
    g_string_append(ret, "-");
    dur = -dur;
  }
  g_string_append(ret, "P");
  
  if (dur >= 3600*24) {
    int d = dur/(3600*24);
    dur -= d*3600*24;
    g_string_sprintfa(ret, "%dD", d);
  }
  if (dur > 0) {
    g_string_append(ret, "T");
    if (dur >= 3600) {
      int h = dur/3600;
      dur -= h*3600;
      g_string_sprintfa(ret, "%dH", h);
    }
    if (dur >= 60) {
      int m = dur/60;
      dur -= m*60;
      g_string_sprintfa(ret, "%dM", m);
    }
    if (dur > 0) {
      g_string_sprintfa(ret, "%dS", (int) dur);
      dur = 0;
    }
  }
  value = ret->str;
  g_string_free(ret, FALSE);
  return(value);
}


//Return a line of data from a vCARD, vCALENDAR etc. Free the string 
// using g_free().
char* sync_get_key_data(char *card, char *key) {
  char *pos = card;
  int l = strlen(key);
  char *res = g_strdup("");

  while (pos) {
    if (!strncmp(pos, key, l) && (pos[l] == ':' || pos[l] == ';')) {
      char *start, *end;
      start = strstr(pos+l, ":");
      if (start) {
	char *tmp, *line, *newstart;
	do {
	  start++;
	  end = strstr(start, "\n");
	  if (!end)
	    end = card+strlen(card);
	  newstart = end+1;
	  if (*(end-1) == '\r')
	    end--;
	  line = g_strndup(start, end-start);
	  tmp = g_strdup_printf("%s%s", res, line);
	  g_free(res);
	  g_free(line);
	  res = tmp;
	  start = newstart;
	} while(start < card+strlen(card) && start[0] == ' ');
	return(res);
      }
    }
    pos = strstr(pos, "\n");
    if (pos)
      pos += 1;
  }
  g_free(res);
  return(NULL);
}

gboolean sync_compare_key_data(char *obj1, char *obj2, char *key) {
  char *d1 = sync_get_key_data(obj1, key);
  char *d2 = sync_get_key_data(obj2, key);
  gboolean equal = FALSE;
  if (d1 && d2 && !g_strcasecmp(d1,d2))
    equal = TRUE;
  if (!d1 && !d2)
    equal = TRUE;
  g_free(d1);
  g_free(d2);
  return(equal);
}

gboolean sync_compare_key_times(char *obj1, char *obj2, char *key) {
  char *d1 = sync_get_key_data(obj1, key);
  char *d2 = sync_get_key_data(obj2, key);
  gboolean equal = FALSE;
  if (d1 && d2 && sync_dt_to_timet(d1) == sync_dt_to_timet(d2))
    equal = TRUE;
  if (!d1 && !d2)
    equal = TRUE;
  g_free(d1);
  g_free(d2);
  return(equal);
}

void sync_append_data_line(GString *out, char* card, char *key, char *title) {
  char *data = sync_get_key_data(card, key);
  if (data) {
    g_string_sprintfa(out, "%s: %s\n", title, data);
    g_free(data);
  }
}
