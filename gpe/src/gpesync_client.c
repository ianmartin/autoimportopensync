/*
 * Copyright (C) 2005 Martin Felis <martin@silef.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Parts of this file are derieved from Phil Blundells libnsqlc.
 * See http://handhelds.org/cgi-bin/cvsweb.cgi/gpe/base/libnsqlc/
 * for more information.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdarg.h>

#include "gpesync_client.h"

int verbose = 0;

struct gpesync_client_query_context
{
  gpesync_client *ctx;

  int type;

  gpesync_client_callback callback;
  void *cb_data;

  int result;
  gchar *error;
  gboolean aborting;
};


static void
write_command (gpesync_client * ctx, const char *buf)
{
  if (verbose)
    fprintf (stderr, "[gpsyncclient %s]: %s\n", __func__, buf);

  if (write (ctx->outfd, buf, strlen (buf)) == -1 && verbose)
    fprintf (stderr, "[gpsyncclient %s]: failed\n", __func__);
}

/*! \brief moves forward to the next line
 *
 * \param data	The string on which we want to move forward.
 *
 * \returns	A pointer to the beginning of the next line
 * 		or on '\0'
 */
char *
get_next_line (const char *data, gsize *len)
{
  GString *string;
 
  if (len)
    *len = 0;

  string = g_string_new (NULL);
  
  while ((data[*len] != '\n') && (data[*len] != '\0'))
  {
    string = g_string_append_c (string, data[*len]);
    *len += 1;
  }

  if (data[*len] == '\n')
    {
      string = g_string_append_c (string, data[*len]);
      *len += 1;
    }
  
  if (!string->str || string->str[0] == '\0')
  {
    g_string_free (string, TRUE);
    return NULL;
  }
  
  return g_string_free (string, FALSE);
}

static void
read_lines (struct gpesync_client_query_context *query_ctx, char *data)
{
  if (!query_ctx->aborting)
    {
      if (verbose)
	fprintf (stderr, "[gpesync_client read_lines] \n<%s>\n", data);
      GSList *lines = NULL, *iter;

      int argc, i;
      char **argv = NULL;
      char *line, *line_iter;

      /* We need to split the data in the sperate lines, so that
       * we can read it as lists. */
      gsize count = 0;
      line_iter = data;
      line = get_next_line (line_iter, &count);
      do
        {
	  lines = g_slist_append (lines, line);
	  line_iter += count;
	  line = get_next_line (line_iter, &count);
	}
      while (line);

      argc = g_slist_length (lines);
      argv = g_malloc0 (sizeof (char *) * argc);
      
      iter = lines;
      for (i = 0; i < argc; i++)
	{
	  argv[i] = iter->data;
	  iter = g_slist_next (iter);
	}

      if ( (query_ctx->callback != NULL) && (query_ctx->callback (query_ctx->cb_data, argc, argv)))
	{
	  fprintf (stderr, "aborting query\n");
	  query_ctx->result = GPESYNC_CLIENT_ABORT;
	  query_ctx->aborting = TRUE;
	}

      memset (argv, 0, sizeof (char *) *argc);
      g_free (argv);

      iter = lines;
      while (iter)
      {
	g_free (iter->data);
	iter = iter->next;
      }
      g_slist_free (lines);
    }
}

static void
read_response (struct gpesync_client_query_context *query_ctx)
{
  int len = 0;
  gboolean have_len = FALSE;
  gpesync_client *ctx;
  GString *buf;

  ctx = query_ctx->ctx;
  buf = g_string_new ("");
  if (!ctx->socket)
  {
    /* Reading from the ssh session. */
    for (;;)
      {
        int rc;
        char c;

        rc = read (ctx->infd, &c, 1);
        if (rc < 0)
          {
            perror ("read");
	    ctx->busy = 0;
            break;
          }

        if (have_len == FALSE)
	  {
	    if (c == ':')
	      {
	        len = atoi (buf->str);
	        have_len = TRUE;
	        g_string_assign (buf, "");
		if (verbose)
		  fprintf (stderr, "[gpesync_client read_response] length:%d\n", len);
	        continue;
	      }
          }
        else
	  {
	    if (buf->len == len - 1)
	      {
	        g_string_append_c (buf, c);
	        break;
	      }
	  }
        g_string_append_c (buf, c);
      }
  } else {
    /* Reading from the tcp/ip socket. */
    int bytesread=BUFFER_LEN-1;
    char buffer [BUFFER_LEN];
    
    while (bytesread == BUFFER_LEN -1)
    {
      bzero (buffer, BUFFER_LEN);
      bytesread = recv (ctx->socket, buffer, BUFFER_LEN-1, 0);
      if (bytesread < 0)
      {
	perror ("Reading");
	exit (1);
      }
      g_string_append_len (buf, buffer, bytesread);
    }
  }   

  if (ctx->busy)
    {
      read_lines (query_ctx, buf->str);
      g_string_free (buf, TRUE);
      buf = NULL;
      ctx->busy = 0;
    }
}

gpesync_client *
gpesync_client_open_ssh (const char *addr, gchar **errmsg)
{
  gpesync_client *ctx;
  gchar *hostname = NULL;
  gchar *username = NULL;
  gchar *str;
  gchar *p;

  int in_fds[2], out_fds[2];
  pid_t pid;

  str = g_strdup (addr);

  p = strchr (str, '@');
  if (p)
    {
      *p = 0;
      hostname = p + 1;
      username = str;
    }
  else
    hostname = str;

  if (hostname == NULL)
    hostname = "localhost";

  if (username == NULL)
    username = (gchar *) g_get_user_name ();

  ctx = g_malloc0 (sizeof (gpesync_client));

  if (pipe (in_fds) && verbose)
     fprintf(stderr, "[gpsyncclient %s]: pipe failed.\n", __func__);

  if (pipe (out_fds) && verbose)
     fprintf(stderr, "[gpsyncclinet %s]: pipe fialed.\n", __func__);

  pid = fork ();
  if (pid == 0)
    {
      dup2 (out_fds[0], 0);
      dup2 (in_fds[1], 1);
      close (out_fds[1]);
      close (in_fds[0]);
      if (verbose)
        fprintf (stderr, "connecting as %s to %s filename: %s\n", username, hostname, "gpesyncd");
      execlp ("ssh", "ssh", "-l", username, hostname, "gpesyncd", "--remote",
	      NULL);
      perror ("exec");
    }

  close (out_fds[0]);
  close (in_fds[1]);

  ctx->outfd = out_fds[1];
  ctx->infd = in_fds[0];

  ctx->hostname = g_strdup(hostname);
  ctx->username = g_strdup(username);
 
  g_free (str);
  
  return ctx;
}

gpesync_client *
gpesync_client_open_local (gchar **errmsg)
{
  gpesync_client *ctx;

  int in_fds[2], out_fds[2];
  pid_t pid;

  ctx = g_malloc0 (sizeof (gpesync_client));

  if (pipe (in_fds) && verbose)
     fprintf(stderr, "[gpsyncclient %s]: pipe failed.\n", __func__);

  if (pipe (out_fds) && verbose)
     fprintf(stderr, "[gpsyncclinet %s]: pipe fialed.\n", __func__);

  pid = fork ();
  if (pid == 0)
    {
      dup2 (out_fds[0], 0);
      dup2 (in_fds[1], 1);
      close (out_fds[1]);
      close (in_fds[0]);
      if (verbose)
        fprintf (stderr, "connecting to gpesyncd locally");
      execlp ("gpesyncd", "gpesyncd", "--remote",
	      NULL);
      perror ("exec");
    }

  close (out_fds[0]);
  close (in_fds[1]);

  ctx->outfd = out_fds[1];
  ctx->infd = in_fds[0];

  return ctx;
}

gpesync_client *
gpesync_client_open (const char *addr, int port, gchar **errmsg)
{
  gpesync_client *ctx;
  ctx = g_malloc0 (sizeof (gpesync_client));

  struct hostent *he;
  struct sockaddr_in server_addr;

  if ((he = gethostbyname (addr)) == NULL)
  {
    herror ("gethostbyname");
    exit (1);
  }

  if ((ctx->socket = socket (PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("socket");
    exit (1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons (port);
  server_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset (&(server_addr.sin_zero), '\0', 8);

  if (connect (ctx->socket, (struct sockaddr *) &server_addr,
	sizeof (struct sockaddr)) == -1)
  {
    perror ("connect");
    exit (1);
  }

  char buffer[BUFFER_LEN];
  bzero (buffer, BUFFER_LEN);

  if (read (ctx->socket, buffer, 255) < 0) {
	  perror ("read");
	  exit (1);
  }

  if (strcasecmp (buffer, "OK\n"))
  {
    if (errmsg)
    {
      *errmsg = g_strdup (buffer);
    }
    gpesync_client_close (ctx);
    return NULL;
  }
 
  return ctx;
}

void
gpesync_client_close (gpesync_client * ctx)
{
  g_free (ctx->hostname);
  g_free (ctx->username);
  
  close (ctx->infd);
  if (ctx->infd != ctx->outfd)
    close (ctx->outfd);

  if (ctx->socket)
    shutdown (ctx->socket, SHUT_RDWR);
  ctx->socket = 0;

  g_free (ctx);
//  ctx = NULL;
}

int
gpesync_client_exec (gpesync_client * ctx, const char *command,
		 gpesync_client_callback cb, void *cb_data, gchar **err)
{
  struct gpesync_client_query_context query;
  GString *cmd = g_string_new ("");

  memset (&query, 0, sizeof (query));
  query.ctx = ctx;
  query.callback = cb;
  query.cb_data = cb_data;
  query.aborting = FALSE;
  query.result = 0;
  query.error = NULL;

  if (!ctx->socket)
  {
    g_string_append_printf (cmd, "%d:%s", (unsigned int) strlen (command), command);

    write_command (ctx, cmd->str);

  } else {
    int bytes_sent=0, n=0;
    
    while (bytes_sent < strlen (command))
    {
      n = send (ctx->socket, command + bytes_sent, strlen (command) - bytes_sent, 0);
      if (n < 0)
      {
	perror ("sending");
        exit (1);
      }
      bytes_sent += n;
    }
  }

  ctx->busy = 1;

  while (ctx->busy)
    read_response (&query);

  if (err)
    *err = query.error;

  g_string_free (cmd, TRUE);

  return query.result;
}

int client_callback_print (void *arg, int argc, char **argv)
{
  int i;
  for (i = 0; i < argc; i++)
	{
	  printf ("%s", argv[i]);
	}
	
  return 0;
}

int client_callback_list (void *arg, int argc, char **argv)
{
  int i;
  GSList  **data_list = (GSList **) arg;
 
  for (i = 0; i < argc; i++)
  {
    *data_list = g_slist_append (*data_list, g_strdup (argv[i]));
  }	
  return 0;
}

int client_callback_string (void *arg, int argc, char **argv)
{
  int i;
  gchar  **data_str = (gchar **) arg;
 
  for (i = 0; i < argc; i++)
	{
	  if (*data_str == NULL)
	    *data_str = g_malloc0 (sizeof (gchar) * strlen (argv[i]) + 1);
	  else
	  {
	    *data_str = g_realloc (*data_str, strlen (*data_str) + strlen (argv[i]) + 1); 
	  }

	  *data_str = strcat (*data_str, argv[i]);
	  *data_str = strcat (*data_str, "\0");
	}
	
  return 0;
}

int client_callback_gstring (void *arg, int argc, char **argv)
{
  int i;
  GString  **data_str = (GString **) arg;

  for (i = 0; i < argc; i++)
	{
	  g_string_append (*data_str, argv[i]);
	}
	
  return 0;
}

int gpesync_client_exec_printf(
  gpesync_client *db,                   /* A connected client */
  const char *query_format,        /* printf-style format string for query
  	                              to gpesyncd.*/
  gpesync_client_callback xCallback,    /* Callback function */
  void *pArg,                   /* 1st argument to callback function */
  char **errmsg,                /* Error msg written here */
  ...                           /* Arguments to the format string. */
){
  va_list ap;
  char *buf = NULL;
  int rc;

  va_start(ap, errmsg);
  g_vasprintf (&buf, query_format, ap);
  va_end(ap);

  rc = gpesync_client_exec (db, buf, xCallback, pArg, errmsg);

  free (buf);
  return rc;
}

