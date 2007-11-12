/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 * 
 * Modified for OpenSync by Armin Bauer (armin.bauer@desscon.com)
 */

#ifndef _OPENSYNC_LIST_H_
#define _OPENSYNC_LIST_H_

#include "opensync.h"

OPENSYNC_BEGIN_DECLS

typedef struct _OSyncList OSyncList;

struct _OSyncList
{
  void * data;
  OSyncList *next;
  OSyncList *prev;
};

typedef int (*OSyncCompareFunc)(void *a, void *b);
typedef int (*OSyncCompareDataFunc)(void *a, void *b, void *user_data);
typedef void (*OSyncFunc)(void *data, void *user_data);

/* Doubly linked lists
 */
OSYNC_EXPORT OSyncList*   osync_list_alloc                   (void);
OSYNC_EXPORT void     osync_list_free                    (OSyncList            *list);
OSYNC_EXPORT void     osync_list_free_1                  (OSyncList            *list);
#define osync_list_free1                   osync_list_free_1
OSYNC_EXPORT OSyncList*   osync_list_append                  (OSyncList            *list,
					 void *          data);
OSYNC_EXPORT OSyncList*   osync_list_prepend                 (OSyncList            *list,
					 void *          data);
OSYNC_EXPORT OSyncList*   osync_list_insert                  (OSyncList            *list,
					 void *          data,
					 int              position);
OSYNC_EXPORT OSyncList*   osync_list_insert_sorted           (OSyncList            *list,
					 void *          data,
					 OSyncCompareFunc      func);
OSYNC_EXPORT OSyncList*   osync_list_insert_sorted_with_data (OSyncList            *list,
					 void *          data,
					 OSyncCompareDataFunc  func,
					 void *          user_data);
OSYNC_EXPORT OSyncList*   osync_list_insert_before           (OSyncList            *list,
					 OSyncList            *sibling,
					 void *          data);
OSYNC_EXPORT OSyncList*   osync_list_concat                  (OSyncList            *list1,
					 OSyncList            *list2);
OSYNC_EXPORT OSyncList*   osync_list_remove                  (OSyncList            *list,
					 void *     data);
OSYNC_EXPORT OSyncList*   osync_list_remove_all              (OSyncList            *list,
					 void *     data);
OSYNC_EXPORT OSyncList*   osync_list_remove_link             (OSyncList            *list,
					 OSyncList            *llink);
OSYNC_EXPORT OSyncList*   osync_list_delete_link             (OSyncList            *list,
					 OSyncList            *link_);
OSYNC_EXPORT OSyncList*   osync_list_reverse                 (OSyncList            *list);
OSYNC_EXPORT OSyncList*   osync_list_copy                    (OSyncList            *list);
OSYNC_EXPORT OSyncList*   osync_list_nth                     (OSyncList            *list,
					 unsigned int             n);
OSYNC_EXPORT OSyncList*   osync_list_nth_prev                (OSyncList            *list,
					 unsigned int             n);
OSYNC_EXPORT OSyncList*   osync_list_find                    (OSyncList            *list,
					 void *     data);
OSYNC_EXPORT OSyncList*   osync_list_find_custom             (OSyncList            *list,
					 void *     data,
					 OSyncCompareFunc      func);
OSYNC_EXPORT int     osync_list_position                (OSyncList            *list,
					 OSyncList            *llink);
OSYNC_EXPORT int     osync_list_index                   (OSyncList            *list,
					 void *     data);
OSYNC_EXPORT OSyncList*   osync_list_last                    (OSyncList            *list);
OSYNC_EXPORT OSyncList*   osync_list_first                   (OSyncList            *list);
OSYNC_EXPORT unsigned int    osync_list_length                  (const OSyncList            *list);
OSYNC_EXPORT void     osync_list_foreach                 (OSyncList            *list,
					 OSyncFunc             func,
					 void *          user_data);
OSYNC_EXPORT OSyncList*   osync_list_sort                    (OSyncList            *list,
					 OSyncCompareFunc      compare_func);
OSYNC_EXPORT OSyncList*   osync_list_sort_with_data          (OSyncList            *list,
					 OSyncCompareDataFunc  compare_func,
					 void *          user_data) ;
OSYNC_EXPORT void * osync_list_nth_data                (OSyncList            *list,
					 unsigned int             n);

OPENSYNC_END_DECLS

#endif /* _OPENSYNC_LIST_H_ */

