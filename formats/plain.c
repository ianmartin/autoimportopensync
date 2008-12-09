/*
 * data - A plugin for data objects for the opensync framework
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
 
#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <opensync/opensync-support.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-xmlformat.h>
#include <glib.h>

static OSyncConvCmpResult compare_plain(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
{
  /* Consider empty block equal NULL pointers */
  if (!leftsize) leftdata = NULL;
  if (!rightsize) rightdata = NULL;
	
  if (!leftdata && !rightdata)
    return OSYNC_CONV_DATA_SAME;
		
  if (leftdata && rightdata && (leftsize == rightsize)) {
    if (!memcmp(leftdata, rightdata, leftsize))
      return OSYNC_CONV_DATA_SAME;
    else
      return OSYNC_CONV_DATA_MISMATCH;
  }
	
  return OSYNC_CONV_DATA_MISMATCH;
}

static osync_bool copy_plain(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
  char *r = g_malloc0(inpsize);

  memcpy(r, input, inpsize);
  *output = r;
  *outpsize = inpsize;
  return TRUE;
}

static osync_bool conv_xmlformatnote_to_memo(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
  const char *body = NULL;
  OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
  OSyncXMLFieldList *xmlfieldlist = osync_xmlformat_search_field(xmlformat, "Description", error, NULL);
  OSyncXMLField *xmlfield = osync_xmlfieldlist_item(xmlfieldlist, 0);

  *free_input = TRUE;
  body = osync_xmlfield_get_key_value(xmlfield, "Content");
  if (!body) {
    body = "";
  }
  *output = g_strdup(body);
  *outpsize = strlen(body);
  return TRUE; 
}

static osync_bool conv_memo_to_xmlformatnote(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
  GString *str;
  const char *p;
  OSyncXMLFormat *ret = NULL;
  OSyncXMLField *field = NULL;

  *free_input = TRUE;

  str = g_string_new("");

  ret = osync_xmlformat_new("note", error);
  field = osync_xmlfield_new(ret, "Description", error);
  for (p = input; p && *p; p++) {
    switch (*p) {
    case '\r':
      if (*(p+1) == '\n')
        p++;
      osync_trace(TRACE_INTERNAL, "[%s] escape carriage returns!!", __func__);
      str = g_string_append (str, "\n");
      break;
    default:
      str = g_string_append_c (str, *p);
      break;
    }
  }
  osync_trace(TRACE_SENSITIVE, "Input : %s", str->str);
  osync_xmlfield_set_key_value(field, "Content", str->str);

  if (!ret)
    return FALSE;
  *output = (char *)ret;
  return TRUE;
}

static void destroy_plain(char *input, unsigned int inpsize)
{
  g_free(input);
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
  OSyncObjFormat *format = osync_objformat_new("plain", "data", error);
  if (!format)
    return FALSE;
	
  osync_objformat_set_compare_func(format, compare_plain);
  osync_objformat_set_copy_func(format, copy_plain);
  osync_objformat_set_destroy_func(format, destroy_plain);

  osync_format_env_register_objformat(env, format);
  osync_objformat_unref(format);

  /* "memo" is the same as "plain" expect the object type is fixed to "note" */
  format = osync_objformat_new("memo", "note", error);
  if (!format)
    return FALSE;
	
  osync_objformat_set_compare_func(format, compare_plain);
  osync_objformat_set_copy_func(format, copy_plain);
  osync_objformat_set_destroy_func(format, destroy_plain);

  osync_format_env_register_objformat(env, format);
  osync_objformat_unref(format);

  return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
  OSyncObjFormat *memo = osync_format_env_find_objformat(env, "memo");
  OSyncObjFormat *xmlformatnote = osync_format_env_find_objformat(env, "xmlformat-note");
  OSyncFormatConverter *conv = NULL;
  if (!memo) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find memo format");
    return FALSE;
  }
  if (!xmlformatnote) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find xmlformat-note format");
    return FALSE;
  }	
  conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformatnote, memo, conv_xmlformatnote_to_memo, error);
  if (!conv)
    return FALSE;
	
  osync_format_env_register_converter(env, conv);
  osync_converter_unref(conv);
	
  conv = osync_converter_new(OSYNC_CONVERTER_CONV, memo, xmlformatnote, conv_memo_to_xmlformatnote, error);
  if (!conv)
    return FALSE;
	
  osync_format_env_register_converter(env, conv);
  osync_converter_unref(conv);
  return TRUE;
}

int get_version(void)
{
  return 1;
}
