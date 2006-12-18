#ifndef LDAP_PLUGIN_H
#define LDAP_PLUGIN_H

#include <opensync/opensync.h>
#include <opensync/opensync-xml.h>
#include <ldap.h>

/*****************************************************************************/
/*  Global variables and definitions                                         */

typedef struct {
	OSyncMember *member;
	OSyncHashTable *hashtable;

	char *servername;			/* Hostname or IP of LDAP server */
	int serverport;				/* Port number, on which runs server */
	char *binddn;				/* Bind DN */
	char *bindpwd;				/* Bind password */
	char *searchbase;			/* Base DN for search */
	char *searchfilter;			/* Search filter */
	char *storebase;			/* Base DN for storing new contacts */
	char *keyattr;				/* Key attribute for storing contacts */
	int scope;				/* Search scope */
	char *authmech;				/* Authentication mechanism (default: SIMPLE) */
	osync_bool encryption;			/* Use encrypted messages for communication */
	osync_bool anonymous;			/* Bind as anonymous user */
	osync_bool ldap_read, ldap_write;	/* Read/Write permissions */
	int ldap_version;

	osync_bool evolution_support;

	LDAP *ld;				/* Pointer to LDAP connection (NULL, if no connection present) */
	char **attrs;				/* NULL-terminated list of mapped LDAP attributes */

} ldap_plgdata;

typedef struct {
	char *dn;		/* DN of the entry */
	char *id;		/* ID of entry, if present, or NULL */
	LDAPMod **attrs;	/* Array of attribute structures (or modifications in case of MODIFY) */
} ldap_entry;

#endif

