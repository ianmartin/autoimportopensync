/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-version.h"
#include "opensync-version_internals.h"

/**
 * @defgroup OSyncVersionPrivateAPI OpenSync Version Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncVersion
 * 
 */
/*@{*/

static int _osync_version_match(char *pattern, char* string, OSyncError **error)
{
#ifndef _WIN32
	osync_assert(pattern);
	osync_assert(string);
	regex_t *preg = osync_try_malloc0(sizeof(regex_t), error);
	if(!preg)
		goto error;
	
	int ret = regcomp(preg, pattern, 0);
	
	char *errbuf;
	size_t errbuf_size;
	if(ret) {
		errbuf_size = regerror(ret, preg, NULL, 0);
		errbuf = osync_try_malloc0(errbuf_size, error);
		if(!errbuf)
			goto error_and_free;
		regerror(ret, preg, errbuf, errbuf_size);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", errbuf);
		g_free(errbuf);
		goto error_and_free;
	}
	
	ret = regexec(preg, string, 0, NULL, 0);
	regfree(preg);
	g_free(preg);

	if(ret != 0) { 
		if(ret == REG_NOMATCH)
			return 0;
		errbuf_size = regerror(ret, preg, NULL, 0);
		errbuf = osync_try_malloc0(errbuf_size, error);
		if(!errbuf)
			goto error;
		regerror(ret, preg, errbuf, errbuf_size);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", errbuf);
		g_free(errbuf);
		goto error;
	}
#else //_WIN32
#warning "_osync_version_match will allways match"
#endif
	return 1;

error_and_free:	
	regfree(preg);
	g_free(preg);
error:
	return -1;
}

/**
 * @brief Loads description from a specified directory and returns an OSyncList
 * with OSyncVersions objects.
 *
 * This internal function allows to adjust the description and schema directories.
 *
 * @param error Pointer to error-struct
 * @param descriptiondir Path to description directory
 * @param schemadir Path to XML schema directory
 * @returns List of OSyncVersion objects, NULL if none found
 */
OSyncList *_osync_version_load_from_descriptions(OSyncError **error, const char *descriptiondir, const char *schemadir)
{
	GDir *dir = NULL;
	GError *gerror = NULL;
	const char *descpath = descriptiondir ? descriptiondir : OPENSYNC_DESCRIPTIONSDIR; 
	const char *schemapath = schemadir ? schemadir : OPENSYNC_SCHEMASDIR; 
	char *filename = NULL;
	const gchar *de = NULL;
	OSyncList *versions = NULL;
	OSyncVersion *version = NULL;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur;
	xmlNodePtr child;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	dir = g_dir_open(descpath, 0, &gerror);
	if (!dir) {
		/* If description directory doesn't exist (e.g. unittests), just ignore this. */
		osync_trace(TRACE_EXIT, "Unable to open directory %s: %s", descpath, gerror->message);
		g_error_free(gerror);
		return NULL;
	}
	
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s%c%s", descpath, G_DIR_SEPARATOR, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || !g_pattern_match_simple("*.xml", filename)) {
			g_free(filename);
			continue;
		}
		
		doc = xmlReadFile(filename, NULL, XML_PARSE_NOBLANKS);
		if(!doc) {
			g_free(filename);
			continue;
		}
		
		g_free(filename);
		
		root = xmlDocGetRootElement(doc);
		if(!root || !xmlStrEqual(root->name, BAD_CAST "versions")) {
			xmlFreeDoc(doc);
			continue;
		}

		char *schemafilepath = g_strdup_printf("%s%c%s", schemapath, G_DIR_SEPARATOR, "descriptions.xsd");
 		osync_bool res = osync_xml_validate_document(doc, schemafilepath);
 		g_free(schemafilepath);

		if(res == FALSE) {
			xmlFreeDoc(doc);
			continue;
		}
		
		cur = root->children;
		for(; cur != NULL; cur = cur->next) {
		
			version = osync_version_new(error);
			if(!version) {
				xmlFreeDoc(doc);
				OSyncList *cur = osync_list_first(versions);
				while(cur) {
					osync_version_unref(cur->data);
					cur = cur->next;	
				}
				goto error;
			}
				
			child = cur->children;
			osync_version_set_plugin(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_priority(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_vendor(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_modelversion(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_firmwareversion(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_softwareversion(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_hardwareversion(version, (const char *)osync_xml_node_get_content(child));
			child = child->next;
			osync_version_set_identifier(version, (const char *)osync_xml_node_get_content(child));
			
			versions = osync_list_append(versions, version);
		}
		
		xmlFreeDoc(doc);
	}
	
	g_dir_close(dir);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, versions);
	return versions;

error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*@}*/

/**
 * @defgroup OSyncVersionAPI OpenSync Version
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncVersion
 * 
 */
/*@{*/

/**
 * @brief Creates a new version object
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated version object or NULL in case of error
 */
OSyncVersion *osync_version_new(OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	OSyncVersion *version = osync_try_malloc0(sizeof(OSyncVersion), error);
	if(!version) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	version->ref_count = 1;
	version->plugin = g_strdup("");
	version->priority = g_strdup("");
	version->vendor = g_strdup("");
	version->modelversion = g_strdup("");
	version->firmwareversion = g_strdup("");
	version->softwareversion = g_strdup("");
	version->hardwareversion = g_strdup("");
	version->identifier = g_strdup("");
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, version);
	return version;
}

/**
 * @brief Increments the reference counter
 * @param version The pointer to a version object
 */
OSyncVersion *osync_version_ref(OSyncVersion *version)
{
	osync_assert(version);
	
	g_atomic_int_inc(&(version->ref_count));

	return version;
}

/**
 * @brief Decrement the reference counter. The version object will 
 *  be freed if there is no more reference to it.
 * @param version The pointer to a version object
 */
void osync_version_unref(OSyncVersion *version)
{
	osync_assert(version);
			
	if (g_atomic_int_dec_and_test(&(version->ref_count))) {

		if(version->plugin)
			g_free(version->plugin);
		if(version->priority)
			g_free(version->priority);
		if(version->vendor)
			g_free(version->vendor);
		if(version->modelversion)
			g_free(version->modelversion);
		if(version->firmwareversion)
			g_free(version->firmwareversion);
		if(version->softwareversion)
			g_free(version->softwareversion);
		if(version->hardwareversion)
			g_free(version->hardwareversion);
		if(version->identifier)
			g_free(version->identifier);

		g_free(version);
	}
}

/**
 * @brief Get Plugin Name
 * @param version The pointer to a version object
 * @returns Plugin Name or NULL
 */
/* FIXME: char* to const char* */
char *osync_version_get_plugin(OSyncVersion *version)
{
	return version->plugin;
}

/**
 * @brief Get Priority 
 * @param version The pointer to a version object
 * @returns Priority or NULL
 */
char *osync_version_get_priority(OSyncVersion *version)
{
	return version->priority;
}

/**
 * @brief Get Vendor 
 * @param version The pointer to a version object
 * @returns Vendor or NULL
 */
char *osync_version_get_vendor(OSyncVersion *version)
{
	return version->vendor;
}

/**
 * @brief Get Model Version
 * @param version The pointer to a version object
 * @returns Model Version or NULL
 */
char *osync_version_get_modelversion(OSyncVersion *version)
{
	return version->modelversion;
}

/**
 * @brief Get Firmware Version
 * @param version The pointer to a version object
 * @returns Firmware Version or NULL
 */
char *osync_version_get_firmwareversion(OSyncVersion *version)
{
	return version->firmwareversion;
}

/**
 * @brief Get Software Version
 * @param version The pointer to a version object
 * @returns Software Version or NULL
 */
char *osync_version_get_softwareversion(OSyncVersion *version)
{
	return version->softwareversion;
}

/**
 * @brief Get Hardware Version
 * @param version The pointer to a version object
 * @returns Hardware Version or NULL
 */
char *osync_version_get_hardwareversion(OSyncVersion *version)
{
	return version->hardwareversion;
}

/**
 * @brief Get Identifier 
 * @param version The pointer to a version object
 * @returns Identifier or NULL
 */
char *osync_version_get_identifier(OSyncVersion *version)
{
	return version->identifier;
}

/**
 * @brief Set Plugin Name 
 * @param version The pointer to a version object
 * @param plugin Plugin Name
 */
void osync_version_set_plugin(OSyncVersion *version, const char *plugin)
{
	if(version->plugin)
		g_free(version->plugin);
	if(!plugin)
		version->plugin = g_strdup("");
	else
		version->plugin = g_strdup(plugin);
}

/**
 * @brief Set Priority
 * @param version The pointer to a version object
 * @param priority Priority
 */
void osync_version_set_priority(OSyncVersion *version, const char *priority)
{
	if(version->priority)
		g_free(version->priority);
	if(!priority)
		version->priority = g_strdup("");
	else
		version->priority =  g_strdup(priority);
}

/**
 * @brief Set Vendor 
 * @param version The pointer to a version object
 * @param vendor Vendor 
 */
void osync_version_set_vendor(OSyncVersion *version, const char *vendor)
{
	if(version->vendor)
		g_free(version->vendor);
	if(!vendor)
		version->vendor = g_strdup("");
	else
		version->vendor =  g_strdup(vendor);
}

/**
 * @brief Set Model Version 
 * @param version The pointer to a version object
 * @param modelversion Model Version 
 */
void osync_version_set_modelversion(OSyncVersion *version, const char *modelversion)
{
	if(version->modelversion)
		g_free(version->modelversion);
	if(!modelversion)
		version->modelversion = g_strdup("");
	else
		version->modelversion =  g_strdup(modelversion);
}

/**
 * @brief Set Firmware Version 
 * @param version The pointer to a version object
 * @param firmwareversion Firmware Version 
 */
void osync_version_set_firmwareversion(OSyncVersion *version, const char *firmwareversion)
{
	if(version->firmwareversion)
		g_free(version->firmwareversion);
	if(!firmwareversion)
		version->firmwareversion = g_strdup("");
	else
		version->firmwareversion =  g_strdup(firmwareversion);
}

/**
 * @brief Set Software Version 
 * @param version The pointer to a version object
 * @param softwareversion Software Version 
 */
void osync_version_set_softwareversion(OSyncVersion *version, const char *softwareversion)
{
	if(version->softwareversion)
		g_free(version->softwareversion);
	if(!softwareversion)
		version->softwareversion = g_strdup("");
	else
		version->softwareversion =  g_strdup(softwareversion);
	
}

/**
 * @brief Set Hardware Version 
 * @param version The pointer to a version object
 * @param hardwareversion Hardware Version 
 */
void osync_version_set_hardwareversion(OSyncVersion *version, const char *hardwareversion)
{
	if(version->hardwareversion)
		g_free(version->hardwareversion);
	if(!hardwareversion)
		version->hardwareversion = g_strdup("");
	else
		version->hardwareversion =  g_strdup(hardwareversion);
}

/**
 * @brief Set Identifier 
 * @param version The pointer to a version object
 * @param identifier Identifier 
 */
void osync_version_set_identifier(OSyncVersion *version, const char *identifier)
{
	if(version->identifier)
		g_free(version->identifier);
	if(!identifier)
		version->identifier = g_strdup("");
	else
		version->identifier =  g_strdup(identifier);
}

/**
 * @brief Matchs the version object with a pattern object and returns the priority
 * of the pattern if it matchs the original version object.
 *
 * @param pattern The pointer to a version object which acts as pattern
 * @param version The version (original) object supplied to find a fitting version pattern
 * @param error Pointer to a error-struct
 * @returns Priority of matching pattern, -1 on error
 */
int osync_version_matches(OSyncVersion *pattern, OSyncVersion *version, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, pattern, version, error);

	osync_assert(pattern);
	osync_assert(version);
	
	int ret;
	
	ret = _osync_version_match(osync_version_get_plugin(pattern), osync_version_get_plugin(version), error);
	if(ret <= 0)
		goto error;
	
	ret = _osync_version_match(osync_version_get_vendor(pattern), osync_version_get_vendor(version), error);
	if(ret <= 0)
		goto error;

	ret = _osync_version_match(osync_version_get_modelversion(pattern), osync_version_get_modelversion(version), error);
	if(ret <= 0)
		goto error;
	
	ret = _osync_version_match(osync_version_get_firmwareversion(pattern), osync_version_get_firmwareversion(version), error);
	if(ret <= 0)
		goto error;
	
	ret = _osync_version_match(osync_version_get_softwareversion(pattern), osync_version_get_softwareversion(version), error);
	if(ret <= 0)
		goto error;
	
	ret = _osync_version_match(osync_version_get_hardwareversion(pattern), osync_version_get_hardwareversion(version), error);
	if(ret <= 0)
		goto error;
	
	ret = atoi(osync_version_get_priority(pattern));

error:
	if(ret >= 0) {
		osync_trace(TRACE_EXIT, "%s: %i" , __func__, ret);
		return ret;
	}
	osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
	return -1;
}

/**
 * @brief Loads description from the default description directory and returns an OSyncList
 * with OSyncVersions objects.
 *
 * @param error Pointer to error-struct
 * @returns List of OSyncVersion objects, NULL if none found
 */
OSyncList *osync_version_load_from_descriptions(OSyncError **error)
{
	return _osync_version_load_from_descriptions(error, NULL, NULL);
}

/**
 * @brief Searching for capabilities for specified OSyncVersion object. 
 *
 * @param version Pointer to OSyncVersion object
 * @param error Pointer to error-struct
 * @returns Pointer to OSyncCapabilties object, NULL if none capabilities are found
 */
OSyncCapabilities *osync_version_find_capabilities(OSyncVersion *version, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, version, error);

	osync_assert(version);

	int priority = -1;
	OSyncVersion *winner = NULL;
	OSyncCapabilities *capabilities = NULL;


	OSyncList *versions = osync_version_load_from_descriptions(error);
	if (*error) /* versions can be null */
		goto error;

	OSyncList *cur = osync_list_first(versions);
	while(cur) {
		int curpriority = osync_version_matches(cur->data, version, error);
		if (curpriority == -1) {
			if (versions)
				osync_list_free(versions);

			if (winner)
				osync_version_unref(winner);

			goto error;
		}

		if( curpriority > 0 && curpriority > priority) {
			if(winner)
				osync_version_unref(winner);

			winner = cur->data;
			osync_version_ref(winner);
			priority = curpriority;
		}
		osync_version_unref(cur->data);
		cur = cur->next;
	}
	osync_list_free(versions);
	
	/* we found or own capabilities */
	if(priority > 0)
	{
		osync_trace(TRACE_INTERNAL, "Found capabilities file by version: %s ", (const char*)osync_version_get_identifier(winner));

		capabilities = osync_capabilities_load((const char*)osync_version_get_identifier(winner), error);
		osync_version_unref(winner);

		if (!capabilities)
			goto error;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;

error:	

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*@}*/
