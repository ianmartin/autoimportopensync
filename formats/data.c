#include <opensync.h>
#include <string.h>

/** @defgroup data_plain data/plain format
 *
 * Definition: pointer to a malloc()ed block of data, or a NULL
 * pointer.
 */

/** data/plain comparison function
 *
 * The comparison function is a memcpy() on the data.
 *
 * @ingroup data_plain
 */
static OSyncConvCmpResult compare_plain(OSyncChange *a, OSyncChange *b)
{
	const char *d1 = osync_change_get_data(a);
	const char *d2 = osync_change_get_data(b);
	size_t s1 = osync_change_get_datasize(a);
	size_t s2 = osync_change_get_datasize(b);

	/* Consider empty block equal NULL pointers */
	if (!s1) d1 = NULL;
	if (!s2) d2 = NULL;

	if (d1 && d2) {
		int r = memcmp(d1, d2, s1 < s2 ? s1 : s2);
		if (!r && s1 == s2)
			return CONV_DATA_SAME;
		else
			return CONV_DATA_MISMATCH;
	} else if (!d1 && !d2)
		return CONV_DATA_SAME;
	else
		return CONV_DATA_MISMATCH;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "data");
	OSyncObjFormat *plain = osync_conv_register_objformat(env, "data", "plain");
	osync_conv_format_set_compare_func(plain, compare_plain);
	osync_conv_format_set_malloced(plain);
}
