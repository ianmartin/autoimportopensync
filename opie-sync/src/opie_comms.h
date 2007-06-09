#ifndef _OPIE_COMMS_H_
#define _OPIE_COMMS_H_

/* 
   Derived from the MultiSync Opie Plugin
   Copyright (C) 2007 Paul Eggleton <bluelightning@bluelightning.org>
   Copyright (C) 2003 Tom Foottit <tom@foottit.com>

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


#include "opie_sync.h"

#define OPIE_ADDRESS_FILE  "Applications/addressbook/addressbook.xml"
#define OPIE_TODO_FILE     "Applications/todolist/todolist.xml"
#define OPIE_CALENDAR_FILE "Applications/datebook/datebook.xml"
#define OPIE_CATEGORY_FILE "Settings/Categories.xml"


/* initialize and cleanup the comms layer - call only once per plugin */
void comms_init();
void comms_shutdown();


/* connect to the device and pull down the data */
gboolean opie_fetch_sink(OpieSinkEnv *env);
gboolean opie_fetch_file(OpiePluginEnv *env, OPIE_OBJECT_TYPE objtype, const char *remotefile, xmlDoc **doc, OSyncObjTypeSink *sink);

/* connect to the device and push the files back */
gboolean opie_put_sink(OpieSinkEnv *env);
gboolean opie_put_file(OpiePluginEnv *env, OPIE_OBJECT_TYPE objtype, const char *remotefile, xmlDoc *doc);


#endif
