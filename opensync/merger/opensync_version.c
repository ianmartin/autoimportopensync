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
#include "opensync-merger_internals.h"

#include "regex.h"

/**
 * @defgroup OSyncVersionPrivateAPI OpenSync Version Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncVersion
 * 
 */
/*@{*/

int _osync_version_match(char *pattern, char* string, OSyncError **error)
{
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
		if(ret == REG_NOMATCH) {
			return 0;
		}
		
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
	version->plugin = NULL;
	version->priority = NULL;
	version->modelversion = NULL;
	version->firmwareversion = NULL;
	version->softwareversion = NULL;
	version->hardwareversion = NULL;
	version->identifier = NULL;
	
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
	version->plugin = g_strdup(plugin);
};

void osync_version_set_priority(OSyncVersion *version, char *priority)
{
	if(version->priority)
		g_free(version->priority);
	version->priority =  g_strdup(priority);
};

void osync_version_set_modelversion(OSyncVersion *version, char *modelversion)
{
	if(version->modelversion)
		g_free(version->modelversion);
	version->modelversion =  g_strdup(modelversion);
};

void osync_version_set_firmwareversion(OSyncVersion *version, char *firmwareversion)
{
	if(version->firmwareversion)
		g_free(version->firmwareversion);
	version->firmwareversion =  g_strdup(firmwareversion);
};

void osync_version_set_softwareversion(OSyncVersion *version, char *softwareversion)
{
	if(version->softwareversion)
		g_free(version->softwareversion);
	version->softwareversion =  g_strdup(softwareversion);
	
};
void osync_version_set_hardwareversion(OSyncVersion *version, char *hardwareversion)
{
	if(version->hardwareversion)
		g_free(version->hardwareversion);
	version->hardwareversion =  g_strdup(hardwareversion);
};

void osync_version_set_identifier(OSyncVersion *version, char *identifier)
{
	if(version->identifier)
		g_free(version->identifier);
	version->identifier =  g_strdup(identifier);
};

int osync_version_matches(OSyncVersion *pattern, OSyncVersion *version, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, pattern, version, error);
	
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

/*@}*/
