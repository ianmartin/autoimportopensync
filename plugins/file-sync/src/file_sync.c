#include "file_sync.h"

#ifdef HAVE_FAM
static gboolean _fam_prepare(GSource *source, gint *timeout_)
{
	*timeout_ = 1;
	return FALSE;
}

static gboolean _fam_check(GSource *source)
{
	return TRUE;
}

static gboolean _fam_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
	filesyncinfo *fsinfo = user_data;
	FAMEvent famEvent;
	if (FAMPending(fsinfo->famConn)) {
		if (FAMNextEvent(fsinfo->famConn, &famEvent) < 0) {
			printf("Error getting fam event\n");
		} else {
			if (famEvent.code == 1 || famEvent.code == 2 || famEvent.code == 5 || famEvent.code == 6)
				osync_member_request_synchronization(fsinfo->member);
		}
	}
	return TRUE;
}

static void fam_setup(filesyncinfo *fsinfo, GMainContext *context)
{
	GSourceFuncs *functions = g_malloc0(sizeof(GSourceFuncs));
	functions->prepare = _fam_prepare;
	functions->check = _fam_check;
	functions->dispatch = _fam_dispatch;
	functions->finalize = NULL;

	GSource *source = g_source_new(functions, sizeof(GSource));
	g_source_set_callback(source, NULL, fsinfo, NULL);
	g_source_attach(source, context);
}
#endif

static void *fs_initialize(OSyncMember *member)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	char *configdata;
	int configsize;
	filesyncinfo *fsinfo = g_malloc0(sizeof(filesyncinfo));
	if (!fsinfo)
		goto error_ret;
	if (!osync_member_get_config(member, &configdata, &configsize))
		goto error_free;
	//FIXME Remove g_strstrip from the next line!
	fsinfo->path = g_strstrip(g_strdup(configdata));
	fsinfo->member = member;
	fsinfo->hashtable = osync_hashtable_new();
	g_free(configdata);

#ifdef HAVE_FAM

	fsinfo->famConn = g_malloc0(sizeof(FAMConnection));
	fsinfo->famRequest = g_malloc0(sizeof(FAMRequest));

	if (FAMOpen(fsinfo->famConn) < 0) {
		printf( "Cannot connect to FAM\n");
	} else {
		if( FAMMonitorDirectory(fsinfo->famConn, fsinfo->path, fsinfo->famRequest, fsinfo ) < 0 ) {
			printf( "Cannot monitor directory %s\n", fsinfo->path);
			FAMClose(fsinfo->famConn);
		} else {
			fam_setup(fsinfo, NULL);
		}
	}
#endif

	return (void *)fsinfo;
	
	error_free:
		g_free(fsinfo);
	error_ret:
		return NULL;
}

static void fs_connect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	GError *direrror = NULL;
	fsinfo->dir = g_dir_open(fsinfo->path, 0, &direrror);
	osync_hashtable_load(fsinfo->hashtable, fsinfo->member);
	
	if (!osync_anchor_compare(fsinfo->member, "path", fsinfo->path))
		osync_member_set_slow_sync(fsinfo->member, "data", TRUE);
	
	if (direrror) {
		//Unable to open directory
		osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to open directory %s", fsinfo->path);
		g_error_free (direrror);
	} else {
		osync_context_report_success(ctx);
	}
}

static char *fs_generate_hash(fs_fileinfo *info)
{
	char *hash = g_strdup_printf("%i-%i", (int)info->filestats.st_mtime, (int)info->filestats.st_ctime);
	return hash;
}

static void fs_get_changeinfo(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);

	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	if (osync_member_get_slow_sync(fsinfo->member, "data")) {
		osync_debug("FILE-SYNC", 1, "Setting slow sync");
		osync_hashtable_set_slow_sync(fsinfo->hashtable, "data");
	}
	

	if (fsinfo->dir) {
		const gchar *de = NULL;
		while ((de = g_dir_read_name(fsinfo->dir))) {
			char *filename = g_strdup_printf ("%s/%s", fsinfo->path, de);

			if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
				continue;
				
			OSyncChange *change = osync_change_new();
			osync_change_set_member(change, fsinfo->member);
			osync_change_set_uid(change, de);
			osync_change_set_objformat_string(change, "file");
			
			fs_fileinfo *info = g_malloc0(sizeof(fs_fileinfo));
			stat(filename, &info->filestats);
			
			char *hash = fs_generate_hash(info);
			osync_change_set_hash(change, hash);
			
			osync_change_set_data(change, (char *)info, sizeof(fs_fileinfo), FALSE);			

			if (osync_hashtable_detect_change(fsinfo->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(fsinfo->hashtable, change);
			}

			g_free(hash);
			g_free(filename);
		}
		osync_hashtable_report_deleted(fsinfo->hashtable, ctx);
	}
	osync_context_report_success(ctx);
}

static void fs_get_data(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	fs_fileinfo *file_info = (fs_fileinfo *)osync_change_get_data(change);
	
	char *filename = g_strdup_printf("%s/%s", fsinfo->path, osync_change_get_uid(change));
	osync_file_read(filename, &file_info->data, &file_info->size);
	osync_change_set_data(change, (char *)file_info, sizeof(fs_fileinfo), TRUE);
	g_free(filename);
	
	osync_context_report_success(ctx);
}

static osync_bool fs_access(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	fs_fileinfo *file_info = (fs_fileinfo *)osync_change_get_data(change);
	
	char *filename = NULL;
	filename = g_strdup_printf ("%s/%s", fsinfo->path, osync_change_get_uid(change));
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			if (!remove(filename) == 0) {
				osync_debug("FILE-SYNC", 0, "Unable to remove file %s", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				g_free(filename);
				return FALSE;
			}
			break;
		case CHANGE_ADDED:
			if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
				osync_debug("FILE-SYNC", 0, "File %s already exists", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_EXISTS, "Entry already exists");
				g_free(filename);
				return FALSE;
			}
		case CHANGE_MODIFIED:
			if (!osync_file_write(filename, file_info->data, file_info->size)) {
				osync_debug("FILE-SYNC", 0, "Unable to write to file %s", filename);
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to write");
				g_free(filename);
				return FALSE;
			}
			stat(filename, &file_info->filestats);
			osync_change_set_hash(change, fs_generate_hash(file_info));
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	osync_context_report_success(ctx);
	g_free(filename);
	return TRUE;
}

static osync_bool fs_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	osync_debug("FILE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));
	if (!fs_access(ctx, change))
		return FALSE;

	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	osync_hashtable_update_hash(fsinfo->hashtable, change);
	return TRUE;
}

static void fs_sync_done(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	osync_hashtable_forget(fsinfo->hashtable);
	osync_anchor_update(fsinfo->member, "path", fsinfo->path);
	osync_context_report_success(ctx);
}

static void fs_disconnect(OSyncContext *ctx)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)osync_context_get_plugin_data(ctx);
	g_dir_close(fsinfo->dir);
	osync_hashtable_close(fsinfo->hashtable);
	osync_context_report_success(ctx);
}

static void fs_finalize(void *data)
{
	osync_debug("FILE-SYNC", 4, "start: %s", __func__);
	filesyncinfo *fsinfo = (filesyncinfo *)data;
	osync_hashtable_free(fsinfo->hashtable);
#ifdef HAVE_FAM
	//FAMCancelMonitor(fsinfo->famConn, fsinfo->famRequest);
	//FAMClose(fsinfo->famConn);
#endif

	g_free(fsinfo->path);
	//g_free(fsinfo);
}

void get_info(OSyncPluginInfo *info) {
	info->name = "file-sync";
	info->version = 1;
	info->is_threadsafe = TRUE;
	
	info->functions.initialize = fs_initialize;
	info->functions.connect = fs_connect;
	info->functions.sync_done = fs_sync_done;
	info->functions.disconnect = fs_disconnect;
	info->functions.finalize = fs_finalize;
	info->functions.get_changeinfo = fs_get_changeinfo;
	info->functions.get_data = fs_get_data;
	
	osync_plugin_accept_objtype(info, "data");
	osync_plugin_accept_objformat(info, "data", "file");
	osync_plugin_set_commit_objformat(info, "data", "file", fs_commit_change);
	osync_plugin_set_access_objformat(info, "data", "file", fs_access);
}
