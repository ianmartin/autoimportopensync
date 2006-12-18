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
	gcry_error_t  err;
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
	gcry_md_hd_t  md5;
	gcry_error_t  err;
	unsigned int  diglen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
	gchar         *mdpass = (gchar*)g_malloc((diglen*2)+1);   // +1 for char '\0' at the end

	// Open MD5 handle
	err = gcry_md_open (&md5, GCRY_MD_MD5, GCRY_MD_FLAG_SECURE);
	if (err) {
		//fprintf(stderr, "gcry_md_open error: %s\n", gcry_strerror(err));
		return NULL;
	}

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


/************************************************/
/*   Replace functions above with these below   */
/*   for glibc older than version 1.2           */
/************************************************/

/*
 * Initialize gcrypt library for MD5 encryption
 * Returns: 0 on success
 *          1 on error
 */
/*
gboolean init_md5 (gboolean warnings)
{
	gboolean retval = 1;

	// Check for GCrypt library version
	if (warnings && !gcry_check_version (GCRYPT_VERSION)) {
		retval = 0;
		fprintf(stderr, _("WARNING: libgcrypt with version %s or above needed\n"), GCRYPT_VERSION);
	}

	// Disable secure memory warnings
	if (gcry_control (GCRYCTL_DISABLE_SECMEM_WARN) && warnings) {
		retval = 0;
		fprintf(stderr, _("WARNING: Couldn't disable secure-memory warnings\n"));
	}

	// Initialize secure memory
	if (gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0) && warnings) {
		retval = 0;
		fprintf(stderr, _("WARNING: Couldn't initialize secure-memory\n"));
	}

	// Initialization finished
	if (gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0) && warnings) {
		retval = 0;
		fprintf(stderr, _("WARNING: Couldn't finish initialization\n"));
	}

	return retval;
}
*/

/*
 * Algorythm for olg gcrypt
 */
/*
gchar *encrypt_md5 (gchar *passwd)
{
	GCRY_MD_HD    md5;
	 unsigned int  diglen = gcry_md_get_algo_dlen (GCRY_MD_MD5);
	gchar         *mdpass = (gchar*)g_malloc((diglen*2)+1);   // +1 for char '\0' at the end

	// Open MD5 handle
	md5 = gcry_md_open (GCRY_MD_MD5, GCRY_MD_FLAG_SECURE);
	if (!md5) {
		//fprintf(stderr, "ERROR: Couldn't open handler for MD5 encryption\n");
		return NULL;
	}

	// Add password string
	gcry_md_write (md5, passwd, strlen(passwd));

	// Finalize string addition -> create digest
	if (gcry_md_final (md5)) {
		//fprintf(stderr, "ERROR: Couldn't create MD5 digest\n");
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
*/
