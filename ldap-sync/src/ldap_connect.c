#include "ldap_connect.h"
#include <string.h>
#include <sasl/sasl.h>

/*********************/
/* SASL functions    */

int lutil_sasl_interact ( LDAP *ld,
			  unsigned flags,
			  lutilSASLdefaults *defaults,
			  void *in )
{
        sasl_interact_t *interact = in;

        if( ld == NULL ) return LDAP_PARAM_ERROR;

        while( interact->id != SASL_CB_LIST_END ) {
		switch( interact->id ) {
			case SASL_CB_GETREALM:
				interact->result = (defaults->realm && *defaults->realm) ? defaults->realm : "";
                		interact->len = strlen( interact->result );
				break;
			case SASL_CB_AUTHNAME:
				interact->result = (defaults->authcid && *defaults->authcid) ? defaults->authcid : "";
                		interact->len = strlen( interact->result );
				break;
			case SASL_CB_PASS:
				interact->result = (defaults->passwd && *defaults->passwd) ? defaults->passwd : "";
                		interact->len = strlen( interact->result );
				break;
			case SASL_CB_USER:
				interact->result = (defaults->authzid && *defaults->authzid) ? defaults->authzid : "";
                		interact->len = strlen( interact->result );
				break;
		}
                interact++;
        }

        return LDAP_SUCCESS;
}

lutilSASLdefaults *lutil_sasl_defaults(LDAP *ld, char *mech, char *realm, char *authcid, char *passwd, char *authzid)
{
        lutilSASLdefaults *defaults;
        defaults = ber_memalloc( sizeof( lutilSASLdefaults ) );

        if( defaults == NULL ) return NULL;

        defaults->mech = mech ? ber_strdup(mech) : NULL;
        defaults->realm = realm ? ber_strdup(realm) : NULL;
        defaults->authcid = authcid ? ber_strdup(authcid) : NULL;
        defaults->passwd = passwd ? ber_strdup(passwd) : NULL;
        defaults->authzid = authzid ? ber_strdup(authzid) : NULL;


        if( defaults->mech == NULL ) {
                ldap_get_option( ld, LDAP_OPT_X_SASL_MECH, &defaults->mech );
        }
        if( defaults->realm == NULL ) {
                ldap_get_option( ld, LDAP_OPT_X_SASL_REALM, &defaults->realm );
        }
        if( defaults->authcid == NULL ) {
                ldap_get_option( ld, LDAP_OPT_X_SASL_AUTHCID, &defaults->authcid );
        }
        if( defaults->authzid == NULL ) {
                ldap_get_option( ld, LDAP_OPT_X_SASL_AUTHZID, &defaults->authzid );
        }
        defaults->resps = NULL;
        defaults->nresps = 0;

        return defaults;
}


/*********************/
/* LDAP functions    */

static gboolean check_modify_on_attr_vals (char **oldvals, char **newvals);
static LDAPMod *create_ldap_mod (int operation, char *attribute, char **values);

/*
 * Connect to LDAP server
 */
osync_bool os_ldap_connect (OSyncContext *ctx, ldap_plgdata *plgdata)
{
	if (ldap_is_ldap_url(plgdata->servername) || ldap_is_ldaps_url(plgdata->servername)) {
		ldap_initialize(&(plgdata->ld), plgdata->servername);
		if( plgdata->ld == NULL ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Could not connect to %s", plgdata->servername);
			return FALSE;
		}
	} else {
		plgdata->ld = (LDAP*)ldap_init(plgdata->servername, plgdata->serverport);
		if( plgdata->ld == NULL ) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Could not connect to %s on %i", plgdata->servername, plgdata->serverport);
			return FALSE;
		}
	}

	return TRUE;
}

/*
 * Disconnect from LDAP server
 */
osync_bool os_ldap_disconnect (OSyncContext *ctx, ldap_plgdata *plgdata)
{
	/* Check if we have an Ldap connection */
	if (plgdata->ld && 
	   (ldap_unbind(plgdata->ld) != LDAP_SUCCESS))
	{
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't unbind from LDAP server");
		return FALSE;
	}

	plgdata->ld = NULL;
	return TRUE;
}

/*
 * Set version on LDAP connection
 */
osync_bool os_ldap_set_version (OSyncContext *ctx, ldap_plgdata *plgdata)
{
	int *ldap_version;
	/* Set version from plgdata on LDAP connection */
	if (ldap_set_option(plgdata->ld, LDAP_OPT_PROTOCOL_VERSION, &(plgdata->ldap_version)) != LDAP_SUCCESS) {
		/* Couldn't set version, store connection's version in plgdata */
		ldap_get_option(plgdata->ld, LDAP_OPT_PROTOCOL_VERSION, ldap_version);
		osync_context_send_log(ctx, "WARNING: Could not set LDAP Version to %i, using %i", plgdata->ldap_version, *ldap_version);
		plgdata->ldap_version = *ldap_version;
		return FALSE;
	}

	return TRUE;
}

/*
 * Set encryption on LDAP connection
 */
osync_bool os_ldap_encrypt (OSyncContext *ctx, ldap_plgdata *plgdata)
{
	if (ldap_start_tls_s(plgdata->ld, NULL, NULL) != LDAP_SUCCESS)
		return FALSE;
	return TRUE;
}

/*
 * Bind to LDAP server
 */
osync_bool os_ldap_makebind(OSyncContext *ctx, ldap_plgdata *plgdata)
{
	unsigned sasl_flags = LDAP_SASL_AUTOMATIC;
	struct berval passwd = { 0, NULL };
	lutilSASLdefaults *defaults;
	char *authmech, *binddn, *bindpwd;

	if (!plgdata->anonymous) {
		binddn = plgdata->binddn;
		bindpwd = plgdata->bindpwd;
		authmech = plgdata->authmech;
	} else {
		binddn = "";
		bindpwd = "";
		authmech = "SIMPLE";
	}

	if (!strcmp(authmech, "SIMPLE")) {
		osync_context_send_log(ctx, "Simple auth selected");
		if (ldap_simple_bind_s(plgdata->ld, binddn, bindpwd)) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to connect and bind to %s as %s", plgdata->servername, binddn);
			return FALSE;
		}
	} else {
		osync_context_send_log(ctx, "Sasl auth selected");
		passwd.bv_val = ber_strdup(bindpwd);
		passwd.bv_len = strlen( passwd.bv_val );

		defaults = lutil_sasl_defaults(plgdata->ld, ber_strdup(authmech), NULL, ber_strdup(binddn), passwd.bv_val, NULL);
		if (ldap_sasl_interactive_bind_s(plgdata->ld, NULL, ber_strdup(authmech), NULL, NULL, sasl_flags, (LDAP_SASL_INTERACT_PROC *)lutil_sasl_interact, (void *)defaults ) != LDAP_SUCCESS) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to connect and sasl bind to %s as %s", plgdata->servername, binddn);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * Check evolution support in LDAP server
 */
osync_bool os_ldap_check_evolution(OSyncContext *ctx, ldap_plgdata *plgdata)
{
	LDAPMessage *res, *res2;
	int i = 0;
	char *attrs[] = {"objectClasses", NULL};
	char **ldapvals = NULL;

	/* Search all entries that apply to the filter */
	if (ldap_search_s(plgdata->ld, "cn=Subschema", LDAP_SCOPE_BASE, "(objectclass=*)", attrs, 0, &res)) {
		/* Unable to search for evolution support */
		return FALSE;
	}

	res2 = ldap_first_entry(plgdata->ld, res);
	if (!res2) {
		/* No objectclass entries found */
		return FALSE;
	}

	ldapvals = (char**)ldap_get_values(plgdata->ld, res2, "objectClasses");

	while (ldapvals[i]) {
		if (strstr(ldapvals[i], "evolutionPerson")) {
			ldap_value_free(ldapvals);
			return TRUE;
		}
		i++;
	}

	ldap_value_free(ldapvals);
	ldap_msgfree(res);
	return FALSE;
}


/*
 * Load the head data from the LDAP server
 */
GList *os_load_ldap_entries (OSyncContext *ctx, ldap_plgdata *plgdata)
{
	LDAPMessage *all_results, *result;
	GList *entrylist = NULL, *attrnames;
	ldap_entry *entry = NULL;
	gchar *filter, *keyattr;
	int i;
	BerElement *berptr = NULL;
	char *attribute = NULL;
	char **id_values;

	/* Search all entries that apply to the filter */
	filter = g_strdup_printf("(&(%s=*)%s)", plgdata->keyattr, plgdata->searchfilter);
	if (ldap_search_s(plgdata->ld, plgdata->searchbase, plgdata->scope, filter, plgdata->attrs, 0, &all_results)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to search on %s with filter %s", plgdata->searchbase, filter);
		g_free(filter);
		return NULL;
	}
	g_free(filter);

	/* Process found entries */
	for (result = ldap_first_entry(plgdata->ld, all_results) ; result ; result = ldap_next_entry(plgdata->ld, result))
	{
		/* Get Entry ID attribute */
		id_values = (char**)ldap_get_values(plgdata->ld, result, plgdata->keyattr);
		if (!id_values) {
			osync_context_send_log(ctx, "WARNING: Entry DN=%s has no attribute '%s'", ldap_get_dn(plgdata->ld, result), plgdata->keyattr);
			continue;
		}

		/* Create ldap_entry object, and store main information */
		entry = g_malloc0(sizeof(ldap_entry));
		entry->dn = g_strdup(ldap_get_dn(plgdata->ld, result));
		entry->id = g_strdup(id_values[0]);
		ldap_value_free(id_values);

		osync_context_send_log(ctx, "Loaded entry: %s", entry->dn);

		/* Get list of attribute names */
		attrnames = NULL;
		for (attribute = ldap_first_attribute(plgdata->ld, result, &berptr) ; attribute ; attribute = ldap_next_attribute(plgdata->ld, result, berptr))
		{
			attrnames = g_list_append(attrnames, attribute);
		}

		/* Load all attributes for entry */
		if(attrnames) {
			/* Allocate pointers to LDAPMod structures - last pointer will be NULL */
			entry->attrs = g_malloc0((g_list_length(attrnames) + 1) * sizeof(LDAPMod *));
			/* Read attribute names and values to these structures */
			for (i = 0 ; i < g_list_length(attrnames) ; i++)
			{
				attribute = (char*)g_list_nth_data(attrnames, i);

				entry->attrs[i] = g_malloc0(sizeof(LDAPMod));
				entry->attrs[i]->mod_type = attribute;
				entry->attrs[i]->mod_values = (char**)ldap_get_values(plgdata->ld, result, attribute);
			}
			/* Free GList (data is not freed) */
			g_list_free(attrnames);
		}

		entrylist = g_list_append(entrylist, entry);
	}

	ldap_msgfree(all_results);
	return entrylist;
}

/*
 * Free ldap_entry data structure
 */
void os_free_ldap_entry (ldap_entry *entry)
{
	int i;

	/* Free DN */
	if(entry->dn) g_free(entry->dn);

	/* Free entry ID, if present */
	if(entry->id) g_free(entry->id);

	/* Free attribute structures */
	for (i = 0 ; entry->attrs[i] ; i++ )
	{
		ldap_memfree(entry->attrs[i]->mod_type);
		ldap_value_free(entry->attrs[i]->mod_values);
		g_free(entry->attrs[i]);
	}

	/* Free attribute pointer-array, and ldap entry object */
	g_free(entry->attrs);
	g_free(entry);
}

/*
 * Free all data allocated by entry list
 */
void os_free_ldap_entries (GList *entrylist)
{
	ldap_entry *entry;
	int i, j;

	for (i = 0 ; i < g_list_length(entrylist) ; i++)
	{
		os_free_ldap_entry ((ldap_entry *)g_list_nth_data(entrylist, i));
	}

	/* Free up GList */
	g_list_free(entrylist);
}

char **os_ldap_get_attr_values (ldap_entry *entry, char *attr)
{
	LDAPMod **attrs = entry->attrs;
	int i, j;
	for (i = 0; attrs[i]; i++) {
		if (!strcmp(attrs[i]->mod_type, attr))
			return attrs[i]->mod_values;
	}
	return NULL;
}

void os_detect_attribute_changes (ldap_plgdata *plgdata, ldap_entry *check_entry)
{
	LDAPMessage *all_results, *result;
	int cnt, i;
	char *attribute = NULL;
	char **ldapvals;
	BerElement *berptr = NULL;

	GList *ldap_attrnames = NULL, *list;
	GList *modifications = NULL;

	for ( i = 0 ; check_entry->attrs[i] ; i++)
		check_entry->attrs[i]->mod_op = LDAP_MOD_REPLACE;

	/* Get entry from LDAP */
	if (ldap_search_s(plgdata->ld, check_entry->dn, LDAP_SCOPE_BASE, "(objectclass=*)", plgdata->attrs, 0, &all_results))
		return;

	result = ldap_first_entry(plgdata->ld, all_results);
	if(!result) return;

	/* Get list of attribute names (got from LDAP) */
	for (attribute = ldap_first_attribute(plgdata->ld, result, &berptr) ; attribute ; attribute = ldap_next_attribute(plgdata->ld, result, berptr))
		ldap_attrnames = g_list_append (ldap_attrnames, attribute);

	/* Check for MODIFIED and/or DELETED attributes (present in LDAP entry, but not in checked entry */

	/* Check LDAP attributes */
	for (list = g_list_first(ldap_attrnames) ; list ; list = g_list_next(list) )
	{
		attribute = (char*)list->data;

		/* Search for LDAP attribute in checked entry */
		for ( i = 0 ; check_entry->attrs[i] ; i++)
		{
			/* Compare checked entry attribute name with LDAP entry attribute name */
			if (!strcmp(check_entry->attrs[i]->mod_type, attribute))
			{
				/* Attribute found -> compare values */

				/* Get attribute values from LDAP */
				ldapvals = (char**)ldap_get_values(plgdata->ld, result, attribute);

				/*---- MODIFIED ----*/
				/* Compare value(s) from LDAP with ones from check_entry */
				if (check_modify_on_attr_vals (ldapvals, check_entry->attrs[i]->mod_values))
					modifications = g_list_append (modifications, create_ldap_mod(LDAP_MOD_REPLACE, attribute, check_entry->attrs[i]->mod_values));

				/* Free up LDAP values */
				ldap_value_free(ldapvals);

				/* Attribute found - check next attribute */
				goto next_ldap_attribute;
			}
		}

		/*---- DELETED ----*/
		/* Attribute not found in checked entry, should be deleted */
		if (strcmp(attribute, plgdata->keyattr))
			modifications = g_list_append (modifications, create_ldap_mod(LDAP_MOD_DELETE, attribute, NULL));

		next_ldap_attribute:;
	}

	/* Check for ADDED attributes (present in checked entry, but not in LDAP entry */
	/* Check entry attributes */
	for ( i = 0 ; check_entry->attrs[i] ; i++)
	{
		/* Search for checked attribute in LDAP attributel-list */
		for (list = g_list_first(ldap_attrnames) ; list ; list = g_list_next(list))
		{
			attribute = (char*)list->data;

			/* If checked attribute found between LDAP attributes, check next attribute */
			if (!strcmp(check_entry->attrs[i]->mod_type, attribute))
				goto next_mod_attribute;
		}

		/*---- ADDED ----*/
		/* Attribute not found between LDAP attributes, should be added */
		/* op still LDAP_MOD_REPLACE - this should be add too */
		modifications = g_list_append (modifications, create_ldap_mod(LDAP_MOD_REPLACE, check_entry->attrs[i]->mod_type, check_entry->attrs[i]->mod_values));

		next_mod_attribute:;
	}

	/* Free LDAP attribute names list */
	for (list = g_list_first(ldap_attrnames) ; list ; list = g_list_next(list))
		ldap_memfree((char*)list->data);
	g_list_free(ldap_attrnames);

	/* Free up ldap search result */
	ldap_msgfree(all_results);

	/*****************************/
	/* Modify check_entry object */

	/* Free up attribute structures */
	for ( i = 0 ; check_entry->attrs[i] ; i++ )
		g_free(check_entry->attrs[i]);
	g_free(check_entry->attrs);

	/* Re-create attributes */
	i = 0;
	check_entry->attrs = (LDAPMod**)g_malloc0(sizeof(LDAPMod*) * (g_list_length(modifications) + 1));
	for (list = g_list_first(modifications) ; list ; list = g_list_next(list))
		check_entry->attrs[i++] = (LDAPMod*)list->data;

	g_list_free(modifications);
}







///*
//struct data_entry *ldap_get_entry(ldap_connection *conn, char *uid)
//{
//	LDAPMessage *res, *res2;
//	struct data_entry *entry = NULL;
//	char *attrs[] = {"modifyTimestamp", NULL};
//	entry = g_malloc0(sizeof(struct data_entry));
//	char filter[1024];
//	uid = quoted_decode(uid);
//
//	ldap_debug(conn, 2, "Loading single head data for %s", ldap_explode_dn(uid, 0)[0]);
//
//	//Search all entries that apply to the filter. get modifyTimestamp
//	sprintf(filter, "(%s)", ldap_explode_dn(uid, 0)[0]);
//	if (ldap_search_s(conn->ld, conn->searchbase, LDAP_SCOPE_ONELEVEL, filter, attrs, 0, &res)) {
//		ldap_debug(conn, 0, "Unable to search on %s", conn->searchbase);
//		return NULL;
//	}
//
//	res2 = ldap_first_entry(conn->ld, res);
//	if (!res2) {
//		ldap_debug(conn, 2, "No entries found");
//		return NULL;
//	}
//
//	do {
//		if (!strcmp(ldap_get_dn(conn->ld, res2), uid)) {
//			entry->modifyTimestamp = (ldap_get_values(conn->ld, res2, "modifyTimestamp"))[0];
//			entry->uid = quoted_encode(ldap_get_dn(conn->ld, res2));
//			ldap_debug(conn, 2, "Loaded entry: %s, %s", entry->modifyTimestamp, entry->uid);
//			break;
//		}
//		res2 = ldap_next_entry(conn->ld, res2);
//	} while (res2);
//
//	ldap_debug(conn, 3, "done: ldap_get_entry");
//	return entry;
//}*/

///*Add an entry to the ldap server*/
//int ldap_add_entry(ldap_connection *conn, LDAPMod **data, char *uidret, int duplicate)
//{
//	int i = 0, x = 0, n = 0, result = 0;
//	char dn[1024];
//	ldap_debug(conn, 3, "start: ldap_add_entry");
//
//	for (i = 0; data[i]; i++) {
//		//Add another attribute
//		data[i]->mod_op = LDAP_MOD_ADD;
//		if (!strcmp(data[i]->mod_type, "cn") && !duplicate) {
//			sprintf(dn, "cn=3D%s,%s", quoted_encode(data[i]->mod_values[0]), quoted_encode(conn->searchbase));
//			strcpy(uidret, dn);
//		}
//	}
//
//	if (duplicate) {
//		sprintf(dn, "uid=3Dduplicate%d,%s",  rand(), quoted_encode(conn->searchbase));
//		strcpy(uidret, dn);
//		data[i] = g_malloc0(sizeof(LDAPMod));
//		data[i]->mod_values = g_malloc0(2 * sizeof(char *));
//		data[i]->mod_type = "uid";
//		data[i]->mod_values[0] = ldap_explode_dn(quoted_decode(dn), 1)[0];
//		data[i]->mod_values[1] = NULL;
//		data[i]->mod_op = LDAP_MOD_ADD;
//		data[i + 1] = NULL;
//	}
//
//	ldap_debug(conn, 2, "Adding: %s", quoted_decode(dn));
//	if (result = ldap_add_s(conn->ld, quoted_decode(dn), data)) {
//		if (result == 68 && !duplicate) {
//			//Multisync wants to duplicate item
//			ldap_debug(conn, 2, "Duplicating Entries");
//			result = ldap_add_entry(conn, data, uidret, 1);
//			return result;
//		}
//		ldap_debug(conn, 1, "Add result: %i: %s", result, ldap_err2string(result));
//		return -1;
//	}
//
//	ldap_debug(conn, 3, "end: ldap_add_entry");
//	return 0;
//}


/***************************************
 * INNER ldap_connect functions        *
 ***************************************/

static gboolean check_modify_on_attr_vals (char **oldvals, char **newvals)
{
	int i, j;
	gboolean found;

	/* Check all oldvals, if they can be found in newvals array */
	for ( i = 0 ; oldvals[i] ; i++)
	{
		found = FALSE;
		for ( j = 0 ; newvals[j] ; j++)
		{
			if (!strcmp(oldvals[i], newvals[j])) {
				found = TRUE;
				break;
			}
		}

		/* If oldval not found in newvals array, values are modified */
		if (!found) return TRUE;
	}

	/* All strings from oldvals[] array found in newvals[]
	 * Now we need to check, if arrays have the same length
	 * (If newvals[] have no additional elements)
	 *
	 * 'i' is now the size of oldvals
	 * need to get size of newvals
	 */
	for ( j = 0 ; newvals[j] ; j++);

	/* Array lengths differs - modify detected */
	if (i != j) return TRUE;

	/* String arrays contains the same values - no modify */
	return FALSE;
}

static LDAPMod *create_ldap_mod (int operation, char *attribute, char **values)
{
	LDAPMod *mod = (LDAPMod*)g_malloc0(sizeof(LDAPMod));
	mod->mod_op = operation;
	mod->mod_type = g_strdup(attribute);
	if(values) {
		int cnt = 0, i;
		while(values[cnt]) cnt++;
		mod->mod_values = (char**)g_malloc0(sizeof(char*)*(cnt+1));
		for (i=0 ; values[i] ; i++) {
			mod->mod_values[i] = g_strdup(values[i]);
		}
	}
	return mod;
}


