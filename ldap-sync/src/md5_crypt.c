#include <gcrypt.h>
#include <stdio.h>

#include "md5_crypt.h"

/*
 * Initialize gcrypt library for MD5 encryption
 * Returns: 0 on success
 *          1 on error
 */
gboolean init_md5 (gboolean warnings)
{
#ifndef OLD_GCRYPT
	gcry_error_t  err;
#else
	int err;
#endif
	gboolean retval = 1;

	// Check for GCrypt library version
	if (warnings && !gcry_check_version (GCRYPT_VERSION)) {
		retval = 0;
		fprintf(stderr, "WARNING: libgcrypt with version %s or above needed\n", GCRYPT_VERSION);
	}

	// Disable secure memory warnings
	err = gcry_control (GCRYCTL_DISABLE_SECMEM_WARN);
	if (warnings && err) {
		retval = 0;
		fprintf(stderr, "WARNING (GCRYCTL_DISABLE_SECMEM): %s\n", gcry_strerror(err));
	}

	// Initialize secure memory
	err = gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
	if (warnings && err) {
		retval = 0;
		fprintf(stderr, "WARNING (GCRYCTL_INIT_SECMEM): %s\n", gcry_strerror(err));
	}

	// Initialization finished
	err = gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
	if (warnings && err) {
		retval = 0;
		fprintf(stderr, "WARNING (GCRYCTL_INITIALIZATION_FINISHED): %s\n", gcry_strerror(err));
	}

	return retval;
}

gchar *encrypt_md5 (gchar *passwd)
{
#ifndef OLD_GCRYPT
	gcry_error_t  err;
	gcry_md_hd_t  md5;
#else
	int err;
	GCRY_MD_HD    md5;
#endif
	unsigned int  diglen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
	gchar         *mdpass = (gchar*)g_malloc((diglen*2)+1);   // +1 for char '\0' at the end

	// Open MD5 handle
#ifndef OLD_GCRYPT
	err = gcry_md_open (&md5, GCRY_MD_MD5, GCRY_MD_FLAG_SECURE);
	if (err) {
		//fprintf(stderr, "gcry_md_open error: %s\n", gcry_strerror(err));
		return NULL;
	}
#else
	md5 = gcry_md_open (GCRY_MD_MD5, GCRY_MD_FLAG_SECURE);
	if (!md5) {
		//fprintf(stderr, "ERROR: Couldn't open handler for MD5 encryption\n");
		return NULL;
	}
#endif

	// Add password string
	gcry_md_write (md5, passwd, strlen(passwd));

	// Finalize string addition -> create digest
	err = gcry_md_final (md5);
	if (err) {
		//fprintf(stderr, "gcry_md_final error: %s\n", gcry_strerror(err));
		gcry_md_close (md5);
		return NULL;
	}

	// Create hexa string from MD5 digest
	{
		unsigned char *digest = gcry_md_read (md5, GCRY_MD_MD5);
		int i;
		for (i=0; i<diglen; i++) sprintf(mdpass+(i*2), "%02x", digest[i]);
		mdpass[diglen*2] = '\0';
	}

	// Close MD5 handle
	gcry_md_close (md5);

	return mdpass;
}

