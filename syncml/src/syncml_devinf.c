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

SmlDevInfProperty *_add_ctcap_property_by_name(
				SmlDevInfCTCap *ctcap,
				const char *name)
{
    osync_trace(TRACE_ENTRY, "%s (%s)", __func__, name);
    g_assert(ctcap);
    g_assert(name);

    SmlDevInfProperty *prop = smlDevInfNewProperty();
    smlDevInfPropertySetPropName(prop, name);
    smlDevInfCTCapAddProperty(ctcap, prop);

    osync_trace(TRACE_EXIT, "%s", __func__);
    return prop;
}

SmlDevInfProperty *_add_ctcap_property_by_name_value(
				SmlDevInfCTCap *ctcap,
				const char*name,
				const char *value)
{
    osync_trace(TRACE_ENTRY, "%s (%s ::= %s)", __func__, name, value);
    g_assert(ctcap);
    g_assert(name);
    g_assert(value);

    SmlDevInfProperty *prop = _add_ctcap_property_by_name(ctcap, name);
    smlDevInfPropertyAddValEnum(prop, value);

    osync_trace(TRACE_EXIT, "%s", __func__);
    return prop;
}

SmlDevInfPropParam *_add_property_param(SmlDevInfProperty *prop, const char *name)
{
    osync_trace(TRACE_ENTRY, "%s (%s)", __func__, name);
    g_assert(prop);
    g_assert(name);

    SmlDevInfPropParam *param = smlDevInfNewPropParam();
    smlDevInfPropParamSetParamName(param, name);
    smlDevInfPropertyAddPropParam(prop, param);

    osync_trace(TRACE_EXIT, "%s", __func__);
    return param;
}

// FIXME: this function is too static
// FIXME: the properties should be load from the format plugin
void add_devinf_ctcap(SmlDevInf *devinf, const char* cttype, const char *verct)
{
    osync_trace(TRACE_ENTRY, "%s (%s %s)", __func__, cttype, verct);
    g_assert(devinf);
    g_assert(cttype);
    g_assert(verct);

    // first we check for an already configure CTCap
    SmlDevInfContentType *ct = smlDevInfNewContentType(cttype, verct);
    if (smlDevInfGetCTCap(devinf, ct) != NULL)
    {
        smlDevInfFreeContentType(ct);
        osync_trace(TRACE_EXIT, "%s - content type already present in devinf", __func__);
        return;
    } else {
        smlDevInfFreeContentType(ct);
        osync_trace(TRACE_INTERNAL, "new content type detected");
    }

    SmlDevInfCTCap *ctcap;
    SmlDevInfProperty *prop;
    SmlDevInfPropParam *param;
    if (!strcmp(cttype, SML_ELEMENT_TEXT_VCARD) &&
        !strcmp(verct, "2.1"))
    {
        osync_trace(TRACE_INTERNAL, "vCard 2.1 detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_VCARD);
        smlDevInfCTCapSetVerCT(ctcap, "2.1");
        prop = _add_ctcap_property_by_name(ctcap, "ADR");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "HOME");
        smlDevInfPropParamAddValEnum(param, "WORK");
        smlDevInfPropParamAddValEnum(param, "PARCEL");
        smlDevInfPropParamAddValEnum(param, "POSTAL");
        smlDevInfPropParamAddValEnum(param, "INTL");
        smlDevInfPropParamAddValEnum(param, "DOM");
        _add_ctcap_property_by_name(ctcap, "AGENT");
        _add_ctcap_property_by_name(ctcap, "BDAY");
        _add_ctcap_property_by_name_value(ctcap, "BEGIN", "VCARD");
        _add_ctcap_property_by_name_value(ctcap, "END", "VCARD");
        prop = _add_ctcap_property_by_name(ctcap, "EMAIL");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "INTERNET");
        _add_ctcap_property_by_name(ctcap, "FN");
        _add_ctcap_property_by_name(ctcap, "GEO");
        prop = _add_ctcap_property_by_name(ctcap, "KEY");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "X509");
        smlDevInfPropParamAddValEnum(param, "PGP");
        prop = _add_ctcap_property_by_name(ctcap, "LABEL");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "HOME");
        smlDevInfPropParamAddValEnum(param, "WORK");
        smlDevInfPropParamAddValEnum(param, "PARCEL");
        smlDevInfPropParamAddValEnum(param, "POSTAL");
        smlDevInfPropParamAddValEnum(param, "INTL");
        smlDevInfPropParamAddValEnum(param, "DOM");
        prop = _add_ctcap_property_by_name(ctcap, "LOGO");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "JPEG");
        _add_ctcap_property_by_name(ctcap, "MAILER");
        _add_ctcap_property_by_name(ctcap, "N");
        _add_ctcap_property_by_name(ctcap, "NOTE");
        _add_ctcap_property_by_name(ctcap, "ORG");
        prop = _add_ctcap_property_by_name(ctcap, "PHOTO");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "JPEG");
        _add_ctcap_property_by_name(ctcap, "REV");
        _add_ctcap_property_by_name(ctcap, "ROLE");
        prop = _add_ctcap_property_by_name(ctcap, "SOUND");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "AIFF");
        smlDevInfPropParamAddValEnum(param, "PCM");
        smlDevInfPropParamAddValEnum(param, "WAVE");
        prop = _add_ctcap_property_by_name(ctcap, "TEL");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "WORK");
        smlDevInfPropParamAddValEnum(param, "VOICE");
        smlDevInfPropParamAddValEnum(param, "PREF");
        smlDevInfPropParamAddValEnum(param, "PAGER");
        smlDevInfPropParamAddValEnum(param, "MSG");
        smlDevInfPropParamAddValEnum(param, "MODEM");
        smlDevInfPropParamAddValEnum(param, "ISDN");
        smlDevInfPropParamAddValEnum(param, "HOME");
        smlDevInfPropParamAddValEnum(param, "FAX");
        smlDevInfPropParamAddValEnum(param, "CELL");
        smlDevInfPropParamAddValEnum(param, "CAR");
        smlDevInfPropParamAddValEnum(param, "BBS");
        _add_ctcap_property_by_name(ctcap, "TITLE");
        smlDevInfPropertyAddValEnum(prop, "TZ");
        smlDevInfPropertyAddValEnum(prop, "UID");
        prop = _add_ctcap_property_by_name(ctcap, "URL");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "WORK");
        smlDevInfPropParamAddValEnum(param, "HOME");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "2.1");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    else if (!strcmp(cttype, SML_ELEMENT_TEXT_VCARD_30) &&
             !strcmp(verct, "3.0"))
    {
	// FIXME: this is no vCard 3.0 spec
	// FIXME: this is in terms of vCard 3.0 a bug
        osync_trace(TRACE_INTERNAL, "vCard 3.0 detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_VCARD_30);
        smlDevInfCTCapSetVerCT(ctcap, "3.0");
        _add_ctcap_property_by_name_value(ctcap, "BEGIN", "VCARD");
        _add_ctcap_property_by_name_value(ctcap, "END", "VCARD");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "3.0");
        _add_ctcap_property_by_name(ctcap, "REV");
        _add_ctcap_property_by_name(ctcap, "N");
        _add_ctcap_property_by_name(ctcap, "TITLE");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "CLASS");
        _add_ctcap_property_by_name(ctcap, "ORG");
        _add_ctcap_property_by_name(ctcap, "EMAIL");
        _add_ctcap_property_by_name(ctcap, "URL");
        prop = _add_ctcap_property_by_name(ctcap, "TEL");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "CELL");
        smlDevInfPropParamAddValEnum(param, "HOME");
        smlDevInfPropParamAddValEnum(param, "WORK");
        smlDevInfPropParamAddValEnum(param, "FAX");
        smlDevInfPropParamAddValEnum(param, "MODEM");
        smlDevInfPropParamAddValEnum(param, "VOICE");
        prop = _add_ctcap_property_by_name(ctcap, "ADR");
        param = _add_property_param(prop, "TYPE");
        smlDevInfPropParamAddValEnum(param, "HOME");
        smlDevInfPropParamAddValEnum(param, "WORK");
        _add_ctcap_property_by_name(ctcap, "BDAY");
        _add_ctcap_property_by_name(ctcap, "NOTE");
        prop = _add_ctcap_property_by_name(ctcap, "PHOTO");
        _add_property_param(prop, "TYPE");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    /* Oracle collaboration Suite uses the content type to distinguish */
    /* the versions of vCalendar (and iCalendar)                       */
    /* text/x-vcalendar --> VERSION 1.0 (vCalendar)                    */
    /* text/calendar    --> VERSION 2.0 (iCalendar)                    */
    /* So be VERY VERY CAREFUL if you change something here.           */
    else if (!strcmp(cttype, SML_ELEMENT_TEXT_VCAL) &&
             !strcmp(verct, "1.0"))
    {
        osync_trace(TRACE_INTERNAL, "vCalendar 1.0 detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_VCAL);
        smlDevInfCTCapSetVerCT(ctcap, "1.0");
        _add_ctcap_property_by_name(ctcap, "AALARM");
        _add_ctcap_property_by_name(ctcap, "ATTACH");
        prop = _add_ctcap_property_by_name(ctcap, "ATTENDEE");
	_add_property_param(prop, "EXCEPT");
	_add_property_param(prop, "RSVP");
	_add_property_param(prop, "STATUS");
	_add_property_param(prop, "ROLE");
        prop = _add_ctcap_property_by_name(ctcap, "BEGIN");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "COMPLETED");
        prop = _add_ctcap_property_by_name(ctcap, "CLASS");
        smlDevInfPropertyAddValEnum(prop, "PUBLIC");
        smlDevInfPropertyAddValEnum(prop, "PRIVATE");
        smlDevInfPropertyAddValEnum(prop, "CONFIDENTIAL");
        _add_ctcap_property_by_name(ctcap, "DALARM");
        _add_ctcap_property_by_name(ctcap, "DAYLIGHT");
        _add_ctcap_property_by_name(ctcap, "DCREATED");
        _add_ctcap_property_by_name(ctcap, "DESCRIPTION");
        _add_ctcap_property_by_name(ctcap, "DTSTART");
        _add_ctcap_property_by_name(ctcap, "DTEND");
        _add_ctcap_property_by_name(ctcap, "DUE");
        prop = _add_ctcap_property_by_name(ctcap, "END");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name(ctcap, "EXDATE");
        _add_ctcap_property_by_name(ctcap, "LAST-MODIFIED");
        _add_ctcap_property_by_name(ctcap, "LOCATION");
        _add_ctcap_property_by_name(ctcap, "PRIORITY");
        _add_ctcap_property_by_name(ctcap, "RRULE");
        _add_ctcap_property_by_name(ctcap, "STATUS");
        _add_ctcap_property_by_name(ctcap, "SUMMARY");
        _add_ctcap_property_by_name(ctcap, "UID");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "1.0");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    else if (!strcmp(cttype, SML_ELEMENT_TEXT_ICAL) &&
             !strcmp(verct, "2.0"))
    {
        // FIXME: this is no iCal spec !!!
        // FIXME: this is nearly a direct copy&paste from vCal
        // FIXME: this is a bug in terms of iCal
        osync_trace(TRACE_INTERNAL, "iCalendar (vCalendar 2.0) detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_ICAL);
        smlDevInfCTCapSetVerCT(ctcap, "2.0");
        _add_ctcap_property_by_name(ctcap, "AALARM");
        _add_ctcap_property_by_name(ctcap, "ATTACH");
        prop = _add_ctcap_property_by_name(ctcap, "ATTENDEE");
	_add_property_param(prop, "RSVP");
	_add_property_param(prop, "PARTSTAT");
	_add_property_param(prop, "ROLE");
        prop = _add_ctcap_property_by_name(ctcap, "BEGIN");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "COMPLETED");
        prop = _add_ctcap_property_by_name(ctcap, "CLASS");
        smlDevInfPropertyAddValEnum(prop, "PUBLIC");
        smlDevInfPropertyAddValEnum(prop, "PRIVATE");
        smlDevInfPropertyAddValEnum(prop, "CONFIDENTIAL");
        _add_ctcap_property_by_name(ctcap, "DALARM");
        _add_ctcap_property_by_name(ctcap, "DAYLIGHT");
        _add_ctcap_property_by_name(ctcap, "DCREATED");
        _add_ctcap_property_by_name(ctcap, "DESCRIPTION");
        _add_ctcap_property_by_name(ctcap, "DTSTART");
        _add_ctcap_property_by_name(ctcap, "DTEND");
        _add_ctcap_property_by_name(ctcap, "DUE");
        prop = _add_ctcap_property_by_name(ctcap, "END");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name(ctcap, "EXDATE");
        _add_ctcap_property_by_name(ctcap, "LAST-MODIFIED");
        _add_ctcap_property_by_name(ctcap, "LOCATION");
        _add_ctcap_property_by_name(ctcap, "PRIORITY");
        _add_ctcap_property_by_name(ctcap, "RRULE");
        _add_ctcap_property_by_name(ctcap, "STATUS");
        _add_ctcap_property_by_name(ctcap, "SUMMARY");
        _add_ctcap_property_by_name(ctcap, "UID");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "2.0");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    else
    {
        /* trace the missing stuff and create a minimal CTCap */
        osync_trace(TRACE_INTERNAL, "unknown content type - %s %s", cttype, verct);
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, cttype);
        smlDevInfCTCapSetVerCT(ctcap, verct);
    }
 
    osync_trace(TRACE_EXIT, "%s - content type newly added to devinf", __func__);
}

SmlDevInfDataStore *add_devinf_datastore(SmlDevInf *devinf, SmlDatabase *database, OSyncError **error)
{
    osync_trace(TRACE_ENTRY, "%s (%p, %p)", __func__, devinf, database);
    g_assert(database);
    g_assert(database->objformat);
    g_assert(database->url);

    SmlError *serror = NULL;
    SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(database->url, &serror);
    if (!datastore) goto error;

    const char *ct = get_database_pref_content_type(database, error);

    if (!strcmp(ct, SML_ELEMENT_TEXT_VCARD))
    {
        // we prefer actually vCard 2.1
        // because the most cellphones support it
        smlDevInfDataStoreSetRx(datastore, SML_ELEMENT_TEXT_VCARD_30, "3.0");
        smlDevInfDataStoreSetTx(datastore, SML_ELEMENT_TEXT_VCARD_30, "3.0");
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD, "2.1");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD_30, "3.0");
    }
    else if (!strcmp(ct, SML_ELEMENT_TEXT_VCARD_30))
    {
        smlDevInfDataStoreSetRx(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        smlDevInfDataStoreSetTx(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD_30, "3.0");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD_30, "3.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD, "2.1");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD_30, "3.0");
    }
    else if (!strcmp(ct, SML_ELEMENT_TEXT_VCAL))
    {
        smlDevInfDataStoreSetRx(datastore, SML_ELEMENT_TEXT_ICAL, "2.0");
        smlDevInfDataStoreSetTx(datastore, SML_ELEMENT_TEXT_ICAL, "2.0");
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCAL, "1.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_ICAL, "2.0");
    }
    else if (!strcmp(ct, SML_ELEMENT_TEXT_ICAL))
    {
        smlDevInfDataStoreSetRx(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
        smlDevInfDataStoreSetTx(datastore, SML_ELEMENT_TEXT_VCAL, "1.0");
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_ICAL, "2.0");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_ICAL, "2.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCAL, "1.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_ICAL, "2.0");
    }
    else if (!strcmp(ct, SML_ELEMENT_TEXT_PLAIN))
    {
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_PLAIN, "1.0");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_PLAIN, "1.0");
    }
    else
    {
        osync_trace(TRACE_INTERNAL, "%s - unknown content type detected (%s)",
                    __func__, ct);
        if (ct != NULL)
            osync_error_set(error, OSYNC_ERROR_GENERIC,
                            "content-type unknown (%s)",
                            ct);
        goto error;
    }

    // configure supported sync modes
    smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_TWO_WAY, TRUE);
    smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SLOW_SYNC, TRUE);
    // server alerted sync means that the client has to interpret alerts !!!
    // FIXME: we receive alerts but we do nothing with it
    if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT)
        // smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_ONE_WAY_FROM_CLIENT, TRUE);
        osync_trace(TRACE_INTERNAL, "SyncML clients only support SLOW and TWO WAY SYNC");
    else
        smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);

    smlDevInfAddDataStore(devinf, datastore);
    osync_trace(TRACE_EXIT, "%s - content type newly added to devinf", __func__);
    return datastore;
error:
    if (serror)
        osync_error_set(error, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&serror));
    osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
    return NULL;
}

/* ************************ */
/*      CACHE DATABASE      */
/* ************************ */

SmlBool init_devinf_database_schema(OSyncDB *db, OSyncError **oerror)
{

    /* check for table devices */
    if (osync_db_exists(db, "devices", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE devices (device_id VARCHAR(64) PRIMARY KEY, device_type VARCHAR(64), manufacturer VARCHAR(64), model VARCHAR(64), oem VARCHAR(64), sw_version VARCHAR(64), hw_version VARCHAR(64), fw_version VARCHAR(64), utc BOOLEAN, large_objects BOOLEAN, number_of_changes BOOLEAN)", oerror))
        goto error;

    /* check for table datastores */
    if (osync_db_exists(db, "datastores", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE datastores (device_id VARCHAR(64), datastore VARCHAR(64), rx_pref_content_type VARCHAR(64), rx_pref_version VARCHAR(64), rx_content_type VARCHAR(64), rx_version VARCHAR(64), tx_pref_content_type VARCHAR(64), tx_pref_version VARCHAR(64), tx_content_type VARCHAR(64), tx_version VARCHAR(64), sync_cap INTEGER, PRIMARY KEY (device_id, datastore))", oerror))
        goto error;

    /* check for table content type capabilities - CTCap */
    if (osync_db_exists(db, "content_type_capabilities", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE content_type_capabilities (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), PRIMARY KEY (device_id, content_type, version))", oerror))
        goto error;

    /* check for table properties */
    if (osync_db_exists(db, "properties", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE properties (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), datatype VARCHAR(64), max_occur INTEGER, max_size INTEGER, no_truncate BOOLEAN, display_name VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property))", oerror))
        goto error;

    /* check for table property_values */
    if (osync_db_exists(db, "property_values", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE property_values (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_value VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_value))", oerror))
        goto error;

    /* check for table property_params */
    if (osync_db_exists(db, "property_params", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE property_params (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_param VARCHAR(64), datatype VARCHAR(64), display_name VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_param))", oerror))
        goto error;

    /* check for table property_param_values */
    if (osync_db_exists(db, "property_param_values", oerror) < 1 &&
        !osync_db_query(db, "CREATE TABLE property_param_values (device_id VARCHAR(64), content_type VARCHAR(64), version VARCHAR(64), property VARCHAR(64), property_param VARCHAR(64), property_param_value VARCHAR(64), PRIMARY KEY (device_id, content_type, version, property, property_param, property_param_value))", oerror))
        goto error;

    return TRUE;

error:
    return FALSE;
}

SmlBool store_devinf(SmlDevInf *devinf, const char *filename, OSyncError **oerror)
{
    osync_trace(TRACE_ENTRY, "%s - %s", __func__, filename);
    g_assert(devinf);
    g_assert(filename);
    g_assert(oerror);
    SmlBool success = TRUE;
    SmlError *error = NULL;

    /* init database stuff */
    OSyncDB *db = osync_db_new(oerror);
    if (!db) goto error;
    if (!osync_db_open(db, filename, oerror)) goto error;
    if (!init_devinf_database_schema(db, oerror)) goto error;

    /* create basic device info */
    char *esc_devid  = osync_db_sql_escape(smlDevInfGetDeviceID(devinf));
    char *esc_vendor = osync_db_sql_escape(smlDevInfGetManufacturer(devinf));
    char *esc_model  = osync_db_sql_escape(smlDevInfGetModel(devinf));
    char *esc_oem    = osync_db_sql_escape(smlDevInfGetOEM(devinf));
    char *esc_sw     = osync_db_sql_escape(smlDevInfGetSoftwareVersion(devinf));
    char *esc_hw     = osync_db_sql_escape(smlDevInfGetHardwareVersion(devinf));
    char *esc_fw     = osync_db_sql_escape(smlDevInfGetFirmwareVersion(devinf));
    const char *device_query = "REPLACE INTO devices (\"device_id\", \"device_type\", \"manufacturer\", \"model\", \"oem\", \"sw_version\", \"hw_version\", \"fw_version\", \"utc\", \"large_objects\", \"number_of_changes\") VALUES ('%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d')";
    char *replace = g_strdup_printf(
                                    device_query, esc_devid, smlDevInfGetDeviceType(devinf),
                                    esc_vendor, esc_model, esc_oem,
                                    esc_sw, esc_hw, esc_fw,
                                    smlDevInfSupportsUTC(devinf),
                                    smlDevInfSupportsLargeObjs(devinf),
                                    smlDevInfSupportsNumberOfChanges(devinf));
    success = osync_db_query(db, replace, oerror);
    g_free(esc_vendor);
    g_free(esc_model);
    g_free(esc_oem);
    g_free(esc_sw);
    g_free(esc_hw);
    g_free(esc_fw);
    g_free(replace);
    if (!success) goto error;

    /* create datastore info */
    unsigned int num = smlDevInfNumDataStores(devinf);
    unsigned int i;
    for (i = 0; i < num; i++)
    {
	osync_trace(TRACE_INTERNAL, "%s: adding datastore %d", __func__, i);
	char *ct;
        char *version;
        SmlDevInfDataStore *datastore = smlDevInfGetNthDataStore(devinf, i);
        char *esc_datastore = osync_db_sql_escape(smlDevInfDataStoreGetSourceRef(datastore));
	char *esc_rx_pref_ct = NULL;
	char *esc_rx_pref_version = NULL;
	if (smlDevInfDataStoreGetRxPref(datastore, &ct, &version))
	{
            esc_rx_pref_ct = osync_db_sql_escape(ct);
            esc_rx_pref_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added RxPref", __func__);
	char *esc_rx_ct = NULL;
	char *esc_rx_version = NULL;
	if (smlDevInfDataStoreGetRx(datastore, &ct, &version))
	{
            esc_rx_ct = osync_db_sql_escape(ct);
            esc_rx_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added Rx", __func__);
	char *esc_tx_pref_ct = NULL;
	char *esc_tx_pref_version = NULL;
	if (smlDevInfDataStoreGetTxPref(datastore, &ct, &version))
	{
            esc_tx_pref_ct = osync_db_sql_escape(ct);
            esc_tx_pref_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added TxPref", __func__);
	char *esc_tx_ct = NULL;
	char *esc_tx_version = NULL;
	if (smlDevInfDataStoreGetTx(datastore, &ct, &version))
	{
            esc_tx_ct = osync_db_sql_escape(ct);
            esc_tx_version = osync_db_sql_escape(version);
	}
	osync_trace(TRACE_INTERNAL, "%s: added Tx", __func__);
	unsigned int bit;
	unsigned int sync_cap = 0;
	for (bit = 0; bit < 8; bit++)
	{
            if (smlDevInfDataStoreGetSyncCap(datastore, bit))
                sync_cap += 1 << bit;
        }
	osync_trace(TRACE_INTERNAL, "%s: parameters ready", __func__);
        const char*datastore_query = "REPLACE INTO datastores (\"device_id\", \"datastore\", \"rx_pref_content_type\", \"rx_pref_version\", \"rx_content_type\", \"rx_version\", \"tx_pref_content_type\", \"tx_pref_version\", \"tx_content_type\", \"tx_version\", \"sync_cap\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d')";
        replace = g_strdup_printf(
                                  datastore_query, esc_devid, esc_datastore,
                                  esc_rx_pref_ct, esc_rx_pref_version,
                                  esc_rx_ct, esc_rx_version,
                                  esc_tx_pref_ct, esc_tx_pref_version,
                                  esc_tx_ct, esc_tx_version,
                                  sync_cap);
        success = osync_db_query(db, replace, oerror);
        g_free(esc_datastore);
        g_free(esc_rx_pref_ct);
        g_free(esc_rx_pref_version);
        g_free(esc_rx_ct);
        g_free(esc_rx_version);
        g_free(esc_tx_pref_ct);
        g_free(esc_tx_pref_version);
        g_free(esc_tx_ct);
        g_free(esc_tx_version);
	g_free(replace);
        if (!success) goto error;
    }

    /* create content type capabilities info */
    num = smlDevInfNumCTCaps(devinf);
    for (i = 0; i < num; i++)
    {
        /* adding basic capability info */
	osync_trace(TRACE_INTERNAL, "%s: adding CTCap %d", __func__, i);
        SmlDevInfCTCap *ctcap = smlDevInfGetNthCTCap(devinf, i);
        char *ct = smlDevInfCTCapGetCTType(ctcap);
        char *version = smlDevInfCTCapGetVerCT(ctcap);
        char *esc_ct = osync_db_sql_escape(ct);
        char *esc_version = osync_db_sql_escape(version);
        g_free(ct);
        g_free(version);
        const char *ctcaps_query = "REPLACE INTO content_type_capabilities (\"device_id\", \"content_type\", \"version\") VALUES ('%s', '%s', '%s')";
        replace = g_strdup_printf(ctcaps_query, esc_devid, esc_ct, esc_version);
        success = osync_db_query(db, replace, oerror);
	g_free(replace);

        /* adding properties */
	osync_trace(TRACE_INTERNAL, "%s: adding properties", __func__);
        unsigned int propNum = smlDevInfCTCapNumProperties(ctcap);
        unsigned int k;
        for (k = 0; k < propNum; k++)
        {
            /* adding basic property info */
	    osync_trace(TRACE_INTERNAL, "%s: adding property %d", __func__, k);
            SmlDevInfProperty *property = smlDevInfCTCapGetNthProperty(ctcap, k);
            char *prop_name = smlDevInfPropertyGetPropName(property);
            char *data_type = smlDevInfPropertyGetDataType(property);
            unsigned int max_occur = smlDevInfPropertyGetMaxOccur(property);
            unsigned int max_size = smlDevInfPropertyGetMaxSize(property);
            SmlBool no_truncate = smlDevInfPropertyGetNoTruncate(property);
            char *display_name = smlDevInfPropertyGetDisplayName(property);
            char *esc_prop_name = osync_db_sql_escape(prop_name);
            char *esc_data_type = osync_db_sql_escape(data_type);
            char *esc_display_name = osync_db_sql_escape(display_name);
            g_free(prop_name);
            g_free(data_type);
            g_free(display_name);
            const char *property_query = "REPLACE INTO properties (\"device_id\", \"content_type\", \"version\", \"property\", \"datatype\", \"max_occur\", \"max_size\", \"no_truncate\", \"display_name\") VALUES ('%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d', '%s')";
            replace = g_strdup_printf(property_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_data_type,
                                      max_occur, max_size, no_truncate,
                                      esc_display_name);
            success = osync_db_query(db, replace, oerror);
	    g_free(replace);
            g_free(esc_data_type);
            g_free(esc_display_name);

            /* adding property values */
	    osync_trace(TRACE_INTERNAL, "%s: adding property values", __func__);
            unsigned int values = smlDevInfPropertyNumValEnums(property);
            unsigned int l;
            for (l = 0; l < values; l++)
            {
	        osync_trace(TRACE_INTERNAL, "%s: adding property value %d", __func__, l);
                char *value = smlDevInfPropertyGetNthValEnum(property, l);
                char *esc_value = osync_db_sql_escape(value);
                g_free(value);
                const char *prop_value_query = "REPLACE INTO property_values (\"device_id\", \"content_type\", \"version\", \"property\", \"property_value\") VALUES ('%s', '%s', '%s', '%s', '%s')";
                replace = g_strdup_printf(prop_value_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_value);
                success = osync_db_query(db, replace, oerror);
	        g_free(replace);
                g_free(esc_value);
	        osync_trace(TRACE_INTERNAL, "%s: adding property value %d", __func__, l);
            }

            /* adding property parameters */
	    osync_trace(TRACE_INTERNAL, "%s: adding property parameters", __func__);
            unsigned int params = smlDevInfPropertyNumPropParams(property);
            for (l = 0; l < params; l++)
            {
                /* adding basic property parameter info */
	        osync_trace(TRACE_INTERNAL, "%s: adding property parameter %d", __func__, l);
                SmlDevInfPropParam *propParam = smlDevInfPropertyGetNthPropParam(property, l);
                char *param_name = smlDevInfPropParamGetParamName(propParam);
                data_type = smlDevInfPropParamGetDataType(propParam);
                display_name = smlDevInfPropParamGetDisplayName(propParam);
                char *esc_param_name = osync_db_sql_escape(param_name);
                esc_data_type = osync_db_sql_escape(data_type);
                esc_display_name = osync_db_sql_escape(display_name);
                g_free(data_type);
                g_free(display_name);
                const char *prop_param_query = "REPLACE INTO property_params (\"device_id\", \"content_type\", \"version\", \"property\", \"property_param\", \"datatype\", \"display_name\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')";
                replace = g_strdup_printf(prop_param_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_param_name,
                                      esc_data_type, esc_display_name);
                success = osync_db_query(db, replace, oerror);
	        g_free(replace);
                g_free(esc_data_type);
                g_free(esc_display_name);

                /* adding property parameter values */
	        osync_trace(TRACE_INTERNAL, "%s: adding property parameter values", __func__);
                values = smlDevInfPropParamNumValEnums(propParam);
                unsigned int m;
                for (m = 0; m < values; m++)
                {
	             osync_trace(TRACE_INTERNAL, "%s: adding property parameter value %d", __func__, m);
                     char *value = smlDevInfPropParamGetNthValEnum(propParam, m);
                     char *esc_value = osync_db_sql_escape(value);
                     g_free(value);
                     const char *param_value_query = "REPLACE INTO property_param_values (\"device_id\", \"content_type\", \"version\", \"property\", \"property_param\", \"property_param_value\") VALUES ('%s', '%s', '%s', '%s', '%s', '%s')";
                     replace = g_strdup_printf(param_value_query, esc_devid, esc_ct, esc_version,
                                      esc_prop_name, esc_param_name,
                                      esc_value);
                     success = osync_db_query(db, replace, oerror);
	             g_free(replace);
                     g_free(esc_value);
	             osync_trace(TRACE_INTERNAL, "%s: adding property parameter value %d", __func__, m);
                }

                /* cleanup property parameter*/
                g_free(esc_param_name);
	        osync_trace(TRACE_INTERNAL, "%s: added property parameter %d", __func__, l);
            }

            /* cleanup property */
            g_free(esc_prop_name);
	    osync_trace(TRACE_INTERNAL, "%s: added property %d", __func__, k);
        }

        /* cleanup capability */
        g_free(esc_ct);
        g_free(esc_version);
        if (!success) goto error;
    }

    /* finalize database */
    g_free(esc_devid);
    if (!osync_db_close(db, oerror)) goto error;
    // FIXME: I cannot unref OSyncDB !?
    // FIXME: Is this an API bug?

    osync_trace(TRACE_EXIT, "%s succeeded", __func__); 
    return TRUE;
error:
    if (error)
    {
        osync_error_set(oerror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
        smlErrorDeref(&error);
    }
    osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(oerror));
    return FALSE;
}

SmlBool load_remote_devinf(SmlPluginEnv *env, OSyncError **error)
{
	env->remote_devinf = smlDevInfAgentGetDevInf(env->agent);
	if (env->remote_devinf)
	{
		osync_trace(TRACE_INTERNAL, "%s: DevInf was sent.", __func__);
		return store_devinf(env->remote_devinf,
			env->devinf_path, error);
	} else {
		osync_trace(TRACE_INTERNAL, "%s: No DevInf was sent.", __func__);
		if (!load_devinf(
				env->agent,
				smlLocationGetURI(smlSessionGetTarget(env->session)),
				env->devinf_path, &error))
		{
			SmlError *serror = NULL;
       			smlDevInfAgentRequestDevInf(
				env->agent,
				env->session,
				&serror);
		} else {
			env->remote_devinf = smlDevInfAgentGetDevInf(env->agent);
		}
	}
	return TRUE;
}

SmlBool load_devinf(SmlDevInfAgent *agent, const char *devid, const char *filename, OSyncError **oerror)
{
    osync_trace(TRACE_ENTRY, "%s - %s from %s", __func__, devid, filename);
    g_assert(agent);
    g_assert(devid);
    g_assert(filename);
    g_assert(oerror);
    SmlBool success = TRUE;
    SmlError *error = NULL;
    SmlDevInf *devinf = NULL;

    /* init database stuff */
    OSyncDB *db = osync_db_new(oerror);
    if (!db) goto error;
    if (!osync_db_open(db, filename, oerror)) goto error;
    if (!init_devinf_database_schema(db, oerror)) goto error;

    /* read basic device info */
    char *esc_devid  = osync_db_sql_escape(devid);
    const char *device_query = "SELECT \"device_type\", \"manufacturer\", \"model\", \"oem\", \"sw_version\", \"hw_version\", \"fw_version\", \"utc\", \"large_objects\", \"number_of_changes\" FROM devices WHERE \"device_id\"='%s'";
    char *query = g_strdup_printf(device_query, esc_devid);
    GList *result = osync_db_query_table(db, query, oerror);
    g_free(query);
    unsigned int count = 0;
    GList *row;
    for (row = result; row; row = row->next)
    {
        count++;
        g_assert(count == 1);
        GList *columns = row->data;

        devinf = smlDevInfNew(devid, atoi(g_list_nth_data(columns, 0)), &error);
        smlDevInfSetManufacturer(devinf, g_list_nth_data(columns, 1));
        smlDevInfSetModel(devinf, g_list_nth_data(columns, 2));
        smlDevInfSetOEM(devinf, g_list_nth_data(columns, 3));
        smlDevInfSetSoftwareVersion(devinf, g_list_nth_data(columns, 4));
        smlDevInfSetHardwareVersion(devinf, g_list_nth_data(columns, 5));
        smlDevInfSetFirmwareVersion(devinf, g_list_nth_data(columns, 6));
        smlDevInfSetSupportsUTC(devinf, atoi(g_list_nth_data(columns, 7)));
        smlDevInfSetSupportsLargeObjs(devinf, atoi(g_list_nth_data(columns, 8)));
        smlDevInfSetSupportsNumberOfChanges(devinf, atoi(g_list_nth_data(columns, 9)));
    }
    osync_db_free_list(result);
    if (count == 0)
    {
        // the device does not exist in this database
        // the caller should ask the remote peer for DevInf
        g_free(esc_devid);
        osync_trace(TRACE_EXIT, "%s - the device was not found in the database", __func__);
        return FALSE;
    }

    /* read datastore info */
    const char*datastore_query = "SELECT \"datastore\", \"rx_pref_content_type\", \"rx_pref_version\", \"rx_content_type\", \"rx_version\", \"tx_pref_content_type\", \"tx_pref_version\", \"tx_content_type\", \"tx_version\", \"sync_cap\" FROM datastores WHERE \"device_id\"='%s'";
    query = g_strdup_printf(datastore_query, esc_devid);
    result = osync_db_query_table(db, query, oerror);
    g_free(query);
    count = 0;
    for (row = result; row; row = row->next)
    {
        count++;
        GList *columns = row->data;

        SmlDevInfDataStore *datastore = smlDevInfDataStoreNew(g_list_nth_data(columns, 0), &error);
        if (g_list_nth_data(columns, 1))
            smlDevInfDataStoreSetRxPref(
                datastore,
                g_list_nth_data(columns, 1),
                g_list_nth_data(columns, 2));
        if (g_list_nth_data(columns, 3))
            smlDevInfDataStoreSetRx(
                datastore,
                g_list_nth_data(columns, 3),
                g_list_nth_data(columns, 4));
        if (g_list_nth_data(columns, 5))
            smlDevInfDataStoreSetTxPref(
                datastore,
                g_list_nth_data(columns, 5),
                g_list_nth_data(columns, 6));
        if (g_list_nth_data(columns, 7))
            smlDevInfDataStoreSetTx(
                datastore,
                g_list_nth_data(columns, 7),
                g_list_nth_data(columns, 8));
        unsigned int sync_cap = atoi(g_list_nth_data(columns, 9));
        unsigned int bit;
        for (bit = 0; bit < 8; bit++)
        {
            smlDevInfDataStoreSetSyncCap(datastore, bit, (sync_cap & (1 << bit)));
        }
        smlDevInfAddDataStore(devinf, datastore);
    }
    osync_db_free_list(result);

    /* read content type capabilities info */
    const char *ctcaps_query = "SELECT \"content_type\", \"version\" FROM content_type_capabilities WHERE  \"device_id\"='%s'";
    query = g_strdup_printf(ctcaps_query, esc_devid);
    result = osync_db_query_table(db, query, oerror);
    g_free(query);
    count = 0;
    for (row = result; row; row = row->next)
    {
        count++;
        GList *columns = row->data;

        SmlDevInfCTCap *ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, g_list_nth_data(columns, 0));
        smlDevInfCTCapSetVerCT(ctcap, g_list_nth_data(columns, 1));
        smlDevInfAddCTCap(devinf, ctcap);
        char *esc_ct = osync_db_sql_escape(g_list_nth_data(columns, 0));
        char *esc_version = osync_db_sql_escape(g_list_nth_data(columns, 1));

        /* reading property */

        /* reading basic property info */
        const char *property_query = "SELECT \"property\", \"datatype\", \"max_occur\", \"max_size\", \"no_truncate\", \"display_name\" FROM properties WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s'";
        query = g_strdup_printf(property_query, esc_devid, esc_ct, esc_version);
        GList *prop_result = osync_db_query_table(db, query, oerror);
        g_free(query);
        unsigned int prop_count = 0;
        GList *prop_row;
        for (prop_row = prop_result; prop_row; prop_row = prop_row->next)
        {
            prop_count++;
            GList *prop_columns = prop_row->data;

            SmlDevInfProperty *property = smlDevInfNewProperty();
            smlDevInfPropertySetPropName(property, g_list_nth_data(prop_columns, 0));
            smlDevInfPropertySetDataType(property, g_list_nth_data(prop_columns, 1));
            smlDevInfPropertySetMaxOccur(property, g_ascii_strtoull(g_list_nth_data(prop_columns, 2), NULL, 0));
            smlDevInfPropertySetMaxSize(property, g_ascii_strtoull(g_list_nth_data(prop_columns, 3), NULL, 0));
            if (atoi(g_list_nth_data(prop_columns, 4)))
                smlDevInfPropertySetNoTruncate(property);
            smlDevInfPropertySetDisplayName(property, g_list_nth_data(prop_columns, 5));
            smlDevInfCTCapAddProperty(ctcap, property);
            char *esc_prop_name = osync_db_sql_escape(g_list_nth_data(prop_columns, 0));

            /* reading property values */
            const char *prop_value_query = "SELECT \"property_value\" FROM property_values WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s'";
            query = g_strdup_printf(
                         prop_value_query, esc_devid,
                         esc_ct, esc_version, esc_prop_name);
            GList *prop_value_result = osync_db_query_table(db, query, oerror);
            g_free(query);
            unsigned int prop_value_count = 0;
            GList *prop_value_row;
            for (prop_value_row = prop_value_result; prop_value_row; prop_value_row = prop_value_row->next)
            {
                prop_value_count++;
                GList *prop_value_columns = prop_value_row->data;

                smlDevInfPropertyAddValEnum(property, g_list_nth_data(prop_value_columns, 0));
            }
            osync_db_free_list(prop_value_result);

            /* reading property parameters */

            /* reading basic property parameter info */
            const char *prop_param_query = "SELECT \"property_param\", \"datatype\", \"display_name\" FROM property_params WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s'";
            query = g_strdup_printf(
                         prop_param_query, esc_devid,
                         esc_ct, esc_version, esc_prop_name);
            GList *prop_param_result = osync_db_query_table(db, query, oerror);
            g_free(query);
            unsigned int prop_param_count = 0;
            GList *prop_param_row;
            for (prop_param_row = prop_param_result; prop_param_row; prop_param_row = prop_param_row->next)
            {
                prop_param_count++;
                GList *prop_param_columns = prop_param_row->data;

                SmlDevInfPropParam *prop_param = smlDevInfNewPropParam();
                smlDevInfPropParamSetParamName(prop_param, g_list_nth_data(prop_param_columns, 0));
                smlDevInfPropParamSetDataType(prop_param, g_list_nth_data(prop_param_columns, 1));
                smlDevInfPropParamSetDisplayName(prop_param, g_list_nth_data(prop_param_columns, 2));
                smlDevInfPropertyAddPropParam(property, prop_param);
                char *esc_param_name = osync_db_sql_escape(g_list_nth_data(prop_param_columns, 0));

                /* reading property parameter values */
                const char *param_value_query = "SELECT \"property_param_value\" FROM property_param_values WHERE \"device_id\"='%s' AND \"content_type\"='%s' AND \"version\"='%s' AND \"property\"='%s' AND \"property_param\"='%s'";
                query = g_strdup_printf(
                             prop_param_query, esc_devid,
                             esc_ct, esc_version, esc_prop_name,
                             esc_param_name);
                GList *param_value_result = osync_db_query_table(db, query, oerror);
                g_free(query);
                unsigned int param_value_count = 0;
                GList *param_value_row;
                for (param_value_row = param_value_result; param_value_row; param_value_row = param_value_row->next)
                {
                    param_value_count++;
                    GList *param_value_columns = param_value_row->data;

                    smlDevInfPropParamAddValEnum(prop_param, g_list_nth_data(param_value_columns, 0));
                }
                osync_db_free_list(param_value_result);
                g_free(esc_param_name);
            }
            osync_db_free_list(prop_param_result);
            g_free(esc_prop_name);
        }
        osync_db_free_list(prop_result);
        g_free(esc_ct);
        g_free(esc_version);
    }
    osync_db_free_list(result);

    /* finalize database */
    g_free(esc_devid);
    if (!osync_db_close(db, oerror)) goto error;
    // FIXME: I cannot unref OSyncDB !?
    // FIXME: Is this an API bug?

    // the device info is published because it is now complete
    smlDevInfAgentSetDevInf(agent, devinf);
    osync_trace(TRACE_EXIT, "%s succeeded", __func__); 
    return TRUE;
error:
    if (error)
    {
        osync_error_set(oerror, OSYNC_ERROR_GENERIC, "%s", smlErrorPrint(&error));
        smlErrorDeref(&error);
    }
    osync_trace(TRACE_EXIT_ERROR, "%s - %s", __func__, osync_error_print(oerror));
    return FALSE;
}

char *get_devinf_identifier()
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    const char *user = g_get_user_name();
    const char *host = g_get_host_name();
    char *id = g_strjoin("@", user, host, NULL);
    osync_trace(TRACE_EXIT, "%s - %s", __func__, id);
    return id;
    // osync_trace(TRACE_INTERNAL, "%s - %s", __func__, id);
    // char *b64 = g_base64_encode(id, strlen(id));
    // g_free(id);
    // osync_trace(TRACE_EXIT, "%s - %s", __func__, b64);
    // return b64;
}

SmlBool init_env_devinf (SmlPluginEnv *env, SmlDevInfDevTyp type, SmlError **serror)
{
    SmlDevInf *devinf;

    /* fix missing identifier */
    if (!env->identifier)
        env->identifier = get_devinf_identifier();

    if (env->fakeDevice)
    {
        osync_trace(TRACE_INTERNAL, "%s: faking devinf", __func__);
        devinf = smlDevInfNew(env->identifier, SML_DEVINF_DEVTYPE_SMARTPHONE, serror);
        smlDevInfSetManufacturer(devinf, env->fakeManufacturer);
        smlDevInfSetModel(devinf, env->fakeModel);
        smlDevInfSetSoftwareVersion(devinf, env->fakeSoftwareVersion);
    } else {
        osync_trace(TRACE_INTERNAL, "%s: not faking devinf", __func__);
        devinf = smlDevInfNew(env->identifier, type, serror);
        smlDevInfSetSoftwareVersion(devinf, env->fakeSoftwareVersion);
    }
    if (!devinf) goto error;

    // libsyncml definitely supports large objects
    // so we must meet the requirement
    // The default are:
    //     MaxMsgSize    100.000
    //     MaxObjSize 10.000.000 (to support images in contacts)
    smlDevInfSetSupportsNumberOfChanges(devinf, TRUE);
    smlDevInfSetSupportsLargeObjs(devinf, TRUE);
    if (!env->onlyLocaltime)
        smlDevInfSetSupportsUTC(devinf, TRUE);
    if (env->recvLimit < 10000) env->recvLimit = 100000;
    if (env->maxObjSize < 10000) env->maxObjSize = 10000000;

    env->devinf = devinf;

    env->agent = smlDevInfAgentNew(env->devinf, serror);
    if (!env->agent) goto error;
	
    if (!smlDevInfAgentRegister(env->agent, env->manager, serror))
        goto error;

    return TRUE;
error:
    smlDevInfUnref(devinf);
    env->devinf = NULL;
    return FALSE;
}
