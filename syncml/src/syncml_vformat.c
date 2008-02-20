
#include "syncml_vformat.h"
#include "syncml_common.h"

GHashTable* get_vcard_hash()
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);

    g_hash_table_insert(hash, "BEGIN", "");
    g_hash_table_insert(hash, "VERSION", "");
    g_hash_table_insert(hash, "END", "");

    g_hash_table_insert(hash, "ADR", "Address");
    g_hash_table_insert(hash, "AGENT", "Agent");
    g_hash_table_insert(hash, "BDAY", "Birthday");
    g_hash_table_insert(hash, "CATEGORIES", "Categories");
    g_hash_table_insert(hash, "CLASS", "Class");
    g_hash_table_insert(hash, "EMAIL", "EMail");
    g_hash_table_insert(hash, "FN", "FormattedName");
    g_hash_table_insert(hash, "GEO", "Location");
    g_hash_table_insert(hash, "KEY", "Key");
    g_hash_table_insert(hash, "LABEL", "AddressLabel");
    g_hash_table_insert(hash, "LOGO", "Logo");
    g_hash_table_insert(hash, "MAILER", "Mailer");
    g_hash_table_insert(hash, "N", "Name");
    g_hash_table_insert(hash, "NICKNAME", "Nickname");
    g_hash_table_insert(hash, "NOTE", "Note");
    g_hash_table_insert(hash, "ORG", "Organization");
    g_hash_table_insert(hash, "PHOTO", "Photo");
    g_hash_table_insert(hash, "REV", "Revision");
    g_hash_table_insert(hash, "ROLE", "Role");
    g_hash_table_insert(hash, "SOUND", "Sound");
    g_hash_table_insert(hash, "TEL", "Telephone");
    g_hash_table_insert(hash, "TITLE", "Title");
    g_hash_table_insert(hash, "TZ", "Timezone");
    g_hash_table_insert(hash, "UID", "Uid");
    g_hash_table_insert(hash, "URL", "Url");

    osync_trace(TRACE_EXIT, "%s", __func__);
    return hash;
}

GHashTable* get_ical_hash()
{
    osync_trace(TRACE_ENTRY, "%s", __func__);
    GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);

    g_hash_table_insert(hash, "BEGIN", "");
    g_hash_table_insert(hash, "VERSION", "");
    g_hash_table_insert(hash, "END", "");

    g_hash_table_insert(hash, "AALARM", "Alarm");
    g_hash_table_insert(hash, "ATTACH", "Attach");
    g_hash_table_insert(hash, "ATTENDEE", "Attandee");
    g_hash_table_insert(hash, "CATEGORIES", "Categories");
    g_hash_table_insert(hash, "COMPLETED", "Completed");
    g_hash_table_insert(hash, "CLASS", "Class");
    g_hash_table_insert(hash, "DALARM", "Alarm");
    // ??? g_hash_table_insert(hash, "DAYLIGHT", "");
    g_hash_table_insert(hash, "DCREATED", "DateCalendarCreated");
    g_hash_table_insert(hash, "DESCRIPTION", "Description");
    g_hash_table_insert(hash, "DTSTART", "DateStarted");
    g_hash_table_insert(hash, "DTEND", "DateEnd");
    g_hash_table_insert(hash, "DUE", "Duration");
    g_hash_table_insert(hash, "EXDATE", "ExceptionDateTime");
    g_hash_table_insert(hash, "LAST-MODIFIED", "LastModified");
    g_hash_table_insert(hash, "LOCATION", "Location");
    g_hash_table_insert(hash, "PRIORITY", "Priority");
    g_hash_table_insert(hash, "RRULE", "RecurrenceRule");
    g_hash_table_insert(hash, "STATUS", "Status");
    g_hash_table_insert(hash, "SUMMARY", "Summary");
    g_hash_table_insert(hash, "UID", "Uid");

    osync_trace(TRACE_EXIT, "%s", __func__);
    return hash;
}

SmlBool set_capabilities(SmlPluginEnv *env, OSyncError **error)
{
    osync_trace(TRACE_ENTRY, "%s", __func__);

    /*check the requirements */
    g_assert(env->remote_devinf);
    g_assert(env->pluginInfo);
    osync_trace(TRACE_INTERNAL, "%s: assertions ok", __func__);

    /* create fresh capabilties */
    OSyncCapabilities *caps = osync_capabilities_new(error);
    if (!caps)
    {
        osync_trace(TRACE_EXIT_ERROR, "%s - cannot instantiate capabilties", __func__);
        return FALSE;
    }

    /* now manage all content type capabilities */
    unsigned int capCount = smlDevInfNumCTCaps(env->remote_devinf);
    unsigned int i;
    for (i=0; i < capCount; i++)
    {
        const SmlDevInfCTCap *ctcap = smlDevInfGetNthCTCap(env->remote_devinf, i);

        /* define objtype */
        char *objtype = NULL;
        GHashTable *hash;
        char *cttype = smlDevInfCTCapGetCTType(ctcap);
        if (strstr(cttype, "calendar"))
        {
            objtype = g_strdup("event");
            hash = get_ical_hash();
        } else {
            if (strstr(cttype, "vcard"))
            {
                objtype = g_strdup("contact");
                hash = get_vcard_hash();
            } else {
                objtype = NULL;
                hash = NULL;
            }
        }

        /* iterate over properties */
        if (objtype)
        {
            
            unsigned int propCount = smlDevInfCTCapNumProperties(ctcap);
            unsigned int k;
            for (k=0; k < propCount; k++)
            {
                const SmlDevInfProperty *prop = smlDevInfCTCapGetNthProperty(ctcap, k);
                char *name = smlDevInfPropertyGetPropName(prop);
                const char *value = g_hash_table_lookup(hash, name);
		if (value == NULL)
		{
                    if (strstr(name, "X-") != name)
                    {
                        /* This is a bug. So we crash with some infos. */
                        g_warning("The %s property %s is not supported.\n", cttype, name);
                    } else {
                        /* X-.* fields are proprietary extensions and so we can ignore them. */
                        g_message("The proprietary %s property %s is not supported.\n", cttype, name);
                    }
		} else {
                    /* Good news - OpenSync knows the field. */
                    if (strlen(value))
                    {
                        OSyncCapability *cap = osync_capability_new(caps, objtype, g_strdup(value), error);
                        if (!cap)
                        {
                            osync_trace(TRACE_INTERNAL, "%s: cannot create new capabilitity", __func__);
                            safe_cfree(&name);
                            return FALSE;
                        }
                    } else {
                        /* This can happen for special fields like BEGIN, VERSION or END.
                         * There is nothing to do here.
                         */
                        osync_trace(TRACE_INTERNAL, "%s: special field %s detected", __func__, name);
                    }
                }
                safe_cfree(&name);
            }
            safe_cfree(&objtype);
            g_hash_table_unref(hash);
        }
        safe_cfree(&cttype);
    }

    /* add fresh capabilities to plugin info */
    osync_trace(TRACE_INTERNAL, "%s: set capabilities", __func__);
    osync_plugin_info_set_capabilities(env->pluginInfo, caps);

    osync_trace(TRACE_EXIT, "%s - success", __func__);
    return TRUE;
}
