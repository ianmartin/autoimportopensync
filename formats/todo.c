#include <opensync.h>
#include <glib.h>
#include <string.h>

static OSyncConvCmpResult compare_vtodo(OSyncChange *leftchange, OSyncChange *rightchange)
{
	return CONV_DATA_MISMATCH;
}

static const char *begin_vcalendar = "BEGIN:VCALENDAR";
static const char *begin_vtodo = "\nBEGIN:VTODO";

static osync_bool detect_plain_as_vtodo(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCAL", 3, "start: %s", __func__);

	// first, check if it is a vcalendar
	if (size < strlen(begin_vcalendar) || strncmp(data, begin_vcalendar, strlen(begin_vcalendar)))
		return FALSE;

	// it is a vcalendar, search for BEGIN:VTODO
	if (g_strstr_len(data, size, begin_vtodo))
		return TRUE;

	return FALSE;
}

/*TODO: Write the vtodo detector */
void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "todo");
	OSyncObjFormat *format = osync_conv_register_objformat(env, "todo", "vtodo");
	osync_conv_format_set_compare_func(format, compare_vtodo);

	osync_conv_register_data_detector(env, "plain", "vtodo", detect_plain_as_vtodo);
	osync_conv_format_set_like(format, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
}
