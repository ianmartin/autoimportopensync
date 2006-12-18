#ifndef _LDAP_CONNECT_H
#define _LDAP_CONNECT_H

#include "ldap_plugin.h"
#include <opensync/opensync.h>
#include <glib.h>

typedef struct lutil_sasl_defaults_s {
        char *mech;
        char *realm;
        char *authcid;
        char *passwd;
        char *authzid;
        char **resps;
        int nresps;
} lutilSASLdefaults;

osync_bool os_ldap_connect (OSyncContext *ctx, ldap_plgdata *plgdata);
osync_bool os_ldap_disconnect (OSyncContext *ctx, ldap_plgdata *plgdata);

osync_bool os_ldap_set_version (OSyncContext *ctx, ldap_plgdata *plgdata);
osync_bool os_ldap_encrypt (OSyncContext *ctx, ldap_plgdata *plgdata);
osync_bool os_ldap_check_evolution(OSyncContext *ctx, ldap_plgdata *plgdata);

osync_bool os_ldap_makebind(OSyncContext *ctx, ldap_plgdata *plgdata);

GList *os_load_ldap_entries (OSyncContext *ctx, ldap_plgdata *plgdata);

void os_free_ldap_entry (ldap_entry *entry);
void os_free_ldap_entries (GList *entrylist);

osync_bool os_modify_ldap_entry (OSyncContext *ctx, ldap_plgdata *plgdata, ldap_entry *entry);

osync_bool os_delete_ldap_entry (OSyncContext *ctx, ldap_plgdata *plgdata, gchar *dn);

char **os_ldap_get_attr_values (ldap_entry *entry, char *attr);

void os_detect_attribute_changes (ldap_plgdata *plgdata, ldap_entry *check_entry);

#endif /* _LDAP_CONNECT_H */
