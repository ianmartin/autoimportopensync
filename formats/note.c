#include <opensync.h>
#include <glib.h>

static OSyncConvCmpResult compare_vnote(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "note");
	OSyncObjFormat *format = osync_conv_register_objformat(env, "note", "vnote");
	osync_conv_format_set_compare_func(format, compare_vnote);
	osync_conv_format_set_like(format, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
}
