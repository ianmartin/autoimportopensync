#include <ldap_plugin.h>

#include <glib.h>

#define LDAPATTR  0		/* Name of LDAP attribute */
#define XMLATTR   1		/* Name of XML attribute */
#define XMLVNAME  2		/* Value field name of XML attribute */
#define XMLPROP1  3		/* First property modifyer of XML attribute */
#define XMLPROP2  4		/* Second property modifyer of XML attribute */

/*
 * FormattedName	Content
 * Name			LastName, FirstName, Additional, Prefix, Suffix
 * Agent		Content
 * Photo		Content
 * Birthday		Content [ osync_time_datestamp() ]
 * Address		PostalBox, ExtendedAddress, Street, City, Region, PostalCode, Country
 * AddressLabel		Content
 * Telephone		Content
 * EMail		Content
 * Mailer		Content
 * Timezone		Content
 * Location		Latitude, Longitude
 * Title		Content
 * Role			Content
 * Logo			Content
 * Organization		Name, Department
 * Note			Content
 * Revision		Content [ osync_time_datestamp() ]
 * Sound		Content
 * Url			Content
 * Uid			Content
 * Key			Content
 * Nickname		Content
 * Class		Content
 * Categories		Category, Category, ...
 */

char *xmlattrs[][5] = {
	//                                                                TYPE attributes
	{ "cn",                            "FormattedName", "Content",    "",                      "" },
	{ "o",                             "Organization",  "Name",       "",                      "" },
	{ "ou",                            "Organization",  "Department", "",                      "" },
	{ "uid",                           "Uid",           "Content",    "",                      "" },
	{ "note",                          "Note",          "Content",    "",                      "" },

	{ "telephoneNumber",               "Telephone",     "Content",    "",                      "" },
	{ "homePhone",                     "Telephone",	    "Content",    "HOME",                  "" },
	{ "companyPhone",                  "Telephone",     "Content",    "WORK",                  "" },	// evo
	{ "facsimileTelephoneNumber",      "Telephone",     "Content",    "FAX",               "WORK" },
	{ "mobile",                        "Telephone",     "Content",    "CELL",                  "" },
	{ "carPhone",                      "Telephone",     "Content",    "CAR",                   "" },	// evo
	{ "homeFacsimileTelephoneNumber",  "Telephone",     "Content",    "HOME",               "FAX" },	// evo
	{ "otherPhone",                    "Telephone",     "Content",    "VOICE",                 "" },	// evo
	{ "otherFax",                      "Telephone",     "Content",    "FAX",                   "" },	// evo
	{ "otherFacsimileTelephoneNumber", "Telephone",     "Content",    "FAX",                   "" },	// evo
	{ "internationaliSDNNumber",       "Telephone",     "Content",    "ISDN",                  "" },
	{ "pager",                         "Telephone",     "Content",    "PAGER",                 "" },
	{ "primaryPhone",                  "Telephone",     "Content",    "PREF",                  "" },	// evo
	{ "callbackPhone",                 "Telephone",     "Content",    "X-EVOLUTION-CALLBACK",  "" },	// evo
	{ "radio",                         "Telephone",     "Content",    "X-EVOLUTION-RADIO",     "" },	// evo
	{ "telex",                         "Telephone",     "Content",    "X-EVOLUTION-TELEX",     "" },	// evo
	{ "tty",                           "Telephone",     "Content",    "X-EVOLUTION-TTYTDD",    "" },	// evo
	{ "assistantPhone",                "Telephone",     "Content",    "X-EVOLUTION-ASSISTANT", "" },	// evo

	{ "birthDate",                     "Birthday",      "Content",    "",                      "" },	// evo

	{ "postalAddress",                 "Address",       "PostalBox",  "WORK",                  "" },
	{ "homePostalAddress",             "Address",       "PostalBox",  "HOME",                  "" },
	{ "otherPostalAddress",            "Address",       "PostalBox",  "",                      "" },

	{ "mail",                          "EMail",         "Content",    "INTERNET",              "" },

	// TODO Add other attribute mappings

	NULL };


struct _ldapattrmap {
	gchar *name;
	gchar **values;
};

struct _xmlattrmap {
	gchar *name, *type1, *type2;	/* Name and types of XML attribute */
	GList *valmap;			/* List of { attr -> values } mappings */
};

char **get_map_attribute_list()
{
	int i;
	char **attrs;
	/* Count attributes */
	for ( i = 0 ; xmlattrs[i][0] ; i++ );
	/* Allocate memory for array */
	attrs = (char**)g_malloc0(sizeof(char*) * (i+3));
	/* Copy attribute names to array */
	for ( i = 0 ; xmlattrs[i][0] ; i++ ) {
		attrs[i] = (char*)g_strdup(xmlattrs[i][LDAPATTR]);
	}
	attrs[i++] = (char*)g_strdup("givenName");
	attrs[i++] = (char*)g_strdup("sn");
	return attrs;
}

static GList *append_xml_map (GList *list, char **xmlattr, char **values)
{
	struct _xmlattrmap *iattr = NULL;
	struct _ldapattrmap *valmap;
	int i;

	if (list) {
		for ( i = 0 ; i < g_list_length(list) ; i++ ) {
			iattr = (struct _xmlattrmap *)g_list_nth_data(list, i);

			if (!xmlStrcmp((xmlChar*)iattr->name, (xmlChar*)xmlattr[XMLATTR]) &&
			    !xmlStrcmp((xmlChar*)iattr->type1, (xmlChar*)xmlattr[XMLPROP1]) &&
			    !xmlStrcmp((xmlChar*)iattr->type2, (xmlChar*)xmlattr[XMLPROP2]))
			{
				break;
			}

			iattr = NULL;
		}
	}

	valmap = (struct _ldapattrmap *)g_malloc(sizeof(struct _ldapattrmap));
	valmap->name = xmlattr[XMLVNAME];
	valmap->values = values;

	/* If entry found, add new value attribute */
	if (iattr)
		iattr->valmap = g_list_append(iattr->valmap, (gpointer)valmap);

	/* Create new entry with value attribute otherwise */
	else {
		iattr = (struct _xmlattrmap *)g_malloc0(sizeof(struct _xmlattrmap));
		iattr->name = xmlattr[XMLATTR];
		iattr->type1 = xmlattr[XMLPROP1];
		iattr->type2 = xmlattr[XMLPROP2];
		iattr->valmap = g_list_append(iattr->valmap, (gpointer)valmap);

		list = g_list_append(list, (gpointer)iattr);
	}

	return list;
}

static void free_xml_map (GList *mlist)
{
	struct _xmlattrmap *iattr;
	struct _ldapattrmap *valmap;
	int i, j;

	if (!mlist)
		return;

	for ( i = 0 ; i < g_list_length(mlist) ; i++ ) {
		iattr = (struct _xmlattrmap *)g_list_nth_data(mlist, i);

		for ( j = 0 ; j < g_list_length(iattr->valmap) ; j++ ) {
			valmap = (struct _ldapattrmap *)g_list_nth_data(iattr->valmap, j);
			g_free(valmap);
		}

		g_list_free(iattr->valmap);
		g_free(iattr);
	}

	g_list_free(mlist);
}

static void create_xml_from_map (xmlNode *parent, GList *mlist)
{
	struct _xmlattrmap *iattr;
	struct _ldapattrmap *valmap;
	xmlNode *node;
	int i, j, k;

	if (!mlist)
		return;

	for ( i = 0 ; i < g_list_length(mlist) ; i++ ) {
		iattr = (struct _xmlattrmap *)g_list_nth_data(mlist, i);

		/* Add node to parent */
		node = xmlNewTextChild (parent, NULL, (xmlChar*)iattr->name, NULL);
		/* Set type of node, if needed */
		if (strlen(iattr->type1))
			xmlNewTextChild (node, NULL, (xmlChar*)"Type", (xmlChar*)iattr->type1);
		if (strlen(iattr->type2))
			xmlNewTextChild (node, NULL, (xmlChar*)"Type", (xmlChar*)iattr->type2);
		/* Add values to node */
		for ( j = 0 ; j < g_list_length(iattr->valmap) ; j++ ) {
			valmap = (struct _ldapattrmap *)g_list_nth_data(iattr->valmap, j);
			/* Add all values */
			for ( k = 0 ; valmap->values[k] ; k++) {
				xmlNewTextChild (node, NULL, (xmlChar*)valmap->name, (xmlChar*)valmap->values[k]);
			}
		}
	}
}

/*
 * Function converting ldap_entry structure to xmlDoc
 */
xmlDoc* convert_ldap2xml (ldap_entry *ldapdata)
{
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	xmlNode *root = osxml_node_add_root(doc, "contact");
	xmlNode *xmlnode;
	int i, j, k;
	LDAPMod *attr;
	gchar **vars = NULL, *title = NULL, *first = NULL, *middle = NULL, *last = NULL, *suffix = NULL, *givenname = NULL, *surname = NULL;
	GList *attrlist = NULL;
	GList *xmlattrmap = NULL;

	for (i = 0 ; ldapdata->attrs[i] ; i++)
	{
		attr = ldapdata->attrs[i];

		/* Handle names */
		/* First the common name */
		if (!xmlStrcmp((xmlChar*)attr->mod_type, (xmlChar*)"cn"))
		{
			vars = g_strsplit(attr->mod_values[0], " ", 5);

			/* Only 1 token found */
			if (!vars[1]) {
				last = vars[0];
			}

			/* 2 tokens found */
			else if (!vars[2]) {
				first = vars[0];
				last = vars[1];
			}

			/* 3 tokens found */
			else if (!vars[3]) {
				first = vars[0];
				middle = vars[1];
				last = vars[2];
			}

			/* 4 tokens found */
			else if (!vars[4]) {
				title = vars[0];
				first = vars[1];
				middle = vars[2];
				last = vars[3];
			}

			/* 5 tokens found */
			else {
				title = vars[0];
				first = vars[1];
				middle = vars[2];
				last = vars[3];
				suffix = vars[4];
			}

			attrlist = g_list_append (attrlist, attr);
		}
		/* givenName */
		else if (!xmlStrcmp((xmlChar*)attr->mod_type, (xmlChar*)"givenName")) {
			givenname = g_strdup(attr->mod_values[0]);
		}
		/* familyName */
		else if (!xmlStrcmp((xmlChar*)attr->mod_type, (xmlChar*)"sn")) {
			surname = g_strdup(attr->mod_values[0]);
		}
		else {
			attrlist = g_list_append (attrlist, attr);
		}
	}

	/* Convert entry attributes */
	for (i = 0; i < g_list_length(attrlist); i++)
	{
		attr = (LDAPMod*)g_list_nth_data(attrlist, i);

		/* Search LDAP attribute in mapping table */
		for (j = 0 ; xmlattrs[j][0] ; j++)
		{
			if (!xmlStrcmp((xmlChar*)attr->mod_type, (xmlChar*)xmlattrs[j][LDAPATTR]))
			{
				/* Map LDAP attribute to inner buffer */
				xmlattrmap = append_xml_map (xmlattrmap, xmlattrs[j], attr->mod_values);

				break;
			}
		}
	}
	g_list_free(attrlist);

	/* Convert mapped LDAP to XML */
	create_xml_from_map (root, xmlattrmap);
	free_xml_map (xmlattrmap);

	/* Add name property */
	xmlnode = xmlNewTextChild (root, NULL, (xmlChar*)"Name", NULL);

	if (givenname || first) xmlNewTextChild (xmlnode, NULL, (xmlChar*)"FirstName", (xmlChar*)((givenname) ? givenname : first));
	if (middle)             xmlNewTextChild (xmlnode, NULL, (xmlChar*)"Additional", (xmlChar*)middle);
	if (surname || last)    xmlNewTextChild (xmlnode, NULL, (xmlChar*)"LastName", (xmlChar*)((surname) ? surname : last));
	if (title)              xmlNewTextChild (xmlnode, NULL, (xmlChar*)"Prefix", (xmlChar*)title);
	if (suffix)             xmlNewTextChild (xmlnode, NULL, (xmlChar*)"Suffix", (xmlChar*)suffix);

	g_strfreev(vars);
	if (givenname) g_free(givenname);
	if (surname) g_free(surname);

	return doc;
}

static char *get_xmlchildvalue (xmlNode *parent, const char *name)
{
	xmlNode *node = (parent)->xmlChildrenNode;
	while(node) {
		if (!xmlStrcmp(node->name, (const xmlChar*)name))
			return (char*)xmlNodeGetContent(node);
		node = node->next;
	}
	return NULL;
}

static GList *get_xmlchildvalues (xmlNode *parent, const char *name)
{
	xmlNode *node = (parent)->xmlChildrenNode;
	GList *contents = NULL;
	while(node) {
		if (!xmlStrcmp(node->name, (const xmlChar*)name))
			contents = g_list_append (contents, xmlNodeGetContent(node));
		node = node->next;
	}
	return contents;
}

static void free_xmlchildvalues (GList *values)
{
	int i;
	xmlChar *value;
	for (i = 0 ; i < g_list_length(values) ; i++) {
		value = (xmlChar*)g_list_nth_data(values, i);
		if (value) xmlFree(value);
	}
	g_list_free(values);
}

static GList *append_ldapmod (GList *list, gchar *ldapattr, gchar *value)
{
	LDAPMod *mod = (LDAPMod*)g_malloc0(sizeof(LDAPMod));
	mod->mod_type = g_strdup(ldapattr);
	mod->mod_values = (char**)g_malloc0(2*sizeof(char*));
	mod->mod_values[0] = g_strdup(value);
	list = g_list_append(list, mod);
	return list;
}

/*
 * Function converting xmlDoc data to ldap_entry structure
 */
ldap_entry *convert_xml2ldap (xmlDoc *xmldata)
{
	xmlNode *root = xmlDocGetRootElement(xmldata);
	xmlNode *xmlnode;
	gchar *type1 = NULL, *type2 = NULL;
	GList *list, *ldapmods = NULL;
	ldap_entry *entry;
	gchar *title = NULL, *first = NULL, *middle = NULL, *last = NULL, *suffix = NULL;
	int i;
	char *value;
	gboolean cn_in = FALSE;

	/* Check if root element found, and if it's correct */
	if (!root)
		return NULL;
	if (xmlStrcmp(root->name, (xmlChar *) "contact"))
		return NULL;
	xmlnode = (root)->xmlChildrenNode;

	/* Check child nodes of root */
	while (xmlnode)
	{
		/* Handle node 'Name' */
		if (!xmlStrcmp(xmlnode->name, (xmlChar *) "Name")) {
			first  = get_xmlchildvalue (xmlnode, "FirstName");
			middle = get_xmlchildvalue (xmlnode, "Additional");
			last   = get_xmlchildvalue (xmlnode, "LastName");
			title  = get_xmlchildvalue (xmlnode, "Prefix");
			suffix = get_xmlchildvalue (xmlnode, "Suffix");

			goto next_node;
		}

		/* Get type(s) of xmlNode */
		list = get_xmlchildvalues (xmlnode, "Type");
		if (list) {
			type1 = g_strdup((gchar*)g_list_nth_data(list, 0));
			type2 = g_strdup((gchar*)g_list_nth_data(list, 1));
			free_xmlchildvalues(list);
		}

		if (!type1) type1 = g_strdup("");
		if (!type2) type2 = g_strdup("");

		/* Search for XML node (by it's name and types) in xmlattrs array */
		for (i = 0 ; xmlattrs[i][0] ; i++)
		{
			if (!xmlStrcmp(xmlnode->name, (xmlChar*)xmlattrs[i][XMLATTR]) &&
			    !xmlStrcmp((xmlChar*)type1, (xmlChar*)xmlattrs[i][XMLPROP1]) &&
			    !xmlStrcmp((xmlChar*)type2, (xmlChar*)xmlattrs[i][XMLPROP2]))
			{
				value = get_xmlchildvalue (xmlnode, xmlattrs[i][XMLVNAME]);
				if (value) {
					/* Property found - add it's ldap attribute with value to ldapmods array */
					fprintf(stderr, "--=> %s: %s\n", xmlattrs[i][LDAPATTR], value);
					ldapmods = append_ldapmod (ldapmods, xmlattrs[i][LDAPATTR], (gchar*)value);
					xmlFree(value);
					if (!xmlStrcmp((xmlChar*)xmlattrs[i][LDAPATTR], (xmlChar*)"cn")) cn_in = TRUE;
				}
				/* Property not found in node, not identical to xmlattrs[i] */
			}
		}

		g_free(type1); type1 = NULL;
		g_free(type2); type2 = NULL;

next_node:
		xmlnode = xmlnode->next;
	}

	/* Add 'cn' attribute, if not present */
	if (!cn_in) {
		GString *cname = g_string_new("");

		if (title)  g_string_append_printf (cname, "%s ", title);
		if (first)  g_string_append_printf (cname, "%s ", first);
		if (middle) g_string_append_printf (cname, "%s ", middle);
		if (last)   g_string_append_printf (cname, "%s ", last);
		if (suffix) g_string_append_printf (cname, "%s ", suffix);

		if (strlen(cname->str)) cname = g_string_truncate (cname, strlen(cname->str) - 1);
		fprintf(stderr, "--=> cn: %s\n", cname->str);
		ldapmods = append_ldapmod (ldapmods, "cn", cname->str);
		g_string_free(cname, TRUE);
	}

	/* Add 'givenname' */
	fprintf(stderr, "--=> givenname: %s\n", first);
	ldapmods = append_ldapmod (ldapmods, "givenName", first);

	/* Add 'sn' */
	fprintf(stderr, "--=> sn: %s\n", last);
	ldapmods = append_ldapmod (ldapmods, "sn", last);

	if(first)  xmlFree(first);
	if(middle) xmlFree(middle);
	if(last)   xmlFree(last);
	if(title)  xmlFree(title);
	if(suffix) xmlFree(suffix);


	/* Create new LDAP entry, and add attributes parsed from XML buffer */
	entry = (ldap_entry *)g_malloc0(sizeof(ldap_entry));
	entry->attrs = (LDAPMod**)g_malloc0((g_list_length(ldapmods) + 1) * sizeof(LDAPMod *));
	for ( i = 0 ; i < g_list_length(ldapmods) ; i++ ) {
		entry->attrs[i] = (LDAPMod*)g_list_nth_data(ldapmods, i);
	}
	if(ldapmods) g_list_free(ldapmods);

	return entry;
}

