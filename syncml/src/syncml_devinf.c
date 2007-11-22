#include "syncml_common.h"

extern const char *get_database_pref_content_type(
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
        ct = SML_ELEMENT_TEXT_VCARD;
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

static SmlDevInfProperty *_add_ctcap_property_by_name(
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

static SmlDevInfProperty *_add_ctcap_property_by_name_value(
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

static SmlDevInfPropParam *_add_property_param(SmlDevInfProperty *prop, const char *name)
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

// FIXME: this function too static
// FIXME: the properties should be load from the format plugin
static void add_devinf_ctcap(SmlDevInf *devinf, const char* cttype, const char *verct)
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
        _add_ctcap_property_by_name_value(ctcap, "BEGIN", "VCARD");
        _add_ctcap_property_by_name_value(ctcap, "END", "VCARD");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "2.1");
        _add_ctcap_property_by_name(ctcap, "REV");
        _add_ctcap_property_by_name(ctcap, "N");
        _add_ctcap_property_by_name(ctcap, "TITLE");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "CLASS");
        _add_ctcap_property_by_name(ctcap, "ORG");
        _add_ctcap_property_by_name(ctcap, "EMAIL");
        _add_ctcap_property_by_name(ctcap, "URL");
        prop = _add_ctcap_property_by_name(ctcap, "TEL");
        smlDevInfPropertyAddValEnum(prop, "CELL");
        smlDevInfPropertyAddValEnum(prop, "HOME");
        smlDevInfPropertyAddValEnum(prop, "WORK");
        smlDevInfPropertyAddValEnum(prop, "FAX");
        smlDevInfPropertyAddValEnum(prop, "MODEM");
        smlDevInfPropertyAddValEnum(prop, "VOICE");
        prop = _add_ctcap_property_by_name(ctcap, "ADR");
        smlDevInfPropertyAddValEnum(prop, "HOME");
        smlDevInfPropertyAddValEnum(prop, "WORK");
        _add_ctcap_property_by_name(ctcap, "BDAY");
        _add_ctcap_property_by_name(ctcap, "NOTE");
        _add_ctcap_property_by_name_value(ctcap, "PHOTO", "TYPE");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    else if (!strcmp(cttype, SML_ELEMENT_TEXT_VCARD) &&
             !strcmp(verct, "3.0"))
    {
        osync_trace(TRACE_INTERNAL, "vCard 3.0 detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_VCARD);
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
        prop = _add_ctcap_property_by_name(ctcap, "BEGIN");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        prop = _add_ctcap_property_by_name(ctcap, "END");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "1.0");
        _add_ctcap_property_by_name(ctcap, "TZ");
        _add_ctcap_property_by_name(ctcap, "LAST-MODIFIED");
        _add_ctcap_property_by_name(ctcap, "DCREATED");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "CLASS");
        _add_ctcap_property_by_name(ctcap, "SUMMARY");
        _add_ctcap_property_by_name(ctcap, "DESCRIPTION");
        _add_ctcap_property_by_name(ctcap, "LOCATION");
        _add_ctcap_property_by_name(ctcap, "DTSTART");
        _add_ctcap_property_by_name(ctcap, "DTEND");
        _add_ctcap_property_by_name(ctcap, "ATTENDEE");
        _add_ctcap_property_by_name(ctcap, "RRULE");
        _add_ctcap_property_by_name(ctcap, "EXDATE");
        _add_ctcap_property_by_name(ctcap, "AALARM");
        _add_ctcap_property_by_name(ctcap, "DALARM");
        _add_ctcap_property_by_name(ctcap, "DUE");
        _add_ctcap_property_by_name(ctcap, "PRIORITY");
        _add_ctcap_property_by_name(ctcap, "STATUS");
        smlDevInfAddCTCap(devinf, ctcap);
    }
    else if (!strcmp(cttype, SML_ELEMENT_TEXT_ICAL) &&
             !strcmp(verct, "2.0"))
    {
        osync_trace(TRACE_INTERNAL, "iCalendar (vCalendar 2.0) detected");
        ctcap = smlDevInfNewCTCap();
        smlDevInfCTCapSetCTType(ctcap, SML_ELEMENT_TEXT_ICAL);
        smlDevInfCTCapSetVerCT(ctcap, "2.0");
        prop = _add_ctcap_property_by_name(ctcap, "BEGIN");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        prop = _add_ctcap_property_by_name(ctcap, "END");
        smlDevInfPropertyAddValEnum(prop, "VCALENDAR");
        smlDevInfPropertyAddValEnum(prop, "VEVENT");
        smlDevInfPropertyAddValEnum(prop, "VTODO");
        _add_ctcap_property_by_name_value(ctcap, "VERSION", "2.0");
        _add_ctcap_property_by_name(ctcap, "TZ");
        _add_ctcap_property_by_name(ctcap, "LAST-MODIFIED");
        _add_ctcap_property_by_name(ctcap, "DCREATED");
        _add_ctcap_property_by_name(ctcap, "CATEGORIES");
        _add_ctcap_property_by_name(ctcap, "CLASS");
        _add_ctcap_property_by_name(ctcap, "SUMMARY");
        _add_ctcap_property_by_name(ctcap, "DESCRIPTION");
        _add_ctcap_property_by_name(ctcap, "LOCATION");
        _add_ctcap_property_by_name(ctcap, "DTSTART");
        _add_ctcap_property_by_name(ctcap, "DTEND");
        _add_ctcap_property_by_name(ctcap, "ATTENDEE");
        _add_ctcap_property_by_name(ctcap, "RRULE");
        _add_ctcap_property_by_name(ctcap, "EXDATE");
        _add_ctcap_property_by_name(ctcap, "AALARM");
        _add_ctcap_property_by_name(ctcap, "DALARM");
        _add_ctcap_property_by_name(ctcap, "DUE");
        _add_ctcap_property_by_name(ctcap, "PRIORITY");
        _add_ctcap_property_by_name(ctcap, "STATUS");
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

extern SmlDevInfDataStore *add_dev_inf_datastore(SmlDevInf *devinf, SmlDatabase *database, OSyncError **error)
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
        // because the most cellphone support it
        smlDevInfDataStoreSetRx(datastore, SML_ELEMENT_TEXT_VCARD, "3.0");
        smlDevInfDataStoreSetTx(datastore, SML_ELEMENT_TEXT_VCARD, "3.0");
        smlDevInfDataStoreSetRxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        smlDevInfDataStoreSetTxPref(datastore, SML_ELEMENT_TEXT_VCARD, "2.1");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD, "2.1");
        add_devinf_ctcap(devinf, SML_ELEMENT_TEXT_VCARD, "3.0");
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
    smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_SERVER_ALERTED_SYNC, TRUE);
    // server alerted sync means that the client has to interpret alerts !!!
    // FIXME: we receive alerts but we do nothing with it
    if (smlDsServerGetServerType(database->server) == SML_DS_CLIENT)
        smlDevInfDataStoreSetSyncCap(datastore, SML_DEVINF_SYNCTYPE_ONE_WAY_FROM_CLIENT, TRUE);
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
