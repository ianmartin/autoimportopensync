/*
 * Test program for vobject parsing. Takes input from file and
 * writes VObject tree.
 *
 * usage: vctest <filename>
 *
 * $Id: vctest.c,v 1.1 2003/07/11 20:57:08 irix Exp $
 */

#include <stdio.h>
#include <string.h>
#include "vcc.h"
#include "vobject.h"

FILE *cfp;

void myMimeErrorHandler(char *s)
{
  printf("%s\n", s);
}

int main(int argc, char **argv)
{

#ifdef _VCTEST_CONSOLE
  cfp = stdout;
  registerMimeErrorHandlerO(myMimeErrorHandler);
#else
  cfp = fopen("vctest.out", "w");
  if (!cfp) return;
#endif
    
  ++argv;
  while (--argc) 
  {
    FILE *fp;
    fprintf(cfp,"processing %s\n",*argv);
    fp = fopen(*argv,"r");

    if (!fp) 
    {
      fprintf(cfp,"error opening file\n");
    }
    else 
    {
      VObjectO *v, *t;
      FILE *ofp;
      char buf[256];
      char *p;
      strcpy(buf,*argv);
      p = strchr(buf,'.');
      if (p) *p = 0;
      strcat(buf,".out");
      fprintf(cfp,"reading text input from '%s'...\n", *argv);
      v = Parse_MIME_FromFileNameO(*argv);
      writeVObjectToFileO(buf,v);
      cleanVObjectO(v);
    }

    cleanStrTblO();
    argv++;

  } /* while */

  if (cfp != stdout) fclose(cfp);
  
  return 0;
}

