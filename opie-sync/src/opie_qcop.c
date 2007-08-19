/* 
   MultiSync Opie Plugin - Synchronize Opie/Zaurus Devices
   Copyright (C) 2003 Tom Foottit <tom@foottit.com>
                      Eike M. Lang <mail@elang.de> 

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
 *  $Id: opie_qcop.c,v 1.6 2004/02/18 23:55:48 irix Exp $
 */


/* 
 * Handles interaction with the QCopBridgeServer running on most Opie
 * devices. Almost all credit must go to Holger "Zecke" Freyther who
 * first implemented all of this in kitchensync 
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <glib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "opie_qcop.h"

#define OPIEQCOP_BUFLEN (1024)   /* maximum response size  */
#define OPIEQCOP_PORT   (4243)   /* port the the QCopBridge server runs on */


/* fxn decls */
void monitor_thread_main(qcop_monitor_data* data);
char* receive_all(qcop_conn* qconn);
gboolean send_allof(qcop_conn* qconn, char* str);
gboolean expect(qcop_conn* qconn, char* str, char *failstr, char* errmsg);

/* globals */
pthread_t monitor_thd = 0;    

        
/* get_line
 *  
 * helper function to receive a all characters up to the next newline from the socket
 * caller is responsible for g_free()ing the returned string resp. for checking that
 * a string was returned at all.
 */
gchar* get_line(qcop_conn* qconn)
{
  GString* buffer;
  char* retval = NULL;
  gchar* curr_char;
  ssize_t received;  

  buffer = g_string_new("");
  curr_char = g_strndup("",1);
  while ((received = read(qconn->socket, curr_char, 1) && !(strstr(curr_char, "\n"))))
  {
    if(received < 0)
    {
      /* TODO - recv failed - log */
      
      break;
    }
    else
      g_string_append(buffer,curr_char);
  }
  
  if(buffer->str && (buffer->len > 0))
  {
    retval = g_strdup(buffer->str);
    g_free(curr_char);
    g_string_free(buffer,TRUE);
  }
  else retval = NULL;
  
  /* caller must g_free() retval */
  return(retval);
}


/* send_allof
 * 
 * helper function to send strings of arbitrary length to socket
 * returns:  
 */
gboolean send_allof(qcop_conn* qconn, char* str)
{
  int len,bytes_sent;
  char* msg;
  gboolean rc = TRUE;
  
  if(str)
  {
    msg = g_strdup(str);
    bytes_sent = 0;
    len = strlen(str);
    while (bytes_sent < len) 
    {
      bytes_sent = send(qconn->socket, str, (len - bytes_sent), 0);
      if(bytes_sent < 0)
      {
        /* TODO - error log */
        rc = FALSE;
        break;
      }
      else
      {
        str = str + bytes_sent;
        len = strlen(str);
      }
    }
    g_free(msg);
  }
  
  return rc;
}

/*
 * Helper that waits for a line from the qcop server which contains the
 * substring str.
 * We assume that either the string will occur eventually or the remote
 * host will close the connection. If failstr is specified and it is found
 * then the function will return immediately.
 * Will set the resultmsg component of the qcop_conn struct to the 
 * value of the errmsg parameter.
 */

gboolean expect(qcop_conn* qconn, char* str, char* failstr, char* errmsg)
{
  char* pc;

  pc = get_line(qconn);
  if (pc) /* did we get anything at all? */
  {
    if (strstr(pc,str)) /* wait for desired substring */
    {
      g_free(pc);
      return TRUE;
    }
    else
    {
      if (failstr && (strlen(failstr) > 0) && (strstr(pc, failstr))) {
        qconn->resultmsg = g_strdup(errmsg);
        return FALSE;
      }
      
      if (strstr(pc,"cancelSync"))
      {
        g_free(pc);
        qconn->resultmsg = g_strdup("User cancelled sync");
        return FALSE;
      }
      else
      {
        g_free(pc);
        return expect(qconn, str, failstr, errmsg);
      }
    }
  }
  else
  {
    qconn->resultmsg = g_strdup(errmsg);
    return FALSE;
  }
}

/* Waits for a line from the qcop server that must match one of the substrings
 * 599 or 200. If flushing is TRUE a matching 200 implies matching flushDone in
 * the following line.
 */
gboolean expect_special(qcop_conn* qconn, char* errmsg, gboolean flushing)
{
  char* pc;

  pc = get_line(qconn);
  if (pc)
  {
    if (strstr(pc,"599") || strstr(pc,"200"))
    {
      if(strstr(pc,"200") && flushing)
      {
        g_free(pc);
        return expect(qconn, "flushDone", NULL, errmsg);
      }
      else
      {
        g_free(pc);
        return TRUE;
      }
    }
    else
    {
      if (strstr(pc,"cancelSync"))
      {
        g_free(pc);
        qconn->resultmsg = g_strdup("User cancelled sync");
        return FALSE;
      }
      else
      {
        g_free(pc);
        return expect_special(qconn, errmsg, flushing);
      }
    }
  }
  else
  {
    qconn->resultmsg = g_strdup(errmsg);
    return FALSE;
  }
}

/* 
 * Establish a connection to the qcop bridge server if one does not yet exist and return
 * the connection struct. Will be freed by the disconnect() method
 * 
 */
qcop_conn* qcop_connect(gchar* addr, gchar* username, gchar* password)
{
  struct sockaddr_in host_addr;
  qcop_conn* qconn = NULL;

  host_addr.sin_family = AF_INET;                            /* host byte order */
  host_addr.sin_port = htons(OPIEQCOP_PORT);                 /* short, network byte order */
  host_addr.sin_addr.s_addr = inet_addr(addr);
  memset(&(host_addr.sin_zero), '\0', 8);                    /* zero the rest of the struct */

  qconn = g_malloc0(sizeof(qcop_conn));

  qconn->socket = socket(AF_INET, SOCK_STREAM, 0);
  qconn->result = FALSE; /* Only set to TRUE if successful */
  if(qconn->socket < 0) 
  {
    qconn->resultmsg = g_strdup_printf("Could not create socket: %s", strerror(errno));
    return qconn;
  }

  /* connect to the QCopBridgeServer */
  if (connect(qconn->socket, (struct sockaddr *)&host_addr, sizeof(host_addr)) != -1) 
  {
    
    if (expect(qconn, "220", NULL, "Failed to log into server - please check sync security settings on device"))
    {
      send_allof(qconn, "USER ");
      send_allof(qconn, username);
      send_allof(qconn, "\n");
    }
    else
      return qconn;

    if(expect(qconn, "331", "530", "Failed to log into server - please check username"))
    {
      send_allof(qconn, "PASS ");
      send_allof(qconn, password);
      send_allof(qconn, "\n");
    }
    else
      return qconn;
    
    if(!expect(qconn, "230", "530", "Failed to log into server - please check username / password"))
      return qconn;
    
    /* connected OK */
    qconn->result = TRUE;
  }
  else 
    qconn->resultmsg = g_strdup_printf("Could not connect to server: %s", strerror(errno));
  
  return qconn;  
}


/*
 *
 */
void qcop_disconnect(qcop_conn* qconn)
{
  if(qconn)
  {
    if(qconn->socket)
    { 
      send_allof(qconn, "QUIT\n");
      close(qconn->socket);
    }
    
    qcop_freeqconn(qconn);
  }
}


/*
 *
 */
void qcop_freeqconn(qcop_conn* qconn)
{
  if (qconn)
  {
    if (qconn->resultmsg)
      g_free(qconn->resultmsg);
    g_free(qconn); 
  }
}


/*
 *
 */
char* qcop_get_root(qcop_conn* qconn)
{
  gchar* temp = NULL;
  char* start;
  gchar* pc;
  
  send_allof(qconn, "CALL QPE/System sendHandshakeInfo()\n");
  
  if (!expect(qconn, "200", NULL, "Failed to obtain HandshakeInfo"))
    return NULL;
  pc = get_line(qconn);
  if(!strstr(pc, "handshakeInfo(QString,bool)"))
  {
    qconn->resultmsg = g_strdup_printf("Unrecognised response: %s", pc);
    g_free(pc);
    return NULL;
  }
    
  if ((start=strstr(strstr(pc,"/")+1,"/"))) /* We need the second slash */
  { 
    /* caller responsible for free()ing temp */
    temp = g_strndup(start,strstr(start," ")-start); /* from slash to blank is our path */
  }
  else if((start=strstr(pc,") ")+2))
  {
    /* Qtopia sends back a base64 encoded utf-16 (big-endian) string */
    guchar *decoded;
    char *startc;
    gsize len = 0;
    gsize len2 = 0;
    GError *err = NULL;

    decoded = g_base64_decode(start, &len);
    if(len > 0) {
      /* first four bytes seem to be \0 \0 \0 (string length) */
      len = decoded[3];
      startc = decoded + 4;
      temp = g_convert(startc, len, "UTF8", "UTF16BE", NULL, &len2, &err);
      if (err != NULL) {
        fprintf(stderr, "UTF16 convert error: %s\n", err->message);
        g_error_free(err);
        if(temp) {
          /* Don't accept partial conversions */
          g_free(temp);
          temp = NULL;
        }
      }
    }
  }

  if(!temp)
    qconn->resultmsg = g_strdup_printf("Unrecognised response: %s", pc);

  g_free(pc);  
  return temp;
}


/*
 *
 */
void qcop_start_sync(qcop_conn* qconn, void (*cancel_routine)())
{
  qcop_monitor_data* data;

  qconn->result = FALSE; /* ..until proven otherwise */

   /* first lock the UI */
  send_allof(qconn, "CALL QPE/System startSync(QString) OpenSync\n");
  if(!expect(qconn, "200", NULL, "Failed to bring up sync screen!"))
    return;
  /* Flush addressbook to storage */   
  send_allof(qconn, "CALL QPE/Application/addressbook flush()\n");
  if (!expect_special(qconn,"Failed to flush addressbook",TRUE))
    return;
  /* Flush datebook to storage */
  send_allof(qconn, "CALL QPE/Application/datebook flush()\n");
  if (!expect_special(qconn,"Failed to flush datebook",TRUE))
    return;
  /* Flush todolist to storage */
  send_allof(qconn, "CALL QPE/Application/todolist flush()\n");
  if (!expect_special(qconn,"Failed to flush todolist",TRUE))
    return;
  
  /* spawn the monitor thread */
  data = g_malloc0(sizeof(qcop_monitor_data));
  data->qconn = qconn;
  data->cancel_routine = cancel_routine;
  qconn->syncing = TRUE;
  pthread_mutex_init(&qconn->access_mutex,NULL);
  if(pthread_create(&monitor_thd, 
                    NULL, 
                    (void *)monitor_thread_main, 
                    (void*)data) != 0)
  {
    /* error creating thread */
    /* TODO - log */ 
  }
  qconn->result = TRUE;
  return;  
}


/*
 *
 */
void qcop_stop_sync(qcop_conn* qconn)
{ 
  if(qconn && qconn->syncing)
  {     
    pthread_mutex_lock(&qconn->access_mutex);
    qconn->result = FALSE; /* ..until... */
    qconn->syncing = FALSE;
    pthread_mutex_unlock(&qconn->access_mutex);
    if (monitor_thd)
        pthread_join(monitor_thd,NULL);
    pthread_mutex_destroy(&qconn->access_mutex);
    /* Reload addressbook data */
    send_allof(qconn, "CALL QPE/Application/addressbook reload()\n");
    if(!expect_special(qconn,"Failed to reload addressbook",FALSE))
      return;
    /* Reload datebook data */
    send_allof(qconn, "CALL QPE/Application/datebook reload()\n");
    if(!expect_special(qconn,"Failed to reload datebook",FALSE))
      return;
    /* Reload todolist data */
    send_allof(qconn, "CALL QPE/Application/todolist reload()\n");
    if(!expect_special(qconn,"Failed to reload todolist",FALSE))
      return;
    /* unlock the GUI */
    send_allof(qconn, "CALL QPE/System stopSync()\n");
    if(!expect(qconn, "200", NULL, "Failed to close sync screen"))
      return;
    qconn->result = TRUE;
  }
  
  return;
}


/*
 *
 */
void monitor_thread_main(qcop_monitor_data* data)
{
  /* listen on the socket and read the input, looking for the cancel
   * when the cancel is found, call the supplied cancel routine
   */
  fd_set qcop_socket;
  struct timeval tv;
  int retval;
  gchar* mon_pc;

  FD_ZERO(&qcop_socket);
  FD_SET(data->qconn->socket,&qcop_socket);

  while(1)
  {
    /* qconn is ours for the time being */
    pthread_mutex_lock(&data->qconn->access_mutex);
    
    /* wait for one second with each select() */
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    retval=select(data->qconn->socket+1,&qcop_socket,NULL,NULL,&tv);
    if (retval > 0)
    {
      mon_pc = get_line(data->qconn);
      if ( mon_pc )
      {
        if (strstr(mon_pc,"cancelSync()"))
        {
          g_free(mon_pc);
          data->qconn->syncing = FALSE;
          (data->cancel_routine)();
        }
        else 
        {
          perror("Error on select() call or no data");
          g_free(mon_pc);
        }
      }
    }
    
    /* we are done */
    if (!data->qconn->syncing)
    {
      pthread_mutex_unlock(&data->qconn->access_mutex);
      g_free(data);
      pthread_exit(NULL);
    }
    
    /* unlock for a moment allow access for other threads */
    pthread_mutex_unlock(&data->qconn->access_mutex);
    sleep(1);
  }
 
}
