#include <opensync/opensync.h>
#include <glib.h>
#include <stdio.h>

static osync_bool detect_plain_as_vnote(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VNOTE11", 3, "start: %s", __func__);
	
	if (!data)
		return FALSE;

	return g_pattern_match_simple("*BEGIN:VNOTE*VERSION:1.1*", data);
}

static OSyncConvCmpResult compare_vnote(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "note");
	OSyncObjFormat *format = osync_conv_register_objformat(env, "note", "vnote11");
	osync_conv_register_data_detector(env, "plain", "vnote11", detect_plain_as_vnote);
	osync_conv_format_set_compare_func(format, compare_vnote);
	osync_conv_format_set_like(format, "plain", 0, CONV_DETECTFIRST);
}
