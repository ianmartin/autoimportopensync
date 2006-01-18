#ifndef _OPIE_COMMS_H_
#define _OPIE_COMMS_H_

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
 *  $Id: opie_comms.h,v 1.5 2004/02/20 15:55:14 irix Exp $
 */


#include "opie_sync.h"

/* opie connection definition */
/*
typedef struct {
  client_connection commondata;
  sync_pair *sync_pair;

  opie_conn_type conn_type;
  opie_device_type device_type;

  char* device_addr;
  unsigned int device_port;
  gboolean enable_qcop;
  char* username;
  char* password;
} opie_conn;
*/

typedef enum {
  OPIE_OBJECT_TYPE_UNKNOWN = 0x00,
  OPIE_OBJECT_TYPE_CALENDAR = 0x01,
  OPIE_OBJECT_TYPE_PHONEBOOK = 0x02,
  OPIE_OBJECT_TYPE_TODO = 0x04,
  OPIE_OBJECT_TYPE_ANY = 0xff
} opie_object_type;


/* calendar event alarm time units */
typedef enum {
  ALARM_MIN=1,
  ALARM_HOUR=60,
  ALARM_DAY=1440
} alarm_time_type;


/* calendar event alarm action */
typedef enum {
  /* Evo has display a message,
   * play a sound or run a program.
   * Opie has silent or loud alarm - map
   * both onto 'display a message' for now.
   */
  ALARM_ACTION_LOUD=1,
  ALARM_ACTION_SILENT=2
} alarm_action_type;


/* calendar event alarm */
typedef struct {
  unsigned int duration;
  alarm_time_type time_type;
  char* related;
  alarm_action_type action_type;
  char* desc;
} alarm_data; 


/* recurrence type */
typedef enum {
  RECURRENCE_DAILY=1,
  RECURRENCE_WEEKLY=2,
  RECURRENCE_MONTHLY=3,
  RECURRENCE_YEARLY=4
} recurrence_type;


/* recurrence definition */
typedef struct {
  recurrence_type type;
  unsigned int frequency;
  unsigned int position;
  time_t end_date;
  /* weekdays is a bitmask with the MSB 
   * as Sunday and the LSB as Saturday */
  short weekdays;
} recurrence_data;


/* generic a/v pair that we don't know anything about */
typedef struct {
  char* attr;
  char* val; 
} anon_data;


/* calendar data record */
typedef struct {
  char* uid;
  GList* cids;
  unsigned int rid;
  unsigned int rinfo;
  char* summary;
  char* desc;
  time_t start_date;
  time_t end_date;
  time_t created_date;
  gboolean all_day;
  char* location;
  /* opie datebook only has 1 alarm per entry */
  alarm_data* alarm;   
  recurrence_data* recurrence;  
  GList* anons;
} cal_data;


/* contact data record */
typedef struct {
  char* uid;
  GList* cids;
  unsigned int rid;
  unsigned int rinfo;
  char* first_name;
  char* middle_name;
  char* last_name;
  char* suffix;
  char* file_as;
  char* department;
  char* company;
  GList* emails;
  char* default_email;
  char* home_phone;
  char* home_fax;
  char* home_mobile;
  char* home_street;
  char* home_city;
  char* home_state;
  char* home_zip;
  char* home_country;
  char* home_webpage;
  char* business_phone;
  char* business_fax;
  char* business_mobile;
  char* business_pager;
  char* business_street;
  char* business_city;
  char* business_state;
  char* business_zip;
  char* business_country;
  char* business_webpage;
  char* spouse;
  int gender;
  char* birthday;
  char* anniversary;
  char* nickname;
  char* children;
  char* notes;
  char* assistant;
  char* manager;
  char* office;
  char* profession;
  char* jobtitle;
  GList* anons;
} contact_data;


/* todo data record */
typedef struct {
  char* uid;
  GList* cids;
  unsigned int rid;
  unsigned int rinfo;
  char* completed;
  char* hasdate;
  char* dateyear;
  char* datemonth;
  char* dateday;
  char* priority;
  char* progress;
  char* desc;
  char* summary;
  GList* anons;
} todo_data;


/* category record */
typedef struct {
  char* cid;
  char* category_name;
} category_data;


/* initialize and cleanup the comms layer - call only once per plugin */
void comms_init();
void comms_shutdown();


/* given a category id, find the category name */
const char* opie_find_category(const char* cid, GList* categories);


/* add a category if it is not already in the list 
 * and return the new cid, or if it is already in 
 * the list just return the cid
 */
const char* opie_add_category(const char* name, GList** categories);


/* connect to the device and pull down the data */
gboolean opie_connect_and_fetch(OpieSyncEnv* env,
                                opie_object_type object_types,
                                GList** calendar,
                                GList** contacts,
                                GList** todos,
                                GList** categories);


/* connect to the device and push the file back */
gboolean opie_connect_and_put(OpieSyncEnv* env,
                              char* contacts_file,
                              opie_object_type obj_type); 


/* convert a cal_data linked list into a string representation */
char* serialize_cal_data(OpieSyncEnv* env, GList* calendar);


/* convert a contact_data linked list into a string representation */
char* serialize_contact_data(OpieSyncEnv* env, GList* contacts);


/* convert a todo_data linked list into a string representation */
char* serialize_todo_data(OpieSyncEnv* env, GList* todos);


/* convert a category_data linked list into a string representation */
char* serialize_category_data(OpieSyncEnv* env, GList* categories);


/* free a calendar data struct */
void free_cal_data(cal_data* calendar);


/* free a contact data struct */
void free_contact_data(contact_data* contact);


/* free a todo struct */
void free_todo_data(todo_data* todo);


/* free a category data struct */
void free_category_data(category_data* category);


/* compare 2 contacts for equality */
gboolean contact_equals(contact_data* c1, contact_data* c2); 


/* compare 2 todos for equality */
gboolean todo_equals(todo_data* t1, todo_data* t2);


/* compare 2 calendar entries for equality */
gboolean cal_equals(cal_data* c1, cal_data* c2);


unsigned char* hash_contact(contact_data* contact);
unsigned char* hash_todo(todo_data* todo);
unsigned char* hash_cal(cal_data* cal);


#endif
