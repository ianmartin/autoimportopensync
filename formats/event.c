#include <opensync.h>
#include <glib.h>
#include <string.h>

/** @defgroup event_vevent event/vevent data format
 *
 * The vevent data should be a malloc()ed block of data. See
 * osync_conv_format_set_malloced().
 *
 * It can be treated as a plain block of data. See
 * osync_conv_format_set_like().
 */

static OSyncConvCmpResult compare_vevent(OSyncChange *leftchange, OSyncChange *rightchange)
{
	/*FIXME: Implement me */
	return CONV_DATA_MISMATCH;
}


/*FIXME: write a proper vevent detector, as vtodos are inside a vcalendar, too */
static osync_bool detect_plain_as_vevent(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);
	if (size >= 15 && !strncmp(data, "BEGIN:VCALENDAR", 15))
		return TRUE;
	return FALSE;
}


void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "event");
	OSyncObjFormat *vcal = osync_conv_register_objformat(env, "event", "vevent");
	osync_conv_format_set_compare_func(vcal, compare_vevent);

	osync_conv_register_data_detector(env, "plain", "vevent", detect_plain_as_vevent);
	osync_conv_format_set_like(vcal, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
}
