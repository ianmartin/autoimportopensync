#ifndef _LDAP_MD5_H
#define _LDAP_MD5_H

#include <glib.h>
#include "config.h"

gboolean init_md5 (gboolean warnings);
gchar *encrypt_md5 (gchar *passwd);

#endif /* _MD5_CRYPT_H */
