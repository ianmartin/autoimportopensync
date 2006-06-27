/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#ifndef OPENSYNC_INTERNALS_H_
#define OPENSYNC_INTERNALS_H_

#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <glib/gprintf.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <errno.h>
extern int errno;

#define osync_assert(x) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: Assertion \"" #x "\" failed\n", __FILE__, __LINE__, __FUNCTION__); abort();}
#define osync_assert_msg(x, msg) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg); abort();}
#define segfault_me char **blablabla = NULL; *blablabla = "test";

#define osync_return_if_fail(condition) do {                                            \
  if (!(condition)) {                                                                   \
    return;                                                                             \
  } } while (0)

#define osync_return_val_if_fail(condition, val) do {                                   \
  if (!(condition)) {                                                                   \
    return (val);                                                                       \
  } } while (0)

#include "opensync_support_internals.h"

#endif //OPENSYNC_INTERNALS_H_
