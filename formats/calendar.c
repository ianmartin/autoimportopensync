#include <opensync.h>
#include <glib.h>

static OSyncConvCmpResult compare_vcalendar(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "calendar");
	OSyncObjFormat *format = osync_conv_register_objformat(env, "calendar", "vcalendar");
	osync_conv_format_set_compare_func(format, compare_vcalendar);
}
