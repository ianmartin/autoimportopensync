#include <opensync.h>
#include <glib.h>
#include <string.h>

static OSyncConvCmpResult compare_vcard(OSyncChange *leftchange, OSyncChange *rightchange)
{
	int leftinpsize = osync_change_get_datasize(leftchange);
	char *leftinput = osync_change_get_data(leftchange);
	int rightinpsize = osync_change_get_datasize(rightchange);
	char *rightinput = osync_change_get_data(rightchange);
	
	if (leftinpsize == rightinpsize) {
		if (!memcmp(leftinput, rightinput, leftinpsize))
			return CONV_DATA_SAME;
	}
	
	//Get the name of the contact and compare
	//If the same, return SIMILAR
	
	return CONV_DATA_MISMATCH;
}

static osync_bool detect_plain_as_vcard(OSyncFormatEnv *env, const char *data, int size)
{
	osync_debug("VCARD", 3, "start: %s", __func__);
	if (size >= 11 && !strncmp(data, "BEGIN:VCARD", 11))
		return TRUE;
	return FALSE;
}

static void create_vcard(OSyncChange *change)
{
	char *vcard = g_strdup_printf("BEGIN:VCARD\r\nVERSION:2.1\r\nN:%s;%s;;;\r\nEND:VCARD\r\n", osync_rand_str(10), osync_rand_str(10));
	osync_change_set_data(change, vcard, strlen(vcard) + 1, TRUE);
	if (!osync_change_get_uid(change))
		osync_change_set_uid(change, osync_rand_str(6));
}

static OSyncFilterAction vcard_categories_filter(OSyncChange *change, char *config)
{
	//Check what categories are supported here.
	return OSYNC_FILTER_IGNORE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *vcard = osync_conv_register_objformat(env, "contact", "vcard");
	osync_conv_format_set_compare_func(vcard, compare_vcard);
	osync_conv_format_set_create_func(vcard, create_vcard);
	
	osync_conv_register_data_detector(env, "plain", "vcard", detect_plain_as_vcard);

	osync_conv_format_set_like(vcard, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
	
	osync_conv_register_filter_function(env, "vcard_categories_filter", "contact", "vcard", vcard_categories_filter);
}
