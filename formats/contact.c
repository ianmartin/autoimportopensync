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

osync_bool detect_vcard(OSyncFormatEnv *env, void *data, int size)
{
	if (!strcmp(data, "BEGIN:VCARD"))
		return TRUE;
	return FALSE;
}

static void create_vcard(OSyncChange *change)
{
	
}

void get_info(OSyncFormatEnv *env)
{
	OSyncObjType *type = osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *format = osync_conv_register_objformat(type, "vcard");
	osync_conv_format_set_compare_func(format, compare_vcard);
	osync_conv_format_set_create_func(format, create_vcard);
	//osync_conv_register_data_detector(format, "contact", "vcard", detect_vcard);
}
