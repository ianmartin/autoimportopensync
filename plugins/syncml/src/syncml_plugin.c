/** @file OpenSync SyncML plugin implementation
 *
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 */

#include <opensync/opensync.h>
#include <libcsyncml/http-binding.h>

#include <glib.h>

struct SyncmlPluginData
{
	char *url;
	struct syncmlHttpSession *session;
};

static void *syncml_initialize(OSyncMember *member, OSyncError **error)
{
	/* Library initialization */
	g_type_init();

	struct SyncmlPluginData *data = malloc(sizeof(struct SyncmlPluginData));
	if (!data) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "No memory to initialize syncml plugin");
		goto error;
	}

	/*FIXME: Test URL. get configuration data */
	data->url = strdup("http://localhost:8080/syncml-test");
	if (!data->url) {
		osync_error_set(error, OSYNC_ERROR_INITIALIZATION, "No memory to initialize syncml plugin");
		goto error;
	}

	return data;

error:
	if (data->url)
		free(data->url);
	if (data)
		free(data);
	return NULL;
}

static void syncml_connect(OSyncContext *ctx)
{
	struct SyncmlPluginData *data = osync_context_get_plugin_data(ctx);
	GError *error = NULL;

	/*FIXME: Should I generate a session ID? */
	data->session = syncmlHttpSessionNew(SYNCML_1_1_2, SYNCML_DS, "1", data->url, &error);
	if (!data->session) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Couldn't create syncml session");
		return;
	}

	osync_context_report_success(ctx);
}

static void syncml_disconnect(OSyncContext *ctx)
{
	osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented, yet");
}

void syncml_sync_done(OSyncContext *ctx)
{
	osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented, yet");
}

static void syncml_finalize(void *data)
{
	struct SyncmlPluginData *mydata = data;
	if (!mydata)
		return;

	if (mydata->url)
		free(mydata->url);
	free(mydata);
}

static void syncml_get_changeinfo(OSyncContext *ctx)
{
	osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented, yet");
}

static void syncml_get_data(OSyncContext *ctx, OSyncChange *change)
{
	osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented, yet");
}

int get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "syncml";
    /*FIXME: i18n */
    info->description = "SyncML plugin";

	info->functions.initialize = syncml_initialize;
	info->functions.connect = syncml_connect;
	info->functions.disconnect = syncml_disconnect;
	info->functions.sync_done = syncml_sync_done;
	info->functions.finalize = syncml_finalize;
	info->functions.get_changeinfo = syncml_get_changeinfo;
	info->functions.get_data = syncml_get_data;
}
