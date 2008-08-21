#include "syncml_common.h"
#include<opensync/db/opensync_db.h>

const char *get_database_pref_content_type(
				SmlDatabase *database,
				OSyncError **error)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    g_assert(database);
    g_assert(database->objformat);
    const char *ct = NULL;

    const char *objtype = osync_objformat_get_objtype(database->objformat);
    const char *name = osync_objformat_get_name(database->objformat);

    if (!strcmp(objtype, "contact")) {
        if (strstr(name, "21") > 0)
            ct = SML_ELEMENT_TEXT_VCARD;
        else
            ct = SML_ELEMENT_TEXT_VCARD_30;
    } else if (!strcmp(objtype, "event") ||
               !strcmp(objtype, "todo")) {
        if (strstr(name, "10") > 0)
            ct = SML_ELEMENT_TEXT_VCAL;
        else
            ct = SML_ELEMENT_TEXT_ICAL;
    } else if (!strcmp(objtype, "note") ||
               !strcmp(objtype, "data")) {
        ct = SML_ELEMENT_TEXT_PLAIN;
    } else {
        osync_trace(TRACE_EXIT_ERROR, "%s - unknown objtype %s found", __func__, name);
        osync_error_set(error, OSYNC_ERROR_GENERIC,
                        "content type for object type unknown (%s)",
                        objtype);
        return NULL;
    }

    osync_trace(TRACE_EXIT, "%s - %s", __func__, ct);
    return ct;
}

/* ************************ */
/*      CACHE DATABASE      */
/* ************************ */

SmlBool init_devinf_database_schema(OSyncDB *db, OSyncError **oerror)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    unsigned int db_schema_version = 1;
    SmlBool schema_update = FALSE;

    /* check if all necessary tables exist */
    osync_trace(TRACE_INTERNAL, "%s - Does all tables exist?", __func__);
    if (osync_db_table_exists(db, "versions", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "devices", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "datastores", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "datastore_rx", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "datastore_tx", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "content_type_capabilities", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "properties", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "property_values", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "property_params", oerror) < 1)
	schema_update = TRUE;
    if (osync_db_table_exists(db, "property_param_values", oerror) < 1)
	schema_update = TRUE;

    /* check the version of the database schema */
    if (!schema_update)
    {
        osync_trace(TRACE_INTERNAL, "%s - Has the database schema the correct version?", __func__);
        const char *version_query = "SELECT \"version\" FROM versions WHERE \"name\"='devinf_schema'";
        OSyncList *result = osync_db_query_table(db, version_query, oerror);
        if (!result && *oerror)
        {
            osync_trace(TRACE_INTERNAL, "%s - There is trouble with the table versions.", __func__);
            schema_update = TRUE;
            osync_error_unref(oerror);
            *oerror = NULL;
        }
        else if (!result ||
                 !((OSyncList *) result)->data ||
                 !((OSyncList *) ((OSyncList *) result)->data)->data)
        {
            /* no row returned or empty version */
            osync_trace(TRACE_INTERNAL, "%s - No version found.", __func__);
            schema_update = TRUE;
        }
        else
        {
            osync_trace(TRACE_INTERNAL, "%s - Evaluating version ...", __func__);
            unsigned int db_version = atoi(((OSyncList *) ((OSyncList *) result)->data)->data);
            if (db_version < db_schema_version)
                schema_update = TRUE;
            else if (db_version > db_schema_version)
            {
                osync_error_set(
                     oerror, OSYNC_ERROR_GENERIC,
                     "The database schema (%d) is newer than the schema version of the plugin (%d).",
                     db_version, db_schema_version);
                goto error;
            }
        }
    }

    /* execute a necessary schema update */
    if (schema_update)
    {
        osync_trace(TRACE_INTERNAL, "%s - Updating to schema %d ...", __func__, db_schema_version);

        /* drop all existing tables */
        if (osync_db_table_exists(db, "versions", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE versions", oerror))
            goto error;
        if (osync_db_table_exists(db, "devices", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE devices", oerror))
            goto error;
        if (osync_db_table_exists(db, "datastores", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE datastores", oerror))
            goto error;
        if (osync_db_table_exists(db, "datastore_rx", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE datastore_rx", oerror))
            goto error;
        if (osync_db_table_exists(db, "datastore_tx", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE datastore_tx", oerror))
            goto error;
        if (osync_db_table_exists(db, "content_type_capabilities", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE content_type_capabilities", oerror))
            goto error;
        if (osync_db_table_exists(db, "properties", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE properties", oerror))
            goto error;
        if (osync_db_table_exists(db, "property_values", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE property_values", oerror))
            goto error;
        if (osync_db_table_exists(db, "property_params", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE property_params", oerror))
            goto error;
        if (osync_db_table_exists(db, "property_param_values", oerror) > 0 &&
            !osync_db_query(db, "DROP TABLE property_param_values", oerror))
            goto error;
        osync_trace(TRACE_INTERNAL, "%s - All tables dropped.", __func__);

        /* create all tables */
        if (!osync_db_query(db, "CREATE TABLE versions (name VARCHAR(64) PRIMARY KEY, version VARCHAR(64))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE devices (device_id VARCHAR(64) PRIMARY KEY, device_type VARCHAR(64), manufacturer VARCHAR(64), model VARCHAR(64), oem VARCHAR(64), sw_version VARCHAR(64), hw_version VARCHAR(64), fw_version VARCHAR(64), utc BOOLEAN, large_objects BOOLEAN, number_of_changes BOOLEAN)", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE datastores (device_id VARCHAR(64), datastore VARCHAR(64), rx_pref_content_type VARCHAR(64), rx_pref_version VARCHAR(64), rx_content_type VARCHAR(64), rx_version VARCHAR(64), tx_pref_content_type VARCHAR(64), tx_pref_version VARCHAR(64), tx_content_type VARCHAR(64), tx_version VARCHAR(64), sync_cap INTEGER, PRIMARY KEY (device_id, datastore))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE datastore_rx (device_id VARCHAR(64), datastore VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), PRIMARY KEY (device_id, datastore, content_type, version))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE datastore_tx (device_id VARCHAR(64), datastore VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), PRIMARY KEY (device_id, datastore, content_type, version))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE content_type_capabilities (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), PRIMARY KEY (device_id, content_type, version))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE properties (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), datatype VARCHAR(64), max_occur INTEGER, max_size INTEGER, no_truncate BOOLEAN, display_name VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE property_values (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_value VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_value))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE property_params (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_param VARCHAR(64), datatype VARCHAR(64), display_name VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_param))", oerror))
            goto error;
        if (!osync_db_query(db, "CREATE TABLE property_param_values (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_param VARCHAR(64), property_param_value VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_param, property_param_value))", oerror))
            goto error;
        osync_trace(TRACE_INTERNAL, "%s - All tables created.", __func__);

        /* insert the actual version */
        const char *insert_version_query = "INSERT INTO versions (\"name\", \"version\") VALUES ('devinf_schema', '%d')";
        char *replace = g_strdup_printf(insert_version_query, db_schema_version);
        if (!osync_db_query(db, replace, oerror))
        {
            safe_cfree(&replace);
            goto error;
        }
        safe_cfree(&replace);
        osync_trace(TRACE_INTERNAL, "%s - Schema version inserted.", __func__);
    }

    osync_trace(TRACE_EXIT, "%s", __func__);
    return TRUE;

error:
    osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
    return FALSE;
}

SmlBool store_devinf(SmlDevInf *devinf, const char *filename, SmlError **error)
{
    osync_trace(TRACE_ENTRY, "%s - %s", __func__, filename);
    g_assert(devinf);
    g_assert(filename);
    g_assert(error);
    OSyncError *oerror;
    SmlBool success = TRUE;

    /* init database stuff */
    OSyncDB *db = osync_db_new(&oerror);
    if (!db) goto oerror;
    if (!osync_db_open(db, filename, &oerror)) goto oerror;
    if (!init_devinf_database_schema(db, &oerror)) goto oerror;

    /* create basic device info */
    char *esc_devid  = osync_db_sql_escape(smlDevInfGetDeviceID(devinf));
    char *replace = NULL;
    {
        // this block internalize some variables
        char *esc_vendor = osync_db_sql_escape(smlDevInfGetManufacturer(devinf));
        char *esc_model  = osync_db_sql_escape(smlDevInfGetModel(devinf));
        char *esc_oem    = osync_db_sql_escape(smlDevInfGetOEM(devinf));
        char *esc_sw     = osync_db_sql_escape(smlDevInfGetSoftwareVersion(devinf));
        char *esc_hw     = osync_db_sql_escape(smlDevInfGetHardwareVersion(devinf));
        char *esc_fw     = osync_db_sql_escape(smlDevInfGetFirmwareVersion(devinf));
        const char *device_query = "REPLACE INTO devices (\"device_id\", \"device_type\", \"manufacturer\", \"model\", \"oem\", \"sw_version\", \"hw_version\", \"fw_version\", \"utc\", \"large_objects\", \"number_of_changes\") VALUES ('%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d')";
        replace = g_strdup_printf(
                                    device_query, esc_devid, smlDevInfGetDeviceType(devinf),
                                    esc_vendor, esc_model, esc_oem,
                                    esc_sw, esc_hw, esc_fw,
                                    smlDevInfSupportsUTC(devinf),
                                    smlDevInfSupportsLargeObjs(devinf),
                                    smlDevInfSupportsNumberOfChanges(devinf));
        success = osync_db_query(db, replace, &oerror);
        if (esc_vendor) safe_cfree(&esc_vendor);
        if (esc_model)  safe_cfree(&esc_model);
        if (esc_oem)    safe_cfree(&esc_oem);
        if (esc_sw)     safe_cfree(&esc_sw);
        if (esc_hw)     safe_cfree(&esc_hw);
        if (esc_fw)     safe_cfree(&esc_fw);
        safe_cfree(&replace);
        if (!success) goto oerror;
    }

    /* create datastore info */
    unsigned int num = smlDevInfNumDataStores(devinf);
    unsigned int i;
    for (i = 0; i < num; i++)
    {
	osync_trace(TRACE_INTERNAL, "%s: adding datastore %d", __func__, i);
	char *ct;
        char *version;
        const SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
        char *esc_datastore = osync_db_sql_escape(smlDevInfDataStoreGetSourceRef(datastore));
	char *esc_rx_pref_ct = NULL;
	char *esc_rx_pref_version = NULL;
	if (smlDevInfDataStoreGetRxPref(datastore, &ct, &version))
	{
            esc_rx_pref_ct = osync_db_sql_escape(ct);
            esc_rx_pref_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added RxPref", __func__);
	char *esc_tx_pref_ct = NULL;
	char *esc_tx_pref_version = NULL;
	if (smlDevInfDataStoreGetTxPref(datastore, &ct, &version))
	{
            esc_tx_pref_ct = osync_db_sql_escape(ct);
            esc_tx_pref_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added TxPref", __func__);
	unsigned int bit;
	unsigned int sync_cap = 0;
	for (bit = 0; bit < 8; bit++)
	{
            if (smlDevInfDataStoreGetSyncCap(datastore, bit))
                sync_cap += 1 << bit;
        }
	osync_trace(TRACE_INTERNAL, "%s: parameters ready", __func__);
        const char*datastore_query = "REPLACE INTO datastores (\"device_id\", \"datastore\", \"rx_pref_content_type\", \"rx_pref_version\", \"tx_pref_content_type\", \"tx_pref_version\", \"sync_cap\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%d')";
        replace = g_strdup_printf(
                                  datastore_query, esc_devid, esc_datastore,
                                  esc_rx_pref_ct, esc_rx_pref_version,
                                  esc_tx_pref_ct, esc_tx_pref_version,
                                  sync_cap);
        success = osync_db_query(db, replace, &oerror);
        safe_cfree(&esc_rx_pref_ct);
        safe_cfree(&esc_rx_pref_version);
        safe_cfree(&esc_tx_pref_ct);
        safe_cfree(&esc_tx_pref_version);
	safe_cfree(&replace);

	/* create Rx of datastore */
	unsigned int numRx = smlDevInfDataStoreNumRx(datastore);
        unsigned int k;
        for (k = 0; k < numRx; k++)
        {
            osync_trace(TRACE_INTERNAL, "%s: adding Rx %d for datastore %d", __func__, k, i);
            const SmlDevInfContentType *ctype = smlDevInfDataStoreGetNthRx(datastore, k);
	    char *rx_ct = smlDevInfContentTypeGetCTType(ctype);
	    char *rx_version = smlDevInfContentTypeGetVerCT(ctype);
            char *esc_rx_ct = osync_db_sql_escape(rx_ct);
            char *esc_rx_version = osync_db_sql_escape(rx_version);
            safe_cfree(&rx_ct);
            safe_cfree(&rx_version);
	    osync_trace(TRACE_INTERNAL, "%s: parameters ready", __func__);
            const char *rx_query = "REPLACE INTO datastore_rx (\"device_id\", \"datastore\", \"content_type\", \"version\") VALUES ('%s', '%s', '%s', '%s')";
            replace = g_strdup_printf(
                                      rx_query, esc_devid, esc_datastore,
                                      esc_rx_ct, esc_rx_version);
            success = osync_db_query(db, replace, &oerror);
            safe_cfree(&esc_rx_ct);
            safe_cfree(&esc_rx_version);
	    safe_cfree(&replace);
	    osync_trace(TRACE_INTERNAL, "%s: added Rx", __func__);
        }

	/* create Tx of datastore */
	unsigned int numTx = smlDevInfDataStoreNumTx(datastore);
        for (k = 0; k < numTx; k++)
        {
            osync_trace(TRACE_INTERNAL, "%s: adding Rx %d for datastore %d", __func__, k, i);
            const SmlDevInfContentType *ctype = smlDevInfDataStoreGetNthTx(datastore, k);
	    char *tx_ct = smlDevInfContentTypeGetCTType(ctype);
	    char *tx_version = smlDevInfContentTypeGetVerCT(ctype);
            char *esc_tx_ct = osync_db_sql_escape(tx_ct);
            char *esc_tx_version = osync_db_sql_escape(tx_version);
            safe_cfree(&tx_ct);
            safe_cfree(&tx_version);
	    osync_trace(TRACE_INTERNAL, "%s: parameters ready", __func__);
            const char *tx_query = "REPLACE INTO datastore_tx (\"device_id\", \"datastore\", \"content_type\", \"version\") VALUES ('%s', '%s', '%s', '%s')";
            replace = g_strdup_printf(
                                      tx_query, esc_devid, esc_datastore,
                                      esc_tx_ct, esc_tx_version);
            success = osync_db_query(db, replace, &oerror);
            safe_cfree(&esc_tx_ct);
            safe_cfree(&esc_tx_version);
	    safe_cfree(&replace);
	    osync_trace(TRACE_INTERNAL, "%s: added Tx", __func__);
        }

        safe_cfree(&esc_datastore);
        if (!success) goto oerror;
    }

    /* create content type capabilities info */
    num = smlDevInfNumCTCaps(devinf);
    for (i = 0; i < num; i++)
    {
        /* adding basic capability info */
	osync_trace(TRACE_INTERNAL, "%s: adding CTCap %d", __func__, i);
        const SmlDevInfCTCap *ctcap = smlDevInfGetNthCTCap(devinf, i);
        char *ct = smlDevInfCTCapGetCTType(ctcap);
        char *version = smlDevInfCTCapGetVerCT(ctcap);
        char *esc_ct = osync_db_sql_escape(ct);
        char *esc_version = osync_db_sql_escape(version);
        safe_cfree(&ct);
        /* SyncML 1.0 and 1.1 send CTCap without version numbers of CTType */
	if (version)
            safe_cfree(&version);
        const char *ctcaps_query = "REPLACE INTO content_type_capabilities (\"device_id\", \"content_type\", \"version\") VALUES ('%s', '%s', '%s')";
        replace = g_strdup_printf(ctcaps_query, esc_devid, esc_ct, esc_version);
        // FIXME: unclean error handling
        success = osync_db_query(db, replace, &oerror);
	safe_cfree(&replace);

        /* adding properties */
	osync_trace(TRACE_INTERNAL, "%s: adding properties", __func__);
        unsigned int propNum = smlDevInfCTCapNumProperties(ctcap);
        unsigned int k;
        for (k = 0; k < propNum; k++)
        {
            /* adding basic property info */
	    osync_trace(TRACE_INTERNAL, "%s: adding property %d", __func__, k);
            const SmlDevInfProperty *property = smlDevInfCTCapGetNthProperty(ctcap, k);
            char *prop_name = smlDevInfPropertyGetPropName(property);
            char *data_type = smlDevInfPropertyGetDataType(property);
            unsigned int max_occur = smlDevInfPropertyGetMaxOccur(property);
            unsigned int max_size = smlDevInfPropertyGetMaxSize(property);
            SmlBool no_truncate = smlDevInfPropertyGetNoTruncate(property);
            char *display_name = smlDevInfPropertyGetDisplayName(property);
            char *esc_prop_name = osync_db_sql_escape(prop_name);
            char *esc_data_type = osync_db_sql_escape(data_type);
            char *esc_display_name = osync_db_sql_escape(display_name);
            if (prop_name)    safe_cfree(&prop_name);
            if (data_type)    safe_cfree(&data_type);
            if (display_name) safe_cfree(&display_name);
            const char *property_query = "REPLACE INTO properties (\"device_id\", \"content_type\", \"version\", \"property\", \"datatype\", \"max_occur\", \"max_size\", \"no_truncate\", \"display_name\") VALUES ('%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d', '%s')";
            replace = g_strdup_printf(property_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_data_type,
                                      max_occur, max_size, no_truncate,
                                      esc_display_name);
            // FIXME: unclean error handling
            success = osync_db_query(db, replace, &oerror);
	    safe_cfree(&replace);
            if (esc_data_type)    safe_cfree(&esc_data_type);
            if (esc_display_name) safe_cfree(&esc_display_name);

            /* adding property values */
	    osync_trace(TRACE_INTERNAL, "%s: adding property values", __func__);
            unsigned int values = smlDevInfPropertyNumValEnums(property);
            unsigned int l;
            for (l = 0; l < values; l++)
            {
	        osync_trace(TRACE_INTERNAL, "%s: adding property value %d", __func__, l);
                char *value = smlDevInfPropertyGetNthValEnum(property, l);
                char *esc_value = osync_db_sql_escape(value);
                safe_cfree(&value);
                const char *prop_value_query = "REPLACE INTO property_values (\"device_id\", \"content_type\", \"version\", \"property\", \"property_value\") VALUES ('%s', '%s', '%s', '%s', '%s')";
                replace = g_strdup_printf(prop_value_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_value);
                // FIXME: unclean error handling
                success = osync_db_query(db, replace, &oerror);
	        safe_cfree(&replace);
                safe_cfree(&esc_value);
	        osync_trace(TRACE_INTERNAL, "%s: adding property value %d", __func__, l);
            }

            /* adding property parameters */
	    osync_trace(TRACE_INTERNAL, "%s: adding property parameters", __func__);
            unsigned int params = smlDevInfPropertyNumPropParams(property);
            for (l = 0; l < params; l++)
            {
                /* adding basic property parameter info */
	        osync_trace(TRACE_INTERNAL, "%s: adding property parameter %d", __func__, l);
                const SmlDevInfPropParam *propParam = smlDevInfPropertyGetNthPropParam(property, l);
                char *param_name = smlDevInfPropParamGetParamName(propParam);
                data_type = smlDevInfPropParamGetDataType(propParam);
                display_name = smlDevInfPropParamGetDisplayName(propParam);
                char *esc_param_name = osync_db_sql_escape(param_name);
                esc_data_type = osync_db_sql_escape(data_type);
                esc_display_name = osync_db_sql_escape(display_name);
                if (data_type)    safe_cfree(&data_type);
                if (display_name) safe_cfree(&display_name);
                const char *prop_param_query = "REPLACE INTO property_params (\"device_id\", \"content_type\", \"version\", \"property\", \"property_param\", \"datatype\", \"display_name\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')";
                replace = g_strdup_printf(prop_param_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_param_name,
                                      esc_data_type, esc_display_name);
                // FIXME: unclean error handling
                success = osync_db_query(db, replace, &oerror);
	        safe_cfree(&replace);
                if (esc_data_type)    safe_cfree(&esc_data_type);
                if (esc_display_name) safe_cfree(&esc_display_name);

                /* adding property parameter values */
	        osync_trace(TRACE_INTERNAL, "%s: adding property parameter values", __func__);
                values = smlDevInfPropParamNumValEnums(propParam);
                unsigned int m;
                for (m = 0; m < values; m++)
                {
	             osync_trace(TRACE_INTERNAL, "%s: adding property parameter value %d", __func__, m);
                     char *value = smlDevInfPropParamGetNthValEnum(propParam, m);
                     char *esc_value = osync_db_sql_escape(value);
                     safe_cfree(&value);
                     const char *param_value_query = "REPLACE INTO property_param_values (\"device_id\", \"content_type\", \"version\", \"property\", \"property_param\", \"property_param_value\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s')";
                     replace = g_strdup_printf(param_value_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_param_name,
                                      esc_value);
                     // FIXME: unclean error handling
                     success = osync_db_query(db, replace, &oerror);
	             safe_cfree(&replace);
                     safe_cfree(&esc_value);
	             osync_trace(TRACE_INTERNAL, "%s: adding property parameter value %d", __func__, m);
                }

                /* cleanup property parameter*/
                safe_cfree(&esc_param_name);
	        osync_trace(TRACE_INTERNAL, "%s: added property parameter %d", __func__, l);
            }

            /* cleanup property */
            safe_cfree(&esc_prop_name);
	    osync_trace(TRACE_INTERNAL, "%s: added property %d", __func__, k);
        }

        /* cleanup capability */
        safe_cfree(&esc_ct);
        /* SyncML 1.0 and 1.1 send CTCap without version numbers of CTType */
	if (esc_version)
            safe_cfree(&esc_version);
        // FIXME: unclean error handling because several times overwritten
        if (!success) goto oerror;
    }

    /* finalize database */
    safe_cfree(&esc_devid);
    if (!osync_db_close(db, &oerror)) goto oerror;
    // FIXME: I cannot unref OSyncDB !?
    // FIXME: Is this an API bug?

    osync_trace(TRACE_EXIT, "%s succeeded", __func__); 
    return TRUE;
oerror:
    smlErrorSet(error, SML_ERROR_GENERIC, "%s", osync_error_print(&oerror));
    osync_error_unref(&oerror);
    osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, smlErrorPrint(error));
    return FALSE;
}

SmlDevInf *load_devinf(const char *devid, const char *filename, SmlError **error)
{
    osync_trace(TRACE_ENTRY, "%s - %s from %s", __func__, devid, filename);
    g_assert(devid);
    g_assert(filename);
    g_assert(error);
    OSyncError *oerror = NULL;
    SmlDevInf *devinf = NULL;

    /* init database stuff */
    OSyncDB *db = osync_db_new(&oerror);
    if (!db) goto oerror;
    if (!osync_db_open(db, filename, &oerror)) goto oerror;
    if (!init_devinf_database_schema(db, &oerror)) goto oerror;

    /* read basic device info */
    char *esc_devid  = osync_db_sql_escape(devid);
    const char *device_query = "SELECT \"device_type\", \"manufacturer\", \"model\", \"oem\", \"sw_version\", \"hw_version\", \"fw_version\", \"utc\", \"large_objects\", \"number_of_changes\" FROM devices WHERE \"device_id\"='%s'";
    char *query = g_strdup_printf(device_query, esc_devid);
    // FIXME: unclean error handling
    OSyncList *result = osync_db_query_table(db, query, &oerror);
    safe_cfree(&query);
    unsigned int count = 0;
    OSyncList *row;
    for (row = result; row; row = row->next)
    {
        count++;
        g_assert(count == 1);
        OSyncList *columns = row->data;

        devinf = smlDevInfNew(devid, atoi(osync_list_nth_data(columns, 0)), error);
        smlDevInfSetManufacturer(devinf, osync_list_nth_data(columns, 1));
        smlDevInfSetModel(devinf, osync_list_nth_data(columns, 2));
        smlDevInfSetOEM(devinf, osync_list_nth_data(columns, 3));
        smlDevInfSetSoftwareVersion(devinf, osync_list_nth_data(columns, 4));
        smlDevInfSetHardwareVersion(devinf, osync_list_nth_data(columns, 5));
        smlDevInfSetFirmwareVersion(devinf, osync_list_nth_data(columns, 6));
        smlDevInfSetSupportsUTC(devinf, atoi(osync_list_nth_data(columns, 7)));
        smlDevInfSetSupportsLargeObjs(devinf, atoi(osync_list_nth_data(columns, 8)));
        smlDevInfSetSupportsNumberOfChanges(devinf, atoi(osync_list_nth_data(columns, 9)));
    }
    osync_db_free_list(result);
    if (count == 0)
    {
        // the device does not exist in this database
        // the caller should ask the remote peer for DevInf
        safe_cfree(&esc_devid);
        osync_trace(TRACE_EXIT, "%s - the device was not found in the database", __func__);
        return FALSE;
    }

    /* read datastore info */
    const char*datastore_query = "SELECT \"datastore\", \"rx_pref_content_type\", \"rx_pref_version\", \"tx_pref_content_type\", \"tx_pref_version\", \"sync_cap\" FROM datastores WHERE \"device_id\"='%s'";
    query = g_strdup_printf(datastore_query, esc_devid);
    // FIXME: unclean error handling
    result = osync_db_query_table(db, query, &oerror);
    safe_cfree(&query);
    for (row = result; row; row = row->next)
    {
        OSyncList *columns = row->data;

        // FIXME: unclean error handling
        char *esc_datastore  = osync_db_sql_escape(osync_list_nth_data(columns, 0));
        SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(osync_list_nth_data(columns, 0), error);
        if (osync_list_nth_data(columns, 1))
            smlDevInfDataStoreSetRxPref(
                datastore,
                osync_list_nth_data(columns, 1),
                osync_list_nth_data(columns, 2));
        if (osync_list_nth_data(columns, 3))
            smlDevInfDataStoreSetTxPref(
                datastore,
                osync_list_nth_data(columns, 3),
                osync_list_nth_data(columns, 4));
        unsigned int sync_cap = atoi(osync_list_nth_data(columns, 5));
        unsigned int bit;
        for (bit = 0; bit < 8; bit++)
        {
            smlDevInfDataStoreSetSyncCap(datastore, bit, (sync_cap & (1 << bit)));
        }

	/* read Rx of datastore */
        const char *rx_query = "SELECT \"content_type\", \"version\" FROM datastore_rx WHERE \"device_id\"='%s' AND \"datastore\"='%s'";
        query = g_strdup_printf(rx_query, esc_devid, esc_datastore);
        // FIXME: unclean error handling
        OSyncList *rx_result = osync_db_query_table(db, query, &oerror);
        safe_cfree(&query);
	OSyncList *rx_row;
        for (rx_row = rx_result; rx_row; rx_row = rx_row->next)
        {
            // FIXME: unclean error handling
            SmlDevInfContentType *ctype = smlDevInfNewContentType(
                                              osync_list_nth_data(columns, 0),
                                              osync_list_nth_data(columns, 1),
                                              error);
            smlDevInfDataStoreAddRx(datastore, ctype);
        }
        osync_db_free_list(rx_result);

	/* read Tx of datastore */
        const char *tx_query = "SELECT \"content_type\", \"version\" FROM datastore_tx WHERE \"device_id\"='%s' AND \"datastore\"='%s'";
        query = g_strdup_printf(tx_query, esc_devid, esc_datastore);
        // FIXME: unclean error handling
        OSyncList *tx_result = osync_db_query_table(db, query, &oerror);
        safe_cfree(&query);
	OSyncList *tx_row;
        for (tx_row = tx_result; tx_row; tx_row = tx_row->next)
        {
            // FIXME: unclean error handling
            SmlDevInfContentType *ctype = smlDevInfNewContentType(
                                              osync_list_nth_data(columns, 0),
                                              osync_list_nth_data(columns, 1),
                                              error);
            smlDevInfDataStoreAddTx(datastore, ctype);
        }
        osync_db_free_list(tx_result);

        /* publish datastore */
        safe_cfree(&esc_datastore);
        smlDevInfAddDataStore(devinf, datastore);
    }
    osync_db_free_list(result);

    /* read content type capabilities info */
    const char *ctcaps_query = "SELECT \"content_type\", \"version\" FROM content_type_capabilities WHERE  \"device_id\"='%s'";
    query = g_strdup_printf(ctcaps_query, esc_devid);
    // FIXME: unclean error handling
    result = osync_db_query_table(db, query, &oerror);
    safe_cfree(&query);
    count = 0;
    for (row = result; row; row = row->next)
    {
        count++;
        OSyncList *columns = row->data;

        SmlDevInfCTCap *ctcap = smlDevInfNewCTCap(error);
	if (!ctcap)
		goto error;

        smlDevInfCTCapSetCTType(ctcap, osync_list_nth_data(columns, 0));
        smlDevInfCTCapSetVerCT(ctcap, osync_list_nth_data(columns, 1));
        smlDevInfAppendCTCap(devinf, ctcap);
        char *esc_ct = osync_db_sql_escape(osync_list_nth_data(columns, 0));
        char *esc_version = osync_db_sql_escape(osync_list_nth_data(columns, 1));

        /* reading property */

        /* reading basic property info */
        const char *property_query = "SELECT \"property\", \"datatype\", \"max_occur\", \"max_size\", \"no_truncate\", \"display_name\" FROM properties WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s'";
        query = g_strdup_printf(property_query, esc_devid, esc_ct, esc_version);
        // FIXME: unclean error handling
        OSyncList *prop_result = osync_db_query_table(db, query, &oerror);
        safe_cfree(&query);
        unsigned int prop_count = 0;
        OSyncList *prop_row;
        for (prop_row = prop_result; prop_row; prop_row = prop_row->next)
        {
            prop_count++;
            OSyncList *prop_columns = prop_row->data;

            SmlDevInfProperty *property = smlDevInfNewProperty(error);
	    if (!property)
		    goto error;

            smlDevInfPropertySetPropName(property, osync_list_nth_data(prop_columns, 0));
            smlDevInfPropertySetDataType(property, osync_list_nth_data(prop_columns, 1));
            smlDevInfPropertySetMaxOccur(property, g_ascii_strtoull(osync_list_nth_data(prop_columns, 2), NULL, 0));
            smlDevInfPropertySetMaxSize(property, g_ascii_strtoull(osync_list_nth_data(prop_columns, 3), NULL, 0));
            if (atoi(osync_list_nth_data(prop_columns, 4)))
                smlDevInfPropertySetNoTruncate(property);
            smlDevInfPropertySetDisplayName(property, osync_list_nth_data(prop_columns, 5));
            smlDevInfCTCapAddProperty(ctcap, property);
            char *esc_prop_name = osync_db_sql_escape(osync_list_nth_data(prop_columns, 0));

            /* reading property values */
            const char *prop_value_query = "SELECT \"property_value\" FROM property_values WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s'";
            query = g_strdup_printf(
                         prop_value_query, esc_devid,
                         esc_ct, esc_version, esc_prop_name);
            // FIXME: unclean error handling
            OSyncList *prop_value_result = osync_db_query_table(db, query, &oerror);
            safe_cfree(&query);
            unsigned int prop_value_count = 0;
            OSyncList *prop_value_row;
            for (prop_value_row = prop_value_result; prop_value_row; prop_value_row = prop_value_row->next)
            {
                prop_value_count++;
                OSyncList *prop_value_columns = prop_value_row->data;

                smlDevInfPropertyAddValEnum(property, osync_list_nth_data(prop_value_columns, 0));
            }
            osync_db_free_list(prop_value_result);

            /* reading property parameters */

            /* reading basic property parameter info */
            const char *prop_param_query = "SELECT \"property_param\", \"datatype\", \"display_name\" FROM property_params WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s'";
            query = g_strdup_printf(
                         prop_param_query, esc_devid,
                         esc_ct, esc_version, esc_prop_name);
            // FIXME: unclean error handling
            OSyncList *prop_param_result = osync_db_query_table(db, query, &oerror);
            safe_cfree(&query);
            unsigned int prop_param_count = 0;
            OSyncList *prop_param_row;
            for (prop_param_row = prop_param_result; prop_param_row; prop_param_row = prop_param_row->next)
            {
                prop_param_count++;
                OSyncList *prop_param_columns = prop_param_row->data;

                SmlDevInfPropParam *prop_param = smlDevInfNewPropParam(error);
		if (!prop_param)
			goto error;

                smlDevInfPropParamSetParamName(prop_param, osync_list_nth_data(prop_param_columns, 0));
                smlDevInfPropParamSetDataType(prop_param, osync_list_nth_data(prop_param_columns, 1));
                smlDevInfPropParamSetDisplayName(prop_param, osync_list_nth_data(prop_param_columns, 2));
                smlDevInfPropertyAddPropParam(property, prop_param);
                char *esc_param_name = osync_db_sql_escape(osync_list_nth_data(prop_param_columns, 0));

                /* reading property parameter values */
                const char *param_value_query = "SELECT \"property_param_value\" FROM property_param_values WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s' AND \"property_param\"='%s'";
                query = g_strdup_printf(
                             param_value_query, esc_devid,
                             esc_ct, esc_version, esc_prop_name,
                             esc_param_name);
                // FIXME: unclean error handling
                OSyncList *param_value_result = osync_db_query_table(db, query, &oerror);
                safe_cfree(&query);
                unsigned int param_value_count = 0;
                OSyncList *param_value_row;
                for (param_value_row = param_value_result; param_value_row; param_value_row = param_value_row->next)
                {
                    param_value_count++;
                    OSyncList *param_value_columns = param_value_row->data;

                    smlDevInfPropParamAddValEnum(prop_param, osync_list_nth_data(param_value_columns, 0));
                }
                osync_db_free_list(param_value_result);
                safe_cfree(&esc_param_name);
            }
            osync_db_free_list(prop_param_result);
            safe_cfree(&esc_prop_name);
        }
        osync_db_free_list(prop_result);
        safe_cfree(&esc_ct);
        safe_cfree(&esc_version);
    }
    osync_db_free_list(result);

    /* finalize database */
    safe_cfree(&esc_devid);
    if (!osync_db_close(db, &oerror)) goto oerror;
    // FIXME: I cannot unref OSyncDB !?
    // FIXME: Is this an API bug?

    osync_trace(TRACE_EXIT, "%s succeeded", __func__); 
    return devinf;
oerror:
    smlErrorSet(error, SML_ERROR_GENERIC, "%s", osync_error_print(&oerror));
    osync_error_unref(&oerror);
error:
    osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, smlErrorPrint(error));
    return NULL;
}

/* ************************************************ */
/* *****     Device Information Callbacks     ***** */
/* ************************************************ */

SmlBool _write_devinf(
		SmlDataSyncObject *dsObject,
		SmlDevInf *devinf,
		void *userdata,
		SmlError **error)
{
	SmlPluginEnv *env = userdata;
	return store_devinf(devinf, env->devinf_path, error);
}

SmlDevInf *_read_devinf(
		SmlDataSyncObject *dsObject,
		const char *devid,
		void *userdata,
		SmlError **error)
{
	SmlPluginEnv *env = userdata;
	return load_devinf(devid, env->devinf_path, error);
}

