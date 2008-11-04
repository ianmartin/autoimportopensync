/*
 * Here you can specify a format plugin if you like.
 * You should use a format plugin, if the format conversion
 * is rather difficult or if other plugins might be able to reuse
 * your conversion. A format plugin can create new formats in get_format_info
 * and provide converters between different formats in get_conversion_info
 * 
 */
#include <opensync/opensync.h>
#include <opensync/opensync-format.h>

#include "plugin.h"

static OSyncConvCmpResult compare_format1(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize)
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
	 * return OSYNC_CONV_DATA_SAME;
	 * The objects are exactly the same. (they might differ in some uid or
	 * timestamp though, but ALL the "real" information is the same)
	 * 
	 * return OSYNC_CONV_DATA_SIMILAR;
	 * The objects are not _exactly_ the same, but they look similar. This is used
	 * to detect conflicts. It is up to you to decide what "similar" means for your
	 * object
	 * 
	 * return OSYNC_CONV_DATA_MISMATCH;
	 * This means the objects are not the same and not similar.
	 * 
	 */
	return OSYNC_CONV_DATA_MISMATCH;
}

static osync_bool conv_format1_to_format2(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	/*
	 * This function can be used to convert your format to another format.
	 * Return TRUE if the conversion was successful or return FALSE and set
	 * the error if something bad has happend.
	 * 
	 */
	
	/* The arguments mean:
	 * 
	 * - conv_data:
	 * Pointer to the data you returned in your init function (if any)
	 * 
	 * - input:
	 * The data you need to convert
	 * - inpsize
	 * The size of the input data
	 * 
	 * - output:
	 * After converting you need to set this
	 * to your result
	 * - outpsize:
	 * The size of the output
	 * 
	 * - free_input:
	 * You need to set this to TRUE if opensync
	 * can free the input after the conversion (so you dont
	 * use any reference from or to the input). A example where
	 * *free_input = FALSE; needs to be done would be a encapsulator
	 * that stores the input reference somewhere in its struct
	 * 
	 * - error:
	 * if something bad happens and you cannot convert, set the error!
	 * 
	 */
	
	return TRUE;
}

static osync_bool conv_format2_to_format1(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, void *userdata, OSyncError **error)
{
	/*
	 * This function can be used to convert another format to your format.
	 * Return TRUE if the conversion was successful or return FALSE and set
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

static osync_bool duplicate_format1(const char *uid, const char *input, unsigned int insize, char **newuid, char **output, unsigned int *outsize, osync_bool *dirty, OSyncError **error)
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

	return TRUE;
}

static char *print_format1(const char *data, unsigned int size)
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

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	/* 
	 * this function is called to register a new format 
	 */
	OSyncObjFormat *format = osync_objformat_new("<your format name>", "<some object type>", error);
	if (!format)
		return FALSE;

	osync_objformat_set_compare_func(format, compare_format1);
	osync_objformat_set_destroy_func(format, destroy_format1);
	osync_objformat_set_duplicate_func(format, duplicate_format1);
	osync_objformat_set_print_func(format, print_format1);


	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}

void *initialize(const char* config, OSyncError **error)
{
	/*
	 * Here you can create converter specific data.
	 * If you return the converter specific data, it is passed
	 * to the conversion and detector function as void *userdata
	 */
	format_data *userdata = osync_try_malloc0(sizeof(format_data), error);
	return (void*)userdata;
}

void finalize(void *userdata)
{
	/*
	 * Here you can free all your converter specific data.
	 */
	format_data *formatdata =(format_data*)userdata;
	g_free(formatdata->data);
	g_free(formatdata);
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	/*
	 * Here you have to give opensync some information about your format
	 * This function will be called directly after the plugin has been loaded
	 * to get converters that convert between different formats
	 */
	
	OSyncObjFormat *format1 = osync_format_env_find_objformat(env, "<your format name>");
	if (!format1) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find <your format name> format");
		return FALSE;
	}

	OSyncObjFormat *format2 = osync_format_env_find_objformat(env, "xmlformat-contact");
	if (!format2) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find xmlformat-contact format");
		return FALSE;
	}

	OSyncFormatConverter *conv = osync_converter_new(OSYNC_CONVERTER_CONV, format1, format2, conv_format1_to_format2, error);
	if (!conv)
		return FALSE;
	/* set init and finalize functions */
	osync_converter_set_initialize_func(conv, initialize);
	osync_converter_set_finalize_func(conv, finalize);
	/* register converter */
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, format2, format1, conv_format2_to_format1, error);
	if (!conv)
		return FALSE;
	/* e.g. this converter doesn't need init and finalize functions therefore don't set them */
	/* register converter */
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	return TRUE;
}

int get_version(void)
{
	/* always return 1 */
	return 1;
}
