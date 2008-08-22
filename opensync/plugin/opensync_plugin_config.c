/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Daniel Gollub <dgollub@suse.de>
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

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-plugin.h"
#include "opensync-format.h"
#include "opensync_plugin_config_internals.h"
#include "opensync_plugin_advancedoptions_internals.h"

#include "opensync_xml.h"

static OSyncPluginAdvancedOptionParameter *_osync_plugin_config_parse_advancedoption_param(OSyncPluginAdvancedOption *option, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, option, cur, error);

	OSyncPluginAdvancedOptionParameter *param = osync_plugin_advancedoption_param_new(error);
	if (!param)
		goto error;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, BAD_CAST "DisplayName"))
			osync_plugin_advancedoption_param_set_displayname(param, str);
		else if (!xmlStrcmp(cur->name, BAD_CAST "Name"))
			osync_plugin_advancedoption_param_set_name(param, str);
		else if (!xmlStrcmp(cur->name, BAD_CAST "Type"))
			osync_plugin_advancedoption_param_set_type(param, 
					osync_plugin_advancedoption_type_string_to_val(str));
		else if (!xmlStrcmp(cur->name, BAD_CAST "ValEnum"))
			osync_plugin_advancedoption_param_add_valenum(param, str);
		else if (!xmlStrcmp(cur->name, BAD_CAST "Value"))
			osync_plugin_advancedoption_param_set_value(param, str);

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return param;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static OSyncPluginAdvancedOption *_osync_plugin_config_parse_advancedoption(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	OSyncPluginAdvancedOptionParameter *param;
	OSyncPluginAdvancedOption *option = osync_plugin_advancedoption_new(error);
	if (!option)
		goto error;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, BAD_CAST "DisplayName"))
			osync_plugin_advancedoption_set_displayname(option, str);
		else if (!xmlStrcmp(cur->name, BAD_CAST "MaxOccurs"))
			osync_plugin_advancedoption_set_maxoccurs(option, atoi(str));
		else if (!xmlStrcmp(cur->name, BAD_CAST "MaxSize"))
			osync_plugin_advancedoption_set_maxsize(option, atoi(str));
		else if (!xmlStrcmp(cur->name, BAD_CAST "Name"))
			osync_plugin_advancedoption_set_name(option, str);
		else if (!xmlStrcmp(cur->name, BAD_CAST "Parameter")) {
			if (!(param = _osync_plugin_config_parse_advancedoption_param(option, cur->xmlChildrenNode, error)))
				goto error;

			option->parameters = osync_list_prepend(option->parameters, param);

		} else if (!xmlStrcmp(cur->name, BAD_CAST "Type")) {
			osync_plugin_advancedoption_set_type(option, osync_plugin_advancedoption_type_string_to_val(str));
		} else if (!xmlStrcmp(cur->name, BAD_CAST "ValEnum")) {
			osync_plugin_advancedoption_add_valenum(option, str);
		} else if (!xmlStrcmp(cur->name, BAD_CAST "Value")) {
			osync_plugin_advancedoption_set_value(option, str);
		}

		xmlFree(str);
	}

	option->parameters = osync_list_reverse(option->parameters);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, option);
	return option;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool _osync_plugin_config_parse_advancedoptions(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{
	OSyncPluginAdvancedOption *option;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"AdvancedOption")) {
			if (!(option = _osync_plugin_config_parse_advancedoption(config, cur->xmlChildrenNode, error)))
				goto error;

			config->advancedoptions = osync_list_prepend(config->advancedoptions, option);
		}
	}

	config->advancedoptions = osync_list_reverse(config->advancedoptions);

	return TRUE;

error:
	return FALSE; 
}

static osync_bool _osync_plugin_config_parse_authentication(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	if (cur == NULL) { // don't set auth if Authentication tag is empty
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;	
	}
	
	OSyncPluginAuthentication *auth = osync_plugin_authentication_new(error);
	if (!auth)
		goto error;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Username"))
			osync_plugin_authentication_set_username(auth, str);
		else if (!xmlStrcmp(cur->name, (const xmlChar *)"Password"))
			osync_plugin_authentication_set_password(auth, str);
		else if (!xmlStrcmp(cur->name, (const xmlChar *)"Reference"))
			osync_plugin_authentication_set_reference(auth, str);

		xmlFree(str);
	}

	osync_plugin_config_set_authentication(config, auth);
	osync_plugin_authentication_unref(auth);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection_bluetooth(OSyncPluginConnection *conn, xmlNode *cur, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, conn, cur, error);

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"MAC")) {
			osync_plugin_connection_bt_set_addr(conn, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"RFCommChannel")) {
			osync_plugin_connection_bt_set_channel(conn, atoi(str));
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"SDPUUID")) {
			osync_plugin_connection_bt_set_sdpuuid(conn, str);
		} else {
//			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", __NULLSTR(cur->name));
			xmlFree(str);
			goto error;
		}

		xmlFree(str);
	}


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection_usb(OSyncPluginConnection *conn, xmlNode *cur, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, conn, cur, error);

	unsigned int usbid;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"VendorID")) {
			sscanf(str, "0x%x", &usbid);
			osync_plugin_connection_usb_set_vendorid(conn, usbid);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"ProductID")) {
			sscanf(str, "0x%x", &usbid);
			osync_plugin_connection_usb_set_productid(conn, usbid);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Interface")) {
			osync_plugin_connection_usb_set_interface(conn, atoi(str));
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);

			xmlFree(str);
			goto error;
		}

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection_irda(OSyncPluginConnection *conn, xmlNode *cur, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, conn, cur, error);

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Service")) {
			osync_plugin_connection_irda_set_service(conn, str);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);
			xmlFree(str);
			goto error;
		}

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection_network(OSyncPluginConnection *conn, xmlNode *cur, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, conn, cur, error);

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Address")) {
			osync_plugin_connection_net_set_address(conn, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Port")) {
			osync_plugin_connection_net_set_port(conn, atoi(str));
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Protocol")) {
			osync_plugin_connection_net_set_protocol(conn, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"DNSSD")) {
			osync_plugin_connection_net_set_dnssd(conn, str);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);
			xmlFree(str);
			goto error;
		}

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection_serial(OSyncPluginConnection *conn, xmlNode *cur, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, conn, cur, error);

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Speed")) {
			osync_plugin_connection_serial_set_speed(conn, atoi(str));
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"DeviceNode")) {
			osync_plugin_connection_serial_set_devicenode(conn, str);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);
			xmlFree(str);
			goto error;
		}

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_connection(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	osync_bool ret = TRUE;

	OSyncPluginConnection *conn = NULL;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Bluetooth")) {
			if (!(conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_BLUETOOTH, error)))
				goto error;

			ret = _osync_plugin_config_parse_connection_bluetooth(conn, cur->xmlChildrenNode, error);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"USB")) {
			if (!(conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_USB, error)))
				goto error;

			ret = _osync_plugin_config_parse_connection_usb(conn, cur->xmlChildrenNode, error);

		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"IrDA")) {
			if (!(conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_IRDA, error)))
				goto error;

			ret = _osync_plugin_config_parse_connection_irda(conn, cur->xmlChildrenNode, error);

		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Network")) {
			if (!(conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_NETWORK, error)))
				goto error;

			ret = _osync_plugin_config_parse_connection_network(conn, cur->xmlChildrenNode, error);

		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Serial")) {
			if (!(conn = osync_plugin_connection_new(OSYNC_PLUGIN_CONNECTION_SERIAL, error)))
				goto error;

			ret = _osync_plugin_config_parse_connection_serial(conn, cur->xmlChildrenNode, error);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);
			goto error;
		}
	}

	if (!ret)
		goto error_and_free;

	osync_plugin_config_set_connection(config, conn);
	osync_plugin_connection_unref(conn);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_and_free:
	osync_plugin_connection_unref(conn);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_localization(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	OSyncPluginLocalization *local = osync_plugin_localization_new(error);
	if (!local)
		goto error;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Encoding"))
			osync_plugin_localization_set_encoding(local, str);
		else if (!xmlStrcmp(cur->name, (const xmlChar *)"Timezone"))
			osync_plugin_localization_set_timezone(local, str);
		else if (!xmlStrcmp(cur->name, (const xmlChar *)"Language"))
			osync_plugin_localization_set_language(local, str);

		xmlFree(str);
	}

	osync_plugin_config_set_localization(config, local);
	osync_plugin_localization_unref(local);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_resource_format(OSyncPluginResource *res, xmlNode *cur, OSyncError **error)
{
	osync_assert(res);
	osync_assert(cur);

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, res, cur, error);

	OSyncObjFormatSink *format_sink = NULL; 

	const char *objformat = NULL, *config = NULL;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		char *str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Name"))
			objformat = str;
		else if (!xmlStrcmp(cur->name, (const xmlChar *)"Config"))
			config = str;
		else
			xmlFree(str);
	}

	osync_assert(objformat);

	format_sink = osync_objformat_sink_new(objformat, error);
	xmlFree((xmlChar *) objformat);

	if (!format_sink)
		goto error_free_config;

	if (config) {
		osync_objformat_sink_set_config(format_sink, config);
		xmlFree((xmlChar *) config);
	}

	osync_plugin_resource_add_objformat_sink(res, format_sink);
	osync_objformat_sink_unref(format_sink);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_free_config:
	xmlFree((xmlChar *) config);
/*error:*/
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse_resource_formats(OSyncPluginResource *res, xmlNode *cur, OSyncError **error)
{
	osync_assert(res);
	osync_assert(cur);

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, res, cur, error);

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (!_osync_plugin_config_parse_resource_format(res, cur->xmlChildrenNode, error))
			goto error;

	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static OSyncPluginResource *_osync_plugin_config_parse_resource(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	char *str = NULL;

	OSyncPluginResource *res = osync_plugin_resource_new(error);
	if (!res)
		goto error;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		str = (char*)xmlNodeGetContent(cur);
		if (!str)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Enabled")) {
			osync_plugin_resource_enable(res, atoi(str));
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Formats")) {
			if (!_osync_plugin_config_parse_resource_formats(res, cur->xmlChildrenNode, error))
				goto error_free_str;
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Name")) {
			osync_plugin_resource_set_name(res, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"MIME")) {
			osync_plugin_resource_set_mime(res, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"ObjType")) {
			osync_plugin_resource_set_objtype(res, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Path")) {
			osync_plugin_resource_set_path(res, str);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Url")) {
			osync_plugin_resource_set_url(res, str);
		}

		xmlFree(str);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return res;

error_free_str:
	xmlFree(str);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static osync_bool _osync_plugin_config_parse_resources(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	OSyncPluginResource *res; 
	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"Resource")) {
			if (!(res = _osync_plugin_config_parse_resource(config, cur->xmlChildrenNode, error)))
				goto error;

			config->resources = osync_list_prepend(config->resources, res);
		}
	}

	config->resources = osync_list_reverse(config->resources);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_parse(OSyncPluginConfig *config, xmlNode *cur, OSyncError **error)
{
	osync_assert(config);
	osync_assert(cur);

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, config, cur, error);

	osync_bool ret = TRUE;

	for (; cur != NULL; cur = cur->next) {

		if (cur->type != XML_ELEMENT_NODE)
			continue;

		if (!xmlStrcmp(cur->name, (const xmlChar *)"AdvancedOptions")) {
			ret = _osync_plugin_config_parse_advancedoptions(config, cur->xmlChildrenNode, error);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Authentication")) {
			ret = _osync_plugin_config_parse_authentication(config, cur->xmlChildrenNode, error);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Connection")) {
			ret = _osync_plugin_config_parse_connection(config, cur->xmlChildrenNode, error);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Localization")) {
			ret = _osync_plugin_config_parse_localization(config, cur->xmlChildrenNode, error);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *)"Resources")) {
			ret = _osync_plugin_config_parse_resources(config, cur->xmlChildrenNode, error);
		} else {
			osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unknown configuration field \"%s\"", cur->name);
			goto error;
		}

		if (!ret)
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool osync_plugin_config_file_load(OSyncPluginConfig *config, const char *path, const char *schemadir, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %s, %p)", __func__, config, __NULLSTR(path), __NULLSTR(schemadir), error);
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	char *schemafile = NULL;
	const char *schemapath = schemadir ? schemadir : OPENSYNC_SCHEMASDIR;

	if (!osync_xml_open_file(&doc, &cur, path, "config", error))
		goto error;

	osync_assert(cur);

	/* Validate plugin configuration file */
	schemafile = g_strdup_printf("%s%c%s", schemapath, G_DIR_SEPARATOR, OSYNC_PLUGIN_CONFING_SCHEMA);
	if (!osync_xml_validate_document(doc, schemafile)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Plugin configuration file is not valid! %s", schemafile);
		g_free(schemafile);
		goto error;
	}
	g_free(schemafile);

	if (!_osync_plugin_config_parse(config, cur, error))
		goto error;

	xmlFreeDoc(doc);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_authentication(xmlNode *cur, OSyncPluginAuthentication *auth, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, auth, error);

	const char *username, *password, *ref;

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"Authentication", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	if ((username = osync_plugin_authentication_get_username(auth)))
		xmlNewChild(node, NULL, (xmlChar*)"Username", (xmlChar*)username);

	if ((password = osync_plugin_authentication_get_password(auth)))
		xmlNewChild(node, NULL, (xmlChar*)"Password", (xmlChar*)password);

	if ((ref = osync_plugin_authentication_get_reference(auth)))
		xmlNewChild(node, NULL, (xmlChar*)"Reference", (xmlChar*)ref);


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_connection(xmlNode *cur, OSyncPluginConnection *conn, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, conn, error);

	char *str;
	const char *mac, *sdpuuid, *address, *protocol, *dnssd, *devicenode, *service;
	unsigned int rfcomm_channel, vendorid, productid, interface, port, speed;

	xmlNode *typenode, *node = xmlNewChild(cur, NULL, (xmlChar*)"Connection", NULL);
	if (!node)
		goto error_nomemory;

	switch (osync_plugin_connection_get_type(conn)) {
		case OSYNC_PLUGIN_CONNECTION_BLUETOOTH:

			typenode = xmlNewChild(node, NULL, (xmlChar*)"Bluetooth", NULL);
			if (!typenode)
				goto error_nomemory;

			mac = osync_plugin_connection_bt_get_addr(conn);
			if (mac)
				xmlNewChild(typenode, NULL, (xmlChar*)"MAC", (xmlChar*)mac);

			rfcomm_channel = osync_plugin_connection_bt_get_channel(conn);
			if (rfcomm_channel) {
				str = g_strdup_printf("%u", rfcomm_channel);
				xmlNewChild(typenode, NULL, (xmlChar*)"RFCommChannel", (xmlChar*)str);
				g_free(str);
			}

			sdpuuid = osync_plugin_connection_bt_get_sdpuuid(conn);
			if (sdpuuid)
				xmlNewChild(typenode, NULL, (xmlChar*)"SDPUUID", (xmlChar*)sdpuuid);

			break;
		case OSYNC_PLUGIN_CONNECTION_USB:

			typenode = xmlNewChild(node, NULL, (xmlChar*)"USB", NULL);
			if (!typenode)
				goto error_nomemory;

			vendorid = osync_plugin_connection_usb_get_vendorid(conn);
			if (vendorid) {
				str = g_strdup_printf("0x%x", vendorid);
				xmlNewChild(typenode, NULL, (xmlChar*)"VendorID", (xmlChar*)str);
				g_free(str);
			}

			productid = osync_plugin_connection_usb_get_productid(conn);
			if (productid) {
				str = g_strdup_printf("0x%x", productid);
				xmlNewChild(typenode, NULL, (xmlChar*)"ProductID", (xmlChar*)str);
				g_free(str);
			}

			interface = osync_plugin_connection_usb_get_interface(conn);
			if (interface) {
				str = g_strdup_printf("%u", interface);
				xmlNewChild(typenode, NULL, (xmlChar*)"Interface", (xmlChar*)str);
				g_free(str);
			}

			break;
		case OSYNC_PLUGIN_CONNECTION_NETWORK:

			typenode = xmlNewChild(node, NULL, (xmlChar*)"Network", NULL);
			if (!typenode)
				goto error_nomemory;

			address = osync_plugin_connection_net_get_address(conn);
			if (address)
				xmlNewChild(typenode, NULL, (xmlChar*)"Address", (xmlChar*)address);

			port = osync_plugin_connection_net_get_port(conn);
			if (port) {
				str = g_strdup_printf("%u", port);
				xmlNewChild(typenode, NULL, (xmlChar*)"Port", (xmlChar*)str);
				g_free(str);
			}

			protocol = osync_plugin_connection_net_get_protocol(conn);
			if (protocol)
				xmlNewChild(typenode, NULL, (xmlChar*)"Protocol", (xmlChar*)protocol);

			dnssd = osync_plugin_connection_net_get_dnssd(conn);
			if (dnssd)
				xmlNewChild(typenode, NULL, (xmlChar*)"DNSSD", (xmlChar*)dnssd);

			break;
		case OSYNC_PLUGIN_CONNECTION_SERIAL:
			typenode = xmlNewChild(node, NULL, (xmlChar*)"Serial", NULL);
			if (!typenode)
				goto error_nomemory;

			speed = osync_plugin_connection_serial_get_speed(conn);
			if (speed) {
				str = g_strdup_printf("%u", speed);
				xmlNewChild(typenode, NULL, (xmlChar*)"Speed", (xmlChar*)str);
				g_free(str);
			}

			devicenode = osync_plugin_connection_serial_get_devicenode(conn);
			if (devicenode)
				xmlNewChild(typenode, NULL, (xmlChar*)"DeviceNode", (xmlChar*)devicenode);

			break;
		case OSYNC_PLUGIN_CONNECTION_IRDA:
			typenode = xmlNewChild(node, NULL, (xmlChar*)"IrDA", NULL);
			if (!typenode)
				goto error_nomemory;

			service = osync_plugin_connection_irda_get_service(conn);
			if (service)
				xmlNewChild(typenode, NULL, (xmlChar*)"Service", (xmlChar*)service);

			break;
		case OSYNC_PLUGIN_CONNECTION_UNKNOWN:
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Unknown connection type is configured. Can't store configuration.");
			goto error;
			break;
	}


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_nomemory:	
	osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

static osync_bool _osync_plugin_config_assemble_localization(xmlNode *cur, OSyncPluginLocalization *local, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, local, error);

	const char *encoding, *tz, *language;

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"Localization", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	if ((encoding = osync_plugin_localization_get_encoding(local)))
		xmlNewChild(node, NULL, (xmlChar*)"Encoding", (xmlChar*)encoding);

	if ((tz = osync_plugin_localization_get_timezone(local)))
		xmlNewChild(node, NULL, (xmlChar*)"Timezone", (xmlChar*)tz);

	if ((language = osync_plugin_localization_get_language(local)))
		xmlNewChild(node, NULL, (xmlChar*)"Language", (xmlChar*)language);


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

static osync_bool _osync_plugin_config_assemble_resource_format(xmlNode *cur, OSyncObjFormatSink *format_sink, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, format_sink, error);

	const char *name, *config;
	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"Format", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	if ((config = osync_objformat_sink_get_config(format_sink)))
		xmlNewChild(node, NULL, (xmlChar*)"Config", (xmlChar*)config);

	if ((name = osync_objformat_sink_get_objformat(format_sink)))
		xmlNewChild(node, NULL, (xmlChar*)"Name", (xmlChar*)name);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_resource(xmlNode *cur, OSyncPluginResource *res, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, res, error);

	OSyncList *o;
	const char *name, *mime, *objtype, *path, *url;

	xmlNode *next, *node = xmlNewChild(cur, NULL, (xmlChar*)"Resource", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	osync_bool res_enabled = osync_plugin_resource_is_enabled(res);
	xmlNewChild(node, NULL, (xmlChar*)"Enabled", res_enabled ? (xmlChar*) "1" : (xmlChar*) "0");

	next = xmlNewChild(node, NULL, (xmlChar*)"Formats", NULL);
	for (o = osync_plugin_resource_get_objformat_sinks(res); o; o = o->next) {
		OSyncObjFormatSink *format_sink = o->data;
		if (!_osync_plugin_config_assemble_resource_format(next, format_sink, error))
			goto error;
	}

	if ((name = osync_plugin_resource_get_name(res)))
		xmlNewChild(node, NULL, (xmlChar*)"Name", (xmlChar*)name);

	if ((mime = osync_plugin_resource_get_mime(res)))
		xmlNewChild(node, NULL, (xmlChar*)"MIME", (xmlChar*)mime);

	objtype = osync_plugin_resource_get_objtype(res);
	osync_assert(objtype); /* ObjType for Resource MUST be set! */
	xmlNewChild(node, NULL, (xmlChar*)"ObjType", (xmlChar*) objtype);

	if ((path = osync_plugin_resource_get_path(res)))
		xmlNewChild(node, NULL, (xmlChar*)"Path", (xmlChar*)path);

	if ((url = osync_plugin_resource_get_url(res)))
		xmlNewChild(node, NULL, (xmlChar*)"Url", (xmlChar*)url);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_resources(xmlNode *cur, OSyncList *resources, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, resources, error);

	OSyncList *res;

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"Resources", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	for (res = resources; res; res = res->next)
		if (!_osync_plugin_config_assemble_resource(node, res->data, error))
			goto error;


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_advancedoption_param(xmlNode *cur, OSyncPluginAdvancedOptionParameter *param, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, param, error);

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"Parameter", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	/* DisplayName */
	if (osync_plugin_advancedoption_param_get_displayname(param))
		xmlNewChild(node, NULL, BAD_CAST "DisplayName", 
				BAD_CAST osync_plugin_advancedoption_param_get_displayname(param));

	/* Name */
	if (!osync_plugin_advancedoption_param_get_name(param)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Name for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Name", BAD_CAST osync_plugin_advancedoption_param_get_name(param));

	/* Type */
	if (!osync_plugin_advancedoption_param_get_type(param)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Type for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Type", BAD_CAST osync_plugin_advancedoption_param_get_type_string(param));

	/* ValEnum */
	OSyncList *v;
	for (v = osync_plugin_advancedoption_param_get_valenums(param); v; v = v->next) {
		char *valenum = v->data;
		xmlNewChild(node, NULL, BAD_CAST "ValEnum", BAD_CAST valenum);
	}

	/* Value */
	if (!osync_plugin_advancedoption_param_get_value(param)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Value for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Value", BAD_CAST osync_plugin_advancedoption_param_get_value(param));

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

static osync_bool _osync_plugin_config_assemble_advancedoption(xmlNode *cur, OSyncPluginAdvancedOption *option, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, option, error);

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"AdvancedOption", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	/* DisplayName */
	if (osync_plugin_advancedoption_get_displayname(option))
		xmlNewChild(node, NULL, BAD_CAST "DisplayName", 
				BAD_CAST osync_plugin_advancedoption_get_displayname(option));

	/* MaxOccurs */
	if (osync_plugin_advancedoption_get_maxoccurs(option)) {
		char *str = g_strdup_printf("%u", osync_plugin_advancedoption_get_maxoccurs(option));
		xmlNewChild(node, NULL, BAD_CAST "MaxOccurs", BAD_CAST str);
		g_free(str);
	}

	/* MaxSize */
	if (osync_plugin_advancedoption_get_maxsize(option)) {
		char *str = g_strdup_printf("%u", osync_plugin_advancedoption_get_maxsize(option));
		xmlNewChild(node, NULL, BAD_CAST "MaxSize", BAD_CAST str);
		g_free(str);
	}

	/* Name */
	if (!osync_plugin_advancedoption_get_name(option)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Name for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Name", BAD_CAST osync_plugin_advancedoption_get_name(option));

	/* Parameters */
	OSyncList *p;
	for (p = osync_plugin_advancedoption_get_parameters(option); p; p = p->next) {
		OSyncPluginAdvancedOptionParameter *param = p->data;
		if (!_osync_plugin_config_assemble_advancedoption_param(node, param, error))
			goto error;
	}

	/* Type */
	if (!osync_plugin_advancedoption_get_type(option)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Type for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Type", BAD_CAST osync_plugin_advancedoption_get_type_string(option));

	/* ValEnum */
	OSyncList *v;
	for (v = osync_plugin_advancedoption_get_valenums(option); v; v = v->next) {
		char *valenum = v->data;
		xmlNewChild(node, NULL, BAD_CAST "ValEnum", BAD_CAST valenum);
	}

	/* Value */
	if (!osync_plugin_advancedoption_get_value(option)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Value for advanced option not set.");
		goto error;
	}

	xmlNewChild(node, NULL, BAD_CAST "Value", BAD_CAST osync_plugin_advancedoption_get_value(option));


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

static osync_bool _osync_plugin_config_assemble_advancedoptions(xmlNode *cur, OSyncList *options, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, cur, options, error);

	OSyncList *o;

	xmlNode *node = xmlNewChild(cur, NULL, (xmlChar*)"AdvancedOptions", NULL);
	if (!node) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "No memory left to assemble configuration.");
		goto error;
	}

	for (o = options; o; o = o->next)
		if (!_osync_plugin_config_assemble_advancedoption(node, o->data, error))
			goto error;


	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

OSyncPluginConfig *osync_plugin_config_new(OSyncError **error)
{
	OSyncPluginConfig *config = osync_try_malloc0(sizeof(OSyncPluginConfig), error);
	if (!config)
		return NULL;

	config->ref_count = 1;

	return config;
}

void osync_plugin_config_unref(OSyncPluginConfig *config)
{
	osync_assert(config);

	if (g_atomic_int_dec_and_test(&(config->ref_count))) {
		if (config->connection)
			osync_plugin_connection_unref(config->connection);

		if (config->localization)
			osync_plugin_localization_unref(config->localization);
			
		if (config->authentication)
			osync_plugin_authentication_unref(config->authentication);

		while (config->advancedoptions) {
			OSyncPluginAdvancedOption *opt = config->advancedoptions->data;
			osync_plugin_advancedoption_unref(opt);
			config->advancedoptions = osync_list_remove(config->advancedoptions, opt);
		}

		while (config->resources) {
			OSyncPluginResource *res = config->resources->data;
			osync_plugin_resource_unref(res);
			config->resources = osync_list_remove(config->resources, res);
		}
			
		g_free(config);
	}
}

OSyncPluginConfig *osync_plugin_config_ref(OSyncPluginConfig *config)
{
	osync_assert(config);
	
	g_atomic_int_inc(&(config->ref_count));

	return config;
}

osync_bool osync_plugin_config_file_save(OSyncPluginConfig *config, const char *path, OSyncError **error)
{
	osync_assert(config);
	osync_assert(path);

	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, config, __NULLSTR(path));

	xmlDocPtr doc = NULL;
	OSyncPluginConnection *conn;
	OSyncPluginAuthentication *auth;
	OSyncPluginLocalization *local;
	OSyncList *resources;
	OSyncList *options;
	
	doc = xmlNewDoc((xmlChar*)"1.0");
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't allocate memory to assemble configuration file."); 
		goto error;
	}
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)"config", NULL);
	if (!doc->children) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Couldn't allocate memory to assemble root node for configuration file."); 
		goto error_and_free;
	}

	/* Set version for plugin configuration  */
	char *version_str = g_strdup_printf("%u.%u", OSYNC_PLUGIN_MAJOR_VERSION, OSYNC_PLUGIN_MINOR_VERSION);
	xmlSetProp(doc->children, (const xmlChar*)"version", (const xmlChar *)version_str);
	g_free(version_str);

	/** Assemble... */
	/* Advanced Options */
	if ((options = osync_plugin_config_get_advancedoptions(config)))
		if (!_osync_plugin_config_assemble_advancedoptions(doc->children, options, error))
			goto error_and_free;

	/* Authentication */
	if ((auth = osync_plugin_config_get_authentication(config)))
		if (!_osync_plugin_config_assemble_authentication(doc->children, auth, error))
			goto error_and_free;

	/* Connection */
	if ((conn = osync_plugin_config_get_connection(config)))
		if (!_osync_plugin_config_assemble_connection(doc->children, conn, error))
			goto error_and_free;

	/* Localization */
	if ((local = osync_plugin_config_get_localization(config)))
		if (!_osync_plugin_config_assemble_localization(doc->children, local, error))
			goto error_and_free;

	/* Resources */
	if ((resources = osync_plugin_config_get_resources(config)))
		if (!_osync_plugin_config_assemble_resources(doc->children, resources, error))
			goto error_and_free;

	xmlSaveFormatFile(path, doc, 1);

	xmlFreeDoc(doc);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_and_free:	
	xmlFreeDoc(doc);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}


/* Advanced Options */
OSyncList *osync_plugin_config_get_advancedoptions(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->advancedoptions;

}

OSyncPluginAdvancedOption *osync_plugin_config_get_advancedoption_value_by_name(OSyncPluginConfig *config, const char *name)
{
	OSyncList *opt;

	osync_assert(config);
	osync_assert(name);

	for (opt = config->advancedoptions; opt; opt = opt->next) {
		OSyncPluginAdvancedOption *option = opt->data; 
		const char *opt_name = osync_plugin_advancedoption_get_name(option);
		osync_assert(opt_name);

		if (!strcmp(opt_name, name))
			return option;
	}

	return NULL;
}

void osync_plugin_config_add_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option)
{
	osync_assert(config);
	osync_assert(option);

	if (osync_list_find(config->advancedoptions, option))
			return;

	osync_plugin_advancedoption_ref(option);
	config->advancedoptions = osync_list_prepend(config->advancedoptions, option);
}

void osync_plugin_config_remove_advancedoption(OSyncPluginConfig *config, OSyncPluginAdvancedOption *option)
{
	osync_assert(config);
	osync_assert(option);

	config->advancedoptions = osync_list_remove(config->advancedoptions, option);
	osync_plugin_advancedoption_unref(option);
}

/* Authentication */
OSyncPluginAuthentication *osync_plugin_config_get_authentication(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->authentication;
}

void osync_plugin_config_set_authentication(OSyncPluginConfig *config, OSyncPluginAuthentication *authentication)
{
	osync_assert(config);
	
	if (config->authentication) {
		osync_plugin_authentication_unref(config->authentication);
		config->authentication = NULL;
	}
	
	if (authentication) {
		config->authentication = osync_plugin_authentication_ref(authentication);
	}
}

/* Localization */
OSyncPluginLocalization *osync_plugin_config_get_localization(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->localization;
}

void osync_plugin_config_set_localization(OSyncPluginConfig *config, OSyncPluginLocalization *localization)
{
	osync_assert(config);
	
	if (config->localization) {
		osync_plugin_localization_unref(config->localization);
		config->localization = NULL;
	}
	if (localization) {
		config->localization = osync_plugin_localization_ref(localization);
	}
}

/* Resources */
OSyncList *osync_plugin_config_get_resources(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->resources;

}

void osync_plugin_config_add_resource(OSyncPluginConfig *config, OSyncPluginResource *resource)
{
	osync_assert(config);
	osync_assert(resource);

	if (osync_list_find(config->resources, resource))
			return;

	osync_plugin_resource_ref(resource);
	config->resources = osync_list_append(config->resources, resource);
}

void osync_plugin_config_remove_resource(OSyncPluginConfig *config, OSyncPluginResource *resource)
{
	osync_assert(config);
	osync_assert(resource);

	config->resources = osync_list_remove(config->resources, resource);
	osync_plugin_resource_unref(resource);
}

OSyncPluginResource *osync_plugin_config_find_active_resource(OSyncPluginConfig *config, const char *objtype)
{
	osync_assert(config);
	osync_assert(objtype);

	OSyncList *r;
	for (r = config->resources; r; r = r->next) {
		OSyncPluginResource *res = r->data;

		if (!osync_plugin_resource_is_enabled(res))
			continue;

		const char *res_objtype = osync_plugin_resource_get_objtype(res);
		if (!res_objtype)
			continue;

		if (!strcmp(res_objtype, objtype))
			return res;

	}

	return NULL;
}


OSyncPluginConnection *osync_plugin_config_get_connection(OSyncPluginConfig *config)
{
	osync_assert(config);
	return config->connection;
}

void osync_plugin_config_set_connection(OSyncPluginConfig *config, OSyncPluginConnection *connection)
{
	osync_assert(config);

	if (config->connection) {
		osync_plugin_connection_unref(config->connection);
		config->connection = NULL;
	}
	if (connection) {
		config->connection = osync_plugin_connection_ref(connection);
	}
}

