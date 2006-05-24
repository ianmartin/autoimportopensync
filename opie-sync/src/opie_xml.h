#ifndef _OPIE_XML_H_
#define _OPIE_XML_H_

/* 
   MultiSync Opie Plugin - Synchronize Opie/Zaurus Devices
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

/*
 *  $Id: opie_xml.h,v 1.1 2003/07/11 21:21:04 irix Exp $
 */

#include <glib.h>

#include "opie_comms.h"
#include "opie_sync.h"

/* pull the calendar data out of the file and parse it */
void parse_cal_data(char* cal_file, 
                    GList** calendar);


/* pull the contact data out of the file and parse it */
void parse_contact_data(char* contact_file, 
                        GList** contacts);


/* pull the todo data out of the file and parse it */
void parse_todo_data(char* todo_file, 
                     GList** todos);


/* pull the category data out of the file and parse it */
void parse_category_data(char* cat_file,
                         GList** categories);


/* convert calendar GList to xml string - caller must free */
char* cal_data_to_xml(OpieSyncEnv* env, GList* calendar);


/* convert contact GList to xml string - caller must free */
char* contact_data_to_xml(OpieSyncEnv* env, GList* contacts);

/* convert todo GList to xml string - caller must free */
char* todo_data_to_xml(OpieSyncEnv* env, GList* todos);


/* convert category GList to xml string - caller must free */
char* category_data_to_xml(OpieSyncEnv* env, GList* categories);

#endif
