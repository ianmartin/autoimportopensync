#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "../gpesync_client.h"

int portno = 6446;

int main (int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf (stderr, "usage: %s TCP|SSH host [port]\n", argv[0]);
    exit (0);
  }
  gpesync_client *ctx = NULL;
  char buffer[BUFFER_LEN];
  char *mode = argv[1];
  char *server = argv[2];
  char *error;
  GString *result=NULL;
  
  if (argc == 4)
    portno = atoi (argv[3]);
 
  bzero (buffer, BUFFER_LEN);
  if (!strcasecmp(mode, "ssh"))
  {
    ctx = gpesync_client_open_ssh (server, &error);
  }
  else if (!strcasecmp (mode, "tcp"))
  {
    ctx = gpesync_client_open (server, portno, &error);
  }
  else
  {
    fprintf (stderr, "Invalid mode: %s. Please use TCP or SSH\n", mode);
    return -1;
  }

  if (!ctx)
  {
    fprintf (stderr, "Error connecting to %s:%d\n", server, portno);
    fprintf (stderr, "Message: %s\n", error);
    return -1;
  }
   
  printf ("Login successful!\n");
  printf ("Please specify command: ");
  fgets (buffer, BUFFER_LEN, stdin);
  result = g_string_new ("");

  while (strncasecmp (buffer, "quit", 4))
  {
    gpesync_client_exec_printf (ctx, buffer, client_callback_gstring, &result, NULL);
    printf ("> %s", result->str);

    memset (buffer, 0, BUFFER_LEN);
    printf ("Please specify command: ");
    fgets (buffer, BUFFER_LEN, stdin);
    g_string_assign (result, "");
  }

  g_string_free (result, TRUE);
  gpesync_client_close (ctx);

  return 0;
}
