
#include <stdlib.h>
#include <kapplication.h>

#include "osyncbase.h"
#include "datasource.h"

extern "C"
{
/* NB: the userdata passed to the sink callbacks is not the sink's userdata, as
 * you might expect, but instead the plugin's userdata, which in our case is the
 * KdePluginImplementation object pointer. So we need to get the sink object from
 * osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info))
 */

static void connect_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->connect(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void disconnect_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->disconnect(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void get_changes_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->get_changes(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static osync_bool read_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx, chg);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	bool ret = obj->read(info, ctx, chg);
	osync_trace(TRACE_EXIT, "%s: %d", __PRETTY_FUNCTION__, ret);
	return ret;
}

static void commit_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx, chg);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->commit(info, ctx, chg);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void sync_done_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->sync_done(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

} // extern "C"

/* Warning: this struct initialisation depends on the order of the fields in 
 * OSyncObjTypeSinkFunctions, but C++ doesn't allow C99-style named structure
 * initialisation, and I can't figure out any other way to do it */
static OSyncObjTypeSinkFunctions wrapper_functions = {
	connect_wrapper,
	disconnect_wrapper,
	get_changes_wrapper,
	commit_wrapper,
	NULL, // write
	NULL, // committed_all
	read_wrapper,
	NULL, // batch_commit
	sync_done_wrapper};

bool OSyncDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);
	
	(void)plugin; // silence "unused argument" warning
	
	sink = osync_objtype_sink_new(objtype, error);
	if (sink == NULL) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(error));
		return false;
	}

	osync_objtype_sink_set_functions(sink, wrapper_functions, this);
	osync_plugin_info_add_objtype(info, sink);

	const char *configdir = osync_plugin_info_get_configdir(info);
	QString tablepath = QString("%1/%2-hash.db").arg(configdir, objtype);
	hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), error);
	if (hashtable == NULL) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(error));
		return false;
	}

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

void OSyncDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	//Detection mechanismn if this is the first sync
	QString anchorpath = QString("%1/anchor.db").arg(osync_plugin_info_get_configdir(info));
	if (!osync_anchor_compare(anchorpath, objtype, "true")) {
		osync_trace(TRACE_INTERNAL, "Setting slow-sync for %s", objtype);
		osync_objtype_sink_set_slowsync(sink, TRUE);
	}
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void OSyncDataSource::sync_done(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	//Detection mechanismn if this is the first sync
	QString anchorpath = QString("%1/anchor.db").arg(osync_plugin_info_get_configdir(info));
	osync_anchor_update(anchorpath, objtype, "true");
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

OSyncDataSource::~OSyncDataSource()
{
	if (sink)
		osync_objtype_sink_unref(sink);

	if (hashtable)
		osync_hashtable_free(hashtable);
}
