#include <opensync.h>
#include <glib.h>

/** @defgroup calendar_vcalendar calendar/vcalendar data format
 *
 * The vcalendar data should be a malloc()ed block of data. See
 * osync_conv_format_set_malloced().
 *
 * It can be treated as a plain block of data. See
 * osync_conv_format_set_plain().
 */

static OSyncConvCmpResult compare_vcalendar(OSyncChange *leftchange, OSyncChange *rightchange)
{
	/*FIXME: Implement me */
	return CONV_DATA_MISMATCH;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "calendar");
	OSyncObjFormat *vcal = osync_conv_register_objformat(env, "calendar", "vcalendar");
	osync_conv_format_set_compare_func(vcal, compare_vcalendar);
	osync_conv_format_set_plain_malloced(vcal);
}
