/*
 * libopensync - A synchronization framework
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@gmail.com>
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
 * Author: Bjoern Ricks <bjoern.ricks@gmail.com>
 * 
 */

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-xmlformat.h"
#include "opensync-xmlformat_internals.h"

static GList *schemas = NULL;
static GStaticMutex schema_mutex = G_STATIC_MUTEX_INIT;

static void _osync_xmlformat_schema_lock_mutex() {
	g_static_mutex_lock(&schema_mutex);
}

static void _osync_xmlformat_schema_unlock_mutex() {
	g_static_mutex_unlock(&schema_mutex);
}


/**
 * @defgroup OSyncXMLFormatPrivateAPI OpenSync XMLFormat Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFormat
 * 
 */
/*@{*/

/**
 * @brief Create new OSyncXMLFormatSchema for xmlformat
 * @param xmlformat The pointer to a xmlformat object. xmlformat->objtype is used to identify the schema file
 * @param path The individual schema path. If NULL the default OPENSYNC_SCHEMASDIR is used.
 * @param error The error which will hold the info in case of an error
 * @return new OSyncXMLFormatSchema or NULL in case of an error
 */
OSyncXMLFormatSchema * osync_xmlformat_schema_new(OSyncXMLFormat *xmlformat, const char *path, OSyncError **error) {
	OSyncXMLFormatSchema * osyncschema = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, xmlformat, path, error);
	
	osync_assert(xmlformat);
	
	osyncschema = osync_try_malloc0(sizeof(OSyncXMLFormatSchema), error);
	if(!osyncschema) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		// release mutex
		return NULL;
	}
	osyncschema->objtype = g_strdup(osync_xmlformat_get_objtype(xmlformat));

	char *schemafilepath = g_strdup_printf("%s%c%s%s%s",
		path ? path : OPENSYNC_SCHEMASDIR,
		G_DIR_SEPARATOR,
		"xmlformat-",
		osyncschema->objtype,
		".xsd");

	osyncschema->ref_count = 1;

 	xmlSchemaParserCtxtPtr xmlSchemaParserCtxt;
	
 	xmlSchemaParserCtxt = xmlSchemaNewParserCtxt(schemafilepath);
	g_free(schemafilepath);
	if ( xmlSchemaParserCtxt == NULL ) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Creation of new XMLFormatSchema failed. Could not create schema parser context.");
		goto error;
	}
 	osyncschema->schema = xmlSchemaParse(xmlSchemaParserCtxt);
 	xmlSchemaFreeParserCtxt(xmlSchemaParserCtxt);
	if ( osyncschema->schema == NULL ) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Creation of new XMLFormatSchema failed. Could not read schema file.");
		goto error;
	}

 	osyncschema->context = xmlSchemaNewValidCtxt(osyncschema->schema);
 	if (osyncschema->context == NULL) {
 		xmlSchemaFree(osyncschema->schema);
 		osync_error_set(error, OSYNC_ERROR_GENERIC, "Creation of new XMLFormatSchema failed. Could not create schema validation context.");
		goto error;
 	}
	osync_trace(TRACE_EXIT, "%s", __func__ );
 	return osyncschema;
error:
 	g_free(osyncschema->objtype);
 	g_free(osyncschema);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/**
 * @brief Get a  schema for the xmlformat.
 *
 * This function creates only one instance of a schema for each objtype. If a xmlformat is passed as a parameter with the same
 * objtype as a xmlformat prior the returned pointer to a OSyncXMLFormatSchema instance is the same as before.
 *
 * @param xmlformat The pointer to a xmlformat object
 * @param schemadir Path of the dir where the schema files are found.  
 * @param error The error which will hold the info in case of an error
 * @return Pointer to a instance of OSyncXMLFormatSchema
 */

OSyncXMLFormatSchema *osync_xmlformat_schema_get_instance_with_path(OSyncXMLFormat *xmlformat, const char *schemadir, OSyncError **error) {
	OSyncXMLFormatSchema *osyncschema = NULL;
	GList * entry;
	const char *objtype;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, xmlformat, error);
	
	osync_assert(xmlformat);
	
	objtype = osync_xmlformat_get_objtype(xmlformat);

	// get mutex
	_osync_xmlformat_schema_lock_mutex();
	// find schema for objtype
	for ( entry = g_list_first(schemas); entry != NULL; entry = g_list_next(entry)) { // should be fast enough for only a few objtypes
		osyncschema = (OSyncXMLFormatSchema *) entry->data;
		osync_assert(osyncschema->objtype);
		if (!strcmp(osyncschema->objtype, objtype) ) {
			osync_xmlformat_schema_ref(osyncschema);
			goto exit;
		}
		osyncschema = NULL;
	}
	if ( osyncschema == NULL ) {
		osyncschema = osync_xmlformat_schema_new(xmlformat, schemadir, error);
		if ( osyncschema != NULL ) {
			schemas = g_list_append(schemas, osyncschema);
		}
	}
exit:
	// release mutex
	_osync_xmlformat_schema_unlock_mutex();
	osync_trace(TRACE_EXIT, "%s, (%p)", __func__, osyncschema );
	return osyncschema;	
}

/*@}*/

/**
 * @defgroup OSyncXMLFormatAPI OpenSync XMLFormat
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFormat
 * 
 */
/*@{*/

/**
 * @brief Get a  schema for the xmlformat.
 *
 * This function creates only one instance of a schema for each objtype. If a xmlformat is passed as a parameter with the same
 * objtype as a xmlformat prior the returned pointer to a OSyncXMLFormatSchema instance is the same as before.
 *
 * @param xmlformat The pointer to a xmlformat object 
 * @param error The error which will hold the info in case of an error
 * @return Pointer to a instance of OSyncXMLFormatSchema
 */

OSyncXMLFormatSchema * osync_xmlformat_schema_get_instance(OSyncXMLFormat *xmlformat, OSyncError **error) {
	return osync_xmlformat_schema_get_instance_with_path(xmlformat, NULL, error);	
}

/**
 * @brief Validate the xmlformat against its schema
 * @param xmlformat The pointer to a xmlformat object 
 * @param schema The pointer to a OSyncXMLFormatSchema object.
 * @param error The error which will hold the info in case of an error
 * @return TRUE if xmlformat valid else FALSE
 */

osync_bool osync_xmlformat_schema_validate(OSyncXMLFormatSchema *schema, OSyncXMLFormat *xmlformat, OSyncError **error)
{
	osync_assert(xmlformat);
	osync_assert(schema);
	
	int rc = 0;

	/* Validate the document */
	rc = xmlSchemaValidateDoc(schema->context, xmlformat->doc);

	if(rc != 0) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "XMLFormat validation failed.");
 		return FALSE;
	}
	return TRUE;
}

/**
 * @brief Decrement the reference counter. The OSyncXMLFormatSchema object will 
 *  be freed if there is no more reference to it.
 * @param osyncschema Pointer to the Schema that shoud be freed
 * @param error The error which will hold the info in case of an error
 */
void osync_xmlformat_schema_unref(OSyncXMLFormatSchema *osyncschema) {

	osync_assert(osyncschema);

	if (g_atomic_int_dec_and_test(&(osyncschema->ref_count))) {
		_osync_xmlformat_schema_lock_mutex();
		schemas = g_list_remove(schemas, osyncschema);
		_osync_xmlformat_schema_unlock_mutex();
		xmlSchemaFreeValidCtxt(osyncschema->context);
		xmlSchemaFree(osyncschema->schema);
		g_free(osyncschema->objtype);
		g_free(osyncschema);
	}
	
}


/**
 * @brief Increments the reference counter
 * @param osyncschema The pointer to a OSyncXMLFormatSchema object
 */
OSyncXMLFormatSchema *osync_xmlformat_schema_ref(OSyncXMLFormatSchema *osyncschema)
{
	osync_assert(osyncschema);
	
	g_atomic_int_inc(&(osyncschema->ref_count));

	return osyncschema;
}
/*@}*/
