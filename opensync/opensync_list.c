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

/* 
 * MT safe
 */

#include <glib.h>
#include "opensync.h"
#include "opensync_list.h"

#define _osync_list_alloc()         g_slice_new (OSyncList)
#define _osync_list_alloc0()        g_slice_new0 (OSyncList)
#define _osync_list_free1(list)     g_slice_free (OSyncList, list)

OSyncList*
osync_list_alloc (void)
{
  return _osync_list_alloc0 ();
}

void
osync_list_free (OSyncList *list)
{
  g_slice_free_chain (OSyncList, list, next);
}

void
osync_list_free_1 (OSyncList *list)
{
  _osync_list_free1 (list);
}

OSyncList*
osync_list_append (OSyncList	*list,
	       void *	 data)
{
  OSyncList *new_list;
  OSyncList *last;
  
  new_list = _osync_list_alloc ();
  new_list->data = data;
  new_list->next = NULL;
  
  if (list)
    {
      last = osync_list_last (list);
      /* g_assert (last != NULL); */
      last->next = new_list;
      new_list->prev = last;

      return list;
    }
  else
    {
      new_list->prev = NULL;
      return new_list;
    }
}

OSyncList*
osync_list_prepend (OSyncList	 *list,
		void *  data)
{
  OSyncList *new_list;
  
  new_list = _osync_list_alloc ();
  new_list->data = data;
  new_list->next = list;
  
  if (list)
    {
      new_list->prev = list->prev;
      if (list->prev)
	list->prev->next = new_list;
      list->prev = new_list;
    }
  else
    new_list->prev = NULL;
  
  return new_list;
}

OSyncList*
osync_list_insert (OSyncList	*list,
	       void *	 data,
	       int	 position)
{
  OSyncList *new_list;
  OSyncList *tmp_list;
  
  if (position < 0)
    return osync_list_append (list, data);
  else if (position == 0)
    return osync_list_prepend (list, data);
  
  tmp_list = osync_list_nth (list, position);
  if (!tmp_list)
    return osync_list_append (list, data);
  
  new_list = _osync_list_alloc ();
  new_list->data = data;
  new_list->prev = tmp_list->prev;
  if (tmp_list->prev)
    tmp_list->prev->next = new_list;
  new_list->next = tmp_list;
  tmp_list->prev = new_list;
  
  if (tmp_list == list)
    return new_list;
  else
    return list;
}

OSyncList*
osync_list_insert_before (OSyncList   *list,
		      OSyncList   *sibling,
		      void * data)
{
  if (!list)
    {
      list = osync_list_alloc ();
      list->data = data;
      g_return_val_if_fail (sibling == NULL, list);
      return list;
    }
  else if (sibling)
    {
      OSyncList *node;

      node = _osync_list_alloc ();
      node->data = data;
      node->prev = sibling->prev;
      node->next = sibling;
      sibling->prev = node;
      if (node->prev)
	{
	  node->prev->next = node;
	  return list;
	}
      else
	{
	  g_return_val_if_fail (sibling == list, node);
	  return node;
	}
    }
  else
    {
      OSyncList *last;

      last = list;
      while (last->next)
	last = last->next;

      last->next = _osync_list_alloc ();
      last->next->data = data;
      last->next->prev = last;
      last->next->next = NULL;

      return list;
    }
}

OSyncList *
osync_list_concat (OSyncList *list1, OSyncList *list2)
{
  OSyncList *tmp_list;
  
  if (list2)
    {
      tmp_list = osync_list_last (list1);
      if (tmp_list)
	tmp_list->next = list2;
      else
	list1 = list2;
      list2->prev = tmp_list;
    }
  
  return list1;
}

OSyncList*
osync_list_remove (OSyncList	     *list,
	       void *  data)
{
  OSyncList *tmp;
  
  tmp = list;
  while (tmp)
    {
      if (tmp->data != data)
	tmp = tmp->next;
      else
	{
	  if (tmp->prev)
	    tmp->prev->next = tmp->next;
	  if (tmp->next)
	    tmp->next->prev = tmp->prev;
	  
	  if (list == tmp)
	    list = list->next;
	  
	  _osync_list_free1 (tmp);
	  
	  break;
	}
    }
  return list;
}

OSyncList*
osync_list_remove_all (OSyncList	*list,
		   void * data)
{
  OSyncList *tmp = list;

  while (tmp)
    {
      if (tmp->data != data)
	tmp = tmp->next;
      else
	{
	  OSyncList *next = tmp->next;

	  if (tmp->prev)
	    tmp->prev->next = next;
	  else
	    list = next;
	  if (next)
	    next->prev = tmp->prev;

	  _osync_list_free1 (tmp);
	  tmp = next;
	}
    }
  return list;
}

static inline OSyncList*
_osync_list_remove_link (OSyncList *list,
		     OSyncList *link)
{
  if (link)
    {
      if (link->prev)
	link->prev->next = link->next;
      if (link->next)
	link->next->prev = link->prev;
      
      if (link == list)
	list = list->next;
      
      link->next = NULL;
      link->prev = NULL;
    }
  
  return list;
}

OSyncList*
osync_list_remove_link (OSyncList *list,
		    OSyncList *link)
{
  return _osync_list_remove_link (list, link);
}

OSyncList*
osync_list_delete_link (OSyncList *list,
		    OSyncList *link)
{
  list = _osync_list_remove_link (list, link);
  _osync_list_free1 (link);

  return list;
}

OSyncList*
osync_list_copy (OSyncList *list)
{
  OSyncList *new_list = NULL;

  if (list)
    {
      OSyncList *last;

      new_list = _osync_list_alloc ();
      new_list->data = list->data;
      new_list->prev = NULL;
      last = new_list;
      list = list->next;
      while (list)
	{
	  last->next = _osync_list_alloc ();
	  last->next->prev = last;
	  last = last->next;
	  last->data = list->data;
	  list = list->next;
	}
      last->next = NULL;
    }

  return new_list;
}

OSyncList*
osync_list_reverse (OSyncList *list)
{
  OSyncList *last;
  
  last = NULL;
  while (list)
    {
      last = list;
      list = last->next;
      last->next = last->prev;
      last->prev = list;
    }
  
  return last;
}

OSyncList*
osync_list_nth (OSyncList *list,
	    unsigned int  n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list;
}

OSyncList*
osync_list_nth_prev (OSyncList *list,
		 unsigned int  n)
{
  while ((n-- > 0) && list)
    list = list->prev;
  
  return list;
}

void *
osync_list_nth_data (OSyncList     *list,
		 unsigned int      n)
{
  while ((n-- > 0) && list)
    list = list->next;
  
  return list ? list->data : NULL;
}

OSyncList*
osync_list_find (OSyncList         *list,
	     void *  data)
{
  while (list)
    {
      if (list->data == data)
	break;
      list = list->next;
    }
  
  return list;
}

OSyncList*
osync_list_find_custom (OSyncList         *list,
		    void *  data,
		    OSyncCompareFunc   func)
{
  g_return_val_if_fail (func != NULL, list);

  while (list)
    {
      if (! func (list->data, data))
	return list;
      list = list->next;
    }

  return NULL;
}


int
osync_list_position (OSyncList *list,
		 OSyncList *link)
{
  int i;

  i = 0;
  while (list)
    {
      if (list == link)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

int
osync_list_index (OSyncList         *list,
	      void *  data)
{
  int i;

  i = 0;
  while (list)
    {
      if (list->data == data)
	return i;
      i++;
      list = list->next;
    }

  return -1;
}

OSyncList*
osync_list_last (OSyncList *list)
{
  if (list)
    {
      while (list->next)
	list = list->next;
    }
  
  return list;
}

OSyncList*
osync_list_first (OSyncList *list)
{
  if (list)
    {
      while (list->prev)
	list = list->prev;
    }
  
  return list;
}

unsigned int
osync_list_length (const OSyncList *list)
{
  unsigned int length;
  
  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }
  
  return length;
}

void
osync_list_foreach (OSyncList	 *list,
		OSyncFunc	  func,
		void *  user_data)
{
  while (list)
    {
      OSyncList *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}

static OSyncList*
osync_list_insert_sorted_real (OSyncList    *list,
			   void *  data,
			   OSyncFunc     func,
			   void *  user_data)
{
  OSyncList *tmp_list = list;
  OSyncList *new_list;
  int cmp;

  g_return_val_if_fail (func != NULL, list);
  
  if (!list) 
    {
      new_list = _osync_list_alloc0 ();
      new_list->data = data;
      return new_list;
    }
  
  cmp = ((OSyncCompareDataFunc) func) (data, tmp_list->data, user_data);

  while ((tmp_list->next) && (cmp > 0))
    {
      tmp_list = tmp_list->next;

      cmp = ((OSyncCompareDataFunc) func) (data, tmp_list->data, user_data);
    }

  new_list = _osync_list_alloc0 ();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->prev = tmp_list;
      return list;
    }
   
  if (tmp_list->prev)
    {
      tmp_list->prev->next = new_list;
      new_list->prev = tmp_list->prev;
    }
  new_list->next = tmp_list;
  tmp_list->prev = new_list;
 
  if (tmp_list == list)
    return new_list;
  else
    return list;
}

OSyncList*
osync_list_insert_sorted (OSyncList        *list,
		      void *      data,
		      OSyncCompareFunc  func)
{
  return osync_list_insert_sorted_real (list, data, (OSyncFunc) func, NULL);
}

OSyncList*
osync_list_insert_sorted_with_data (OSyncList            *list,
				void *          data,
				OSyncCompareDataFunc  func,
				void *          user_data)
{
  return osync_list_insert_sorted_real (list, data, (OSyncFunc) func, user_data);
}

static OSyncList *
osync_list_sort_merge (OSyncList     *l1, 
		   OSyncList     *l2,
		   OSyncFunc     compare_func,
		   void *  user_data)
{
  OSyncList list, *l, *lprev;
  int cmp;

  l = &list; 
  lprev = NULL;

  while (l1 && l2)
    {
      cmp = ((OSyncCompareDataFunc) compare_func) (l1->data, l2->data, user_data);

      if (cmp <= 0)
        {
	  l->next = l1;
	  l1 = l1->next;
        } 
      else 
	{
	  l->next = l2;
	  l2 = l2->next;
        }
      l = l->next;
      l->prev = lprev; 
      lprev = l;
    }
  l->next = l1 ? l1 : l2;
  l->next->prev = l;

  return list.next;
}

static OSyncList* 
osync_list_sort_real (OSyncList    *list,
		  OSyncFunc     compare_func,
		  void *  user_data)
{
  OSyncList *l1, *l2;
  
  if (!list) 
    return NULL;
  if (!list->next) 
    return list;
  
  l1 = list; 
  l2 = list->next;

  while ((l2 = l2->next) != NULL)
    {
      if ((l2 = l2->next) == NULL) 
	break;
      l1 = l1->next;
    }
  l2 = l1->next; 
  l1->next = NULL; 

  return osync_list_sort_merge (osync_list_sort_real (list, compare_func, user_data),
			    osync_list_sort_real (l2, compare_func, user_data),
			    compare_func,
			    user_data);
}

OSyncList *
osync_list_sort (OSyncList        *list,
	     OSyncCompareFunc  compare_func)
{
  return osync_list_sort_real (list, (OSyncFunc) compare_func, NULL);
			    
}

OSyncList *
osync_list_sort_with_data (OSyncList            *list,
		       OSyncCompareDataFunc  compare_func,
		       void *          user_data)
{
  return osync_list_sort_real (list, (OSyncFunc) compare_func, user_data);
}
