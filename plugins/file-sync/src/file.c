#include <opensync/opensync.h>
#include <glib.h>
#include <file_sync.h>

static OSyncConvCmpResult compare_file(OSyncChange *leftchange, OSyncChange *rightchange)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *leftfile = (fs_fileinfo *)osync_change_get_data(leftchange);
	fs_fileinfo *rightfile = (fs_fileinfo *)osync_change_get_data(rightchange);
	
	gboolean data_same = FALSE;
	gboolean path_same = FALSE;
	
	if (!strcmp(osync_change_get_uid(leftchange), osync_change_get_uid(rightchange)))
		path_same = TRUE;
	
	if (leftfile->size == rightfile->size) {
		if (leftfile->data == rightfile->data) {
			data_same = TRUE;
		} else {
			if (!memcmp(leftfile->data, rightfile->data, leftfile->size))
				data_same = TRUE;
		}
	}
	
	if (data_same && path_same)
		return CONV_DATA_SAME;
	if (path_same)
		return CONV_DATA_SIMILAR;
	
	return CONV_DATA_MISMATCH;
}

#ifdef STRESS_TEST
static void create_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file_info = g_malloc0(sizeof(fs_fileinfo));
	int file_size = g_random_int_range(0, 1000);
	osync_change_set_data(change, (char *)file_info, sizeof(fs_fileinfo), TRUE);
	
	file_info->data = g_malloc0(file_size * 105 * sizeof(char));
	file_info->size = file_size * 100 * sizeof(char);
	
	char *datap  = file_info->data;
	FILE *fd = fopen("/dev/urandom", "r");
	if (fd) {
		for (; file_size > 5; file_size--) {
			fread(datap, 100, 1, fd);
			datap += 100 * sizeof(char);
		}
	}
	fclose(fd);
	osync_change_set_uid(change, osync_rand_str(6));
}
#endif

static osync_bool conv_file_to_plain(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file = (fs_fileinfo *)input;
	*output = g_malloc0(file->size * sizeof(char));
	memcpy(*output, file->data, file->size);
	
	*outpsize = file->size * sizeof(char);
	return TRUE;
}

static osync_bool conv_plain_to_file(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	fs_fileinfo *file = g_malloc0(sizeof(fs_fileinfo));

	/* Just take the data pointers, as we are a CONV_TAKEOVER converter */
	/*FIXME: all CONV_TAKEOVER converters will need a typecast because input is const */
	file->data = (char*)input;
	file->size = inpsize;
	
	*output = (char *)file;
	*outpsize = sizeof(file);
	return TRUE;
}

void duplicate_file(OSyncChange *change)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	char *newuid = g_strdup_printf ("%s-dupe", osync_change_get_uid(change));
	osync_change_set_uid(change, newuid);
	g_free(newuid);
}

static osync_bool detect_file(OSyncFormatEnv *env, OSyncChange *change)
{
	osync_debug("FILE", 3, "start: %s", __func__);
	fs_fileinfo *file = (fs_fileinfo *)osync_change_get_data(change);
	
	//Call the data detectors here
	if (osync_conv_detect_data(env, change, file->data, file->size))
		return TRUE;

	return FALSE;
}

void get_info(OSyncFormatEnv *env)
{
	OSyncObjType *type = osync_conv_register_objtype(env, "data");
	g_assert(type);
	
	OSyncObjFormat *format = osync_conv_register_objformat(env, "data", "file");
	osync_conv_format_set_compare_func(format, compare_file);
	osync_conv_format_set_detect_func(format, detect_file);
	osync_conv_format_set_duplicate_func(format, duplicate_file);
#ifdef STRESS_TEST
	osync_conv_format_set_create_func(format, create_file);
#endif
	osync_conv_register_converter(env, CONVERTER_CONV, "file", "plain", conv_file_to_plain, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "plain", "file", conv_plain_to_file, CONV_NOTLOSSY|CONV_TAKEOVER);
}
