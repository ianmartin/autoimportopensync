/* 
   MultiSync Evolution2 Plugin
   Copyright (C) 2004 Tom Foottit <tom@foottit.com>
   Copyright (C) 2004 Bo Lincoln <lincoln@lysator.liu.se>

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
 *  $Id: evolution_config.c,v 1.1.2.2 2004/04/16 20:06:26 irix Exp $
 */

#include "evolution_config.h"
#include <gconf/gconf-client.h>

#define EVO2_SYNC_GCONF_PATH "/apps/multisync/evolution2-sync"
#define EVO2_CAL_PATH_KEY "calendarpath"
#define EVO2_ADDR_PATH_KEY "addressbookpath"
#define EVO2_TODO_PATH_KEY "todopath"

GConfClient *evo2_gconf = NULL;

/*
 * initalize the evo2 config system
 * (set up gconf)
 *
 */
void evo2_init_config()
{
  evo2_gconf = gconf_client_get_default();  
}


/*
 * load an existing plugin configuration
 */
gboolean evo2_load_config(eds_conn_t* conn)
{
  gchar* path = g_strdup_printf("%s%s", 
                                EVO2_SYNC_GCONF_PATH,
                                sync_get_datapath(conn->sync_pair));
  path = g_strdelimit(path,".",'_');
  
  gconf_client_add_dir(evo2_gconf, 
                       path, 
                       GCONF_CLIENT_PRELOAD_NONE,
                       NULL);    
  
  
  gchar* calpath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_CAL_PATH_KEY);
  
  gchar* addrpath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_ADDR_PATH_KEY);
  
  gchar* todopath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_TODO_PATH_KEY);
  
  conn->calendarpath = gconf_client_get_string(evo2_gconf, 
                                               calpath_key, 
                                               NULL);
  
  conn->addressbookpath = gconf_client_get_string(evo2_gconf, 
                                                  addrpath_key, 
                                                  NULL);
  
  conn->todopath = gconf_client_get_string(evo2_gconf, 
                                           todopath_key, 
                                           NULL);
    
  g_free(calpath_key);
  g_free(addrpath_key);
  g_free(todopath_key);
  g_free(path);
  
  return TRUE;
}


/*
 * save the current plugin configuration
 */
gboolean evo2_save_config(eds_conn_t* conn)
{
  gchar* path = g_strdup_printf("%s%s", 
                                EVO2_SYNC_GCONF_PATH,
                                sync_get_datapath(conn->sync_pair));
  path = g_strdelimit(path,".",'_');
  
  gconf_client_add_dir(evo2_gconf, 
                       path, 
                       GCONF_CLIENT_PRELOAD_NONE,
                       NULL);    
  
  
  gchar* calpath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_CAL_PATH_KEY);
  
  gchar* addrpath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_ADDR_PATH_KEY);
  
  gchar* todopath_key = g_strdup_printf("%s/%s", 
                                path,
                                EVO2_TODO_PATH_KEY);
  
  gconf_client_set_string(evo2_gconf, calpath_key,
                          conn->calendarpath, NULL);
  
  gconf_client_set_string(evo2_gconf, addrpath_key,
                          conn->addressbookpath, NULL);
  
  gconf_client_set_string(evo2_gconf, todopath_key,
                          conn->todopath, NULL);
  
  g_free(calpath_key);
  g_free(addrpath_key);
  g_free(todopath_key);
  g_free(path);
  
  return TRUE;
}
