/*
 * Here you can specify a plugin if you like.
 * You should use a format plugin, if the format conversion
 * is rather difficult or if other plugins might be able to reuse
 * your conversion
 * 
 */
#include <opensync/opensync.h>
#include "plugin.h"

static OSyncConvCmpResult compare_format1(OSyncChange *leftchange, OSyncChange *rightchange)
{
	/*
	 * This function can be used to compare two types of your formats.
	 * This is optional. For example, you can only provide a conversion
	 * from and to the xml format and let all the comparison be done there
	 * 
	 */

	/*
	 * Compare your objects here. You might need to cast the data of the change
	 * objects to you own defined types.
	 * 
	 * The possible result of the comparison are:
	 * 
	 * return CONV_DATA_SAME;
	 * The objects are exactly the same. (they might differ in some uid or
	 * timestamp though, but ALL the "real" information is the same)
	 * 
	 * return CONV_DATA_SIMILAR;
	 * The objects are not _exactly_ the same, but they look similar. This is used
	 * to detect conflicts. It is up to you to decide what "similar" means for your
	 * object
	 * 
	 * return CONV_DATA_MISMATCH;
	 * This means the objects are not the same and not similar.
	 * 
	 */
	return CONV_DATA_MISMATCH;
}

static osync_bool conv_format1_to_format2(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	/*
	 * This function can be used to convert your format to another format.
	 * Return TRUE if the conversion was successfull or return FALSE and set
	 * the error if something bad has happend.
	 * 
	 */
	
	return TRUE;
}

static osync_bool conv_format2_to_format1(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	/*
	 * This function can be used to convert another format to your format.
	 * Return TRUE if the conversion was successfull or return FALSE and set
	 * the error if something bad has happend.
	 * 
	 */
	
	return TRUE;
}

static void destroy_format1(char *input, size_t inpsize)
{
	/*
	 * Here you have to free the data allocated by your format
	 * 
	 */
}

static void duplicate_format1(OSyncChange *change)
{
	/*
	 * This function can be used to duplicate your format.
	 * Duplication does not mean to make 2 objects out of one,
	 * but to change to uid of the object in such a way that it
	 * differes from the original uid.
	 * 
	 * Most format will never need this.
	 * 
	 */
}

static char *print_format1(OSyncChange *change)
{
	/*
	 * If your format is not in a human printable format already
	 * you have to return a human readable string here describing the object
	 * as closely as possible. This information will be used by the user to decide
	 * which object to pick in a conflict.
	 * 
	 */
	 return NULL;
}

void get_info(OSyncFormatEnv *env)
{
	/*
	 * Here you have to give opensync some information about your format
	 * This function will be called directly after the plugin has been loaded.
	 * 
	 */
	
	//Tell opensync for which object type you want to add a format.
	//If the object type does not exist yet, it will be created.
	osync_conv_register_objtype(env, "<some object type>");
	
	//Tell opensync that we want to register a new format
	OSyncObjFormat *format = osync_conv_register_objformat(env, "<some object type>", "<your format name>");
	//Now we can set the function on your format we have created above
	osync_conv_format_set_compare_func(format, compare_format1);
	osync_conv_format_set_duplicate_func(format, duplicate_format1);
	osync_conv_format_set_destroy_func(format, destroy_format1);
	osync_conv_format_set_print_func(format, print_format1);
	
	/*
	 * Now we can register the converters.
	 * You can specify 2 types of converters:
	 * 
	 * CONVERTER_CONV:
	 * This will specify a converter that directly converts between 2 formats
	 * 
	 * CONVERTER_ENCAP:
	 * This specifies a converter that "encapsulates" other data.
	 * Example:
	 * you can store a vcard inside a file. This actually means that you
	 * can store the vcard format inside the file format ("encapsulation")
	 * 
	 * CONVERTER_DESENCAP:
	 * like the example above, but in the other direction
	 * 
	 */
	osync_conv_register_converter(env, CONVERTER_CONV, "<another format name>", "<your format name>", conv_format2_to_format1, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "<your format name>", "<another format name>", conv_format1_to_format2, 0);
}
