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

int _osync_version_match(char *pattern, char* string, OSyncError **error)
{
	osync_assert(pattern);
	osync_assert(string);
	regex_t *preg = osync_try_malloc0(sizeof(regex_t), error);
	if(!preg)
		return -1;
	
	int ret = regcomp(preg, pattern, 0);
	
	char *errbuf;
	size_t errbuf_size;
	if(ret) {
		errbuf_size = regerror(ret, preg, NULL, 0);
		errbuf = osync_try_malloc0(errbuf_size, error);
		if(!errbuf)
			return -1;
		regerror(ret, preg, errbuf, errbuf_size);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", errbuf);
		g_free(errbuf);
		return -1;
	}
	
	ret = regexec(preg, string, 0, NULL, 0);
	regfree(preg);
	if(ret != 0) { 
		if(ret == REG_NOMATCH)
			return 0;
		errbuf_size = regerror(ret, preg, NULL, 0);
		errbuf = osync_try_malloc0(errbuf_size, error);
		if(!errbuf)
			return -1;
		regerror(ret, preg, errbuf, errbuf_size);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", errbuf);
		g_free(errbuf);
		return -1;
	}
	return 1;
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
void osync_version_ref(OSyncVersion *version)
{
	osync_assert(version);
	
	g_atomic_int_inc(&(version->ref_count));
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
		g_free(version);
		if(version->plugin)
			g_free(version->plugin);
		if(version->priority)
			g_free(version->priority);
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
	}
}

char *osync_version_get_plugin(OSyncVersion *version)
{
	return version->plugin;
};

char *osync_version_get_priority(OSyncVersion *version)
{
	return version->priority;
};

char *osync_version_get_modelversion(OSyncVersion *version)
{
	return version->modelversion;
};

char *osync_version_get_firmwareversion(OSyncVersion *version)
{
	return version->firmwareversion;
};

char *osync_version_get_softwareversion(OSyncVersion *version)
{
	return version->softwareversion;
};

char *osync_version_get_hardwareversion(OSyncVersion *version)
{
	return version->hardwareversion;
};

char *osync_version_get_identifier(OSyncVersion *version)
{
	return version->identifier;
};

void osync_version_set_plugin(OSyncVersion *version, char *plugin)
{
	if(version->plugin)
		g_free(version->plugin);
	if(!plugin)
		version->plugin = g_strdup("");
	else
		version->plugin = g_strdup(plugin);
};

void osync_version_set_priority(OSyncVersion *version, char *priority)
{
	if(version->priority)
		g_free(version->priority);
	if(!priority)
		version->priority = g_strdup("");
	else
		version->priority =  g_strdup(priority);
};

void osync_version_set_modelversion(OSyncVersion *version, char *modelversion)
{
	if(version->modelversion)
		g_free(version->modelversion);
	if(!modelversion)
		version->modelversion = g_strdup("");
	else
		version->modelversion =  g_strdup(modelversion);
};

void osync_version_set_firmwareversion(OSyncVersion *version, char *firmwareversion)
{
	if(version->firmwareversion)
		g_free(version->firmwareversion);
	if(!firmwareversion)
		version->firmwareversion = g_strdup("");
	else
		version->firmwareversion =  g_strdup(firmwareversion);
};

void osync_version_set_softwareversion(OSyncVersion *version, char *softwareversion)
{
	if(version->softwareversion)
		g_free(version->softwareversion);
	if(!softwareversion)
		version->softwareversion = g_strdup("");
	else
		version->softwareversion =  g_strdup(softwareversion);
	
};
void osync_version_set_hardwareversion(OSyncVersion *version, char *hardwareversion)
{
	if(version->hardwareversion)
		g_free(version->hardwareversion);
	if(!hardwareversion)
		version->hardwareversion = g_strdup("");
	else
		version->hardwareversion =  g_strdup(hardwareversion);
};

void osync_version_set_identifier(OSyncVersion *version, char *identifier)
{
	if(version->identifier)
		g_free(version->identifier);
	if(!identifier)
		version->identifier = g_strdup("");
	else
		version->identifier =  g_strdup(identifier);
};

int osync_version_matches(OSyncVersion *pattern, OSyncVersion *version, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, pattern, version, error);

	osync_assert(pattern);
	osync_assert(version);
	
	int ret;
	
	ret = _osync_version_match(osync_version_get_plugin(pattern), osync_version_get_plugin(version), error);
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
};


OSyncList *osync_version_load_from_descriptions(OSyncError **error)
{
	GDir *dir = NULL;
	GError *gerror = NULL;
	const char *path = OPENSYNC_DESCRIPTIONSDIR; 
	char *filename = NULL;
	const gchar *de = NULL;
	OSyncList *versions = NULL;
	OSyncVersion *version = NULL;
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur;
	xmlNodePtr child;
	
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);
	
	dir = g_dir_open(path, 0, &gerror);
	if (!dir) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Unable to open directory %s: %s", path, gerror->message);
		g_error_free(gerror);
		goto error;
	}
	
	while ((de = g_dir_read_name(dir))) {
		filename = g_strdup_printf ("%s/%s", path, de);
		
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_file_test(filename, G_FILE_TEST_IS_SYMLINK) || !g_pattern_match_simple("*.xml", filename)) {
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

		char *schemafilepath = g_strdup_printf("%s%c%s", OPENSYNC_SCHEMASDIR, G_DIR_SEPARATOR, "descriptions.xsd");
 		osync_bool res = osxml_validate_document(doc, schemafilepath);
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
			osync_version_set_plugin(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_priority(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_modelversion(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_firmwareversion(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_softwareversion(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_hardwareversion(version, (char *)osxml_node_get_content(child));
			child = child->next;
			osync_version_set_identifier(version, (char *)osxml_node_get_content(child));
			
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

OSyncCapabilities *osync_version_find_capabilities(OSyncVersion *version, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, version, error);

	osync_assert(version);

	int priority = -1;
	OSyncVersion *winner = NULL;
	OSyncCapabilities *capabilities = NULL;


	OSyncList *versions = osync_version_load_from_descriptions(error);
	if (*error) /* versions can be null */
		goto error_free_version;

	OSyncList *cur = osync_list_first(versions);
	while(cur) {
		int curpriority = osync_version_matches(cur->data, version, error);
		if (curpriority == -1) {
			if (versions)
				osync_list_free(versions);

			if (winner)
				osync_version_unref(winner);

			goto error_free_version;
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
		if (capabilities)
			osync_capabilities_unref(capabilities);

		capabilities = osync_capabilities_load((const char*)osync_version_get_identifier(winner), error);
		osync_version_unref(winner);

		if (!capabilities)
			goto error_free_version;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;

error_free_version:
	if (version)
		osync_version_unref(version);

	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/*@}*/
