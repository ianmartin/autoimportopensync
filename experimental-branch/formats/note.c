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

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "vnote11");
	osync_env_register_detector(env, "plain", "vnote11", detect_plain_as_vnote);
	osync_env_format_set_compare_func(env, "vnote11", compare_vnote);
}
