#include <opensync/opensync.h>
#include <glib.h>
#include <opensync/opensync_support.h>
#include <stdio.h>
#include <string.h>

static osync_bool detect_plain_as_vnote(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VNOTE11", 3, "start: %s", __func__);
	
	if (!data)
		return FALSE;

	return osync_pattern_match("*BEGIN:VNOTE*VERSION:1.1*", data, size);
}

static OSyncConvCmpResult compare_vnote(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

static void create_vnote11(OSyncChange *change)
{
	char *vnote = g_strdup_printf("BEGIN:VNOTE\r\nVERSION:1.1\r\nBODY:%s\r\nSUMMARY:%s\r\nEND:VNOTE", osync_rand_str(20), osync_rand_str(6));
	
	osync_change_set_data(change, vnote, strlen(vnote) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(8));
}

void get_info(OSyncEnv *env)
{
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "vnote11");
	osync_env_register_detector(env, "plain", "vnote11", detect_plain_as_vnote);
	osync_env_format_set_create_func(env, "vnote11", create_vnote11);
	osync_env_format_set_compare_func(env, "vnote11", compare_vnote);
	
	osync_env_register_objtype(env, "note");
	osync_env_register_objformat(env, "note", "memo");
}
