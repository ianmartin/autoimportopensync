/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * Copyright (C) 2007  Graham R. Cobb <g+opensync@cobb.uk.net>
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

#include "gpe_sync.h"

/*This just opens the config file and returns the settings in the env */
osync_bool gpe_parse_settings(gpe_environment *env, const char *data)
{
	xmlDocPtr doc = NULL;
	xmlXPathContextPtr ctx = NULL;
	xmlXPathObjectPtr obj = NULL;
	///	xmlNodePtr cur;

	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p)", __func__, env, data);

	// Set the defaults
	env->device_addr = g_strdup("127.0.0.1");
	env->device_port = 6446;
	env->username = g_strdup("gpeuser");
	env->command = g_strdup("gpesyncd --remote");
	env->use_ssh = 0;
	env->use_local = 0;
	env->use_remote = 0;
	env->debuglevel = 0;
	env->calendar = NULL;
	
	xmlInitParser();

	doc = xmlParseMemory(data, strlen(data)+1);
	
	if(!doc) {
	  osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: Could not parse data!", __func__);
	  return FALSE;
	}

	ctx = xmlXPathNewContext(doc);

	/* Work out which type of connection to use */
	obj = xmlXPathEval("/config/local", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  env->use_local = 1;
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <local> seen", __func__);
	}
	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("/config/ssh", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (env->use_local) {
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: <local> and <ssh> both found in config file", __func__);
	    goto error;
	  }
	  env->use_ssh = 1;
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <ssh> seen", __func__);
	}
	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("/config/remote", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (env->use_local) {
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: <local> and <remote> both found in config file", __func__);
	    goto error;
	  }
	  if (env->use_ssh) {
	    osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: <ssh> and <remote> both found in config file", __func__);
	    goto error;
	  }
	  env->use_remote = 1;
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <remote> seen", __func__);
	}
	if (!(env->use_local || env->use_ssh || env->use_remote)) {
	  osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: one of <local>, <remote>, <ssh> must be specified in config file", __func__);
	  goto error;
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//handheld_ip/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (!(env->use_remote || env->use_ssh)) {
	    osync_trace(TRACE_ERROR, "GPE-SYNC %s: <handheld_ip> should only be specified in <remote> or <ssh> sections", __func__);
	  }
	  g_free(env->device_addr);
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->device_addr = g_strdup(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <handheld_ip> = %s", __func__, env->device_addr);
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//handheld_user/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (!(env->use_ssh)) {
	    osync_trace(TRACE_ERROR, "GPE-SYNC %s: <handheld_user> should only be specified in <ssh> section", __func__);
	  }
	  g_free(env->username);
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->username = g_strdup(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <handheld_user> = %s", __func__, env->username);
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//command/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (!(env->use_local || env->use_ssh)) {
	    osync_trace(TRACE_ERROR, "GPE-SYNC %s: <command> should only be specified in <local> or <ssh> sections", __func__);
	  }
	  g_free(env->username);
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->command = g_strdup(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <command> = %s", __func__, env->command);
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//handheld_port/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  if (!(env->use_remote)) {
	    osync_trace(TRACE_ERROR, "GPE-SYNC %s: <handheld_port> should only be specified in <remote> section", __func__);
	  }
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->device_port = atoi(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <handheld_port> = %d", __func__, env->device_port);
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//debug/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->debuglevel = atoi(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <debug> = %d", __func__, env->debuglevel);
	}

	if (obj) xmlXPathFreeObject(obj);
	obj = xmlXPathEval("//calendar/text()", ctx);
	if (obj && obj->nodesetval && obj->nodesetval->nodeNr) {
	  xmlChar *str=xmlXPathCastToString(obj);
	  env->calendar = g_strdup(str);
	  xmlFree(str);
	  osync_trace(TRACE_INTERNAL, "GPE-SYNC %s: <calendar> = %s", __func__, env->calendar);
	}

	if (obj) xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
	return TRUE;
 error:
	if (obj) xmlXPathFreeObject(obj);
	if (ctx) xmlXPathFreeContext(ctx);
	if (doc) xmlFreeDoc(doc);
	xmlCleanupParser();
	return FALSE;
}
